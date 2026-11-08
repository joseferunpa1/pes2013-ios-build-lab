// Phase 76: real PowerPC-lifted bctrl inside the ORIGINAL ps3recomp PPU dispatcher.
// No game code or copyrighted game data. Reuses 6 synthetic PowerPC functions from Phase 69.
// The host calls production ps3_indirect_call only for entry 0x100A0. Inside that lifted
// function, the PowerPC instruction bctrl must invoke production ps3_indirect_call again.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "ppu_recomp.h"

extern "C" void ppu_recomp_register(void);
extern "C" uint32_t ppu_function_count(void);
extern "C" void ps3_indirect_call(ppu_context* ctx);
extern "C" int ios_guest_vm_init(void);
extern "C" int ios_guest_vm_shutdown(void);
extern "C" uint8_t* vm_base;

static int run_case(uint32_t target, uint64_t expected) {
    const uint64_t initial_sp=0x0002A000ULL;
    const uint64_t initial_lr=0x1122334455667788ULL;
    if (!vm_base) return 30;
    std::memset(vm_base+0x29F00,0,0x200);
    ppu_context ctx = {};
    ctx.gpr[1]=initial_sp;
    ctx.gpr[3]=35;
    ctx.gpr[12]=target;  // mtctr r12 generated from PowerPC must set CTR to this.
    ctx.ctr=0x100A0;  // Entry to lifted PowerPC indirect-call caller, not the callee.
    ctx.lr=initial_lr;
    ctx.thread_id=1;
    ps3_indirect_call(&ctx);  // Real production entry dispatch.
    // The generated bctrl should invoke the SAME production dispatcher internally.
    bool stack_ok=(ctx.gpr[1]==initial_sp);
    bool lr_ok=(ctx.lr==initial_lr);
    bool ctr_ok=(ctx.ctr==target);
    bool saved_be=true;
    for (unsigned i=0;i<8;i++) {
        uint8_t lr_byte=(uint8_t)(initial_lr>>(56-8*i));
        uint8_t sp_byte=(uint8_t)(initial_sp>>(56-8*i));
        if(vm_base[initial_sp+16+i]!=lr_byte) saved_be=false;
        if(vm_base[initial_sp-144+i]!=sp_byte) stack_ok=false;
    }
    std::printf("PRODUCTION_NESTED_TARGET=0x%08X INPUT=35 OUTPUT=%llu EXPECTED=%llu STACK=%s LR=%s CTR=%s BIG_ENDIAN=%s\n",
        target, (unsigned long long)ctx.gpr[3], (unsigned long long)expected,
        stack_ok?"YES":"NO", lr_ok?"YES":"NO", ctr_ok?"YES":"NO", saved_be?"YES":"NO");
    return ctx.gpr[3]==expected && stack_ok && lr_ok && ctr_ok && saved_be ? 0 : 31;
}

int main() {
    int code=ios_guest_vm_init();
    std::printf("VM_INIT_RESULT=%d\n",code);
    if (code!=0) return 10;
    ppu_recomp_register();
    uint32_t count=ppu_function_count();
    std::printf("PRODUCTION_REGISTRY_COUNT=%u\n",count);
    int result=0;
    if(count!=6) result=11;
    if(!result) result=run_case(0x00010080u,47);
    if(!result) result=run_case(0x000100E0u,51);
    int shutdown=ios_guest_vm_shutdown();
    std::printf("VM_SHUTDOWN_RESULT=%d\n",shutdown);
    if(shutdown!=0 && !result) result=40;
    if(result==0) std::puts("PHASE76_GUEST_PASS");
    else std::printf("PHASE76_GUEST_FAIL=%d\n",result);
    return result;
}
