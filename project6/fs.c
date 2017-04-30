// ---------- Source Files ----------
#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

// ---------- Macros ----------

#define FS_MAGIC           0xf0f03410
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024

// ---------- Global Variables ----------

char * free_block_bitmap = 0; // Usage:
									//	1 - block is in use
									//	0 - block is empty
int ninodeblocks = 0;

// ---------- Data Structures ----------

struct fs_superblock {
	int magic;
	int nblocks;
	int ninodeblocks;
	int ninodes;
};

struct fs_inode {
	int isvalid;
	int size;
	int direct[POINTERS_PER_INODE];
	int indirect;
};

union fs_block {
	struct fs_superblock super;
	struct fs_inode inode[INODES_PER_BLOCK];
	int pointers[POINTERS_PER_BLOCK];
	char data[DISK_BLOCK_SIZE];
};

// ---------- Helper Functions ----------

struct fs_inode inode_load( int inumber ) {
	// --- convert inumber to block number and offset ---
	int i; 
	int blocknum;// = 0;
	struct fs_inode inode;
	int offset = inumber % INODES_PER_BLOCK;
	for (i=1; i <=ninodeblocks; i++) {
		if (inumber < INODES_PER_BLOCK*i) {
			blocknum = i;
			break;
		}
	}
	// --- Load Block ---
	union fs_block block;
	disk_read(blocknum, block.data);
	// --- Load Data Into Inode Pointer ---
	inode.isvalid = block.inode[offset].isvalid;
	inode.size = block.inode[offset].size;
	for (i=0; i<POINTERS_PER_INODE; i++) {
		inode.direct[i] = block.inode[offset].direct[i];
	}
	inode.indirect = block.inode[offset].indirect;
	// --- Return Inode ---
	return inode;
}

void inode_save( int inumber, struct fs_inode inode ) {
	// --- convert inumber to block number and offset ---
	int i;
	int blocknum = 0;
	int offset = inumber % INODES_PER_BLOCK;
	for (i=1; i <=ninodeblocks; i++) {
		if (inumber < INODES_PER_BLOCK*i) {
			blocknum = i;
			break;
		}
	}
	// --- Load Block ---
	if (blocknum != 0) {
		union fs_block block;
		disk_read(blocknum, block.data);
		// --- Place inode into Block ---
		block.inode[offset].isvalid = inode.isvalid;
		block.inode[offset].size = inode.size;
		for (i=0; i<POINTERS_PER_INODE; i++) {
			block.inode[offset].direct[i] = inode.direct[i];
		}
		block.inode[offset].indirect = inode.indirect;
		// --- Write Block to Disk
		disk_write(blocknum, block.data);
	}
}

int claim_bitmap() {
	// --- return next availaible block from bitmap or 0 if bitmap full ---
	int block = 0;
	int i;
	int dsk_size = disk_size();
	for (i=0; i<dsk_size; i++) {
		if (free_block_bitmap[i] == 0) {
			free_block_bitmap[i] = 1;
			block = i;
			return block;
		}
	}
	return block;
}

// ---------- Primary FS Functions ----------

int fs_format()
{
	// --- Check for Existing Mounted Disk ---
	if (free_block_bitmap) {
		return 0;
	}
	// --- Create New Superblock ---
	union fs_block superblock;
	superblock.super.magic = FS_MAGIC;
	superblock.super.nblocks = disk_size();
	ninodeblocks = superblock.super.nblocks/10; // store in global for reference without loads
	if (ninodeblocks == 0) {
		ninodeblocks = 1; // ensure inodes can be created
	}
	superblock.super.ninodeblocks = ninodeblocks;
	superblock.super.ninodes = superblock.super.ninodeblocks * INODES_PER_BLOCK;
	// --- Write Superblock to Disk --- 
	disk_write(0, superblock.data);
	// --- Create Empty Inode Block ---
	union fs_block emptyinode;
	int i;
	for (i=0; i < INODES_PER_BLOCK; i++) {
		emptyinode.inode[i].isvalid = 0;
	}
	// --- Write Empty Inode Block into Every Allocated Inode Block ---
	for (i=1; i <= superblock.super.ninodeblocks; i++) {
		disk_write(i, emptyinode.data);
	}
	// --- Exit Successfully ---
	return 1;
}

