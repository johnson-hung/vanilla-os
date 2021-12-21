/*
 File: ContFramePool.C
 
 Author: Chien-Chiang Hung
 Date  : September 24 2021
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define KB * (0x1 << 10)
#define MB * (0x1 << 20)

// For each frame, we need 2 bits to maintain its status:
// - 11 : FREE
// - 10 : HEAD-OF-SEQUENCE
// - 01 : INACCESSIBLE
// - 00 : ALLOCATED

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

// For holding a list of frame pools
ContFramePool* ContFramePool::poolListHead;

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    // Bitmap has to fit in a single frame
    assert(_n_frames <= FRAME_SIZE * 8);

    // Initialize data
    base_frame_no = _base_frame_no;
    n_frames = _n_frames;
    n_free_frames = n_frames;
    info_frame_no = _info_frame_no;
    n_info_frames = _n_info_frames;

    // If info_frame_no is zero then we keep management info in the first
    // frame, else we use the provided frame to keep management info 
    if (info_frame_no == 0){
        bitmap = (unsigned char*) (base_frame_no * FRAME_SIZE);
    }
    else{
        bitmap = (unsigned char*) (info_frame_no * FRAME_SIZE);
    }

    // Number of frames should fit the bitmap
    assert((n_frames % 8) == 0);

    // Set all bits in the bitmap to 0xFF (FREE frames)
    // 128 * 8 = 1024 bits => 512 frames (KERNEL_POOL_SIZE)
    for (unsigned int i = 0; i*8 < n_frames*2; i++){
        bitmap[i] = 0xFF;
    }

    // If info_frame_no is 0, we choose the first frame starts from
    // base_frame_no to be info frame and set the n_info_frames to 1 
    if (info_frame_no == 0){
        info_frame_no = base_frame_no;
        n_info_frames = 1;
    }
    // assert(base_frame_no <= info_frame_no);
       
    // Mark all info frames as ALLOCATED (00) if they are used
    unsigned long cur_frame_no = base_frame_no;
    unsigned long info_frame_end_no = info_frame_no + n_info_frames - 1;
    for (unsigned int i = 0; i*8 < n_frames*2; i++){
        unsigned char mask = 0xC0; // 0xC0: 11 00 00 00
        for (int cur_bit = 7; cur_bit > 0; cur_bit = cur_bit - 2){
            if (cur_frame_no > info_frame_end_no){
                break;
            }
            else{ // We can use XOR here because initially all elements in bitmaps are 0xFF
                bitmap[i] ^= mask;
                n_free_frames--;
            }
            cur_frame_no++;
            mask >>= 2;
        }
        if (cur_frame_no > info_frame_end_no){ // Check if current frame no is out of bound
            break;
        }
    }
 
    // Frame pool list setup   
    if (ContFramePool::poolListHead == NULL){
        ContFramePool::poolListHead = this;
    }
    else{
        ContFramePool* pre = ContFramePool::poolListHead;
        ContFramePool::poolListHead = this;
        ContFramePool::poolListHead->poolListNext = pre;
    }
    
    Console::puts("Frame Pool initialized\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // Check if we have enough free frames to allocate
    if (n_free_frames <= 0){
        Console::puts("Unable to get frames: ");
        Console::puts("n_free_frames = "); Console::putui(n_free_frames);
        Console::puts(",_n_frames = "); Console::putui(_n_frames);
        Console::puts("\n");
        return 0;
    }

    // Find consecutive free frames for requested _n_frames
    unsigned long free_frame_count = 0; // Count of consecutive FREE frames
    unsigned long free_frame_start_no = base_frame_no;
    unsigned long bitmap_start_i = 0;
    unsigned long bitmap_start_offset = 0; // Offset in bitmap[bitmap_start_i]
    unsigned long cur_frame_no = base_frame_no;

    for (unsigned int i = 0; i*8 < n_frames*2; i++){
        unsigned char mask = 0xC0; // 0xC0: 11 00 00 00
        for (int cur_bit = 7; cur_bit > 0; cur_bit = cur_bit - 2){
            if ((bitmap[i] & mask) == mask){ // Got an available frame, keep counting
                if (free_frame_count == 0){ // Check if this is the first FREE frame
                    free_frame_start_no = cur_frame_no;
                    bitmap_start_i = i;
                    bitmap_start_offset = cur_bit;
                }
                free_frame_count++;
            }
            else{ // Current frame unavailable, we have to count from 0 again
                free_frame_count = 0;
            }

            if (free_frame_count == _n_frames){
                break;
            }
            cur_frame_no++;
            mask >>= 2;
        }
        if (free_frame_count == _n_frames){
            break;
        }
    }

    if (free_frame_count != _n_frames){
        Console::puts("Consecutive free frames not enough for ");
        Console::puts("_n_frames = "); Console::putui(_n_frames);
        Console::puts("\n");
        return 0;
    }
    
    // Allocate _n_frames consecutive frames:
    // - Update bitmap: Mark HEAD-OF-SEQUENCE (10)
    bitmap[bitmap_start_i] |= (0x1 << bitmap_start_offset);
    bitmap[bitmap_start_i] &= ~(0x1 << (bitmap_start_offset-1));
    free_frame_count--;

    // - Update bitmap: Mark ALLOCATED (00)
    unsigned char mask = 0x3 << (bitmap_start_offset-1);
    unsigned long cur_i = bitmap_start_i;
    while (free_frame_count > 0){
        mask = mask >> 2;
        if (mask == 0){
            mask = 0xC0;
            cur_i++;
        }
        bitmap[cur_i] ^= mask;
        free_frame_count--;
    }

    // - Update number of free frames left
    n_free_frames -= _n_frames;

    return free_frame_start_no;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    // Handle special cases
    assert(_base_frame_no >= base_frame_no && _n_frames > 0);

    // Mark INACCESSIBLE according to _base_frame_no and _n_frames:
    unsigned int i = (_base_frame_no - base_frame_no) / 4; // Calculate bitmap index
    unsigned long cur_frame_no = base_frame_no + i*4; // Start from the MSB in bitmap[i]
    for (; i*8 < n_frames*2; i++){
        for (int cur_bit = 7; cur_bit > 0; cur_bit = cur_bit - 2){
            if (cur_frame_no >= _base_frame_no){ // Mark current frame as INACCESSIBLE (01)
                bitmap[i] &= ~(0x1 << i);
                bitmap[i] |= (0x1 << (i-1));
                n_free_frames--;
                _n_frames--;
            }
            if (_n_frames == 0){ // Check if we have marked all requested frames as INACCESSIBLE
                break;
            }
            cur_frame_no++;
        }
        if (_n_frames == 0){
            break;
        }
    }
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    // Find corresponding frame pool with given _first_frame_no
    ContFramePool* cur = ContFramePool::poolListHead;
    while (cur != NULL){
        if (_first_frame_no < cur->base_frame_no 
         || _first_frame_no > cur->base_frame_no + cur->n_frames){
            cur = cur->poolListNext;
        }
        else break;
    }
    assert(cur != NULL);

    // Set frames FREE (11):
    bool is_released = false; // Flag for checking if all requested frames are released
    unsigned char* bitmap = cur->bitmap;
    unsigned int i = (_first_frame_no - cur->base_frame_no) / 4; // Calculate bitmap index
    unsigned long cur_frame_no = cur->base_frame_no + i*4; // Start from the MSB in bitmap[i]
    for (; i*8 < cur->n_frames*2; i++){
        for (int cur_bit = 7; cur_bit > 0; cur_bit = cur_bit - 2){
            if (cur_frame_no == _first_frame_no){ // Check HEAD_OF_SEQUENCE (10)
                bool msb_matched = (bitmap[i] & (0x1 << cur_bit)) != 0x0;
                bool lsb_matched = (bitmap[i] & (0x1 << (cur_bit-1))) == 0x0;
                if (!msb_matched || !lsb_matched){ // Not HEAD-OF-SEQUENCE at _first_frame_no
                    Console::puts("First frame: "); 
                    Console::puti(msb_matched); Console::puti(!lsb_matched);
                    Console::puts(", is not the HEAD-OF-SEQUENCE (10).\n");
                    assert(false);
                }
                bitmap[i] |= (0x3 << (cur_bit-1));
                cur->n_free_frames++;
            }
            else if (cur_frame_no > _first_frame_no){ // Check ALLOCATED (00)
                bool msb_matched = (bitmap[i] & (0x1 << cur_bit)) == 0x0;
                bool lsb_matched = (bitmap[i] & (0x1 << (cur_bit-1))) == 0x0;
                if (!msb_matched || !lsb_matched){ // Sequence ends, we can now break
                    is_released = true;
                    break;
                }
                bitmap[i] |= (0x3 << (cur_bit-1));
                cur->n_free_frames++;

            }
            cur_frame_no++;
        }

        if (is_released){ // Check if all request frames are released
            Console::puts("_first_frame_no = "); Console::puti(_first_frame_no); Console::puts(", ");
            Console::puti(cur_frame_no - _first_frame_no); Console::puts(" frames released.\n");
            break;
        }
    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    return _n_frames / (4 * 4 KB) + (_n_frames % (4 * 4 KB) > 0? 1 : 0);
}
