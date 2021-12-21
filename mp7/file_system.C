/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define BLOCK_USED 0x0
#define BLOCK_FREE 0x1

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store 
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("[FS] In file system constructor.\n");

    // Initialize local data structures
    disk = NULL;
    size = 0;
    inodes = NULL;
    cur_inum = -1;
    cur_bnum = -1;
    free_blocks = NULL;
}

FileSystem::~FileSystem() {
    Console::puts("[FS] Unmounting file system\n");
    /* Make sure that the inode list and the free list are saved. */
}


/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

unsigned int FileSystem::MAX_BLOCKS;

bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("[FS] Mounting file system from disk\n");

    /* Here you read the inode list and the free list into memory */
    
    // Associate this file system with a disk
    disk = _disk;

    // Read inode list into memory and assign current file system
    disk->read(0, (unsigned char*) inodes);
    for (int i = 0; i < MAX_INODES; i++){
        inodes[i].fs = this;
    }

    // Read free list into memory
    char buf[512];
    memset(buf, 0, 512);
    disk->read(1, (unsigned char*) buf);
    free_blocks = (unsigned char*) buf;

    // Update size of current file system
    size = 2 * SimpleDisk::BLOCK_SIZE;

    return true;
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) { // static!
    Console::puts("[FS] Formatting disk\n");
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */
    
    // Set max number of blocks this file system can accomomdate
    unsigned int n_blocks = _size / SimpleDisk::BLOCK_SIZE; 
    MAX_BLOCKS = n_blocks;
    Console::puts("[FS] MAX_BLOCKS = "); Console::puti(MAX_BLOCKS); Console::puts("\n");
    Console::puts("[FS] MAX_INODES = "); Console::puti(MAX_INODES); Console::puts("\n");

    // Initialize an inode list and write it to the disk
    Inode inode_list[MAX_INODES]; // MAX_INODES * sizeof(Inode) = SimpleDisk::BLOCK_SIZE
    for (int i = 0; i < MAX_INODES; i++){
        inode_list[i].id = -1;
        inode_list[i].block_no = -1;
        inode_list[i].fs = NULL;
        inode_list[i].file_size = 0;
    }
    _disk->write(0, (unsigned char*) inode_list);

    // Initialize free list and write it to the disk
    unsigned char free_list[MAX_BLOCKS];
    free_list[0] = BLOCK_USED; // Mark block for inode list as USED
    free_list[1] = BLOCK_USED; // Mark block for free list as USED
    for (int i = 2; i < MAX_BLOCKS; i++){ // Mark the rest of blocks as FREE
        free_list[i] = (char) BLOCK_FREE;
    }
    _disk->write(1, (unsigned char*) free_list);
    
    return true;
}

Inode * FileSystem::LookupFile(int _file_id) {
    Console::puts("[FS] Looking up file with id = "); Console::puti(_file_id); Console::puts("\n");
    /* Here you go through the inode list to find the file. */

    Inode* inode = NULL;
    for (int i = 0; i < MAX_INODES; i++){
        if (inodes[i].id == _file_id){
            inode = &(inodes[i]);
            break;
        }
    }
    
    if (inode->id != _file_id){
        Console::puts("[FS] File not found\n");
    }
    else{ 
        Console::puts("[FS] Found file with id = "); Console::puti(inode->id); Console::puts("\n");
    }    
    return inode;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("[FS] Creating file with id = "); Console::puti(_file_id); Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */
    
    Inode* inode = LookupFile(_file_id);
    if (inode->id == _file_id){
        Console::puts("[FS] File already exists\n");
        return false;
    }

    // Check free block and inode
    cur_bnum = GetFreeBlock();
    cur_inum = GetFreeInode();

    if (cur_bnum == -1 || cur_inum == -1){
        return false;
    }
    
    // Update free list and write to disk
    free_blocks[cur_bnum] = BLOCK_USED;
    disk->write(1, (unsigned char*) free_blocks);

    // Update inode info and write to disk
    inodes[cur_inum].id = _file_id;
    inodes[cur_inum].block_no = cur_bnum;    
    disk->write(0, (unsigned char*) inodes);
    
    // Update size of file system
    size += SimpleDisk::BLOCK_SIZE;

    Console::puts("[FS] File (id: "); Console::puti(_file_id); Console::puts(") created\n");
    return true;
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("[FS] Deleting file with id = "); Console::puti(_file_id); Console::puts("\n");
    /* First, check if the file exists. If not, throw an error. 
       Then free all blocks that belong to the file and delete/invalidate 
       (depending on your implementation of the inode list) the inode. */

    // Find the inode with given file ID
    Inode* inode = LookupFile(_file_id);

    // Check if the file does exist
    if (inode->block_no == -1){
        return false;
    }

    // Clear data at the given block
    unsigned char buf[512];
    memset(buf, 0, 512);
    disk->write(inode->block_no, (unsigned char*) buf);
    
    // Update the state of block to FREE
    free_blocks[inode->block_no] = (char) BLOCK_FREE;
    disk->write(1, free_blocks);

    // Reset data stored in the inode
    inode->id = -1;
    inode->block_no = -1;
    inode->file_size = 0;
    disk->write(0, (unsigned char*) inodes);
    
    // Update size of file system
    size -= SimpleDisk::BLOCK_SIZE;

    Console::puts("[FS] File (id: "); Console::puti(_file_id); Console::puts(") deleted\n");
    return true;
}

int FileSystem::GetFreeBlock(){
    //Console::puti(MAX_BLOCKS);Console::puts("\n");
    
    // Go through free list and see if we still have free block
    for (int i = 2; i < MAX_BLOCKS; i++){
        if (free_blocks[i] == 1){
            Console::puts("[FS] Got free block#"); Console::puti(i); Console::puts("\n");
            return i;
        }
    }
    Console::puts("[FS] No free block\n");
    return -1;
}

short FileSystem::GetFreeInode(){
    //Console::puti(MAX_INODES);Console::puts("\n");

    // Go through inode list and see if we still have free inode
    for (int i = 0; i < MAX_INODES; i++){
        if (inodes[i].block_no == -1){
            Console::puts("[FS] Got free inode#"); Console::puti(i); Console::puts("\n");
            return i;
        }
    }
    Console::puts("[FS] No free inode\n");
    return -1;
}
