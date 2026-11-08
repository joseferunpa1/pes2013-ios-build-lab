// Phase 78: original ppu_load_elf + original ppu_run boots a synthetic PPC64 ELF.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "ppu_recomp.h"
extern "C" void ppu_recomp_register(void);
extern "C" uint32_t ppu_function_count(void);
extern "C" uint32_t ppu_load_elf(const char* path);
extern "C" int ppu_opd_resolve(uint32_t opd, uint32_t* code, uint32_t* toc);
extern "C" int ppu_run(uint32_t entry_opd, uint32_t stack_top);
extern "C" uint32_t vm_read32(uint64_t addr);
extern "C" int ios_guest_vm_init(void);
extern "C" int ios_guest_vm_shutdown(void);
extern "C" uint8_t* vm_base;

extern "C" int phase78_probe_main(const char* elf_path) {
    if (!elf_path || !*elf_path) { puts("PHASE78_ELF_PATH_MISSING");return 7; }
    int init=ios_guest_vm_init();
    printf("VM_INIT_RESULT=%d\n",init);
    if (init!=0 || !vm_base) return 10;
    int failed=0;
    ppu_recomp_register();
    uint32_t n=ppu_function_count();
    printf("PRODUCTION_REGISTRY_COUNT=%u EXPECTED=7\n",n);
    if (n!=7) failed=11;
    if (!failed) {
        // Nonzero prefill distinguishes BSS clearing from pre-zeroed VM.
        memset(vm_base+0x40010,0xCC,0x30);
        memset(vm_base+0x50010,0xCC,0x70);
        uint32_t entry=ppu_load_elf(elf_path);
        printf("PRODUCTION_ELF_ENTRY=0x%08X EXPECTED=0x00040000\n",entry);
        uint32_t code=0,toc=0;
        int r=ppu_opd_resolve(entry,&code,&toc);
        printf("PRODUCTION_OPD_RESOLVE=%d CODE=0x%08X TOC=0x%08X\n",r,code,toc);
        bool data=(vm_read32(0x50000)==0x12345678u);
        bool codebytes=(vm_read32(0x10100)==0x3C800005u && vm_read32(0x10104)==0x3860002Au && vm_read32(0x10108)==0x90640060u);
        bool bss=(vm_read32(0x50060)==0);
        printf("PHASE78_ELF_CODE_BYTES_MATCH=%s\n",codebytes?"YES":"NO");
        printf("PHASE78_BSS_BEFORE_ENTRY_ZERO=%s\n",bss?"YES":"NO");
        if (entry!=0x40000 || r!=1 || code!=0x10100 || toc!=0x60000 || !data || !codebytes || !bss) failed=12;
        if (!failed) puts("PHASE78_ELF_LOAD_PASS");
        if (!failed) puts("PHASE78_OPD_PASS");
        if (!failed) {
            // ppu_run is from original ppu_loader.cpp, not our own dispatcher.
            const uint32_t stack_top=0x002A0000u;
            int result=ppu_run(entry,stack_top);
            uint32_t marker=vm_read32(0x50060u);
            printf("PRODUCTION_PPU_RUN_RETURN=%d EXPECTED=0\n",result);
            printf("PRODUCTION_PPU_ENTRY_MARKER=0x%08X EXPECTED=0x0000002A\n",marker);
            if(result!=0 || marker!=42u) failed=13;
            if (!failed) puts("PHASE78_ENTRY_EXEC_PASS");
        }
    }
    int shutdown=ios_guest_vm_shutdown();
    printf("VM_SHUTDOWN_RESULT=%d\n",shutdown);
    if (shutdown!=0 && !failed) failed=14;
    if (!failed) puts("PHASE78_GUEST_PASS");
    else printf("PHASE78_GUEST_FAIL=%d\n",failed);
    return failed;
}
