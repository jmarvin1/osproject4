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
	superblock.super.ninodeblocks = superblock.super.nblocks/10;
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
	// --- Return Successfully ---
	return 1;
}

int fs_create()
{
	return 0;
}

int fs_delete( int inumber )
{
	return 0;
}

int fs_getsize( int inumber )
{
	return -1;
}

int fs_read( int inumber, char *data, int length, int offset )
{
	return 0;
}

int fs_write( int inumber, const char *data, int length, int offset )
{
	return 0;
}
