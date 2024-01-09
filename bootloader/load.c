//
// ramdisk that uses the disk image loaded by qemu -initrd fs.img
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "layout.h"
#include "buf.h"

/* In-built function to load NORMAL/RECOVERY kernels */
void kernel_copy(enum kernel ktype, struct buf *b)
{
  if(b->blockno >= FSSIZE)  // Check if the block number in the buffer is too large.
    panic("ramdiskrw: blockno too big");

  uint64 diskaddr = b->blockno * BSIZE; // Calculate the disk address based on the block number. 
  char* addr = 0x0; // Initialize a character pointer addr to NULL (0x0).
  

  if (ktype == NORMAL) // if kernel type is normal then buf address will be RAMDISK(0x84000000)
    addr = (char *)RAMDISK + diskaddr;
  else if (ktype == RECOVERY)// if kernel type is normal then buf address will be RAMDISK(0x84500000)
    addr = (char *)RECOVERYDISK + diskaddr ;

  memmove(b->data, addr, BSIZE); // Copy data from addr to the buffer's data field, with a size of BSIZE.
  b->valid = 1; // set the buffer as valid
}