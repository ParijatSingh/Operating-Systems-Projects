/*vmm.c
  Designing a Virtual Memory Manager
  Operating System - Assignment 2  
  Author: Parijat Singh
  
  This program implement a virtual memory manager as described in page 458 of the book. 
  It also implement page replacement policy as suggested in the modifications section of the project description. 
  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NO_VAL -1
#define PAGE_TABLE_ENTRIES 256
#define FRAME_SIZE 256
// CHANGE NO_OF_FRAMES to 128 to verify modification part of the assignment 
#define NO_OF_FRAMES 256
#define TLB_ENTRIES 16
//count of logical addresses in input file
int addr_translated_count;
//2^8 entries in the page table
int page_table[PAGE_TABLE_ENTRIES];
// page eviction table, 2nd dimension is for second chance bit
int page_swap_table[NO_OF_FRAMES][2];
//first index of swap_table
int first_index_swap_table;
//2-D array - 16 entries in the TLB, 2nd dimension for storing frame no. 
int tlb[TLB_ENTRIES][2];
// index of first entry in TLB
int first_tlb_entry_index;
// this is the total memory no of frames x frame size
char phys_mem[FRAME_SIZE*NO_OF_FRAMES];
// flag to check if no free frame available
int memory_full;
// File Pointer to Backstore file
FILE *backst_fp;
//Out file
FILE *output_fp;
int page_fault_counter = 0;
int tbl_hit_counter = 0;
int frameidx = 0;
void open_output_file()
{
	output_fp = fopen("output.txt", "w");
	if (output_fp == 0)
    {
        fprintf(stderr, "failed to open output.txt\n");
        exit(1);
    }
}

void open_disk_store()
{
	backst_fp = fopen("BACKING_STORE.bin", "r");
	if (backst_fp == 0)
    {
        fprintf(stderr, "failed to open BACKING_STORE.bin - make sure the file exists in the same folder as this executable\n");
        exit(1);
    }
}

/* If page swap happened then it should be replaced in TLB as well */
void swap_in_tlb(int old_page_num, int new_page_num){
	for (int i = 0; i < TLB_ENTRIES; i++) 
	{
        if (tlb[i][0] == old_page_num) 
		{
            tlb[i][0] = new_page_num;
			break;
        }
    }
}

/* This function is to swap the page Round robin - FIFO policy implemented*/
int evict_page(int page_num)
{
	int page_to_swap = page_swap_table[first_index_swap_table][0];
	int frame_to_swap = page_table[page_to_swap];
	frame_to_swap = frame_to_swap >> 8;
	page_table[page_to_swap] = -1;	
	page_swap_table[first_index_swap_table][0] = page_num; 
	first_index_swap_table = (first_index_swap_table+1)%NO_OF_FRAMES;
	//printf("swaping page %d with page %d, frame = %d\n", page_to_swap, page_num, frame_to_swap);
	swap_in_tlb(page_to_swap, page_num);
	return frame_to_swap;
}


/* This function read the page from disk store and copies the page data to the frame in physical memory*/
void load_from_diskstore(int page, int frame)
{
	// move pointer to page*256 bytes from the beginning of the store file
	fseek(backst_fp, page*FRAME_SIZE, SEEK_SET);
	// read and copy to memory
	fread((phys_mem + (frame*FRAME_SIZE)), sizeof(char), FRAME_SIZE, backst_fp);
}

/* Returns the physical memory frame from page table. If page is not in page table then loads it from back store*/
int get_frame_from_page_table(int page_num)
{
	int frame_num = page_table[page_num];
	// encountered page fault 
	if(frame_num == NO_VAL)
	{
		// increment fault counter
		page_fault_counter++;
		//printf("Page [%d] fault. Loading from store.\n", page_num);
		// if no frame available then evict a page based on second chance algorithm before loading from backstore
		if(memory_full) {
			frameidx = evict_page(page_num);
		}
		// now load from the backstore
		load_from_diskstore(page_num, frameidx);		
		//printf("Page [%d] will have frame:%d\n", page_num, frameidx);
		//now update page table with the new frame
		page_table[page_num] = frameidx << 8 ;
		if(!memory_full) {
			page_swap_table[frameidx][0] = page_num;
			page_swap_table[frameidx][1] = 0;
			frameidx++;
		}
		if(frameidx == NO_OF_FRAMES){
			 memory_full = 1;
		}
	}
	frame_num = page_table[page_num];
	return frame_num;
}