void fs_debug()
{
	// --- Load Superblock ---
	union fs_block superblock;
	disk_read(0,superblock.data);
	printf("superblock:\n");
	if (superblock.super.magic == FS_MAGIC) {
		printf("    magic number is valid\n");
	}
	else {
		printf("    magic number is invalid\n");
	}
	printf("    %d blocks\n",superblock.super.nblocks);
	printf("    %d inode blocks\n",superblock.super.ninodeblocks);
	printf("    %d inodes\n",superblock.super.ninodes);
	// --- Load Inode Blocks ---
	int i, j, k;
	union fs_block block;
	union fs_block indirect;
	for (i=1; i <= superblock.super.ninodeblocks; i++) {
		disk_read(i, block.data);
		for (j=0; j < INODES_PER_BLOCK; j++) {
			// --- Only Print Valid Inodes ---
			if (block.inode[j].isvalid) {
				printf("inode %d\n", j);
				printf("    size: %d bytes\n", block.inode[j].size);
				printf("    direct blocks: ");
				// --- Print Valid Direct Pointers ---
				for (k=0; k < POINTERS_PER_INODE; k++) {
					if (block.inode[j].direct[k]) {
						printf("%d ", block.inode[j].direct[k]);
					}
				}
				printf("\n");
				// --- Follow Indirect Pointer To Find Pointers in Indirect Data Block ---
				if (block.inode[j].indirect) {
					printf("    indirect: %d\n", block.inode[j].indirect);
					disk_read(block.inode[j].indirect, indirect.data);
					printf("    indirect data blocks: ");
					// --- Print Valid Indirect Pointers ---
					for(k=0; k < POINTERS_PER_BLOCK; k++) {
						if (indirect.pointers[k]){
							printf("%d ", indirect.pointers[k]);
						}
					}
					printf("\n");
				}
			}
		}
	}
}

int fs_mount()
{
	// --- Read Superblock ---
	union fs_block superblock;
	disk_read(0, superblock.data);
	ninodeblocks = superblock.super.ninodeblocks;
	// --- Exit if Magic Number Invalid ---
	if (superblock.super.magic != FS_MAGIC) {
		return 0;
	}
	// --- Allocate Free Block Bitmap ---
	free_block_bitmap = malloc(sizeof(char) * superblock.super.nblocks);
	free_block_bitmap[0] = 1;
	int i, j, k;
	for (i = 1; i < superblock.super.nblocks; i++) {
		free_block_bitmap[i] = 0;
	}
	// --- Scan Through Inodes to Fill Free Block Bitmap ---
	union fs_block block;
	union fs_block indirect;
	for (i=1; i <= superblock.super.ninodeblocks; i++) {
		free_block_bitmap[i] = 1;
		disk_read(i, block.data);
		for (j=0; j < INODES_PER_BLOCK; j++) {
			// --- Only Examine Valid Inodes ---
			if (block.inode[j].isvalid) {
				// --- Mark Valid Direct Pointers ---
				for (k=0; k < POINTERS_PER_INODE; k++) {
					if (block.inode[j].direct[k]) {
						free_block_bitmap[block.inode[j].direct[k]] = 1;
					}
				}
				// --- Follow Indirect Pointer To Find Pointers in Indirect Data Block ---
				if (block.inode[j].indirect) {
					free_block_bitmap[block.inode[j].indirect] = 1;
					disk_read(block.inode[j].indirect, indirect.data);
					// --- Print Valid Indirect Pointers ---
					for(k=0; k < POINTERS_PER_BLOCK; k++) {
						if (indirect.pointers[k]){
							free_block_bitmap[indirect.pointers[k]] = 1;
						}
					}
				}
			}
		}
	}
	/* --- Debug ---
	for (i=0; i<superblock.super.nblocks; i++) {
		printf("%d ", free_block_bitmap[i]);
	}
	printf("\n"); */
	// --- Return Successfully ---
	return 1;
}

