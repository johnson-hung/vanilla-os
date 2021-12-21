/*
 File: vm_pool.C
 
 Author: Chien-Chiang Hung
 Date  : 10/20/2021
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define GB * (0x1 << 30)
#define MB * (0x1 << 20)
#define KB * (0x1 << 10)

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"

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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    
    base_address = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;

    // Register current virtual memory pool with the given page table
    page_table->register_pool(this);
    
    // Setup logical start address of the pool and initialize cur pointer
    vm_mem_pool = (unsigned long *) base_address;
    vm_mem_pool_cur = -1;

    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    unsigned long cur_addr = 0;
    unsigned int page_size = PageTable::PAGE_SIZE;

    // Calculate number of pages to allocate
    int num_pages = _size / page_size;
    num_pages += (num_pages % page_size > 0)? 1 : 0;

    // Get address in the current VMPool
    if (vm_mem_pool_cur < 0){
        cur_addr = base_address + page_size + 1;
    }
    else{
        cur_addr = vm_mem_pool[vm_mem_pool_cur];
    }

    unsigned long next_addr = cur_addr + num_pages * page_size;
    if (next_addr < base_address + size && next_addr >= base_address){
        vm_mem_pool_cur++; vm_mem_pool[vm_mem_pool_cur] = cur_addr;
        vm_mem_pool_cur++; vm_mem_pool[vm_mem_pool_cur] = next_addr;
        Console::puts("Allocated region of memory.\n");
        return cur_addr;
    }
    else{
        Console::puts("Cannot allocate requested region of memory.\n");
        return 0;
    }
}

void VMPool::release(unsigned long _start_address) {
    int page_size = PageTable::PAGE_SIZE;
    
    // Get start index in vm_mem_pool for the _start_address
    int cur = 0;
    for (; cur <= vm_mem_pool_cur; cur = cur + 2){
        if (vm_mem_pool[cur] == _start_address) {
            break;
        }
    }

    // Release memory in the corresponding range
    unsigned long start = vm_mem_pool[cur];
    unsigned long end = vm_mem_pool[cur + 1];
    for (int count = 0; count < (end - start) / page_size; count++){
        page_table->free_page(start);
        start += page_size;
    }
    // Update current page table
    page_table->load();
    
    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    Console::puts("Checked whether address is part of an allocated region.\n");
    
    unsigned long end = base_address + size;
    return _address < end && _address >= base_address;
}

