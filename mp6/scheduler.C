/*
 File: scheduler.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "blocking_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/


extern BlockingDisk* SYSTEM_DISK;

Scheduler::Scheduler() {
    //Console::puti(ready_queue.size());
    Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
    //if (Machine::interrupts_enabled()){
    //    Machine::disable_interrupts();
    //}

    Thread* t = ready_queue.dequeue();
    if (SYSTEM_DISK->is_ready() && SYSTEM_DISK->disk_queue.size() > 0){
        Thread* disk_ops = SYSTEM_DISK->disk_queue.dequeue();
        //Thread::dispatch_to(t);
        this->resume(disk_ops);
        Console::puts("Disk is ready. Resumed Thread#");
        Console::puti(disk_ops->ThreadId()); Console::puts(" request to ready queue.\n");
    }
    Thread::dispatch_to(t);

    /*
    else{
        if (ready_queue.size() == 0){
            Console::puts("Cannot yield. Size of ready queue is zero.\n");
        }
        else{
            //Console::puts("Dispatch to the next thread.\n");
            Thread* t = ready_queue.dequeue();
            Thread::dispatch_to(t);
        }
    }
    */
    //if (!Machine::interrupts_enabled()){
    //    Machine::enable_interrupts();
    //}
}

void Scheduler::resume(Thread * _thread) {
    // Add the given thread to the ready_queue of the scheduler
    //if (Machine::interrupts_enabled()){
    //    Machine::disable_interrupts();
    //}
    
    ready_queue.enqueue(_thread);

    //if (!Machine::interrupts_enabled()){
    //    Machine::enable_interrupts();
    //}
}

void Scheduler::add(Thread * _thread) {
    // Make the given thread runnable by the scheduler
    //if (Machine::interrupts_enabled()){
    //    Machine::disable_interrupts();
    //}
    
    ready_queue.enqueue(_thread);

    //if (!Machine::interrupts_enabled()){
    //    Machine::enable_interrupts();
    //}
}

void Scheduler::terminate(Thread * _thread) {
    //if (Machine::interrupts_enabled()){
    //    Machine::disable_interrupts();
    //}
    
    if (ready_queue.size() != 0){
        for (int i = ready_queue.size(); i > 0; i--){
            Thread* rm = ready_queue.dequeue();
            if (rm->ThreadId() != _thread->ThreadId()){
                ready_queue.enqueue(rm);
            }
        }
    }
    
    //if (!Machine::interrupts_enabled()){
    //    Machine::enable_interrupts();
    //}
}
