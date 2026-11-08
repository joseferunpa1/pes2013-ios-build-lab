// No manually written PPU operation is implemented here.
// Call the function the upstream ps3recomp lifter generated from tiny.ppc.bin.
#include <stdint.h>
#include <string.h>
#include "Generated/ppu_recomp.h"

int ios_lifted_add7(uint64_t input, uint64_t *out, int *table_ok) {
    if (!out || !table_ok) return -1;
    *table_ok = (function_table_count == 1 &&
                 function_table[0].addr == 0x10000ULL &&
                 function_table[0].func != NULL);
    if (!*table_ok) return -2;
    ppu_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.gpr[3] = input;
    function_table[0].func(&ctx);
    *out = ctx.gpr[3];
    return 0;
}
