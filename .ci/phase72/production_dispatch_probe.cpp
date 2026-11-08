#include <stdint.h>
#include <stdio.h>
#include "ppu_recomp.h"

extern "C" void ppu_recomp_register(void);
extern "C" uint32_t ppu_function_count(void);
extern "C" void ps3_indirect_call(ppu_context* ctx);
extern "C" int ios_guest_vm_init(void);
extern "C" int ios_guest_vm_shutdown(void);

/* The code below uses the production registry and dispatcher from ppu_loader.cpp.
   It does not provide any replacement implementation of ps3_indirect_call. */
int main(void) {
    int code = ios_guest_vm_init();
    printf("VM_INIT_RESULT=%d\n", code);
    if (code != 0) return 10;

    ppu_recomp_register();
    uint32_t count = ppu_function_count();
    printf("PRODUCTION_REGISTRY_COUNT=%u\n", count);
    if (count < 6) return 11;

    const uint32_t targets[] = {0x00010080u, 0x000100E0u};
    const uint64_t expected[] = {42u, 46u};
    for (int i = 0; i < 2; ++i) {
        ppu_context ctx = {};
        ctx.gpr[3] = 35;
        ctx.gpr[1] = 0x00080000;
        ctx.ctr = targets[i];
        ctx.thread_id = 1;
        ps3_indirect_call(&ctx);
        printf("PRODUCTION_DISPATCH_TARGET=0x%08X OUTPUT=%llu EXPECTED=%llu\n",
               targets[i], (unsigned long long)ctx.gpr[3],
               (unsigned long long)expected[i]);
        if (ctx.gpr[3] != expected[i]) return 12 + i;
    }
    code = ios_guest_vm_shutdown();
    printf("VM_SHUTDOWN_RESULT=%d\n", code);
    if (code != 0) return 14;
    puts("PHASE72_RUNTIME_PASS");
    return 0;
}
