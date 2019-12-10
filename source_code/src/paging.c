
#include "mem.h"
#include "cpu.h"
#include "loader.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv) {
	if (argc < 2) {
		printf("Cannot find input process\n");
		exit(1);
	}
	struct pcb_t * proc = load(argv[1]);
	unsigned int i;
	for (i = 0; i < proc->code->size; i++) {
		// run(proc);
		run_new(proc, 0);
	}
	LOG_INFO(
		printf("____________FINAL____________\n");
	)
	dump();
	return 0;
}

