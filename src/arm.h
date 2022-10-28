#ifndef ARM_INCLUDE
#define ARM_INCLUDE

#include "device/arm_virt.h"

// TCR_EL1
#define TCR_EL1_T0SZ   0
#define TCR_EL1_RES0   6
#define TCR_EL1_EPD0   7
#define TCR_EL1_IRGN0  8
#define TCR_EL1_ORGN0  10
#define TCR_EL1_SH0    12
#define TCR_EL1_TG0    14
#define TCR_EL1_T1SZ   16
#define TCR_EL1_A1     22
#define TCR_EL1_EPD1   23
#define TCR_EL1_IRGN1  24
#define TCR_EL1_ORGN1  26
#define TCR_EL1_SH1    28
#define TCR_EL1_TG1    30
#define TCR_EL1_IPS    32
#define TCR_EL1_AS     36
#define TCR_EL1_TBI0   37
#define TCR_EL1_TBI1   38
#define TCR_EL1_HA     39
#define TCR_EL1_HD     40
#define TCR_EL1_HPD0   41
#define TCR_EL1_HPD1   42
#define TCR_EL1_HWU059 43
#define TCR_EL1_HWU060 44
#define TCR_EL1_HWU061 45
#define TCR_EL1_HWU062 46
#define TCR_EL1_HWU159 47
#define TCR_EL1_HWU160 48
#define TCR_EL1_HWU161 49
#define TCR_EL1_HWU162 50
#define TCR_EL1_TBID0  51
#define TCR_EL1_TBID1  52
#define TCR_EL1_NFD0   53
#define TCR_EL1_NFD1   54
#define TCR_EL1_E0PD0  55
#define TCR_EL1_E0PD1  56
#define TCR_EL1_TCMA0  57
#define TCR_EL1_TCMA1  58
#define TCR_EL1_DS     59


// trap frame: in ARM, there are seven modes. Among the 16 regular registers,
// r13 (sp), r14(lr), r15(pc) are banked in all modes.
// 1. In xv6_a, all kernel level activities (e.g., Syscall and IRQ) happens
// in the SVC mode. CPU is put in different modes by different events. We
// switch them to the SVC mode, by shoving the trapframe to the kernel stack.
// 2. during the context switched, the banked user space registers should also
// be saved/restored.
//
// Here is an example:
// 1. a user app issues a syscall (via SWI), its user-space registers are
// saved on its kernel stack, syscall is being served.
// 2. an interrupt happens, it preempted the syscall. the app's kernel-space
// registers are again saved on its stack.
// 3. interrupt service ended, and execution returns to the syscall.
// 4. kernel decides to reschedule (context switch), it saves the kernel states
// and switches to a new process (including user-space banked registers)
#ifndef __ASSEMBLER__
struct trapframe {
    uint64    r0;
    uint64    r1;
    uint64    r2;
    uint64    r3;
    uint64    r4;
    uint64    r5;
    uint64    r6;
    uint64    r7;
    uint64    r8;
    uint64    r9;
    uint64    r10;
    uint64    r11;
    uint64    r12;
    uint64    r13;
    uint64    r14;
    uint64    r15;
    uint64    r16;
    uint64    r17;
    uint64    r18;
    uint64    r19;
    uint64    r20;
    uint64    r21;
    uint64    r22;
    uint64    r23;
    uint64    r24;
    uint64    r25;
    uint64    r26;
    uint64    r27;
    uint64    r28;
    uint64    r29;
    uint64    r30;	// user mode lr
    uint64    sp;
    uint64    elr;
    uint64    spsr;
};
#endif

// cpsr/spsr bits
#define NO_INT      0xc0
#define DIS_INT     0x80

// Multiprocessor affinity
#define MPIDR_EL1_U    (1 << 30)
#define MPIDR_EL1_AFF0 (1 << 0) 
#define MPIDR_EL1_AFF1 (1 << 8)
#define MPIDR_EL1_AFF2 (1 << 16)

#endif
