    #include "types.h"
    #include "param.h"
    #include "memlayout.h"
    #include "riscv.h"
    #include "spinlock.h"
    #include "proc.h"
    #include "defs.h"
    #include "vmstate.h"

    struct vm_virtual_state virtual_state;

    struct vm_reg* search_register_from_uimm(uint32 uimm){
            struct vm_reg* registers[]={
                &virtual_state.mhartid,
                &virtual_state.mepc,
                &virtual_state.mstatus,
                &virtual_state.satp,
                &virtual_state.medeleg ,
                &virtual_state.mideleg,
                &virtual_state.sie,
                &virtual_state.stvec,
                &virtual_state.sstatus,
                &virtual_state.sepc

            };
            for(int i=0;i<sizeof(registers);i++){
                if(registers[i]->code==uimm){
                    return registers[i];
                }
            }
            return NULL;
    }

    int power2(int i){
        int result=1;
        for(int k=0;k<i;k++){
            result*=2;
        }
        return result;
    }
    uint64 get_register_by_rs1(uint32 rs1,struct proc *p){
        p=myproc();
        if(rs1==0xa){
            return p->trapframe->a0;
        }else if(rs1==0xb){
            return p->trapframe->a1;
        }else if(rs1==0xc){
            return p->trapframe->a2;
        }else if(rs1==0xd){
            return p->trapframe->a3;
        }else if(rs1==0xe){
            return p->trapframe->a4;
        }else if(rs1==0xf){
            return p->trapframe->a5;
        }
        return 0;
    };
    uint64* get_register_by_rd(uint32 rd,struct proc *p){
        p=myproc();
        if(rd==0xa){
            return &p->trapframe->a0;
        }else if(rd==0xb){
            return &p->trapframe->a1;
        }else if(rd==0xc){
            return &p->trapframe->a2;
        }else if(rd==0xd){
            return &p->trapframe->a3;
        }else if(rd==0xe){
            return &p->trapframe->a4;
        }else if(rd==0xf){
            return &p->trapframe->a5;
        }
        return 0;
    };
    void trap_and_emulate(void) {
        
        /* Comes here when a VM tries to execute a supervisor instruction. */
        struct proc *p = myproc();
        uint32 instruction;
        char *buffer=kalloc();
        copyin(p->pagetable,buffer,r_sepc(),PGSIZE);
        instruction=*(uint32*)buffer;

        uint64 addr     = p->trapframe->epc;

        uint32 op       = instruction%power2(7);
        uint32 rd       = (instruction%power2(11))/power2(7); ///destination
        uint32 funct3   = (instruction%power2(14))/power2(12); 
        uint32 rs1      = (instruction%power2(19))/power2(15); ///source
        uint32 uimm     = instruction/power2(20);

        

        struct vm_reg* priviged_register;
        
        int currentmode=virtual_state.currentmode;

        if(funct3==0x1){//csrw unprivileged to privileged rs1==f (a5) or  d (a1)
            printf("(PI at %p) op = %x, rd = %x, funct3 = %x, rs1 = %x, uimm = %x\n", addr, op, rd, funct3, rs1, uimm); 
           
            priviged_register=search_register_from_uimm(uimm);
            if(currentmode>=priviged_register->mode){
                    priviged_register->val=get_register_by_rs1(rs1,p);
                
            }else{
                exit(-1);
            }
        }else if(funct3==0x2){//csrr privileged to un privileged
            uint64 *written_registed=get_register_by_rd(rd,p);
            printf("(PI at %p) op = %x, rd = %x, funct3 = %x, rs1 = %x, uimm = %x\n", addr, op, rd, funct3, rs1, uimm);
            priviged_register=search_register_from_uimm(uimm);
            if(currentmode>=priviged_register->mode){
                *written_registed=priviged_register->val;
            }else{
                exit(-1);
            }
        }else if(uimm==0x302){//mret(machine to supervisor)
           // uint64 mask= virtual_state.mstatus.val & MSTATUS_MPP_MASK;
            printf("(PI at %p) op = %x, rd = %x, funct3 = %x, rs1 = %x, uimm = %x\n", addr, op, rd, funct3, rs1, uimm);
            if(currentmode==M ){
                virtual_state.mstatus.val &= ~MSTATUS_MPP_MASK;
                virtual_state.mstatus.val |= MSTATUS_MPP_S;
                virtual_state.satp.val=0;
                virtual_state.medeleg.val=0xffff;
                virtual_state.mideleg.val=0xffff;
                virtual_state.sie.val=(virtual_state.sie.val|SIE_SEIE | SIE_STIE | SIE_SSIE);
                p->trapframe->epc=virtual_state.mepc.val-4;
                virtual_state.currentmode=S;
            }else{
                exit(-1);
            }

        }else if(uimm==0x102){//sret (supervisor to usermode);
            
            printf("(PI at %p) op = %x, rd = %x, funct3 = %x, rs1 = %x, uimm = %x\n", addr, op, rd, funct3, rs1, uimm);
            if(currentmode==S ){
                
                virtual_state.sstatus.val&= ~SSTATUS_SPP;
                virtual_state.sstatus.val|= SSTATUS_SPIE;
                p->trapframe->epc=virtual_state.sepc.val-4;
                virtual_state.currentmode=U;
            }else{
                exit(-1);
            }
        }else if(uimm==0){//ecall
            if(currentmode==U){//cureent is user and need to jump to supervisor mode
                printf("(EC at %p)\n", addr);
                virtual_state.sepc.val=p->trapframe->epc;
                p->trapframe->epc=virtual_state.stvec.val-4;
                virtual_state.currentmode=S;

            }else if(currentmode==S){//current is supervisor and need to jump to machine mode
                printf("(EC at %p)\n", addr);
                virtual_state.mepc.val=p->trapframe->epc;
                p->trapframe->epc=virtual_state.mtvec.val-4;
                virtual_state.currentmode=M;
            }else{
                exit(-1);
            }
            
        };
        
    };
    void trap_and_emulate_init(void) {
        /* Create and initialize all state for the VM */
        //virtual_state.mscratch = (struct vm_reg) {.code=0x340, .mode=M, .val=0};
        virtual_state.mepc = (struct vm_reg) {.code=0x341, .mode=M, .val=0};
        //virtual_state.mcause = (struct vm_reg) {.code=0x342, .mode=M, .val=0};
        //virtual_state.mtval = (struct vm_reg) {.code=0x343, .mode=M, .val=0};
       // virtual_state.mip = (struct vm_reg) {.code=0x344, .mode=M, .val=0};
        //virtual_state.mtinst = (struct vm_reg) {.code=0x34A, .mode=M, .val=0};
       // virtual_state.mtval2 = (struct vm_reg) {.code=0x34B, .mode=M, .val=0};
        virtual_state.mstatus = (struct vm_reg) {.code=0x300, .mode=M, .val=0};
        //virtual_state.misa = (struct vm_reg) {.code=0x301, .mode=M, .val=0};
        virtual_state.medeleg = (struct vm_reg) {.code=0x302, .mode=M, .val=0};
        virtual_state.mideleg = (struct vm_reg) {.code=0x303, .mode=M, .val=0};
        //virtual_state.mie = (struct vm_reg) {.code=0x304, .mode=M, .val=0};
        //virtual_state.mtvec = (struct vm_reg) {.code=0x305, .mode=M, .val=0};
        //virtual_state.mcounteren = (struct vm_reg) {.code=0x306, .mode=M, .val=0};
        //virtual_state.mstatush = (struct vm_reg) {.code=0x310, .mode=M, .val=0};
        virtual_state.mvendorid = (struct vm_reg) {.code=0xF11, .mode=M, .val=0x637365353336};
        //virtual_state.marchid = (struct vm_reg) {.code=0xF12, .mode=M, .val=0};
        //virtual_state.mimpid = (struct vm_reg) {.code=0xF13, .mode=M, .val=0};
        virtual_state.mhartid = (struct vm_reg) {.code=0xF14, .mode=M, .val=0};
        //virtual_state.mconfigptr = (struct vm_reg) {.code=0xF15, .mode=M, .val=0};
        //virtual_state.pmpcfg0 = (struct vm_reg) {.code=0x3A0, .mode=U, .val=0};
        virtual_state.satp = (struct vm_reg) {.code=0x180, .mode=S, .val=0};
        virtual_state.sstatus = (struct vm_reg) {.code=0x100, .mode=S, .val=0};
        virtual_state.sie = (struct vm_reg) {.code=0x104, .mode=S, .val=0};
        virtual_state.stvec = (struct vm_reg) {.code=0x105, .mode=S, .val=0};
        //virtual_state.scounteren = (struct vm_reg) {.code=0x106, .mode=S, .val=0};
        virtual_state.sepc =(struct vm_reg) {.code=0x141, .mode=S, .val=0};
        //virtual_state.uscratch = (struct vm_reg) {.code=0x040, .mode=U, .val=0};
        //virtual_state.uepc = (struct vm_reg) {.code=0x041, .mode=U, .val=0};
        //virtual_state.ucause = (struct vm_reg) {.code=0x042, .mode=U, .val=0};
        //virtual_state.ubadaddr = (struct vm_reg) {.code=0x043, .mode=U, .val=0};
        //virtual_state.uip = (struct vm_reg) {.code=0x044, .mode=U, .val=0};
        //virtual_state.ustatus = (struct vm_reg) {.code=0x000, .mode=U, .val=0};
        //virtual_state.uie = (struct vm_reg) {.code=0x004, .mode=U, .val=0};
        //virtual_state.utvec = (struct vm_reg) {.code=0x005, .mode=U, .val=0};
        virtual_state.currentmode = M;
    }
