//NAME: Ashwin Dharne
//EMAIL: ashwindharne@gmail.com
//UID: 004807188
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
//#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ext2_fs.h"

#define BASE_OFFSET 1024  /* location of the super-block in the first group */
#define BLOCK_OFFSET(block,block_size) (BASE_OFFSET + (block-1)*block_size)

int sb4;
int fsfd;
void indirect_recursive(int inode_number, uint32_t block_number, uint32_t offset, int lvl)
{
	uint32_t addrPtr[sb4/(int)sizeof(uint32_t)];
	pread(fsfd, addrPtr, sb4, BLOCK_OFFSET(block_number, sb4));
	for(int i=0;i<sb4/(int)sizeof(uint32_t);i++)
	{
		if(addrPtr[i]!=0){
			printf("INDIRECT,%d,%d,%d,%d,%d\n",inode_number, lvl, offset, block_number, addrPtr[i]);
			if(lvl==2){
				indirect_recursive(inode_number, addrPtr[i],offset, lvl-1);
				continue;
			}
			else if(lvl==3){
				indirect_recursive(inode_number, addrPtr[i],offset,lvl-1);
				continue;
			}
		}
		if(lvl==1)
			offset++;
	}
}
void timeStr(uint32_t time, char* buf) {
	time_t t = time;
	struct tm stamp = *gmtime(&t);
	strftime(buf, 80, "%m/%d/%y %H:%M:%S", &stamp);
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Error: pass in only one argument\n");
		exit(1);
	}
	char* fs = argv[1];				// file system
	fsfd = open(fs, O_RDONLY); 			// file system file descriptor
	if (fsfd == -1) {
		fprintf(stderr, "Error: failed to open %s\n", fs);
		exit(2);
	}

	/* SUPERBLOCK */
	struct ext2_super_block superblock;
	if (pread(fsfd, &superblock, sizeof(superblock), BASE_OFFSET) != sizeof(superblock)) {
		fprintf(stderr, "Error: pread did not read the correct number of bytes\n");
		exit(2);
	}
	int sb2 = superblock.s_blocks_count;		//numberOfBlocks
	int sb3 = superblock.s_inodes_count;		//numberOfInodes
	sb4 = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;//blockSize
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
	struct ext2_group_desc group[numberOfGroups];
	int g2 = 0;					//groupNumber
	for (g2 = 0; g2 < numberOfGroups; g2++) {
		if (pread(fsfd, &group[g2], sizeof(struct ext2_group_desc), BASE_OFFSET + sizeof(superblock) + g2 * sb4 * sb6) != sizeof(struct ext2_group_desc)) {
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
		int g5 = group[g2].bg_free_blocks_count;			//numberOfFreeBlocks;
		int g6 = group[g2].bg_free_inodes_count;			//numberOfFreeInodes;
		int g7 = group[g2].bg_block_bitmap;
		int g8 = group[g2].bg_inode_bitmap;
		int g9 = group[g2].bg_inode_table;							//groupFirstFreeInode;
		printf("GROUP,%i,%i,%i,%i,%i,%i,%i,%i\n", g2, g3, g4, g5, g6, g7, g8, g9);
	}

	/* FREE BLOCK ENTRIES */
	int i, j, k;
	for (i = 0; i < numberOfGroups; i++) {
		int blockBitMap = group[i].bg_block_bitmap;
		int numberOfBlocks = sb6;
		if (i == numberOfGroups - 1)
			if (sb2%sb6 != 0)
				numberOfBlocks = sb2%sb6;
		for (j = 0; j < numberOfBlocks/8; j++) {
			unsigned char buff;
			if (pread(fsfd, &buff, 1, BLOCK_OFFSET(blockBitMap, sb4) + j) != 1) {
				fprintf(stderr, "Error: pread did not read the correct number of bytes\n");
                        	exit(2);
                	}
			unsigned char mask = 0x01;
			for (k = 0; k < 8; k++) {
				int freeBlockNumber = 1 + i*sb6  + j*8 + k;	// 1 because block counts start at 1, not 0
				if (freeBlockNumber > numberOfBlocks + i*sb6)
					break;
				if ((buff & (mask << k)) == 0)			// when k = 0 shift 0x01 by 7 is 0x80
					printf("BFREE,%i\n",freeBlockNumber);
			}
		}
	}

	/* FREE INODE ENTRIES */
	for (i = 0; i < numberOfGroups; i++) {
		int inodeBitMap = group[i].bg_inode_bitmap;
		int numberOfInodes = sb7;
		if (i == numberOfGroups - 1) {
			if (sb3%sb7 != 0)
				numberOfInodes = sb3%sb7;
		}
		for (j = 0; j < numberOfInodes/8; j++) {
			unsigned char buff;
			if (pread(fsfd, &buff, 1, BLOCK_OFFSET(inodeBitMap, sb4) + j) != 1) {
				fprintf(stderr, "Error: pread did not read the correct number of bytes\n");
				exit(2);
			}
			unsigned char mask = 0x01;
			for (k = 0; k < 8; k++) {
				int freeInodeNumber = 1 + i*sb7 + j*8 + k;
				if (freeInodeNumber > numberOfInodes + i*sb7)
					break;
				if ((buff & (mask << k)) == 0)
					printf("IFREE,%i\n",freeInodeNumber);
			}
		}
	}

	/* INODE SUMMARY */
	struct ext2_inode curInode;
	char ftype='?';
	for(int i=0; i< numberOfGroups; i++)
	{
		int offset=BASE_OFFSET + 4*sb4;
		for(int j=0; j<(int)superblock.s_inodes_per_group;j++)
		{
			pread(fsfd, &curInode, sizeof(struct ext2_inode), offset + j*sizeof(struct ext2_inode));
			if(curInode.i_mode != 0 && curInode.i_links_count != 0)
			{
				if(curInode.i_mode & 0x4000)
					ftype='d';
				else if(curInode.i_mode & 0x8000)
					ftype='f';
				else if(curInode.i_mode & 0xA000)
					ftype='s';

				char ctime[20], mtime[20], atime[20];
				timeStr(curInode.i_ctime, ctime);
                timeStr(curInode.i_mtime, mtime);
                timeStr(curInode.i_atime, atime);

				printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",j+1,ftype,curInode.i_mode & 0xFFF,curInode.i_uid,curInode.i_gid,curInode.i_links_count
						,ctime,mtime,atime,curInode.i_size,curInode.i_blocks);
				for(int k=0;k<EXT2_N_BLOCKS;k++){
					printf(",%d",curInode.i_block[k]);
				}
				printf("\n");

				/* DIRECTORY */
				if(ftype=='d'){
					struct ext2_dir_entry dir;
					for(int k=0;k<EXT2_NDIR_BLOCKS;k++)
					{
						if(curInode.i_block[k]==0)
							break;
                        int dirOffset = 0;
                        while (dirOffset < sb4) {
                            pread(fsfd, &dir, sizeof(struct ext2_dir_entry), curInode.i_block[k]*sb4 + dirOffset);
							if(dir.inode==0){
								dirOffset += dir.rec_len;
								continue;
							}
							printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n", j+1, dirOffset, dir.inode,
            						dir.rec_len, dir.name_len, dir.name);
                            dirOffset += dir.rec_len;
                        }
					}
				}
				/*INDIRECT*/
				//because the first 12 are used
				if(curInode.i_block[12]!=0){
					indirect_recursive(j+1, curInode.i_block[12],12,1);
				}
				//first 268 used by first indirect
				if(curInode.i_block[13]!= 0){
					indirect_recursive(j+1,curInode.i_block[13],268,2);
				}
				//first 65804 used by second indirect
				if(curInode.i_block[14]!=0){
					indirect_recursive(j+1,curInode.i_block[14],65804,3);
				}
			}
		}
	}
	return 0;
}
