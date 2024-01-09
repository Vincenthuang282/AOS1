/* These files have been taken from the open-source xv6 Operating System codebase (MIT License).  */

#include "types.h"
#include "param.h"
#include "layout.h"
#include "riscv.h"
#include "defs.h"
#include "buf.h"
#include "measurements.h"
#include <stdbool.h>
#define SYSINFOADDR 0x80080000 ///********************************************really not sure about the BL END and DR END
#define BL_START_ADDR 0x80000000
#define BL_END_ADDR 0x8005be1d
#define DR_START_ADDR 0x80000000
#define DR_END_ADDR 0x88000000

void main();
void timerinit();

/* entry.S needs one stack per CPU */
__attribute__((aligned(16))) char bl_stack[STSIZE * NCPU];

/* Context (SHA-256) for secure boot */
SHA256_CTX sha256_ctx;

/* Structure to collects system information */
struct sys_info
{
  /* Bootloader binary addresses */
  uint64 bl_start;
  uint64 bl_end;
  /* Accessible DRAM addresses (excluding bootloader) */
  uint64 dr_start;
  uint64 dr_end;
  /* Kernel SHA-256 hashes */
  BYTE expected_kernel_measurement[32];
  BYTE observed_kernel_measurement[32];
};
struct sys_info *sys_info_ptr;

void kernel_copy_from_buffer(size_t total_size, char *destination_addr,enum kernel ktype)
{
  size_t elf_header_size = 4 * 1024;
  struct buf kernel_buffer;
  int block_number = 4;
  size_t remaining_bytes = total_size - elf_header_size;
  size_t size = 0;

  while (size < remaining_bytes)
  {
    kernel_buffer.blockno = block_number;
    kernel_copy(ktype, &kernel_buffer);
    memmove(destination_addr, kernel_buffer.data, BSIZE);
    destination_addr += BSIZE;
    size += BSIZE;
    block_number++;
  }
}

extern void _entry(void);
void panic(char *s)
{
  for (;;)
    ;
}

/* CSE 536: Boot into the RECOVERY kernel instead of NORMAL kernel
 * when hash verification fails. */
void set_up_kernel(enum kernel ktype){
    /* CSE 536: Load the DECOVERY kernel binary (assuming secure boot isn't passed). */
  uint64 kernel_load_addr = find_kernel_load_addr(ktype);
  uint64 kernel_binary_size = find_kernel_size(ktype);
  uint64 kernel_entry = find_kernel_entry_addr(ktype);

  kernel_copy_from_buffer((size_t)kernel_binary_size, (char *)kernel_load_addr,ktype);

  /* CSE 536: Write the correct kernel entry point */
  w_mepc((uint64)kernel_entry);
}



/* CSE 536: Function verifies if NORMAL kernel is expected or tampered. */
bool is_secure_boot(void)
{
  //bool verification = true;

  /* Read the binary and update the observed measurement
   * (simplified template provided below) */

  sha256_init(&sha256_ctx);
  uint64 kernel_binary_size = find_kernel_size(NORMAL);
  sha256_update(&sha256_ctx, (uchar *)RAMDISK, kernel_binary_size);
  sha256_final(&sha256_ctx, sys_info_ptr->observed_kernel_measurement);

  // Three more tasks required below:
  //  1. Compare observed measurement with expected hash
  for (int i = 0; i < 32; i++)
  {
    sys_info_ptr-> expected_kernel_measurement[i] = trusted_kernel_hash[i];
  }
  //  2. Setup the recovery kernel if comparison fails
  for(int i=0;i<32;i++){
      if(sys_info_ptr->expected_kernel_measurement[i]!=sys_info_ptr->observed_kernel_measurement[i]){
          //verification=false;
          //setup_recovery_kernel();
          //break;
          return false;
      }
  }
  //  3. Copy expected kernel hash to the system information table */
  return true;
}

// entry.S jumps here in machine mode on stack0.
void start()
{
  /* CSE 536: Define the system information table's location. */
  // sys_info_ptr = (struct sys_info*) 0x0;
  sys_info_ptr = (struct sys_info *)SYSINFOADDR;
  sys_info_ptr->bl_start = BL_START_ADDR;
  sys_info_ptr->bl_end = BL_END_ADDR;
  sys_info_ptr->dr_start = DR_START_ADDR;
  sys_info_ptr->dr_end = DR_END_ADDR;

  // keep each CPU's hartid in its tp register, for cpuid().
  int id = r_mhartid();
  w_tp(id);

  // set M Previous Privilege mode to Supervisor, for mret.
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  w_mstatus(x);

  // disable paging
  w_satp(0);

/* CSE 536: Unless kernelpmp[1-2] booted, allow all memory
 * regions to be accessed in S-mode. */
#if !defined(KERNELPMP1) || !defined(KERNELPMP2)
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf);
#endif

/* CSE 536: With kernelpmp1, isolate upper 10MBs using TOR */
#if defined(KERNELPMP1)
  w_pmpaddr0(0x21d40000); // 0x87500000
  w_pmpcfg0(0xf);
#endif

/* CSE 536: With kernelpmp2, isolate 118-120 MB and 122-126 MB using NAPOT */
#if defined(KERNELPMP2)
  w_pmpaddr0(0x21d80000);  // 118MB ok
  w_pmpaddr1(0x21dbffff);  // 118MB ok
  w_pmpaddr2(0x21e3ffff);  // 120MB-----121 21E40000 122 21E80000 123 21Ec0000 124 21F00000 125 21F40000 126 21F80000
  w_pmpaddr3(0x21E7FFFF);  // 122MB ok
  w_pmpaddr4(0x21fbffff);  // 126MB-------ok
  w_pmpcfg0(0x1f181f180f); //-----ok

#endif

  /* CSE 536: Verify if the kernel is untampered for secure boot */
  if (is_secure_boot()==true)
  {
    set_up_kernel(NORMAL);
  }
  else{
    set_up_kernel(RECOVERY);
  }

  /* CSE 536: Provide system information to the kernel. */

  /* CSE 536: Send the observed hash value to the kernel (using sys_info_ptr) */

  // delegate all interrupts and exceptions to supervisor mode.
  w_medeleg(0xffff);
  w_mideleg(0xffff);
  w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

  // return address fix
  uint64 addr = (uint64)panic;
  asm volatile("mv ra, %0"
               :
               : "r"(addr));

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
}
