/*	CPU Module Routines */
/* 	Maxwell Crawford
	Date: 2-29-16
*/

#include "cpu.h"
#include "queue.h"
#include <stdlib.h>
#include <math.h>

int compareToID(PCB *pcb1, PCB *pcb2);
int compareToPri(PCB *pcb1, PCB *pcb2);
char *toString(PCB *pcb);

//main stuff:
Queue myQueue;
QueueNode *p;
int i;
PCB *pcb;
int base_priority = 1; //init to 1 at start, PCB's start here
int priorities[5] = {-1, 0, 1, 2, 3}; //preset priorities, highest to lowest
int Quanta[5] = {10, 18, 25, 22, 18}; //range of Quantum
double alpha; //allow some fluctuation, but closer to last burst
double estimate_temp; //hold estimate calc, then convert using ceil()
int last_estimate = 5; //hold previous burst estimate ( t(i+1)=ATi + (1-A)ti )
int next_estimate = 5;
int q = 20; //quantum placeholder
// -- 
//PCB comparison routines:
int compareToID(PCB *pcb1, PCB *pcb2)
{
    return pcb1->pcb_id - pcb2->pcb_id;
}

int compareToPri(PCB *pcb1, PCB *pcb2)
{
    return pcb1->priority - pcb2->priority; //lower number = higher
}

//Last estimate routine:
void getEstimate(PCB *pcb)
{
	if (pcb == NULL)
	{
		return;
	}

	else if (pcb->last_cpuburst <= 0)
	{
		return;
	}

	last_estimate = next_estimate; //set to our last guess
	estimate_temp = (alpha*pcb->last_cpuburst + (1-alpha)*last_estimate);
	next_estimate = ceil(estimate_temp);
}

//CPU Module routines:
void cpu_init()
{
	initQueue(&myQueue);
	pcb = NULL;
	base_priority = 1;
	alpha = 0.95;
	last_estimate = 5; //init t0 for first estimate
}

void insert_ready(PCB *pcb)
{
	//Set priority of process:
	if (pcb->accumulated_cpu == 0)
	{
		pcb->priority = base_priority;
	}

	if (pcb->status == running)
	{
		if (Int_Vector.cause == timeint) //time int means used up time slot
		{
			//Set priority based on last burst:
			getEstimate(pcb);
			if (next_estimate > 25)
			{
				q = Quanta[4];
				pcb->priority = priorities[4]; //lowest pri
			}

			else if (next_estimate > 21)
			{
				q = Quanta[3];
				pcb->priority = priorities[3];
			}

			else if (next_estimate > 17)
			{
				q = Quanta[2];
				pcb->priority = priorities[2];
			}

			else if (pcb->priority < 3) //don't go higher than 3 p-val
			{
				pcb->priority +=1;
			}
			
		}
	}

	if (pcb->status == waiting) //finished I/O
	{
		//Set priority based on last burst:
		getEstimate(pcb);
		if (next_estimate < 6) 
		{
			q = Quanta[1];
			pcb->priority = priorities[0]; //highest pri
		}

		else if (next_estimate < 11)
		{
			q = Quanta[0];
			pcb->priority = priorities[1];
		}

		else if (pcb->priority > -1) //don't go lower than -1 p-val
		{
			pcb->priority -=1;
		}
	}

	//Ready the process
	pcb->status = ready;

	//Add process to Q, smallest p-val at front
	enQueueSorted(&myQueue, pcb, compareToPri);
	return;
}

void dispatch()
{
	//-- Determine which process gets the CPU next! --

	//CHECK IF PTBR IS INITIALIZED and grab process' node:
	if (PTBR == NULL)
	{

        p = frontNode(&myQueue); //pcb=p's data
		deQueue(&myQueue);
		

		// 4) No more processes to run, Nullify and return:
		if (p == NULL)
		{
			PTBR = NULL;
			return;
		}

		// 5) Next process to run:
		if (p != NULL)
		{
			pcb = p->data; //PCB is stored in node's data pointer!

			pcb->status = running; //run next process
			PTBR = pcb->page_tbl; //reset page table ptr
			prepage(pcb); //load into mem
			pcb->last_dispatch = get_clock(); //reset dispatch time
			if (______trace_switch) //DEBUG
			{
	            printf("CLOCK> %6d# CPU: dispatch(pcb=%d), Process dispatched!\n",
	            get_clock(),PTBR->pcb->pcb_id);
	        }
			set_timer(q); //restart timer
			return;		
		}
	} //end null ptbr

	//DONE w/ NULL CASE

	// 1) Currently running, Quantum NOT expired:
	if (pcb->status == running)
	{
		
		// 2) Currently running, Quantum expired:
		if (Int_Vector.cause == timeint)
		{
			insert_ready(pcb);
		}

		//Quantum NOT expired:
		else
		{
			pcb->last_dispatch = get_clock();
			if (______trace_switch) //DEBUG
			{
                printf("CLOCK> %6d# CPU: dispatch(pcb=%d), Process dispatched!\n",
                get_clock(),PTBR->pcb->pcb_id);
            }
			return;
		}
		
	}

	p = frontNode(&myQueue);
	deQueue(&myQueue);
	
	if (p == NULL)
	{
		PTBR = NULL;
		return;
	}

	if (p != NULL)
	{
		pcb = p->data; //PCB is stored in node's data pointer!

		pcb->status = running; //run next process
		PTBR = pcb->page_tbl; //reset page table ptr
		prepage(pcb); //load into mem
		pcb->last_dispatch = get_clock(); //reset dispatch time
		if (______trace_switch) //DEBUG
		{
            printf("CLOCK> %6d# CPU: dispatch(pcb=%d), Process dispatched!\n",
            get_clock(),PTBR->pcb->pcb_id);
        }
		set_timer(q); //restart timer
		return;		
	}

	return;
}

char *toString(PCB *pcb)
{
	static char result[BUFSIZ];
    sprintf (result, "PCB ID: %d(Priority = %d, Last Dispatch = %d) ", pcb->pcb_id, pcb->priority, 
    	pcb->last_dispatch);
    return result;
}

