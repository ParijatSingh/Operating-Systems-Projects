/*sfs.c
  Simple File System (SFS) on Virtual Disk
  CS/SE 4348 Operating Systems - Assignment 4
  UT Dallas  
  Author: Parijat Singh
  
  The goal of this project is to design and implement a very simple file system on a virtual disk. 
  This file contains all the function related to SFS. These functions are grouped as management functions and access functions.
  
*/

#include "sfs.h"
#include "disk.c"

typedef struct{
	char filename[FNAME_LENGTH];
	int filesize;
	short firstblock;
} directory_entry ;

directory_entry dir_entries[MAX_FILES] = {0};
int last_file_index;

//keep the file descriptor of the disk here
int diskfd;

/*
Keeps track of all open file descriptors. It will be initialized during sfs mount.
The index itself will be the fd. If the first value at that index is NO_VAL - it means the fd is available.
First dimension of the array will keep the file index. 
The second dimension will keep the cursor position within the block indicated by first dimension
*/
int fd_arry[MAX_OPEN_FILES][2];


/*
FAT contains information about next block where the file data is continuing.
*/
short fat[MAX_BLOCKS];

/*
This function creates the new file system on the virtual disk specified by disk_name. 
As part of this function, open_disk() is invoked to create a new disk. 
Then the necessary disk structures are created create (and initialize)for the file system. 
The function returns 0 on success, and -1 on failure (when the disk could not be created, opened, or properly initialized.)
*/
int make_sfs(char *disk_name){
	//first create the disk
	create_disk(disk_name, BLOCK_SIZE*MAX_BLOCKS); 
	
	// lets create necessary structures
	// 1. initialize the FAT table to NO_VAL (-1)
	memset(fat, NO_VAL, sizeof(short) * MAX_DATA_BLOCKS);
	// initialize directory
	//dir_entries = {0};
	//lets write it to disk: directory entries in first block, fat table in second block
	int disk = open_disk(disk_name);
	write_block(disk, 0, (unsigned char *)dir_entries, sizeof(dir_entries));
	write_block(disk, 1, (unsigned char *)fat, sizeof(fat));
	//necessary structures are written. lets close the disk.
	close_disk(disk);
}
/*
This function mounts the disk (disk_name). The on disk structures are read into memory to handle the file system operations. 
The function returns 0 on success, and -1 on failure.
*/
int mount_sfs(char *disk_name){
	//read necessary structures (directory entries and FAT table from disk)
	int disk = open_disk(disk_name);
	char dir_data[sizeof(dir_entries)];
	int res = read_block(disk, 0, dir_data, sizeof(dir_entries));
	memcpy(&dir_entries, dir_data, sizeof(dir_entries));
	char fat_data[sizeof(fat)];
	res = read_block(disk, 1, fat_data, sizeof(fat));
	memcpy(fat, fat_data, sizeof(fat));
	//close(disk);
	diskfd = disk;
	//lets initialize in-memory structures - fd_arry
	for(int i=0; i<MAX_FILES; i++){
		fd_arry[i][0] = NO_VAL;
		fd_arry[i][1] = NO_VAL;
	}	
	return 0;
}


/*
This function unmounts the disk (disk_name) from SFS file system. All the meta data and file system related data that are on memory are written into appropriate on disk structures. Finally, it calls close_disk(). The function returns 0 on success, and -1 on failure.
*/
int umount_sfs(char *disk_name){
	//save the directory entries and FAT
	int disk = open_disk(disk_name);
	write_block(disk, 0, (unsigned char *)dir_entries, sizeof(dir_entries));
	write_block(disk, 1, (unsigned char *)fat, sizeof(fat));
	close_disk(disk);
	return 0;	
}


/*
The file (file_name) is opened for reading and writing. This function returns the file descriptor of the file on success and -1 on failure.
The same file may opened many times, and for each open call a different file descriptor is returned. 
The maximum number of files that can remain opened at a time is specified by constant MAX_OPEN_FILES in the sfs.h header file.
*/
int sfs_open(char *file_name){
	//lets check if file exists by this name
	int idx = get_file_index(file_name);
	if(idx == -1){
		fprintf(stderr,"No file with name %s.\n", file_name);
		return -1;
	}
	//lets finf the first available file descriptor
	for(int i=0; i<MAX_OPEN_FILES; i++){
		if(fd_arry[i][1]==-1){
			fd_arry[i][0] = idx;
			fd_arry[i][1] = 0;
			return i;
		}
	}
	// if you have reached here it means no file descriptor is available
	fprintf(stderr,"Max open files reached.\n");
	return -1;
}


