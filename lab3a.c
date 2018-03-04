#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ext2_fs.h"





int main(int argc, char **argv) 
{
	if (argc != 2) {
		fprintf(stderr, "Error: pass in only one argument\n");
		exit(1);
	}
	char* fs = argv[1];				// file system
	int fsfd = open(fs, O_RDONLY); 			// file system file descriptor
	if (fsfd == -1) {
		fprintf(stderr, "Error: failed to open %s\n", fs);
		exit(2);
	}

	/* SUPERBLOCK */
	struct ext2_super_block superblock;
	if (pread(fsfd, &superblock, sizeof(superblock), 1024) != sizeof(superblock)) {
		fprintf(stderr, "Error: pread did not read the correct number of bytes\n");
		exit(2);
	}
	int sb2 = superblock.s_blocks_count;		//numberOfBlocks
	int sb3 = superblock.s_inodes_count;		//numberOfInodes
	int sb4 = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;//blockSize
	int sb5 = superblock.s_inode_size;		//inodeSize
	int sb6 = superblock.s_blocks_per_group;	//blocksPerGroup
	int sb7 = superblock.s_inodes_per_group;	//inodesPerGroup
	int sb8 = superblock.s_first_ino;		//firstFreeInode
	printf("SUPERBLOCK,%i,%i,%i,%i,%i,%i,%i\n", sb2, sb3, sb4, sb5, sb6, sb7, sb8);

	/* GROUP */
	int numberOfGroups = (sb3+(sb7-1))/sb7;			//numberOfInodes/inodesPerGroup ROUNDED UP
	if (numberOfGroups != (sb2+(sb6-1))/sb6) {
		fprintf(stderr, "Inconsistency detected in number of groups\n");
		exit(2);
	}
	struct ext2_group_desc group;
	int g2 = 0;					//groupNumber
	for (g2 = 0; g2 < numberOfGroups; g2++) {
		if (pread(fsfd, &group, sizeof(group), 1024 + sizeof(superblock) + g2*sizeof(group)) != sizeof(group)) {
			fprintf(stderr, "Error: pread did not read the correct number of bytes\n");
			exit(2);
		}
		int g3 = sb6;
		int g4 = sb7;
		if (g2 == numberOfGroups - 1) {
			if (sb2%sb6 != 0)
				g3 = sb2%sb6;
			if (sb3%sb7 != 0)
				g4 = sb3%sb7;
		}
		int g5 = group.bg_free_blocks_count;			//numberOfFreeBlocks;
		int g6 = group.bg_free_inodes_count;			//numberOfFreeInodes;
		int g7 = group.bg_block_bitmap;			
		int g8 = group.bg_inode_bitmap;			
		int g9 = group.bg_inode_table;							//groupFirstFreeInode;
		printf("GROUP,%i,%i,%i,%i,%i,%i,%i,%i\n", g2, g3, g4, g5, g6, g7, g8, g9);
	}
	return 0;
}
