#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int test1(int argc, char *argv[]){
	make_sfs("test.disk");
	mount_sfs("test.disk");
	sfs_create("my_file_001.txt");
	sfs_create("miami_beach_in_florida.txt");
	sfs_create("utd.txt");
	sfs_create("dallas_folks.txt");
	sfs_create("marriott.txt");
	sfs_create("singapore.doc");
	sfs_create("marriott.text");
	list();
	umount_sfs("test.disk");
}
void test2(){
	mount_sfs("test.disk");
	list();
	sfs_delete("dallas_folks.txt");
	list();
	umount_sfs("test.disk");	
}
void test3(){
	mount_sfs("test.disk");
	int fd = sfs_open("mumbai.doc");
	printf("FD = %d", fd);
	char* buf = "Wikipedia is a multilingual, web-based, free-content encyclopedia that is based on a model of openly editable content. It is the largest and most-popular general reference work on the Internet,and is named as one of the most popular websites. It is owned and supported by the Wikimedia Foundation, a non-profit organization which operates on whatever money it receives from its annual fund drives.";
	printf("About to write %d chars\n", strlen(buf));
	int bytes_writen = sfs_write(fd, buf, strlen(buf));	
	printf("Bytes written = \n", bytes_writen);
	printf("About to write %d chars\n", strlen(buf));
	umount_sfs("test.disk");	
}
void test4(){
	mount_sfs("test.disk");
	int fd = sfs_open("marriott.txt");
	char* buf;
	sfs_read(fd, buf, 100);	
	printf("Following was read:\n%s\n", buf);
	umount_sfs("test.disk");	
}

void test5(){
	mount_sfs("test.disk");
	int fd = sfs_open("marriott.txt");
	printf("FD = %d", fd);
	char* buf = "This guide was created as an overview of the Linux Operating System, geared toward new users as an exploration tour and getting started guide, with exercises at the end of each chapter. For more advanced trainees it can be a desktop reference, and a collection of the base knowledge needed to proceed with system and network administration.";
	printf("About to write %d chars\n", strlen(buf));
	int bytes_writen = sfs_write(fd, buf, strlen(buf));	
	printf("Bytes written = \n", bytes_writen);
	printf("About to write %d chars\n", strlen(buf));
	umount_sfs("test.disk");	
}


int main(int argc, char *argv[]){
	//test1();
	test2();
	//test3();
	//test4();
	//test5();
    exit(0);
}

