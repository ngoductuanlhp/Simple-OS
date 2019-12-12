
#ifndef CPU_H
#define CPU_H

#include "common.h"
#include <stdlib.h>
#include <stdio.h>

/* Execute an instruction of a process. Return 0
 * if the instruction is executed successfully.
 * Otherwise, return 1. */
int run(struct pcb_t * proc);
int run_new(struct pcb_t *proc, int id);
void clean_mem(struct pcb_t * proc);

#endif

