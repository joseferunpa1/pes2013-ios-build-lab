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
    *table_ok=(function_table_count==2 && lookup(0x10000ULL)!=NULL && lookup(0x10020ULL)!=NULL);
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
