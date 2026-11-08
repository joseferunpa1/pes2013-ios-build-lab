/* Phase 69: TEST-ONLY indirect PowerPC dispatch for synthetic functions.
 * The lifter emits ps3_indirect_call(ctx) when lowering bctrl. It is
 * important to resolve the guest CTR address at RUN TIME and call the
 * generated PPU function, not hardcode the desired arithmetic result.
 *
 * This is not the production game dispatcher: it does not yet handle
 * ELF/OPD descriptors, HLE imports, relocations, or executable regions.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "Generated/ppu_recomp.h"

static void dispatch_guest_target(ppu_context *ctx, uint64_t guest_address) {
    if (ctx == NULL) {
        fputs("PHASE69_DISPATCH_NULL_CONTEXT\n", stderr);
        abort();
    }
    for (uint64_t i = 0; i < function_table_count; ++i) {
        if (function_table[i].addr == guest_address &&
            function_table[i].func != NULL) {
            function_table[i].func(ctx);
            return;
        }
    }
    /* Never silently ignore a failed branch to an unknown address. */
    fprintf(stderr, "PHASE69_UNKNOWN_GUEST_TARGET=0x%llx\n",
            (unsigned long long)guest_address);
    abort();
}

void ps3_indirect_call(ppu_context *ctx) {
    if (ctx == NULL) {
        fputs("PHASE69_DISPATCH_NULL_CONTEXT\n", stderr);
        abort();
    }
    dispatch_guest_target(ctx, ctx->ctr);
}

/* Declared by the lifter for other branch forms; included for clarity,
 * but not exercised by the phase 69 test instructions. */
void ps3_indirect_call_to(ppu_context *ctx, uint32_t target) {
    dispatch_guest_target(ctx, target);
}
