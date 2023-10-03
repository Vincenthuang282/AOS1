/* This file contains code for a generic page fault handler for processes. */
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

int loadseg(pagetable_t pagetable, uint64 va, struct inode *ip, uint offset, uint sz);
int flags2perm(int flags);
int heap_tracker_region;

/* CSE 536: (2.4) read current time. */
uint64 read_current_timestamp() {
  uint64 curticks = 0;
  acquire(&tickslock);
  curticks = ticks;
  wakeup(&ticks);
  release(&tickslock);
  return curticks;
}

bool psa_tracker[PSASIZE];

/* All blocks are free during initialization. */
void init_psa_regions(void)
{
    for (int i = 0; i < PSASIZE; i++) 
        psa_tracker[i] = false;
}

int find_victim(struct proc* p){
    int victim_index =-1;
    uint64 oldest_load_time=-1;
    for(int i=0;i<p->resident_heap_pages;i++){
        if(oldest_load_time==-1||p->heap_tracker[i].last_load_time<oldest_load_time){
            oldest_load_time=p->heap_tracker[i].last_load_time;
            victim_index=i;
        }
    }
    return victim_index;
}
int tracking_psa(){
    int blockno=0;
    while(true){
        if(psa_tracker[blockno]==false){
            for(int i=blockno;i<blockno+4;i++){
                psa_tracker[i]=true;
            }
            return blockno;
        }
        else{
            blockno++;
        }
    }
}
/* Evict heap page to disk when resident pages exceed limit */
void evict_page_to_disk(struct proc* p) {
    
    /* Find free block */

    
    int victim=find_victim(p);
    p->heap_tracker[victim].startblock=tracking_psa(); 
    /* Read memory from the user to kernel memory first. */
    char *kernel=kalloc();
    copyin(p->pagetable,kernel,p->heap_tracker[victim].addr,PGSIZE);
    
    /* Write to the disk blocks. Below is a template as to how this works */
    struct buf* b;
    for(int i=0;i<4;i++){
        b=bread(1,PSASTART+(p->heap_tracker[victim].startblock)+i);
        memmove((char *)b->data,kernel+i*BSIZE,BSIZE);
        bwrite(b);
        brelse(b);
        psa_tracker[p->heap_tracker[victim].startblock+i]=true;
    }

    /* Unmap swapped out page */
    uvmunmap(p->pagetable, p->heap_tracker[victim ].addr , 1 ,0);
    /* Update the resident heap tracker. */
    p->resident_heap_pages--;
    p->heap_tracker[victim].last_load_time=0xFFFFFFFFFFFFFFFF;
    p->heap_tracker[victim].loaded=false;
    /* Print statement. */
    print_evict_page(p->heap_tracker[victim].addr, p->heap_tracker[victim].startblock);
}

/* Retrieve faulted page from disk. */
void retrieve_page_from_disk(struct proc* p, int index ) {

    int startblock=p->heap_tracker[index].startblock;
    char *kernel=kalloc();
    struct buf *b;
    for(int i=0;i<4;i++){
        b=bread(1,PSASTART+startblock+i);
        memmove(kernel+i*BSIZE,b->data,BSIZE); 
        brelse(b);
        copyout(p->pagetable,p->heap_tracker[index].addr+i*BSIZE,kernel,BSIZE);
        psa_tracker[startblock+i]=false;
    };
    /* Copy from temp kernel page to uvaddr (use copyout) */

    
    print_retrieve_page(p->heap_tracker[index].addr, startblock);
}
    


void page_fault_handler(void) 
{
    struct elfhdr elf;
    struct proghdr ph;
    struct inode *ip;
    /* Current process struct */
    struct proc *p = myproc();
    char *path=p->name;
    
    /*------------------------------Find VA from STVAL*/
    /* Track whether the heap page should be brought back from disk or not. */
    bool load_from_disk = false;

    /* Find faulting address. */
    uint64 faulting_addr =r_stval();
    faulting_addr = (faulting_addr>>12)<<12;
    print_page_fault(p->name, faulting_addr);
    begin_op(); 

    if((ip = namei(path)) == 0){
        end_op();
        return -1;
    }
    ilock(ip);
    if(readi(ip, 0, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf))
        printf("Problem with reading the ELF\n");
    if(elf.magic != ELF_MAGIC)
        printf("Problem with ELF_MAGIC\n");
    // Iterate through each program section header (using the binary’s ELF).
    for(int i=0, off=elf.phoff;i<elf.phnum; i++, off+=sizeof(ph)){
        if(readi(ip, 0, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
            printf("Problem with reading program header\n");
        if(ph.type != ELF_PROG_LOAD)
            continue;
        if(faulting_addr>=ph.vaddr&&faulting_addr<ph.vaddr+ph.memsz){
          uvmalloc(p->pagetable, ph.vaddr, ph.vaddr + ph.memsz, flags2perm(ph.flags));
          loadseg(p->pagetable,ph.vaddr,ip,ph.off,ph.filesz);
          print_load_seg(faulting_addr, ph.off, ph.memsz);
          ///break;
        }
        
    }
   
    iunlockput(ip);
    end_op();
    
    if( p->cow_enabled){
        printf("getin");
        copy_on_write(p,faulting_addr);
        printf("CoW: proc(%s)[%d] Addr (%x)\n", p->name, p->pid, faulting_addr);
    }
    
    /* Check if the fault address is a heap page. Use p->heap_tracker */

    for(heap_tracker_region=0;heap_tracker_region<MAXHEAP;heap_tracker_region++){
        if ( faulting_addr==p->heap_tracker[heap_tracker_region].addr  ) {
            goto heap_handle;
        }
    }
    /* If it came here, it is a page from the program binary that we must load. */
    //print_load_seg(faulting_addr, 0, 0);

    /* Go to out, since the remainder of this code is for the heap. */
    goto out;

heap_handle:
    /* 2.4: Check if resident pages are more than heap pages. If yes, evict. */
    if (p->resident_heap_pages == MAXRESHEAP) {
        evict_page_to_disk(p);
    }
    if (p->heap_tracker[heap_tracker_region].startblock!=-1){
        retrieve_page_from_disk(p,heap_tracker_region);
    }
    /* 2.4: Heap page was swapped to disk previously. We must load it from disk. */

    uvmalloc(p->pagetable,p->heap_tracker[heap_tracker_region].addr,p->heap_tracker[heap_tracker_region].addr+PGSIZE,PTE_W);
    p->heap_tracker[heap_tracker_region].last_load_time=read_current_timestamp();
    p->heap_tracker[heap_tracker_region].startblock=-1;
    p->heap_tracker[heap_tracker_region].loaded=true;
    p->resident_heap_pages++;
    
    /* 2.4: Update the last load time for the loaded heap page in p->heap_tracker. */
    /* Track that another heap page has been brought into memory. */
out:
    /* Flush stale page table entries. This is important to always do. */
    sfence_vma();
    return ;
}
