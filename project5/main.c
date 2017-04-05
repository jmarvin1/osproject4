/*
 Main program for the virtual memory project.
 Make all of your modifications to this file.
 You may add or rearrange any code or data as you need.
 The header files page_table.h and disk.h explain
 how to use the page table and disk interfaces.
 */

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

/* Global Variables */
struct disk * disk; //the disk
const char * algorithm; //argv[3] - the sorting algorithm to use
int * frame_track;
char * virtmem;
char * physmem;

void page_fault_handler( struct page_table *pt, int page )
{
	//bits = <PROT_READ|PROT_WRITE|PROT_EXEC>
	
	/* Page Table Functions */
	//void page_table_set_entry(pt, int page, int frame, int bits);
	//void page_table_get_entry(pt, int page, int *frame, int *bits);
	//void page_table_print_entry(pt, int page);
	//void page_table_print(pt);
	//int page_table_get_nframes(pt);
	//int page_table_get_npages(pt);
	//char * page_table_get_virtmem(pt);
	//char * page_table_get_physmem(pt);
	
	/* Disk Functions */
	//void disk_write(disk, int block, char * data);
	//void disk_read(disk, int block, char * data);
	//int disk_nblocks(disk);
	
	//printf("page fault on page #%d\n",page);
	
	//Run replacement algorithm based on input argument
	if (strcmp(algorithm,"rand") == 0) { //--------------------------------------------------------------------
		//random replacement
		
		//give write permission if required and missing
		int pframe = -1;
		int pbits = -1;
		page_table_get_entry(pt, page, &pframe, &pbits);
		if (pbits == 1) { //PROT_READ
			//page requires write permissions
			page_table_set_entry(pt, page, pframe, 3);
			return;
		}
		
		//get random page to overwrite
		int nframes = page_table_get_nframes(pt);
		int newframe = rand() % nframes;
		int oldpage = frame_track[newframe];
		//look if an empty frame is available instead
		int i;
		for (i=0; i<nframes; i++) {
			if (frame_track[i] == -1) {
				//empty place found in frame, change newframe and indicate than no page needs to be overwritten
				newframe = i;
				oldpage= -1;
				break;
			}
		}
		//save old page to disk if necissary
		if (oldpage != -1) {
			page_table_get_entry(pt, oldpage, &pframe, &pbits);
			if (pbits == 3) { //PROT_READ|PROT_WRITE
				disk_write(disk, oldpage, &physmem[newframe*PAGE_SIZE]);
			}
		}
		//read page into physical memory and update frame tracker
		disk_read(disk, page, &physmem[newframe*PAGE_SIZE]);
		frame_track[newframe] = page;
		//update page table for old page if one was overwritten
		if (oldpage != -1) {
			page_table_set_entry(pt, oldpage, 0, 0);
		}
		//update page table for new page
		page_table_set_entry(pt, page, newframe, 1);
		
	}
	else if (strcmp(algorithm,"fifo") == 0) { //--------------------------------------------------------------------
		//fifo replacement
		
	}
	else if (strcmp(algorithm,"custom") == 0) { //--------------------------------------------------------------------
		//custom replacement
		
	}
	else if (strcmp(algorithm,"test") == 0) {//--------------------------------------------------------------------
		//Generic Solution - Will Always Fault, but Produces Correct Answer for Testing Purposes
		page_table_set_entry(pt,page,page,PROT_READ|PROT_WRITE);
	}
	else {
		printf("error: invalid replacement algorithm\nselect from: <rand|fifo|custom|test>\n");
		exit(1);
	}
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom|test> <sort|scan|focus>\n");
		return 1;
	}
	
	//initialize random number generator
	time_t t;
	srand((unsigned) time(&t));
	
	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	algorithm = argv[3]; //global
	const char *program = argv[4];
	
	disk = disk_open("myvirtualdisk",npages); //global
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}
	
	
	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}
	
	int frame_tracking [nframes];
	int i;
	for (i=0;i<nframes;i++) {
		frame_tracking[i] = -1;
	}
	frame_track = frame_tracking; //global
	
	virtmem = page_table_get_virtmem(pt); //global
	
	physmem = page_table_get_physmem(pt); //global
	
	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);
		
	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);
		
	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);
		
	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);
		return 1;
	}
	
	page_table_delete(pt);
	disk_close(disk);
	
	return 0;
}
