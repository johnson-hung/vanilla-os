/*
     File        : blocking_disk.c

     Author      : Chien-Chiang Hung
     Modified    : 11/22/2021

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
    //Console::puti(disk_queue.size());
    Console::puts("BlockingDisk initialized.\n");
}

void BlockingDisk::wait_until_ready(){
    if (!this->is_ready()){
        // If the device is not ready, enqueue it to disk queue
        Thread* current_thread = Thread::CurrentThread();
        Console::puts("Thread #"); Console::puti(current_thread->ThreadId());
        Console::puts(": Disk is not ready. Waits until ready, yield.\n");

        disk_queue.enqueue(current_thread);
        SYSTEM_SCHEDULER->yield();
    }
}


/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
    SimpleDisk::read(_block_no, _buf);
    //Console::puts("\n");
}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
    SimpleDisk::write(_block_no, _buf);
    //Console::puts("\n");
}
