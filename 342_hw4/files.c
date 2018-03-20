//Maxwell Crawford
//4-12-2016 8:12 PM
#include <stdio.h>
#include "files.h"

#define MAX_OPENFILE 10

//GIVEN VARS
FILE_DIR_ENTRY theDirectory[MAX_OPENFILE];
BOOL freeDirEntry[MAX_OPENFILE];
INODE inodesTbl[MAX_OPENFILE];
int numFreeBlocks[MAX_DEV];

//MY VARS
OFILE *current_ofile;
int current_page_id;
IORB *current_iorb;
INODE *current_inode;
DEV_ENTRY current_dev;
int current_dev_id;
PCB *current_pcb;
int i;

// -- START MAIN FUNCTIONS --

void files_init()
{
	// 1)
	for (i=0; i<MAX_OPENFILE; i++)
	{
		theDirectory[i].filename = NULL;

		// 2)
		theDirectory[i].inode = &inodesTbl[i];

		// 3)
		inodesTbl[i].inode_id = i;

		// 4)
		freeDirEntry[i] = true;
	}
	
	for (i=0; i<MAX_DEV; i++)
	{
		// 5)
		numFreeBlocks[i] = MAX_BLOCK;

		// 6)
		int b;
		for (b=0; b<MAX_BLOCK; b++)
		{
			Dev_Tbl[i].free_blocks[b] = true;
		}
		
	}

}

void openf(char *filename, OFILE *file)
{
	current_ofile = file;
	int file_index = -1; //default=not found

	// 1)
	file_index = search_file(filename);
	if (file_index != -1)
	{
		current_inode = theDirectory[file_index].inode;
	}

	// 2)
	else if (file_index == -1)
	{
		file_index = new_file(filename);
		current_inode = theDirectory[file_index].inode;
	}

	// 3)
	current_ofile->inode = current_inode;
	current_ofile->dev_id = current_inode->dev_id;
	current_ofile->iorb_count = 0;
	current_inode->count += 1;
}

EXIT_CODE closef(OFILE *file)
{
	current_ofile = file;
	current_inode = current_ofile->inode;

	// 1)
	if (current_ofile->iorb_count > 0)
	{
		return fail;
	}

	// 2)
	current_inode->count -= 1;

	// 3)
	if (current_inode->count == 0)
	{
		int current_dir_num = -1;
		for (i=0; i<MAX_OPENFILE; i++)
		{
			if (theDirectory[i].inode == current_inode)
			{
				current_dir_num = i; //store index
			}
		}

		delete_file(current_dir_num);
	}

	// 4)
	return ok;
}

EXIT_CODE readf(OFILE *file, int position, int page_id, IORB *iorb)
{
	// 1)
	current_pcb = PTBR->pcb;

	// 2)
	if ((position < 0) || (position >= file->inode->filesize))
	{
		iorb->dev_id = -1;
		return fail;
	}

	// 3)
	int logical_block_num = position / PAGE_SIZE;

	// 4)
	int physical_block_num = current_inode->allocated_blocks[logical_block_num];

	// 5)
	file->iorb_count +=1;

	// 6)
	iorb->dev_id = current_inode->dev_id;
	iorb->block_id = physical_block_num;
	iorb->action = read;
	iorb->page_id = page_id;
	iorb->pcb = current_pcb;
	iorb->file = file;

	// 7)
	// IO Request Interrupt:
	iorb->event->happened = false;
	Int_Vector.event = iorb->event;
	Int_Vector.iorb = iorb;
	Int_Vector.cause = iosvc;
	gen_int_handler();

	// 8)
	return ok;
}

