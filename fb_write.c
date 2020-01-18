//
// fb_write.c
// written by Billy Ellis (@bellis1000) on 18/01/2020
// 
// 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <mach/mach.h>
// requires iso_font.c from https://github.com/apple/darwin-xnu/blob/master/osfmk/console/iso_font.c
#include "iso_font.c"

#define WHITE 0xffffffff
#define BLACK 0x00000000
#define VINFO_ADDR 0x8037F260
#define LC_SIZE 0x0000000f
#define FRAMEBUFFER_OFFSET 0x10

mach_port_t get_kernel_task_port(){
  mach_port_t kernel_task;
  kern_return_t kr;
  if ((kr = task_for_pid(mach_task_self(), 0, &kernel_task)) != KERN_SUCCESS){
    return -1;
  }
  return kernel_task;
}

uint32_t do_kernel_read(uint32_t addr){
  size_t size = 4;
  uint32_t data = 0;
  
  kern_return_t kr = vm_read_overwrite(get_kernel_task_port(),(vm_address_t)addr,size,(vm_address_t)&data,&size);
  if (kr != KERN_SUCCESS){
    printf("[!] Read failed. %s\n",mach_error_string(kr));
    return -1;
  }
  return data;
}

void do_kernel_write(uint32_t addr, uint32_t data){
  kern_return_t kr = vm_write(get_kernel_task_port(),(vm_address_t)addr,(vm_address_t)&data,sizeof(data));

  if (kr != KERN_SUCCESS){
    printf("Error writing!\n");
    return;
  }
}

uint32_t get_kernel_slide(){
  uint32_t slide;
  uint32_t base = 0x80001000;
  uint32_t slid_base;

  for (int slide_byte = 256; slide_byte >= 1; slide_byte--){
    slide = 0x01000000 + 0x00200000 * slide_byte;
    slid_base = base + slide;

    if (do_kernel_read(slid_base) == 0xfeedface){
      if (do_kernel_read(slid_base + 0x10) == LC_SIZE){
        return slide;
      }
    }
  }
  return -1;
}

uint32_t get_frame_buffer_address(){
  return do_kernel_read(VINFO_ADDR + FRAMEBUFFER_OFFSET + get_kernel_slide());
}

void write_char(char c, int col){
  uint32_t addr = get_frame_buffer_address();
    
  for (int y = 0; y < 15; y += 1){
    int value = iso_font[c * 16 + y];

	  for (int x = col; x < col + 8; x++){

	    if ((value & (1 << (x - col)))){
	      do_kernel_write(addr + (x * 4) + (y * 256), WHITE);
	    }else{
		    do_kernel_write(addr + (x * 4) + (y * 256), BLACK);
	    }

	  }
  }
}

void print(char *str, int len){
  for (int i = 0; i < len; i++){
    write_char(str[i], i * 8);
  }
}

int main(int argc, char *argv[]){
  if (argc < 2){
  	printf("Usage: %s <string>\n", argv[0]);
	  exit(0);
  }  
  print(argv[1], strlen(argv[1]));
  return 0;
}
