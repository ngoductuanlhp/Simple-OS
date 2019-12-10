
#include "cpu.h"
#include "timer.h"
#include "sched.h"
#include "loader.h"
#include "mem.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int time_slot;
static int num_cpus;
static int done = 0;

static struct ld_args{
	char ** path; //paths to load processes
	unsigned long * start_time; //Arrival time array of processes
} ld_processes;
int num_processes;

struct cpu_args {
	struct timer_id_t * timer_id;
	int id;
};

static void * cpu_routine(void * args) {
	struct timer_id_t * timer_id = ((struct cpu_args*)args)->timer_id;
	int id = ((struct cpu_args*)args)->id;
	/* Check for new process in ready queue */
	int time_left = 0;
	struct pcb_t * proc = NULL;
	while (1) {
		/* Check the status of current process */
		if (proc == NULL) {
			/* No process is running, the we load new process from
		 	* ready queue */
		 	print_status_queue(); // Print curretn processes in ready_queue
			proc = get_proc();
		}else if (proc->pc == proc->code->size) {
			/* The porcess has finish it job */
			printf("\tCPU %d: Processed %2d has finished\n",
				id ,proc->pid);
			free(proc);
			print_status_queue(); // Print curretn processes in ready_queue
			proc = get_proc(); 
			time_left = 0;
		}else if (time_left == 0) {
			/* The process has done its job in current time slot */
			printf("\tCPU %d: Put process %2d to run queue\n",
				id, proc->pid);
			put_proc(proc); // Put current process to run_queue
			print_status_queue(); // Print curretn processes in ready_queue
			proc = get_proc(); // Get new process from ready_queue
		}
		
		/* Recheck process status after loading new process */
		if (proc == NULL && done) { // Loaded all processes anh no process in ready_queue and run_queue
			/* No process to run, exit */
			printf("\tCPU %d stopped\n", id);
			break;
		}else if (proc == NULL) {
			/* There may be new processes to run in
			 * next time slots, just skip current slot */
			next_slot(timer_id);
			continue;
		}else if (time_left == 0) {
			/* Dispatch new process, restart the quantum time for this process*/
			printf("\tCPU %d: Dispatched process %2d\n",
				id, proc->pid);
			time_left = time_slot;
		}
		
		/* Run current process */
		run_new(proc, id);
		time_left--;
		next_slot(timer_id);
	}
	detach_event(timer_id);
	pthread_exit(NULL);
}

static void * ld_routine(void * args) {
	struct timer_id_t * timer_id = (struct timer_id_t*)args;
	int i = 0;
	/*Load one by one process and put it to ready queue*/
	while (i < num_processes) {
		/*Initialize a proc and load its data into RAM*/
		struct pcb_t * proc = load(ld_processes.path[i]);

		/*Wait to the arrival time of this process to put it to ready queue*/
		while (current_time() < ld_processes.start_time[i]) {
			next_slot(timer_id);
		}
		printf("\tLoaded a process at %s, PID: %d\n",
			ld_processes.path[i], proc->pid);

		/*Put the process to ready queue*/
		add_proc(proc);

		/*Deallocate the process path*/
		free(ld_processes.path[i]);
		i++;
		/*Wait to next time slot*/
		next_slot(timer_id);
	}

	/*Loaded all processes*/
	free(ld_processes.path);
	free(ld_processes.start_time);
	done = 1; //Turn the flag done
	detach_event(timer_id);
	pthread_exit(NULL);
}

static void read_config(const char * path) {
	FILE * file;
	if ((file = fopen(path, "r")) == NULL) {
		printf("Cannot find configure file at %s\n", path);
		exit(1);
	}

	/*Read the first line to get: quantum time, number of CPUs, numer of processes to be loaded*/
	fscanf(file, "%d %d %d\n", &time_slot, &num_cpus, &num_processes);
	
	
	ld_processes.path = (char**)malloc(sizeof(char*) * num_processes);
	ld_processes.start_time = (unsigned long*)
		malloc(sizeof(unsigned long) * num_processes);

	/*Read path of each process to be loaded and store these paths in ld_processes*/
	int i;
	for (i = 0; i < num_processes; i++) {
		ld_processes.path[i] = (char*)malloc(sizeof(char) * 100);
		ld_processes.path[i][0] = '\0';
		strcat(ld_processes.path[i], "input/proc/");
		char proc[100];
		fscanf(file, "%lu %s\n", &ld_processes.start_time[i], proc);

		//Path to load process has the form: "input/proc/<name>"
		strcat(ld_processes.path[i], proc);
	}
}

int main(int argc, char * argv[]) {
	/*Input 2 args: executed file name, config path*/
	/* Read config */
	if (argc != 2) {
		printf("Usage: os [path to configure file]\n");
		return 1;
	}
	char path[100];
	path[0] = '\0';

	/*Config path has the form: "input/<name>"*/
	strcat(path, "input/");
	strcat(path, argv[1]);
	read_config(path);

	/*Create threads for CPUs and a thread for loader*/
	pthread_t * cpu = (pthread_t*)malloc(num_cpus * sizeof(pthread_t));
	struct cpu_args * args =
		(struct cpu_args*)malloc(sizeof(struct cpu_args) * num_cpus);
	pthread_t ld;
	
	/* Init timer */
	int i;
	for (i = 0; i < num_cpus; i++) {
		args[i].timer_id = attach_event();
		args[i].id = i;
	}
	struct timer_id_t * ld_event = attach_event();
	start_timer();

	/* Init scheduler */
	init_scheduler();

	/* Run CPU and loader */
	pthread_create(&ld, NULL, ld_routine, (void*)ld_event);
	for (i = 0; i < num_cpus; i++) {
		pthread_create(&cpu[i], NULL,
			cpu_routine, (void*)&args[i]);
	}

	/* Wait for CPU and loader finishing */
	for (i = 0; i < num_cpus; i++) {
		pthread_join(cpu[i], NULL);
	}
	pthread_join(ld, NULL);

	/* Stop timer */
	stop_timer();


	printf("\nMEMORY CONTENT: \n");
	dump();

	return 0;

}



