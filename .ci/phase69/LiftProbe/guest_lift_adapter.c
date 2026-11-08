// Actual generated PPU functions from upstream lifter, not hand-written translations.
#include <stdint.h>
#include <string.h>
#include "Generated/ppu_recomp.h"
/* Read back guest bytes directly so the test can verify the 32-bit transport. */
extern uint8_t *vm_base;

static void (*lookup(uint64_t ea))(ppu_context*) {
    for (uint64_t i=0; i<function_table_count; i++) {
        if (function_table[i].addr == ea) return function_table[i].func;
    }
    return NULL;
}

int ios_lifted_branch(uint64_t input, uint64_t *output, int *table_ok) {
    if (!output || !table_ok) return -1;
    *table_ok=(function_table_count==6 && lookup(0x10000ULL)!=NULL && lookup(0x10020ULL)!=NULL && lookup(0x10040ULL)!=NULL && lookup(0x10080ULL)!=NULL && lookup(0x100a0ULL)!=NULL && lookup(0x100e0ULL)!=NULL);
    if (!*table_ok) return -2;
    ppu_context ctx;
    memset(&ctx,0,sizeof(ctx));
    ctx.gpr[3]=input;
    lookup(0x10000ULL)(&ctx);
    *output=ctx.gpr[3];
    return 0;
}

int ios_lifted_memory(uint32_t input, uint64_t *output, uint32_t *stored, int *bytes_ok) {
    if (!output || !stored || !bytes_ok || !vm_base) return -1;
    void (*fn)(ppu_context*)=lookup(0x10020ULL);
    if (!fn) return -2;
    const uint32_t ea=0x24000;
    memset(vm_base+ea,0,4);
    ppu_context ctx;
    memset(&ctx,0,sizeof(ctx));
    ctx.gpr[3]=input;
    ctx.gpr[4]=ea;
    fn(&ctx);
    *output=ctx.gpr[3];
    *stored=((uint32_t)vm_base[ea]<<24)|((uint32_t)vm_base[ea+1]<<16)|
            ((uint32_t)vm_base[ea+2]<<8)|((uint32_t)vm_base[ea+3]);
    *bytes_ok=(vm_base[ea]==(uint8_t)(input>>24) &&
               vm_base[ea+1]==(uint8_t)(input>>16) &&
               vm_base[ea+2]==(uint8_t)(input>>8) &&
               vm_base[ea+3]==(uint8_t)input);
    return 0;
}

/* New in phase 68: real cross-function BL generated from PowerPC. The PPC
   caller saves its original LR at 16(r1), creates a 144-byte guest frame,
   calls a second lifted function, restores the stack and original LR. */
int ios_lifted_direct_call(uint64_t input, uint64_t *output,
                           int *stack_ok, int *lr_ok,
                           int *saved_be, int *callee_registered) {
    if (!vm_base || !output || !stack_ok || !lr_ok || !saved_be || !callee_registered)
        return -1;
    void (*caller)(ppu_context*)=lookup(0x10040ULL);
    void (*callee)(ppu_context*)=lookup(0x10080ULL);
    *callee_registered=(function_table_count==6 && caller!=NULL && callee!=NULL);
    if (!*callee_registered) return -2;
    const uint64_t initial_sp=0x28000ULL;
    const uint64_t initial_lr=0xCAFEBABE0BADC0DEULL;
    memset(vm_base+0x27F00, 0, 0x200);
    ppu_context ctx;
    memset(&ctx,0,sizeof(ctx));
    ctx.gpr[1]=initial_sp;
    ctx.gpr[3]=input;
    ctx.lr=initial_lr;
    caller(&ctx);
    *output=ctx.gpr[3];
    *stack_ok=(ctx.gpr[1]==initial_sp);
    *lr_ok=(ctx.lr==initial_lr);
    /* Byte-for-byte check: saved guest LR at old_sp+16 must be big-endian. */
    *saved_be=1;
    for (unsigned i=0; i<8; i++) {
        uint8_t expected=(uint8_t)(initial_lr>>(56-8*i));
        if (vm_base[initial_sp+16+i]!=expected) *saved_be=0;
    }
    /* stdu also stored old SP at the newly allocated frame's first 8 bytes. */
    for (unsigned i=0; i<8; i++) {
        uint8_t expected=(uint8_t)(initial_sp>>(56-8*i));
        if (vm_base[initial_sp-144+i]!=expected) *stack_ok=0;
    }
    return 0;
}

/* Phase 69: EXERCISE THE GENERATED INDIRECT bctrl, NOT a host-side manual
   lookup of the target. Set PPC r12 to a guest effective address, then run
   the lifted PPC caller. Its mtctr/bctrl must resolve and invoke the target.
   The host lookup below obtains the ENTRY function only, not its callee. */
int ios_lifted_indirect_call(uint64_t input, uint64_t guest_target,
                             uint64_t *output, int *stack_ok, int *lr_ok,
                             int *ctr_ok, int *saved_be, int *registered) {
    if (!vm_base || !output || !stack_ok || !lr_ok || !ctr_ok ||
        !saved_be || !registered) return -1;
    void (*caller)(ppu_context*)=lookup(0x100a0ULL);
    *registered=(function_table_count==6 && caller!=NULL &&
                 lookup(0x10080ULL)!=NULL && lookup(0x100e0ULL)!=NULL);
    if (!*registered) return -2;
    if (guest_target!=0x10080ULL && guest_target!=0x100e0ULL) return -3;
    const uint64_t initial_sp=0x2A000ULL;
    const uint64_t initial_lr=0x1122334455667788ULL;
    memset(vm_base+0x29F00,0,0x200);
    ppu_context ctx;
    memset(&ctx,0,sizeof(ctx));
    ctx.gpr[1]=initial_sp;
    ctx.gpr[3]=input;
    ctx.gpr[12]=guest_target; /* the PPC-generated mtctr must consume this */
    ctx.lr=initial_lr;
    caller(&ctx);
    *output=ctx.gpr[3];
    *stack_ok=(ctx.gpr[1]==initial_sp);
    *lr_ok=(ctx.lr==initial_lr);
    *ctr_ok=(ctx.ctr==guest_target);
    *saved_be=1;
    for (unsigned i=0;i<8;i++) {
        uint8_t lr_byte=(uint8_t)(initial_lr>>(56-8*i));
        uint8_t sp_byte=(uint8_t)(initial_sp>>(56-8*i));
        if (vm_base[initial_sp+16+i]!=lr_byte) *saved_be=0;
        if (vm_base[initial_sp-144+i]!=sp_byte) *stack_ok=0;
    }
    return 0;
}
