// Phase 77 — exercise ORIGINAL ppu_load_elf and ppu_opd_resolve in iOS Simulator.
// The test ELF is original and synthetic; no PES 2013 game assets are used.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "ppu_recomp.h"

extern "C" void ppu_recomp_register(void);
extern "C" uint32_t ppu_function_count(void);
extern "C" void ps3_indirect_call(ppu_context* ctx);
extern "C" uint32_t ppu_load_elf(const char* path);
extern "C" int ppu_opd_resolve(uint32_t opd, uint32_t* code, uint32_t* toc);
extern "C" uint32_t vm_read32(uint64_t addr);
extern "C" int ios_guest_vm_init(void);
extern "C" int ios_guest_vm_shutdown(void);
extern "C" uint8_t* vm_base;

static bool check_zero(uint32_t addr, uint32_t length) {
    for (uint32_t i=0; i<length; ++i) if (vm_base[addr+i] != 0) return false;
    return true;
}

static bool run_existing_indirect_dispatch() {
    for (uint32_t target : {0x00010080u, 0x000100E0u}) {
        ppu_context ctx = {};
        ctx.gpr[1]=0x2A000;
        ctx.gpr[3]=35;
        ctx.gpr[12]=target;
        ctx.ctr=0x100A0;
        ctx.lr=0x1122334455667788ULL;
        ctx.thread_id=1;
        ps3_indirect_call(&ctx);
        uint64_t expected=(target==0x10080u)?47:51;
        std::printf("PHASE77_DISPATCH_TARGET=0x%08X RESULT=%llu EXPECTED=%llu\n",
            target,(unsigned long long)ctx.gpr[3],(unsigned long long)expected);
        if(ctx.gpr[3]!=expected || ctx.gpr[1]!=0x2A000 || ctx.lr!=0x1122334455667788ULL || ctx.ctr!=target)
            return false;
    }
    return true;
}

extern "C" int phase77_probe_main(const char* elf_path) {
    if(!elf_path || !*elf_path) { puts("PHASE77_ELF_PATH_MISSING"); return 7; }
    int init=ios_guest_vm_init();
    std::printf("VM_INIT_RESULT=%d\n",init);
    if(init!=0 || !vm_base) return 10;
    int failed=0;
    ppu_recomp_register();
    uint32_t count=ppu_function_count();
    std::printf("PRODUCTION_REGISTRY_COUNT=%u\n",count);
    if(count!=6) failed=11;
    if(!failed) {
        // Prove the loader zero-fills the destination, rather than finding zeroed memory.
        std::memset(vm_base+0x40010,0xCC,0x30);
        std::memset(vm_base+0x50010,0xCC,0x70);
        uint32_t entry=ppu_load_elf(elf_path); // original production implementation
        std::printf("PRODUCTION_ELF_ENTRY=0x%08X EXPECTED=0x00040000\n",entry);
        if(entry!=0x40000u) failed=12;
        uint32_t code=0,toc=0;
        int resolved=ppu_opd_resolve(0x40000u,&code,&toc);
        std::printf("PRODUCTION_OPD_RESOLVE=%d CODE=0x%08X TOC=0x%08X\n",resolved,code,toc);
        if(resolved!=1 || code!=0x100A0u || toc!=0x60000u) failed=13;
        uint32_t data=vm_read32(0x50000u);
        uint32_t data2=vm_read32(0x50004u);
        bool bss=check_zero(0x40010u,0x30u)&&check_zero(0x50010u,0x70u);
        std::printf("PRODUCTION_PT_LOAD_DATA=0x%08X SECOND=0x%08X EXPECTED=0x12345678,0xAABBCCDD\n",data,data2);
        std::printf("PRODUCTION_BSS_ZERO_FILLED=%s\n",bss?"YES":"NO");
        if(data!=0x12345678u || data2!=0xAABBCCDDu || !bss) failed=14;
        if(!failed) puts("PHASE77_ELF_LOAD_PASS");
        if(!failed) puts("PHASE77_OPD_PASS");
    }
    if(!failed) {
        bool regression=run_existing_indirect_dispatch();
        std::printf("PHASE77_DISPATCH_REGRESSION=%s\n",regression?"PASS":"FAIL");
        if(!regression) failed=15;
    }
    int shutdown=ios_guest_vm_shutdown();
    std::printf("VM_SHUTDOWN_RESULT=%d\n",shutdown);
    if(shutdown!=0 && !failed) failed=16;
    if(!failed) puts("PHASE77_GUEST_PASS");
    else std::printf("PHASE77_GUEST_FAIL=%d\n",failed);
    return failed;
}
