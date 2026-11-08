// Compile this adapter against production ps3recomp headers. No game code is used.
#include <stdint.h>
#include "runtime/ppu/ppu_memory.h"

// Production function provided by toolkit/libs/system/sysPrxForUser.c.
extern int32_t sys_process_getpid(void);
int32_t ios_hle_getpid(void) { return sys_process_getpid(); }

// Exercise the real inline PPU big-endian translation of guest memory.
// Only vm_read32 is needed; no fake SPU/coherency APIs are provided.
int ios_hle_check_big_endian(void) {
    if (!vm_base) return 0;
    const uint32_t ea = 0x12000;
    vm_base[ea+0] = 0x12;
    vm_base[ea+1] = 0x34;
    vm_base[ea+2] = 0x56;
    vm_base[ea+3] = 0x78;
    return vm_read32(ea) == 0x12345678u;
}
