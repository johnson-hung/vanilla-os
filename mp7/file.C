/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id) {
    Console::puts("[File#"); Console::puti(_id); Console::puts("] ");
    Console::puts("Opening file.\n");
    
    // Initialize data structures
    fs = _fs;
    id = _id;
    size = 0;
    last = 0;

    // Fetch data for this file
    inode = fs->LookupFile(id);
    if (inode->id == _id){
        block_no = inode->block_no;
        size = inode->file_size;

        Console::puts("[File#"); Console::puti(inode->id); Console::puts("] ");
        Console::puts("Got data from block#"); Console::puti(inode->block_no); Console::puts("\n");
    }   
}

File::~File() {
    Console::puts("[File#"); Console::puti(inode->id); Console::puts("] ");
    Console::puts("Closing file.\n");
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */

    inode->file_size = size;
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf) {
    Console::puts("[File#"); Console::puti(inode->id); Console::puts("] ");
    Console::puts("reading from file#"); Console::puti(id);
    Console::puts(" (block#"); Console::puti(block_no); Console::puts(")\n");

    if (fs == NULL || block_no == -1){
        Console::puts("[File#"); Console::puti(inode->id); Console::puts("] ");
        if (fs == NULL) Console::puts("Corresponding FileSystem not found\n");
        if (block_no == -1) Console::puts("No available block\n");
        return 0;
    }

    // Fix: File size is limited to 512 bytes maximum
    if (last >= 512){
        return 0;
    }

    // Read data from the block into cache
    unsigned char buf[512];
    memset(buf, 0, 512);
    fs->disk->read(block_no, buf);

    // Starting from last index, set _buf with data read and stored in buf
    int i = 0;
    for (int n = _n; n > 0; n--){
        if (EoF()) return i;
        _buf[i] = buf[last];
        last++;

        /*
        // If last position in read buffer is out of bound, we may move to next block
        // (For basic implementation, each file only takes 1 block)
        if (last >= 512){
            // Update current buffer for next read operation
            
            memset(buf, 0, 512);
            fs->disk->read(block_no, (unsigned char *) buf);
            last = 0;
            
            return i;
        }
        */

        i++;
    }
    return i;
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("[File#"); Console::puti(inode->id); Console::puts("] ");
    Console::puts("writing to file#"); Console::puti(id);
    Console::puts(" (block#"); Console::puti(block_no); Console::puts(")\n");

    if (fs == NULL || block_no == -1){
        Console::puts("[File: "); Console::puti(inode->id); Console::puts("] ");
        if (fs == NULL) Console::puts("Corresponding FileSystem not found\n");
        if (block_no == -1) Console::puts("No available block\n");
        return 0;
    }

    // Fix: File size is limited to 512 bytes maximum
    if (last >= 512){
        return 0;
    }

    // Fetch previous data into cache
    unsigned char buf[512];
    memset(buf, 0, 512);
    fs->disk->read(block_no, (unsigned char*) buf);

    // Starting from last index, set buf with given data
    int i = 0;
    for (int n = _n; n > 0; n--){
        buf[last] = _buf[i];
        last++;
        if (last > size) size++;
        
        // If last position in write buffer is out of bound, we write the block to disk
        if (last >= 512 && n > 0){
            break;

            /*
            fs->disk->write(block_no, (unsigned char*) buf);
            
            // Get new block (-1 if no available block)
            block_no = fs->GetFreeBlock();
            if (block_no == -1) return i;

            // Update current buffer for next write operation
            memset(buf, 0, 512);
            fs->disk->read(block_no, (unsigned char *) buf);
            last = 0;
            */
        }
        i++;
    }

    // Write the remaining part of data in current buffer to disk
    fs->disk->write(block_no, (unsigned char*) buf);

    // Return number of chars written
    return i;
}

void File::Reset() {
    Console::puts("[File#"); Console::puti(inode->id); Console::puts("] ");
    Console::puts("resetting file\n");
    last = 0;
}

bool File::EoF() {
    Console::puts("[File#"); Console::puti(inode->id); Console::puts("] ");
    Console::puts("checking for EoF: idx = "); Console::puti(last);
    Console::puts(", size = "); Console::puti(size); Console::puts("\n"); 
    return last >= size;
}
