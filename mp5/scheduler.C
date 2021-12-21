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

Scheduler::Scheduler() {
    //Console::puti(ready_queue.size());
    Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
    if (ready_queue.size() == 0){
        Console::puts("Cannot yield. Size of ready queue is zero.\n");
    }
    else{
        Thread* next_thread = ready_queue.dequeue();
        Thread::dispatch_to(next_thread);
    }
}

void Scheduler::resume(Thread * _thread) {
    // Add the given thread to the ready_queue of the scheduler
    ready_queue.enqueue(_thread);
}

void Scheduler::add(Thread * _thread) {
    // Make the given thread runnable by the scheduler
    ready_queue.enqueue(_thread);
}

void Scheduler::terminate(Thread * _thread) {
    if (ready_queue.size() == 0) return;

    Thread* rm = ready_queue.dequeue();
    for (int i = ready_queue.size(); i >= 0; i--){
        if (rm->ThreadId() != _thread->ThreadId()){
            ready_queue.enqueue(rm);
        }
        rm = ready_queue.dequeue();
    }
}