/*
This function closes file descriptor fd. The closed file descriptor can no longer be used to access the corresponding file. 
However, the closed file descriptor can be reused during another sfs_open()call. This function returns 0 on success and -1 on failure.
*/
int sfs_close(int fd){
	//if the fd is not open then return -1
	if(fd_arry[fd][0] = NO_VAL) return -1;
	fd_arry[fd][0] = NO_VAL;
	fd_arry[fd][1] = NO_VAL;
	return 0;
}


/*
This function creates the file specified by file_name. The file is created only if it is not already present. 
The length of the file name is specified by the constant FNAME_LENGTH. 
The function returns 0 on success and -1 on failure. The number of files that are in a directory is restricted to MAX_FILES.
*/
int sfs_create(char *file_name){
	//check the length of the filename
	if(strlen(file_name) > FNAME_LENGTH){
		fprintf(stderr,"Failed to create file. File can not exceed %d characters.\n", FNAME_LENGTH);
			return -1;
	}
	//lets check if there is already a file by the same name
	int idx = get_file_index(file_name);
	if(idx != -1){
		fprintf(stderr,"Failed to create file. %s already exists.\n", file_name);
		return -1;
	}
	
	//lets find an empty slot in directory
	for(int i=0; i<=MAX_FILES; i++){
		if ((dir_entries[i].filename != NULL) && (dir_entries[i].filename[0] == '\0')){
			idx = i;
			break;
		}
	}
	if(idx == -1){
		fprintf(stderr,"Maximum number of files reached. Delete some file before creating new.\n");
		return -1;
	}
	//lets find a free block and allocate it to this file. 
	int firstblock = NO_VAL;
	for(int i=MAX_BLOCKS-MAX_DATA_BLOCKS; i<MAX_BLOCKS; i++){
		if(fat[i] == NO_VAL){
			firstblock = i;
			fat[i] = 0;
			break;
		}
	}
	// if no block available then throw error message and return -1
	if(firstblock == -1){		
		fprintf(stderr,"Failed to create file. Disk is full, no space available.\n");
		return -1;
	}
	// all good. lets create the file
	printf("at index %d about to add %s. First block starts at %d.\n",idx,file_name,firstblock);
	//directory_entry d = {file_name,0,(short)firstblock};
	
	strcpy(dir_entries[idx].filename, file_name);
	dir_entries[idx].filesize = 0;
	dir_entries[idx].firstblock = firstblock;
	return 0;
}


/*
This function deletes the file specified by file_name. The file is deleted only if it has been already created. 
On deletion all the data blocks, on disk and on memory data structures related to the file data and meta data are released. 
Also, the file can be deleted only if it is not open. This function returns 0 on success and -1 on failure.
*/
int sfs_delete(char *file_name){
	// lets check if the file exists by file_name
	int idx = get_file_index(file_name);
	if(idx == -1) {
		fprintf(stderr,"No such file by name %s exists.\n", file_name);
		return idx;
	}
	// lets check if there is any file descriptor open for this file
	for(int i=0; i<MAX_OPEN_FILES; i++){
		if(fd_arry[i][0] == idx){
			fprintf(stderr,"Failed to delete file %s. It is open.\n", file_name);
		}
	}
	
	//lets find all the blocks used by this file and delete the data
	int index = -1;
	for(int i=0; i<=last_file_index; i++){
		if(strcmp(dir_entries[i].filename, file_name) == 0){
			int block_to_delete = dir_entries[i].firstblock;
			do{
				int tmp = block_to_delete;
				if(fat[block_to_delete] > 0){
					block_to_delete = fat[block_to_delete];
				}
			}while(block_to_delete != -1);
		}	
	}
	
	//lets remove this file from root directory	
	strcpy(dir_entries[idx].filename, "");
	dir_entries[idx].filesize = 0;
	dir_entries[idx].firstblock = 0;
}


