//3/30/2016 10:05pm
#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include "queue.h"

//main stuff:
int compareToID(PCB *pcb1, PCB *pcb2);
Queue myQueue;
QueueNode *p;
int i;
PCB *current_pcb;
PCB *vic_pcb;
FRAME *current_frame;
FRAME *vic_frame;
PAGE_ENTRY current_page;
PAGE_ENTRY vic_page;
unsigned int current_page_id;
unsigned int current_offset;
unsigned int current_frame_id;
unsigned int physical_addr;

char *toString(FRAME *f)
{
	static char result[BUFSIZ];
	// For each from that is printed, print the frame number, pcb id of the 
	// process that owns it, the page number of the page it contains,
	// D or C if it is dirty or clean, and the lock count.

	if (f == NULL)
		sprintf(result, "(null) ");

	else if (f->hook == NULL)
	{
		sprintf(result, "Frame %d(pcb-%d,page-%d,%c,lock-%d,refbit-null) ",
			(f - &Frame_Tbl[0]),
			f->pcb->pcb_id, f->page_id, 
			(f->dirty ? 'D' : 'C'), f->lock_count);
	}

	else
	{
		sprintf(result, "Frame %d(pcb-%d,page-%d,%c,lock-%d,refbit-%d) ",
			(f - &Frame_Tbl[0]),
			f->pcb->pcb_id, f->page_id, 
			(f->dirty ? 'D' : 'C'), f->lock_count, *(f->hook));
	}

	return result;
}

int compareToID(PCB *pcb1, PCB *pcb2)
{
    return pcb1->pcb_id - pcb2->pcb_id;
}

void memory_init()
{
	initQueue(&myQueue);
	current_pcb = NULL;
	vic_pcb = NULL;
	current_page_id = 0;
	current_offset = 0;
	current_frame_id = 0;
	physical_addr = 0;

	//Init frame table:
	for (i=0; i<MAX_FRAME; i++)
	{
		Frame_Tbl[i].free = true;
		Frame_Tbl[i].pcb = NULL;
		//Frame_Tbl[i].page_id = -1;
		Frame_Tbl[i].dirty = false;
		Frame_Tbl[i]->hook = (int *)calloc(1,sizeof(int));
		Frame_Tbl[i]->hook = 0; //1?
	}
}

void prepage(PCB *pcb)
{
	current_pcb = pcb;
	int count = 0;

	for (i=0; i<MAX_PAGE; i++)
	{
		if (count >= 3)
		{
			break;
		}

		else if (current_pcb->page_tbl->page_entry[i].valid == false)
		{
			get_page(current_pcb, i);
			count += 1;
		}
	}
}

int start_cost(PCB *pcb)
{
	int swap_count = 0;
	current_pcb = pcb;
	for (i=0; i<MAX_PAGE; i++)
	{
		if (current_pcb->page_tbl->page_entry[i].valid == false)
		{
			swap_count += 1;
		}
	}

	return (swap_count * COST_OF_PAGE_TRANSFER);
}

void deallocate(PCB *pcb)
{
	current_pcb = pcb;

	for (i=0; i<MAX_FRAME; i++)
	{
		if (Frame_Tbl[i].pcb == current_pcb)
		{
			Frame_Tbl[i].free = true;
		}
	}

	int comparison = -1;
	setCurrent(&myQueue, frontNode(&myQueue)); //set pointer to head
	QueueNode *pnext = NULL; //temp nodes

	while (! isPastEnd(&myQueue))
	{
		p = getCurrentNode(&myQueue);
		if (getCurrentValue(&myQueue) != NULL)
			current_frame = getCurrentValue(&myQueue);
		else
		{
			advance(&myQueue);
			continue;
		}
		comparison = compareToID(current_frame->pcb, current_pcb);
		if (comparison == 0)
		{
			pnext = p->next;
			removeNode(&myQueue, p);
			setCurrent(&myQueue, pnext);
		}
		advance(&myQueue);
	}

	p = frontNode(&myQueue);
	setCurrent(&myQueue, p); //reset to front
}

