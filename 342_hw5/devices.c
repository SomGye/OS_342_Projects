#include "queue.h"
#include "devices.h"
// Maxwell Crawford, 4-25-2016 10:34 PM

//Main vars:
Queue IORB_Queue[MAX_DEV];
QueueNode *p;
BOOL head_dir; //true=right, false=left!, default is right
int current_block_id;
int current_track;
int current_dev_id;
int i;

//Comparison function:
int compareToTrack(IORB *iorb1, IORB *iorb2)
{
	int block_id1 = iorb1->block_id;
	int block_id2 = iorb2->block_id;
	int track1 = (block_id1 * PAGE_SIZE) / TRACK_SIZE;
	int track2 = (block_id2 * PAGE_SIZE) / TRACK_SIZE;
	return track1-track2;
}

//Main functions:
void devices_init()
{
	
	for (i=0; i<MAX_DEV; i++)
	{
		initQueue(IORB_Queue[i]);
		Dev_Tbl[i].iorb = NULL;
		Dev_Tbl[i].busy = false;
	}

	head_dir = true;
}

void enq_io(IORB *iorb)
{
	current_dev_id = iorb->dev_id;
	enQueueSorted(&IORB_Queue[current_dev_id], iorb, compareToTrack);

	//Check if dev is busy:
	if (Dev_Tbl[current_dev_id]->busy == false)
	{
		lock_page(iorb);
		Dev_Tbl[current_dev_id].iorb = iorb;
		Dev_Tbl[current_dev_id].busy = true;
		siodev(iorb);
	}
}

void deq_io(IORB *iorb)
{
	current_dev_id = iorb->dev_id;
	QueueNode *newp; //temp for new node
	IORB *new_iorb;

	// 1)
	p = getCurrentNode(&IORB_Queue[current_dev_id]);

	// 2)
	unlock_page(iorb);

	// 3)
	notify_files(iorb);

	// 4)
	BOOL done = false;

	if (head_dir)
	{
		advance(&IORB_Queue[current_dev_id]);
	}

	else
	{
		retreat(&IORB_Queue[current_dev_id]);
	}

	removeNode(&IORB_Queue[current_dev_id], p);

	newp = getCurrentNode(&IORB_Queue[current_dev_id]); //store current location
	new_iorb = getCurrentValue(&IORB_Queue[current_dev_id]);

	if (isPastEnd(&IORB_Queue[current_dev_id]))
	{
		if (head_dir == false) //current=back, so new=forward
		{
			setCurrent(&IORB_Queue[current_dev_id], frontNode(&IORB_Queue[current_dev_id]));
		}

		else
		{
			setCurrent(&IORB_Queue[current_dev_id], backNode(&IORB_Queue[current_dev_id]));
		}
	}

	if (isPastEnd(&IORB_Queue[current_dev_id]))
		done = true; //queue is empty!

	// 5)
	if (done == false)
	{
		//new_iorb = getCurrentValue(&IORB_Queue[current_dev_id]);
		lock_page(new_iorb);
		Dev_Tbl[current_dev_id].iorb = new_iorb;
		Dev_Tbl[current_dev_id].busy = true;
		siodev(new_iorb);
	}

	else
	{
		Dev_Tbl[current_dev_id].busy = false;
		Dev_Tbl[current_dev_id].iorb = NULL;
	}

}

void purge_iorbs(PCB *pcb)
{
	IORB *current_iorb; //IORB of current dev_entry node
	IORB *temp_iorb;
	QueueNode *current;
	QueueNode *next; //temp for next node
	for (i=0; i<MAX_DEV; i++) //loop thru all devices
	{
		current_iorb = Dev_Tbl[i].iorb;
		current = getCurrentNode(&IORB_Queue[i]);
		temp_iorb = current->data;
		next = current->next;
		while (current != NULL)
		{
			if (temp_iorb != current_iorb)
			{
				if (isOwner(pcb, temp_iorb) == true)
				{
					notify_files(temp_iorb);
					removeNode(&IORB_Queue[i], current);
				}
			}

			current = next; //move up
			temp_iorb = current->data;
			next = current->next;
		}
	}
}

//Another function for seeing if this PCB is the owner of IORB
BOOL isOwner(PCB *pcb, IORB *iorb)
{
	if (iorb->pcb == pcb)
		return true;
	else
		return false;
}