int fs_create()
{
	union fs_block block;
	int i;
	int j = 1;
	int inumber = 0;
	for (i=1; i <= ninodeblocks; i++) {
		disk_read(i, block.data);
		for (j=INODES_PER_BLOCK*(i-1); j < INODES_PER_BLOCK*i; j++) {
			if (j!=0 && block.inode[j%INODES_PER_BLOCK].isvalid == 0) {
				inumber = j;
				block.inode[inumber].isvalid = 1;
				block.inode[inumber].size = 0;
				for (j=0; j < POINTERS_PER_INODE; j++) {
					block.inode[inumber].direct[j] = 0;
				}
				block.inode[inumber].indirect = 0;
				disk_write(i, block.data);
				return inumber;
			}
		}
	}
	return inumber;
}

int fs_delete( int inumber )
{
	// --- Ensure Valid inumber ---
	if (inumber == 0) {
		return 0;
	}
	// --- Load Relevant Inode ---
	struct fs_inode inode;
	inode = inode_load(inumber);
	// --- Reject if Inode Is Invalid ---
	if (inode.isvalid == 0) {
		return 0;
	}
	// --- Free Associated Blocks In free_block_bitmap ---
	int i;
	// --- Direct Blocks ---
	for (i=0; i<POINTERS_PER_INODE; i++) {
		if (inode.direct[i]) {
			free_block_bitmap[inode.direct[i]] = 0;
		}
	}
	// --- Indirect Block ---
	if (inode.indirect) {
		free_block_bitmap[inode.indirect] = 0;
		union fs_block block;
		disk_read(inode.indirect, block.data);
		for(i=0; i<POINTERS_PER_BLOCK; i++) {
			if (block.pointers[i]) {
				free_block_bitmap[block.pointers[i]] = 0;
			}
		}
	}
	// --- Mark Inode as Invalid on Disk
	inode.isvalid = 0;
	inode_save(inumber, inode);
	// --- Exit Successfully ---
	return 1;
}

int fs_getsize( int inumber )
{
	// --- Load Inode ---
	struct fs_inode inode;
	inode = inode_load(inumber);
	// --- Return Failure if Invalid ---
	if (inode.isvalid == 0) {
		return -1;
	}
	// --- Return Inode Size ---
	return inode.size;
}

int fs_read( int inumber, char *data, int length, int offset )
{
	// --- Ensure Valid inumber ---
	// --- Load Relevant Inode ---
	struct fs_inode inode;
	inode = inode_load(inumber);
	// --- Reject if Inode Is Invalid ---
	if (inode.isvalid == 0) {
		return 0;
	}
	if(offset >= inode.size){
		return 0;
	}
	
	// --- Copy "length" bytes from inode to "data" starting at offset ---
	//Get start point
	int blocknum=-1;
	int i;
	char isindirect=1;
	for(i=0;i<POINTERS_PER_INODE;i++){
		if(offset < (DISK_BLOCK_SIZE*(i+1)))
		{
			blocknum=i;
			isindirect=0;
			break;
		}
	}
	union fs_block indirect;
	if(blocknum==-1){
		//look at indirect block
		disk_read(inode.indirect, indirect.data);
		isindirect=1;
		for(i=0;i<POINTERS_PER_BLOCK;i++)
		{
			if(offset < ((DISK_BLOCK_SIZE*(i+1))+(DISK_BLOCK_SIZE*POINTERS_PER_INODE)))
			{
				blocknum=i;
				break;
			}
				
		}
	}

	//start reading the data 
	union fs_block block;
	int bytes_read=0;
	int bytes_remaining;
	while(length != bytes_read){
		// --- Get Next Block ---
		if(isindirect==0){
			disk_read(inode.direct[blocknum],block.data);
		}
		else{
			disk_read(indirect.pointers[blocknum],block.data);
		}
		// --- Write to Data ---
		if(bytes_read==0 && offset%DISK_BLOCK_SIZE !=0){
			bytes_remaining= DISK_BLOCK_SIZE - offset;
			for(i=0;i<bytes_remaining;i++)
			{
				data[bytes_read]=block.data[i];
				bytes_read++;
				if (bytes_read+offset == inode.size) {
					return bytes_read;
				}
			}
			//increment block num and load indirect if necessary
			blocknum++;
			if(blocknum>=POINTERS_PER_INODE && isindirect==0)
			{
				disk_read(inode.indirect,indirect.data);
				blocknum=0;
				isindirect=1;
			}
		}
		else if((length-bytes_read)>=DISK_BLOCK_SIZE){
			//read full block to data
			for(i=0;i<DISK_BLOCK_SIZE;i++)
			{
				data[bytes_read]=block.data[i];
				bytes_read++;
				if (bytes_read+offset == inode.size) {
					return bytes_read;
				}
			}
			//increment block num and load indirect if necessary
			blocknum++;
			if(blocknum>=POINTERS_PER_INODE && isindirect==0)
			{
				disk_read(inode.indirect,indirect.data);
				blocknum=0;
				isindirect=1;
			}
		}
		else{
			bytes_remaining=length-bytes_read;
			//read partial block to data 
			for(i=0;i<bytes_remaining;i++)
			{
				data[bytes_read]=block.data[i];
				bytes_read++;
				if (bytes_read+offset == inode.size) {
					return bytes_read;
				}
			}

		}
		// --- Ensure Next Data Block is Valid ---
		if (isindirect == 0) {
			if (inode.direct[blocknum] == 0) {
				return bytes_read;
			}
		}
		else {
			if (indirect.pointers[blocknum] == 0) {
				return bytes_read;
			}
		}
	}
	return bytes_read;
}

