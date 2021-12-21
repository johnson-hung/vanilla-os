#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

#define PAGE_DIRECTORY_SIZE 1
#define PAGE_TABLE_SIZE 1

#define PD_SHIFT 22
#define PT_SHIFT 12

#define PTE_PRESENT 1
#define PTE_WRITE 2
#define PTE_USER_LEVEL 4


PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
    PageTable::kernel_mem_pool = _kernel_mem_pool;
    PageTable::process_mem_pool = _process_mem_pool;
    PageTable::shared_size = _shared_size;
    Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
    vm_pool_no = -1;

    page_directory = (unsigned long *) (kernel_mem_pool->get_frames(PAGE_DIRECTORY_SIZE) * PAGE_SIZE);
    unsigned long* direct_mapped_page_table = (unsigned long *) (kernel_mem_pool->get_frames(PAGE_TABLE_SIZE) * PAGE_SIZE);

    // Calculate number of shared frames
    unsigned long shared_frames = PageTable::shared_size / PAGE_SIZE;

    // Mark shared portion of memory
    unsigned long mask = 0x0;
    for (int i = 0; i < shared_frames; i++){
         direct_mapped_page_table[i] = mask | PTE_PRESENT | PTE_WRITE;
         mask += PAGE_SIZE;
    }

    // Setup the first entry in page_directory
    page_directory[0] = (unsigned long) direct_mapped_page_table | PTE_PRESENT | PTE_WRITE;

    // Mark non-shared portion of memory
    mask = 0x0;
    for (int i = 1; i < shared_frames; i++){
        page_directory[i] = mask | PTE_WRITE;
    }

    // Let the last PDE point to the page directory itself
    page_directory[1023] = (unsigned long) page_directory | PTE_PRESENT | PTE_WRITE;
    Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
    current_page_table = this;

    // Store the address of page directory into CR3 register
    write_cr3((unsigned long) page_directory);
   
    Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
    paging_enabled = 1;
    
    // Enable paging by setting particular bit in CR0 register
    write_cr0(read_cr0() | 0x80000000);
   
    Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
    unsigned long addr = read_cr2();
    unsigned long mask = 0x3FF; // 11 1111 1111
    unsigned long pd_addr = (addr >> PD_SHIFT) & mask;
    unsigned long pt_addr = (addr >> PT_SHIFT) & mask;
  
    // Get corresponding VMPool with address we got from the read_cr2()
    VMPool* vm_pool = NULL;
    for (int i = 0; i <= PageTable::current_page_table->vm_pool_no; i++){
        if (PageTable::current_page_table->registered_vm_pools[i]->is_legitimate(addr)){
            vm_pool = PageTable::current_page_table->registered_vm_pools[i];
            break;
        }
    }

    if (vm_pool == NULL){
        Console::puts("Failed to get VMPool with given address\n");
    }

    mask = 0xFFFFF000; // 1111 1111 1111 1111 1111 1111 0000 0000 0000
    unsigned long* page_directory = (unsigned long *) mask;
    if ((page_directory[pd_addr] & 1) == 0){
        unsigned long frame = PageTable::process_mem_pool->get_frames(PAGE_DIRECTORY_SIZE);
        unsigned long* page_table = (unsigned long *) (frame * PAGE_SIZE);
        page_directory[pd_addr] = (unsigned long) page_table | PTE_PRESENT | PTE_WRITE;
    }

    unsigned long* page_table = (unsigned long *) ((0xFFC00000) | (pd_addr << 12));
    unsigned long frame = PageTable::process_mem_pool->get_frames(PAGE_TABLE_SIZE);
    if (frame == 0){
        Console::puts("Failed to get frame\n");
        //assert(false);
    }

    addr = frame * PAGE_SIZE;
    page_table[pt_addr] = addr;
    page_table[pt_addr] = page_table[pt_addr] | PTE_PRESENT | PTE_WRITE;

    Console::puts("Handled page fault\n");
}

// New in MP4
void PageTable::register_pool(VMPool * _vm_pool){
    vm_pool_no++;
    registered_vm_pools[vm_pool_no] = _vm_pool;
    Console::puts("registered VM pool\n");
}

void PageTable::free_page(unsigned long _page_no){
    unsigned long mask = 0x3FF; // 11 1111 1111
    unsigned long pd_addr = (_page_no >> PD_SHIFT) & mask;
    unsigned long pt_addr = (_page_no >> PT_SHIFT) & mask;
    unsigned long* page_table = (unsigned long *) ((0xFFC00000) | (pd_addr << 12));
    
    // Get corresponding frame of given page to release and mark invalid
    int frame_no = page_table[pt_addr] / PAGE_SIZE;
    page_table[pt_addr] &= ~PTE_PRESENT;
    ContFramePool::release_frames(frame_no);

    Console::puts("freed page\n");
}
