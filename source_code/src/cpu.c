
#include "cpu.h"
#include "mem.h"

static int calc(struct pcb_t * proc) {

	/* Return 0 */
	return ((unsigned long)proc & 0UL);
}

static int alloc(struct pcb_t * proc, uint32_t size, uint32_t reg_index) {
	if(proc->regs[reg_index] != 0) {
		free_mem(proc->regs[reg_index], proc);
	}
	addr_t addr = alloc_mem(size, proc);
	if (addr == 0) {
		return 1;
	}else{
		proc->regs[reg_index] = addr;
		return 0;
	}
}

static int free_data(struct pcb_t * proc, uint32_t reg_index) {
	int check = free_mem(proc->regs[reg_index], proc);
	if(check == 0)
		proc->regs[reg_index] = 0;
	return check;
}

static int read(
		struct pcb_t * proc, // Process executing the instruction
		uint32_t source, // Index of source register
		uint32_t offset, // Source address = [source] + [offset]
		uint32_t destination) { // Index of destination register
	
	BYTE data;
	if (read_mem(proc->regs[source] + offset, proc,	&data)) {
		proc->regs[destination] = data;
		return 0;		
	}else{
		return 1;
	}
}

static int write(
		struct pcb_t * proc, // Process executing the instruction
		BYTE data, // Data to be wrttien into memory
		uint32_t destination, // Index of destination register
		uint32_t offset) { 	// Destination address =
					// [destination] + [offset]
	return write_mem(proc->regs[destination] + offset, proc, data);
} 

void clean_mem(struct pcb_t * proc) {
	for(uint32_t i = 0; i < 10; i ++) {
		free_mem(proc->regs[i], proc);
	}
}

int run(struct pcb_t * proc) {
	/* Check if Program Counter point to the proper instruction */
	if (proc->pc >= proc->code->size) {
		return 1;
	}

	/* Get the current instruction */
	struct inst_t ins = proc->code->text[proc->pc];
	proc->pc++;
	int stat = 1;
	switch (ins.opcode) {
	case CALC:
		stat = calc(proc);
		break;
	case ALLOC:
		stat = alloc(proc, ins.arg_0, ins.arg_1);
		break;
	case FREE:
		stat = free_data(proc, ins.arg_0);
		break;
	case READ:
		stat = read(proc, ins.arg_0, ins.arg_1, ins.arg_2);
		break;
	case WRITE:
		stat = write(proc, ins.arg_0, ins.arg_1, ins.arg_2);
		break;
	default:
		stat = 1;
	}
	return stat;
}

int run_new(struct pcb_t * proc, int id) {
	/* Check if Program Counter point to the proper instruction */
	if (proc->pc >= proc->code->size) {
		return 1;
	}

	/* Get the current instruction */
	struct inst_t ins = proc->code->text[proc->pc];
	proc->pc++;
	int stat = 1;
	LOG_INFO(
		printf("\tCPU %d - PID %d - PC: %d - Type: %d\n", id, proc->pid, proc->pc - 1, ins.opcode);
	);
	switch (ins.opcode) {
	case CALC:
		// LOG_INFO(
		// 	printf("Calc\n");
		// );
		stat = calc(proc);
		break;
	case ALLOC:
		// LOG_INFO(
		// 	printf("Alloc\n");
		// );
		stat = alloc(proc, ins.arg_0, ins.arg_1);
		break;
	case FREE:
		// LOG_INFO(
		// 	printf("Free\n");
		// );
		stat = free_data(proc, ins.arg_0);
		break;
	case READ:
		// LOG_INFO(
		// 	printf("Read\n");
		// );
		stat = read(proc, ins.arg_0, ins.arg_1, ins.arg_2);
		break;
	case WRITE:
		// LOG_INFO(
		// 	printf("Write\n");
		// );
		stat = write(proc, ins.arg_0, ins.arg_1, ins.arg_2);
		break;
	default:
		stat = 1;
	}
	return stat;
}


