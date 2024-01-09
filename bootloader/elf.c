#include "types.h"
#include "param.h"
#include "layout.h"
#include "riscv.h"
#include "defs.h"
#include "buf.h"    
#include "elf.h"

#include <stdbool.h>

struct elfhdr* kernel_elfhdr;
struct proghdr* kernel_phdr;

uint64 get_kernel_type(enum kernel ktype){
	if(ktype==NORMAL){
		return RAMDISK;
	}
	else{
		return RECOVERYDISK;
	}
}

uint64 find_kernel_load_addr(enum kernel ktype) { 
    //Point an ELF struct (elfhdr) to RAMDISK (where the kernel is currently loaded) to initialize it with the kernel binary's ELF header
    kernel_elfhdr = (struct elfhdr*)get_kernel_type(ktype);
    //Grab the offset where program section are specified 
    uint64 offset = kernel_elfhdr->phoff ;
    //and the size of program header
    uint64 phentsize = kernel_elfhdr->phentsize ;
    //Navigate to the program header section’s second address. This is the header for the .text section and its address is RAMDISK + phoff + phsize.
    uint64 t_section_addr=get_kernel_type(ktype) + offset +phentsize;
    kernel_phdr = (struct proghdr* )(t_section_addr) ;
    //finding the starting address of the .text by retreiving the vaddr field within proghdr
    return kernel_phdr -> vaddr;
}

uint64 find_kernel_size(enum kernel ktype) {
    /* CSE 536: Get kernel binary size from headers */
    kernel_elfhdr= (struct elfhdr*) get_kernel_type(ktype);
    uint64 shoff=kernel_elfhdr->shoff;
    ushort shnum=kernel_elfhdr->shnum;
    ushort shentsize=kernel_elfhdr->shentsize;
    return shoff+(shnum*shentsize);
}

uint64 find_kernel_entry_addr(enum kernel ktype) {
    /* CSE 536: Get kernel entry point from headers */
    kernel_elfhdr= (struct elfhdr*) get_kernel_type(ktype);
    return kernel_elfhdr->entry;
}