EXIT_CODE writef(OFILE *file, int position, int page_id, IORB *iorb)
{
	// 1)
	current_pcb = PTBR->pcb;

	// 2)
	if (position < 0) //write can go beyond current filesize!!
	{
		iorb->dev_id = -1;
		return fail;
	}

	// 3)
	int logical_block_num = position / PAGE_SIZE;

	// 4)
	// Determine if file is empty/new first:
	int empty = 0;
	current_inode = file->inode;
	for (i=0; i<MAX_BLOCK; i++)
	{
		if (current_inode->allocated_blocks[i] == -1)
		{
			empty = 1;
		}

		else
		{
			empty = 0;
			break;
		}
	} //end empty check loop

	//Determine last block:
	int last_block = 0;

	if (empty)
	{
		last_block = -1;
	}

	else
	{
		last_block = (current_inode->filesize - 1) / PAGE_SIZE;
	}

	// 5)
	int blocks_needed = logical_block_num - last_block;

	//Allocate free blocks:
	if (logical_block_num > last_block)
	{
		EXIT_CODE current_code;
		current_code = allocate_blocks(current_inode, blocks_needed);

		if (current_code == fail)
		{
			iorb->dev_id = -1;
			return fail;
		}
	}

	// 6)
	if (current_inode->filesize <= position)
	{
		current_inode->filesize = (position+1);
	}

	// 7)
	int physical_block_num = current_inode->allocated_blocks[logical_block_num];

	// 8)
	file->iorb_count +=1;

	// 9)
	// Fill in IORB:
	iorb->dev_id = current_inode->dev_id;
	iorb->block_id = physical_block_num;
	iorb->action = write;
	iorb->page_id = page_id;
	iorb->pcb = current_pcb;
	iorb->file = file;

	// 10)
	// IO Request Interrupt:
	iorb->event->happened = false;
	Int_Vector.event = iorb->event;
	Int_Vector.iorb = iorb;
	Int_Vector.cause = iosvc;
	gen_int_handler();

	// 11)
	return ok;
}

void notify_files(IORB *iorb)
{
	iorb->file->iorb_count -= 1;
}

// -- START OPTIONAL FUNCTIONS --

EXIT_CODE allocate_blocks(INODE *inode, int numBlocksNeeded)
{
	current_inode = inode;
	current_dev_id = current_inode->dev_id;
	current_dev = Dev_Tbl[current_dev_id];

	// a)
	if (numBlocksNeeded > numFreeBlocks[current_dev_id])
	{
		return fail;
	}

	// b)
	int logical_block_num;
	if (current_inode->filesize == 0)
	{
		logical_block_num = 0;
	}

	else
	{
		logical_block_num = (current_inode->filesize - 1) / (PAGE_SIZE + 1)
	}

	// c)
	for (i=0; i<MAX_BLOCK; i++)
	{
		if (Dev_Tbl[current_dev_id].free_blocks[i])
		{
			Dev_Tbl[current_dev_id].free_blocks[i] = false;
			numFreeBlocks[current_dev_id] -= 1;
			current_inode->allocated_blocks[logical_block_num] = i;
			logical_block_num += 1;
		}

		else if (blocks_free == numBlocksNeeded)
		{
			break;
		}
	}

	// d)
	return ok;
}

int search_file(char *filename)
{
	int file_index = 0;
	for (i=0; i<MAX_OPENFILE; i++)
	{
		if (theDirectory[i].filename == filename)
		{
			file_index = i;
			break;
		}

		else
		{
			file_index = -1;
		}
	}

	return file_index;
}

int new_file(char *filename)
{
	// a)
	int current_dir_num = -1;
	for (i=0; i<MAX_OPENFILE; i++)
	{
		if (freeDirEntry[i])
		{
			current_dir_num = i; //store index
		}
	}

	if (current_dir_num == -1)
	{
		//ERROR:
		printf("\n\t\tError: No more free space for files.");
		return -1;
	}

	// b)
	FILE_DIR_ENTRY current_dir = theDirectory[current_dir_num];
	current_dir.filename = filename;

	// c)
	freeDirEntry[current_dir_num] = false;

	// d)
	current_dir.inode->filesize = 0;
	current_dir.inode->count = 0;

	// e)
	int biggest = 0;
	for (i=0; i< MAX_DEV; i++)
	{
		if (numFreeBlocks[i] > biggest)
		{
			biggest = i;
		}
	}

	current_dev_id = biggest;
	current_dir.inode->dev_id = current_dev_id;

	// f)
	for (i=0; i<MAX_BLOCK; i++)
	{
		current_inode->allocated_blocks[i] = -1;
	}

	// g)
	return current_dir_num;
}

void delete_file(int dirNum)
{
	FILE_DIR_ENTRY current_dir = theDirectory[dirNum];
	current_inode = current_dir.inode;
	current_dev_id = current_inode->dev_id;

	// 1)
	int physical_block_num; 
	for (i=0; i<MAX_BLOCK; i++)
	{
		if (current_inode->allocated_blocks[i] != -1)
		{
			physical_block_num = current_inode->allocated_blocks[i];
			Dev_Tbl[current_dev_id].free_blocks[physical_block_num] = true;
			numFreeBlocks[current_dev_id] +=1;
			current_inode->allocated_blocks[i] = -1;
		}
	}

	// 2)
	freeDirEntry[dirNum] = true;
	current_dir.filename = NULL;
}

// -- END OPTIONAL FUNCTIONS --

void print_dir_tbl()
{
	//filled in on server
}

void print_my_disk_map()
{
	//filled in on server
}

//end file module