void get_page(PCB *pcb, int page_id)
{
	current_pcb = pcb;
	current_page_id = page_id;

	// 1)
	int found = 0;
	current_frame_id = current_pcb->page_tbl->page_entry[current_page_id].frame_id;

	for (i=0; i<MAX_FRAME; i++)
	{
		if ((Frame_Tbl[i].free) && (Frame_Tbl[i].lock_count < 1))
		{
			found = 1;
			current_frame_id = i;
			*vic_frame = Frame_Tbl[current_frame_id]; //victim found
			vic_pcb = vic_frame->pcb;
			vic_page = vic_pcb->page_tbl->page_entry[vic_frame->page_id];
			break;
		}
		else
			found = 0;
	}

	// 2)
	while (found != 1)
	{
		if (frontValue(&myQueue) == NULL)
		{
			break; //if head is NULL, no valid frames
		}
		else
		{
			current_frame = frontValue(&myQueue);
			current_frame_id = current_frame->pcb->page_tbl->page_entry[current_page_id].frame_id;
		}

		deQueue(&myQueue);

		if ((*current_frame->hook) == 1)
		{
			*Frame_Tbl[current_frame_id].hook = 0;
			enQueue(&myQueue, &Frame_Tbl[current_frame_id]);
		}

		else if ((*current_frame->hook) == 0)
		{
			if (Frame_Tbl[current_frame_id].lock_count != 0)
				enQueue(&myQueue, &Frame_Tbl[current_frame_id]);
			else
			{
				current_frame_id = current_frame->pcb->page_tbl->page_entry[current_page_id].frame_id;
				*vic_frame = Frame_Tbl[current_frame_id]; //victim found
				vic_pcb = vic_frame->pcb;
				vic_page = vic_pcb->page_tbl->page_entry[vic_frame->page_id];
				found = 1;
			}
		}

	} //end search

	if (vic_frame.dirty)
	{
		vic_frame->lock_count = 1;
		siodrum(write, current_pcb, current_page_id, current_frame_id);
		vic_frame->dirty = false;
	}

	vic_page = vic_frame->pcb->page_tbl->page_entry[current_page_id];
	vic_page.valid = false;

	// 3)
	Frame_Tbl[current_frame_id].free = true;
	Frame_Tbl[current_frame_id].pcb = vic_pcb;
	Frame_Tbl[current_frame_id].page_id = vic_frame->page_id;
	Frame_Tbl[current_frame_id].dirty = false;

	vic_frame->lock_count = 1;

	siodrum(read, vic_pcb, vic_frame->page_id, vic_page.frame_id);

	vic_frame->lock_count = 0;

	current_pcb->page_tbl->page_entry[current_page_id].frame_id = current_frame_id;
	current_pcb->page_tbl->page_entry[current_page_id].valid = true;

	vic_frame->dirty = false;

	*vic_frame->hook = 1;

	enQueue(&myQueue, &vic_frame);

}

void lock_page(IORB *iorb)
{
	current_page_id = iorb->page_id;
	current_pcb = iorb->pcb;
	current_frame_id = current_pcb->page_tbl->page_entry[current_page_id].frame_id;
	*current_frame = Frame_Tbl[current_frame_id];

	if (current_pcb->page_tbl->page_entry[current_page_id].valid != true)
	{
		// Generate page fault:
		Int_Vector.cause = pagefault;
		Int_Vector.pcb = current_pcb;
		Int_Vector.page_id = current_page_id;
		gen_int_handler();
	}

	current_frame->lock_count +=1;

	if (iorb->action == read)
	{
		current_frame->dirty = true;
	}

	*Frame_Tbl[current_frame_id].hook = 1;
}

void unlock_page(IORB *iorb)
{
	current_page_id = iorb->page_id;
	current_pcb = iorb->pcb;
	current_frame_id = current_pcb->page_tbl->page_entry[current_page_id].frame_id;
	*current_frame = Frame_Tbl[current_frame_id];

	current_frame->lock_count -= 1;
}

void refer(int logic_addr, REFER_ACTION action)
{
	// 1)
	current_pcb = PTBR->pcb;

	// 2)
	current_page_id = (logic_addr)/(PAGE_SIZE);
	current_offset = (logic_addr)%(PAGE_SIZE);

	// 3)
	if ((current_page_id >= MAX_PAGE) || (current_offset >= PAGE_SIZE))
		return; //invalid addr

	// 4)
	if (current_pcb->page_tbl->page_entry[current_page_id].valid == false)
	{
		//Generate page fault;
		Int_Vector.cause = pagefault;
		Int_Vector.pcb = current_pcb;
		Int_Vector.page_id = current_page_id;
		gen_int_handler(); //page now loaded.
	}

	// 5)
	if (action == store)
	{
		current_frame_id = current_pcb->page_tbl->page_entry[current_page_id].frame_id;
		Frame_Tbl[current_frame_id].dirty = true;
	}

	*Frame_Tbl[current_frame_id].hook = 1;

	physical_addr = PTBR->page_entry[current_page_id].frame_id * PAGE_SIZE + current_offset;
}

//end memory module