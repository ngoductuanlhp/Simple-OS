#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

/*Put a new PCB to queue at tail*/
void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */	
	if(q->size >= MAX_QUEUE_SIZE) return;
	q->proc[q->size] = proc;
	q->size++;
}

/*Get the highest priority PCB out of queue*/
struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if(empty(q)) return NULL;
	//int currentPriority = q->proc[0]->priority;
	int idx = q->size - 1;
	for(int i = idx; i >= 0; i--) {
		if(q->proc[i]->priority > q->proc[idx]->priority) {
			idx = i;
		}
	}
	struct pcb_t *proc = q->proc[idx];
	for(int i = idx; i < q->size - 1; i++) {
		q->proc[i] = q->proc[i + 1];
	}
	q->size--;
	return proc;
}

/*get the PCB at the tail of the queue*/
struct pcb_t * dequeueAtTail(struct queue_t * q) {
	if(empty(q)) return NULL;
	struct pcb_t * proc = q->proc[q->size - 1];
	q->size--;
	return proc;
}

