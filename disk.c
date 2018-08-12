/*disk.c
  Simple File System (SFS) on Virtual Disk
  CS/SE 4348 Operating Systems - Assignment 4
  UT Dallas  
  Author: Parijat Singh
  
  The goal of this project is to design and implement a very simple file system on a virtual disk. 
  This file contains all the function related to a virtual disk. It emulate the virtual disk using a very large file.
  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BLOCK_SIZE 4096
#define MAX_BLOCKS 4096


/*This function creates the disk. To be specific, the file (filename) of length specified by nbytes is created. 
On success, it returns 0. Otherwise returns -1.
*/
int create_disk(char* filename, size_t nbytes){
	FILE *fp=fopen(filename, "w");
	ftruncate(fileno(fp), nbytes);
	fclose(fp);
	return 0;
}
/*This functions opens the file (specified by filename) for reading and writing. 
On success, returns the file descriptor of the opened file. Otherwise returns -1.
*/
int open_disk(char* filename){
	FILE *fp=fopen(filename, "rb+");
	int fd = fileno(fp);
	return fd;
}
/*This function reads disk block block_num from the disk pointed to by disk (file descriptor returned by open_disk) into a buffer pointed to by buf. 
On success it returns 0. Otherwise it returns -1.
*/
int read_block(int disk, int block_num, char *buf, size_t nbytes){
	int offset = block_num*BLOCK_SIZE;
	lseek(disk,offset,SEEK_SET);
	//ssize_t  res =  read(disk, buf, sizeof(buf));
	//ssize_t  res =  read(disk, buf, BLOCK_SIZE);
	ssize_t  res =  read(disk, buf, nbytes);
	if(res >= 0) return 0;
	else return -1;
}
/*This function writes the data in buf to disk block block_num pointed to by disk. 
On success it returns 0. Otherwise returns -1.
*/
int write_block(int disk, int block_num, char *buf, size_t nbytes){	
	int offset = block_num*BLOCK_SIZE;
	lseek(disk,offset,SEEK_SET);
	//ssize_t  res = write(disk, buf, sizeof(buf));
	//ssize_t  res = write(disk, buf, BLOCK_SIZE);
	ssize_t  res = write(disk, buf, nbytes);
	if(res >= 0) return 0;
	else return -1;
}
/*This function writes the data in buf to disk block block_num pointed to by disk. 
On success it returns 0. Otherwise returns -1.

int write_block_short(int disk, int block_num, short* data){	
	int offset = block_num*BLOCK_SIZE;
	lseek(disk,offset,SEEK_SET);
	ssize_t  res = write(disk, data, sizeof(data));
	//ssize_t  res = write(disk, buf, BLOCK_SIZE);
	if(res >= 0) return 0;
	else return -1;
}*/

/*This function closes the disk. On success returns 0. Otherwise returns -1.
*/
int close_disk(int disk){
	return close(disk);
}


/*int main(int argc, char *argv[]){
	create_disk("test.disk", BLOCK_SIZE*MAX_BLOCKS);
	int disk = open_disk("test.disk");
	char data[BLOCK_SIZE] = {'a','l','l'};//"All is well darling.";
	write_block(disk, 0, data);
	//close_disk(disk);
	
	disk = open_disk("test.disk");
	
	char blk_data[BLOCK_SIZE];
	int res = read_block(disk, 0, blk_data);
	printf("%d\n", res);
	printf("%s", blk_data);
	close_disk(disk);
    exit(0);
}*/