/* Function to check the TLB first */
int get_frame_from_tlb(int page_number) 
{	
    for (int i = 0; i < TLB_ENTRIES; i++) 
	{
        if (tlb[i][0] == page_number) 
		{
            // hit in TLB
            tbl_hit_counter++;
            return tlb[i][1];
        }
    }
	// page number not found in TLB
    return -1;
}

/* Function to add entry in TLB if there is TLB miss */
void add_entry_tlb(int page_num, int frame_num)
{
	tlb[first_tlb_entry_index][0] = page_num;
	tlb[first_tlb_entry_index][1] = frame_num;
	//printf("Added page %d at index %d in TLB\n", page_num, first_tlb_entry_index);
	// make the next entry as the first entry. this will make entries FIFO
	first_tlb_entry_index = (first_tlb_entry_index+1)%TLB_ENTRIES;
}

void write_stats()
{
	fputs("\n ---------- STATISTICS -----------\n",output_fp);
	fprintf(output_fp, "Number of Translated Addresses = %d\n", addr_translated_count);
	fprintf(output_fp, "Page Fault count = %d\n", page_fault_counter);
	fprintf(output_fp, "Page-fault rate = %.3f(%.1f%%)\n", page_fault_counter/(float)addr_translated_count, page_fault_counter*100/(float)addr_translated_count);
	fprintf(output_fp, "TLB hit count = %d\n", tbl_hit_counter);
	fprintf(output_fp, "TLB hit rate = %.3f(%.1f%%)\n", tbl_hit_counter/(float)addr_translated_count,tbl_hit_counter*100/(float)addr_translated_count);
}

int main(int argc, char *argv[]){
	
	FILE* fp;
	unsigned addr;
	int frameidx = 0;
	int page_num;
	int offset;
	char line[16];
	open_disk_store();
	open_output_file();
	// lets initialize the page table to 
	memset(page_table, NO_VAL, sizeof(int) * PAGE_TABLE_ENTRIES);
	// Open file in read mode
	fp = fopen("addresses.txt", "r");
	if (fp == 0)
    {
        fprintf(stderr, "failed to open addresses.txt - make sure the file exists in the same folder as this executable\n");
        exit(1);
    }
	//Start from beginning
	fseek(fp, 0, SEEK_SET);
	while(!feof(fp)) 
	{
		fgets(line, sizeof(line), fp);
		//increment the input count, this will be used in statistics
		addr_translated_count++;		
		//printf("%s\n", line);
		addr = atoi(line);
		// mask rightmost 16 bits of logical address
		addr = addr & 0xFFFF;
		//first 8 bit - page number
		page_num = addr >> 8;
		//next 8 bit - offset
		offset = addr & 0x00FF;
		line[0] = '\0';
		
		//let's check if the page is in TLB
		int frame_addr = get_frame_from_tlb(page_num);
		if(frame_addr == -1)
		{
			//TLB miss. so now get it from page table			
			frame_addr = get_frame_from_page_table(page_num);
			//now add it to TLB
			add_entry_tlb(page_num, frame_addr);
		}
		//int physical_addr = (frame << 8) + offset;
		int physical_addr = frame_addr + offset;
		//char data = *(phys_mem + physical_addr);
		char data = phys_mem[physical_addr];
		//fprintf(output_fp, "Logical Address:%d\tPhisical Address:%d\tPage:%d\tOffset:%d\tFrame:%d\t", addr, physical_addr, page_num, offset, frame_addr >> 8);
		fprintf(output_fp, "Virtual address: %d Physical address: %d ", addr, physical_addr);
		fprintf(output_fp, "Value: %d", data);
		fputs("\n",output_fp);		
	}
	// now write the statistics to file
	write_stats();
	
	printf("\nPhysical Memory size is %d*%d.\n", NO_OF_FRAMES, FRAME_SIZE);	
	printf("\nGenrated results in 'output.txt'.\n");
	fclose(output_fp);
	fclose(backst_fp);
    exit(0);
}