/*
This function attempts to read up to count bytes from file descriptor fd into the buffer starting at buf. The read operation begins at the current file offset, and the file offset is incremented by the number of bytes read. If the current file offset is at or past the end of file, no bytes are read, and the call returns zero. On success, the number of bytes read is returned (zero indicates end of file), and the file position is advanced by this number. It is not an error if this number is smaller than the number of bytes requested; this may happen because fewer bytes are actually available right now (maybe the current file position is close to end of file). On error, -1 is returned.
*/
int sfs_read(int fd, void *buf, size_t count){
	int file_index = fd_arry[fd][0];
	if(file_index == NO_VAL){
		fprintf(stderr,"Invalid FD %d.\n", fd);
		return -1;
	}
	int offset = fd_arry[fd][1];
	//lets first find the block from the offset
	directory_entry file = dir_entries[file_index];
	short lastblock = file.firstblock;
	
	int bytes_read = read_block(diskfd, dir_entries[file_index].firstblock, buf, count);
	return bytes_read;
}


/*
This function attempts to writes up to count bytes from the buffer buf pointed to the file referred to by the file descriptor fd. The write operation begins at the current file offset, and the file offset is incremented by the number of bytes written. If end of file is reached while writing, the file is automatically extended. Upon successful completion, the number of bytes that were actually written is returned. This number could be smaller than count when the disk runs out of space (when writing to a full disk, the function returns zero). On error, the function returns -1.
*/
int sfs_write(int fd, void *buf, size_t count){
	int file_index = fd_arry[fd][0];
	if(file_index == NO_VAL){
		fprintf(stderr,"Invalid FD %d.\n", fd);
		return -1;
	}
	int offset = fd_arry[fd][1];
	//lets first find the block from the offset
	directory_entry file = dir_entries[file_index];		
	// we will assume the offset for file is the size of file
	offset = dir_entries[file_index].filesize;
	short lastblock = file.firstblock;
	
	//lets first read the block and then write it back
	char block_data[BLOCK_SIZE]; 
	read_block(diskfd, dir_entries[file_index].firstblock, block_data, BLOCK_SIZE);
	memcpy(&block_data[offset], buf, count);	
	
	//write_block(diskfd, lastblock, buf, count);	
	
	write_block(diskfd, lastblock, block_data, BLOCK_SIZE);
	dir_entries[file_index].filesize = offset + count;
	//lets find the how many blocks further seek pointer is pointing to	
	/*int block_count = offset%BLOCK_SIZE;
	int offset_in_block = offset/BLOCK_SIZE;
	
	while(block_count > 0){
		lastblock = fat[lastblock];
		block_count--;
	}
	int absolute_offset = lastblock*BLOCK_SIZE + offset;
	int remaining_in_block = BLOCK_SIZE - offset;
	// if remaining_in_block > count then the block will have to be extended
	if(remaining_in_block > count){
		// write data to disk
		write_block(diskfd, lastblock, buf, count);
		//adjust the offset of the fd
		fd_arry[fd][1] = fd_arry[fd][1]+count;
		return count;
	}*/
	return -1;
}

/*
The function sets the current offset of the file corresponding to file descriptor fd to the value specified by offset. This function returns 0 on success and -1 on failure. It is an error if offset is larger than the file size.
*/
int sfs_seek(int fd, int offset){
	//first find the file for this fd
	int file_index = fd_arry[fd][0];	
	if(offset > dir_entries[file_index].filesize)
	{
		fprintf(stderr,"Offset is larger than file size (%d).\n", dir_entries[file_index].filesize);
		return -1;
	}
	fd_arry[fd][1] = offset;
	return 0;	
}

/*
This is utility function to get the index of the file. Returns the index of the file if found else -1.
*/

int get_file_index(char* file_name){
	//printf("length of filename = %d\n",strlen(file_name));
	for(int i=0; i<=MAX_FILES; i++){
		//printf("length %s = %d\n",dir_entries[i].filename,strlen(dir_entries[i].filename));
		if(strcmp(dir_entries[i].filename, file_name) == 0){
			printf("Returning %d\n", i);
			return i;
		}	
	}
	return -1;
}

/*
This function lists all the files in the root directory.
*/
void list(){
	int total=0;
	for(int i=0; i<MAX_FILES; i++){
		if ((dir_entries[i].filename != NULL) && (dir_entries[i].filename[0] == '\0')) continue;
		printf("%s\t %d bytes\t %d\n",dir_entries[i].filename,dir_entries[i].filesize, dir_entries[i].firstblock);
		total++;
	}
	printf("Total %d files.\n",total);
}
