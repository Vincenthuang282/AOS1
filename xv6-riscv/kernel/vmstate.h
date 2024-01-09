#include "types.h"

// Struct to keep VM registers (Sample; feel free to change.)
struct vm_reg {
    uint64     code;
    int     mode;
    uint64  val;
};

enum Current_Mode {U,S,M};

// Keep the virtual state of the VM's privileged registers


struct vm_virtual_state {
    //• Machine trap handling registers
    struct vm_reg mscratch;
    struct vm_reg mepc;
    struct vm_reg mcause;
    struct vm_reg mtval;
    struct vm_reg mip;
    struct vm_reg mtinst;
    struct vm_reg mtval2;
    //• Machine setup trap registers
    struct vm_reg mstatus;
    struct vm_reg misa;
    struct vm_reg medeleg;
    struct vm_reg mideleg;
    struct vm_reg mie;
    struct vm_reg mtvec;
    struct vm_reg mcounteren;
    struct vm_reg mstatush;
    //• Machine information state registers
    struct vm_reg mvendorid;
    struct vm_reg marchid;
    struct vm_reg mimpid;
    struct vm_reg mhartid;
    struct vm_reg mconfigptr;
    //• Machine physical memory protection registers
    struct vm_reg pmpcfg0;
    //• Supervisor page table register (satp)
    struct vm_reg satp;
    //• Supervisor trap setup registers
    struct vm_reg sstatus;
    struct vm_reg sie;
    struct vm_reg stvec;
    struct vm_reg scounteren;
    //Supervisor Trap Handling
    struct vm_reg sepc;
    //• User trap handling registers
    struct vm_reg uscratch;
    struct vm_reg uepc;
    struct vm_reg ucause;
    struct vm_reg ubadaddr;
    struct vm_reg uip;
    //• User trap setup registers
    struct vm_reg ustatus;
    struct vm_reg uie;
    struct vm_reg utvec;
    //• Current execution mode (e.g., U-mode, S-mode, M-mode)
    enum Current_Mode currentmode;
};