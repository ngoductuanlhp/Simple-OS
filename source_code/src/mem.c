
#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct {
	uint32_t proc;	// ID of process currently uses this page
	int index;	// Index of the page in the list of pages allocated
			// to the process.
	int next;	// The next page in the list. -1 if it is the last
			// page.
} _mem_stat [NUM_PAGES];

// mutex for modify _mem_stat
static pthread_mutex_t mem_lock;

// mutex for modfify or read _ram
static pthread_mutex_t ram_lock;

void init_mem(void) {
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr) {
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr) {
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr) {
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct page_table_t * get_page_table(
		addr_t index, 	// Segment level index
		struct seg_table_t * seg_table) // first level table
	{ 
	
	/*
	 * TODO: Given the Segment index [index], you must go through each
	 * row of the segment table [seg_table] and check if the v_index
	 * field of the row is equal to the index
	 *
	 * */

	if(seg_table == NULL)
		return NULL;

	for (int i = 0; i < seg_table->size; i++) {
		// Enter your code here
		if(seg_table->table[i].v_index == index)
			return seg_table->table[i].pages;
	}
	return NULL;

}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
		addr_t virtual_addr, 	// Given virtual address
		addr_t * physical_addr, // Physical address to be returned
		struct pcb_t * proc) {  // Process uses given virtual address

	/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr);
	
	/* Search in the first level */
	struct page_table_t * page_table = NULL;
	page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL) {
		return 0;
	}

	int i;
	for (i = 0; i < page_table->size; i++) {
		if (page_table->table[i].v_index == second_lv) {
			/* TODO: Concatenate the offset of the virtual addess
			 * to [p_index] field of page_table->table[i] to 
			 * produce the correct physical address and save it to
			 * [*physical_addr]  */
			*physical_addr = (page_table->table[i].p_index << OFFSET_LEN) | offset;
			return 1;
		}
	}
	return 0;	
}

addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */

	// (size % PAGE_SIZE) ??????????? //
	// uint32_t num_pages = (size % PAGE_SIZE) ? size / PAGE_SIZE :
	// 	(size / PAGE_SIZE) + 1; // Number of pages we will use
	uint32_t num_pages = (size % PAGE_SIZE == 0) ? size / PAGE_SIZE :
		(size / PAGE_SIZE) + 1; // Number of pages we will use
	int mem_avail = 0; // We could allocate new memory region or not?

	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */
	int avail_pages = 0;
	for(int i = 0; i < NUM_PAGES; i++) {
		if(_mem_stat[i].proc == 0) {
			avail_pages++;
			if(avail_pages == num_pages)
				break;
		}
	}

	// Not enough free pages in physical memory
	if(avail_pages < num_pages)
		return 0;
	
	// proc->bp manages the current size of the heap segment, if proc->bp + size_of_new_alloc_mem > RAM
	// => Exceed virtual memory
	if(proc->bp + num_pages * PAGE_SIZE > RAM_SIZE)
		return 0;

	mem_avail = 1;
	
	if (mem_avail) {
		/* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		proc->bp += num_pages * PAGE_SIZE;
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */
		int allocated_idx = 0;
		int last_allocated_idx = -1;
		for(int i = 0; i < NUM_PAGES; i++) {
			if(_mem_stat[i].proc == 0) {
				_mem_stat[i].proc = proc->pid;
				_mem_stat[i].index = allocated_idx;
				// allocated_idx++;
				if(last_allocated_idx != -1) {
					_mem_stat[last_allocated_idx].next = i;
				}
				last_allocated_idx = i;
				
				// Virtual address of current allocated page
				addr_t v_address = ret_mem + allocated_idx * PAGE_SIZE;
				addr_t v_segment = get_first_lv(v_address);

				struct page_table_t* page_table = get_page_table(v_segment, proc->seg_table);
				if(page_table == NULL) {
					// Create new page table in segment table
					int page_table_idx = proc->seg_table->size;
					proc->seg_table->table[page_table_idx].v_index = v_segment;
					proc->seg_table->table[page_table_idx].pages = (struct page_table_t*) malloc(sizeof(struct page_table_t));
					page_table = proc->seg_table->table[page_table_idx].pages;
					proc->seg_table->size++;
				}
				int page_idx = page_table->size;
				page_table->size++;
				page_table->table[page_idx].v_index = get_second_lv(v_address);
				page_table->table[page_idx].p_index = i;

				if(allocated_idx + 1 == num_pages) {
					_mem_stat[i].next = -1;
					break;
				}
				allocated_idx++;
			}
		}

	}
	pthread_mutex_unlock(&mem_lock);
	LOG_INFO(
		printf("__________ALLOCATE__________\n");
		printf("No. of pages: 	%d\n", num_pages);
		dump();
	);
	// Virtual address of the first byte of the new allocated region
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t * proc) {
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */
	pthread_mutex_lock(&mem_lock);
	addr_t v_address = address;
	addr_t p_address = 0;

	// Translate v_address to p_address
	if(!translate(v_address, &p_address, proc))
		return 1;

	// Clear physical pages
	int num_free_pages = 0;
	addr_t physical_page_idx = p_address >> OFFSET_LEN;
	while(physical_page_idx != -1) {
		_mem_stat[physical_page_idx].proc = 0;
		physical_page_idx = _mem_stat[physical_page_idx].next;
		num_free_pages++;
	}

	// Clear virtual pages
	for(int i = 0; i < num_free_pages; i++) {
		addr_t current_v_address = v_address + i * PAGE_SIZE;
		addr_t v_segment = get_first_lv(current_v_address);
		addr_t v_page = get_second_lv(current_v_address);

		struct page_table_t* page_table = get_page_table(v_segment, proc->seg_table);
		if(page_table != NULL) {
			for(int j = 0; j < page_table->size; j++) {
				if(page_table->table[j].v_index == v_page) {
					// Swap this entry with the last entry and delete
					int last_entry = page_table->size - 1;
					page_table->table[j].v_index = page_table->table[last_entry].v_index;
					page_table->table[j].p_index = page_table->table[last_entry].p_index;
					// page_table->table[j].v_index = 0;
					// page_table->table[j].p_index = 0;
					page_table->size--;
					break;
				}
			}
		}

		// Remove the page table
		if(page_table->size == 0) {
			for(int j = 0; j < proc->seg_table->size; j++) {
				if(proc->seg_table->table[j].v_index == v_segment) {
					// Swap this entry with the last entry and delete
					int last_entry = proc->seg_table->size - 1;
					proc->seg_table->table[j] = proc->seg_table->table[last_entry];
					proc->seg_table->table[last_entry].v_index = 0;
					free(proc->seg_table->table[last_entry].pages);
					proc->seg_table->size--;
				}
			}
		}
	}

	// Update break point
	if(v_address + num_free_pages * PAGE_SIZE == proc->bp) {
		proc->bp = v_address;
	}
	pthread_mutex_unlock(&mem_lock);
	LOG_INFO(
		printf("_________DEALLOCATE_________\n");
		printf("No. of free pages: 	%d\n", num_free_pages);
		dump();
	);
	return 0;
}

int read_mem(addr_t address, struct pcb_t * proc, BYTE * data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		pthread_mutex_lock(&ram_lock);
		*data = _ram[physical_addr];
		pthread_mutex_unlock(&ram_lock);
		return 0;
	}else{
		return 1;
	}
}

int write_mem(addr_t address, struct pcb_t * proc, BYTE data) {
	addr_t physical_addr;
	LOG_INFO(
		printf("V_address: %05x\n", address);
	);
	if (translate(address, &physical_addr, proc)) {
		// Add mutexlock
		pthread_mutex_lock(&ram_lock);
		_ram[physical_addr] = data;
		pthread_mutex_unlock(&ram_lock);
		LOG_INFO(
			printf("_________WRITE DATA__________\n");
			printf("Write %02x at %05x\n", data, physical_addr);
		);
		return 0;
	}else{
		return 1;
	}
}

void dump(void) {
	int i;
	for (i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc != 0) {
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				i << OFFSET_LEN,
				((i + 1) << OFFSET_LEN) - 1,
				_mem_stat[i].proc,
				_mem_stat[i].index,
				_mem_stat[i].next
			);
			int j;
			for (	j = i << OFFSET_LEN;
				j < ((i+1) << OFFSET_LEN) - 1;
				j++) {
				
				if (_ram[j] != 0) {
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
					
			}
		}
	}
}