int fs_write( int inumber, const char *data, int length, int offset )
{
	// --- Load and Validate Inode ---
	struct fs_inode inode;
	inode = inode_load(inumber);
	if (inode.isvalid == 0) {
		return 0;
	}
	// --- Variables ---
	union fs_block block;
	union fs_block indirect;
	int blocknum = -1;
	int bytes_written = 0;
	char isindirect = 1;
	int i, disk_block;
	// ---------- Write Data To Disk ----------
	
	// ---------- Find Block Cooresponding to Start of Data ----------
	// --- Look Through Direct Blocks ---
	for (i=0; i<POINTERS_PER_INODE; i++) {
		if (offset < (DISK_BLOCK_SIZE*(i+1))) {
			blocknum = i;
			isindirect = 0;
			// --- Ensure Desired Direct Block Exists ---
			if (inode.direct[blocknum] == 0) {
				inode.direct[blocknum] = claim_bitmap();
				if (inode.direct[blocknum] == 0) {
					// --- Disk Full ---
					return bytes_written;
				}
				inode_save(inumber, inode);
			}
			break;
		}
	}
	// --- If Necissary, Look Through Indirect Blocks ---
	if (isindirect) {
		// --- Ensure Indirect Block Exists ---
		if (inode.indirect == 0) {
			inode.indirect = claim_bitmap();
			if (inode.indirect == 0) {
				// --- Disk Full ---
				return bytes_written;
			}
			inode_save(inumber, inode);
		}
		// --- Load Indirect Block ---
		disk_read(inode.indirect, indirect.data);
		// --- Look Through Indirect Blocks ---
		for (i=0; i<POINTERS_PER_BLOCK; i++) {
			if (offset < (DISK_BLOCK_SIZE*(i+1) + DISK_BLOCK_SIZE*POINTERS_PER_INODE)) {
				blocknum = i;
				break;
			}
		}
		// --- Fail if Size of Indirect Block Exceeded ---
		if (blocknum == -1) {
			return bytes_written;
		}
		// --- Check if Block Avaialble in Indirect Block ---
		if (indirect.pointers[blocknum] == 0) {
			// --- Create New Indirect Data Block ---
			indirect.pointers[blocknum] = claim_bitmap();
			if (indirect.pointers[blocknum] == 0) {
				// --- Disk Full ---
				return bytes_written;
			}
			disk_write(inode.indirect, indirect.data);
		}
	}
	
	// --------- Loop Until All Data Written or Until Disk Full ----------
	while(bytes_written != length) {
		// --- Get Next Block ---
		if(isindirect==0){
			disk_read(inode.direct[blocknum],block.data);
			disk_block = inode.direct[blocknum];
		}
		else{
			disk_read(indirect.pointers[blocknum],block.data);
			disk_block = indirect.pointers[blocknum];
		}
		// ---------- Write To Disk ----------
		// --- Special Case: Starting In the Middle of an Existing Block
		if (bytes_written == 0 && offset%DISK_BLOCK_SIZE != 0) {
			for (i=offset; i<DISK_BLOCK_SIZE; i++) {
				block.data[i] = data[bytes_written];
				bytes_written++;
				// --- Exit If Full Length Written ---
				if (bytes_written == length) {
					// --- Save Block ---
					disk_write(disk_block, block.data);
					// --- Update Inode Size ---
					if ((offset+length) > inode.size) {
						inode.size = offset+length;
						inode_save(inumber, inode);
					}
					// --- Exit Successfully ---
					return bytes_written;
				}
			}
		}
		// --- Write Full Block or Until Write is Completed ---
		else {
			for (i=0; i<DISK_BLOCK_SIZE; i++) {
				block.data[i] = data[bytes_written];
				bytes_written++;
				// --- Exit If Full Length Written ---
				if (bytes_written == length) {
					// --- Save Block ---
					disk_write(disk_block, block.data);
					// --- Update Inode Size ---
					if ((offset+length) > inode.size) {
						inode.size = offset+length;
						inode_save(inumber, inode);
					}
					// --- Exit Successfully ---
					return bytes_written;
				}
			}
		}
		// --- Save Fully Written Block ---
		disk_write(disk_block, block.data);
		// --- Find Next Block ---
		blocknum++;
		if (blocknum >= POINTERS_PER_INODE && isindirect == 0) {
			// --- Ensure Indirect Block Exists ---
			if (inode.indirect == 0) {
				inode.indirect = claim_bitmap();
				if (inode.indirect == 0) {
					// --- Disk Full ---
					// --- Update Inode Size ---
					if ((offset+bytes_written) > inode.size) {
						inode.size = offset+bytes_written;
						inode_save(inumber, inode);
					}
					// --- Exit ---
					return bytes_written;
				}
				inode_save(inumber, inode);
			}
			disk_read(inode.indirect, indirect.data);
			blocknum = 0;
			isindirect = 1;
		}
		// --- Ensure Next Block Is Available ---
		if (isindirect == 0) {
			if (inode.direct[blocknum] == 0) {
				inode.direct[blocknum] = claim_bitmap();
				if (inode.direct[blocknum] == 0) {
					// --- Disk Full ---
					// --- Update Inode Size ---
					if ((offset+bytes_written) > inode.size) {
						inode.size = offset+bytes_written;
						inode_save(inumber, inode);
					}
					// --- Exit ---
					return bytes_written;
				}
				inode_save(inumber, inode);
			}
		}
		else {
			// --- Fail if Size of Indirect Block Exceeded ---
			if (blocknum >= POINTERS_PER_BLOCK) {
				// --- Update Inode Size ---
				if ((offset+bytes_written) > inode.size) {
					inode.size = offset+bytes_written;
					inode_save(inumber, inode);
				}
				// --- Exit ---
				return bytes_written;
			}
			// --- Check if Block Avaialble in Indirect Block ---
			if (indirect.pointers[blocknum] == 0) {
				// --- Create New Indirect Data Block ---
				indirect.pointers[blocknum] = claim_bitmap();
				if (indirect.pointers[blocknum] == 0) {
					// --- Disk Full ---
					// --- Update Inode Size ---
					if ((offset+bytes_written) > inode.size) {
						inode.size = offset+bytes_written;
						inode_save(inumber, inode);
					}
					// --- Exit ---
					return bytes_written;
				}
				disk_write(inode.indirect, indirect.data);
			}
		}
	}
	return bytes_written;
}
