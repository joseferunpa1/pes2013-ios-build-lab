// TEST-ONLY bounded guest-memory transport for generated PPU 32-bit loads/stores.
// It is functional for ordinary mapped RAM but is NOT the production runtime's
// MMIO, SPU coherency, reservation, or page-watch implementation.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

extern uint8_t *vm_base;
extern uint32_t ppu_vm_size;

// The generated header references these upstream runtime tuning variables.
// Disable inline fast paths so generated lwz/stw use bounded transport below.
/* Direct lifted BL calls may drain a cross-fragment trampoline through TLS.
   Keep the real dispatch slot; the diagnostic never sets it manually. */
__thread void (*g_trampoline_fn)(void*) = 0;

int vm_inline_ok=0;
int vm_store_inline_ok=0;
uint32_t ppu_hle_inject_base=0;
unsigned char g_spu_coh_bitmap[458752]={0};

static void check_ea(uint64_t ea) {
    if (!vm_base || ea < 0x10000u || ea > (uint64_t)ppu_vm_size - 4u) {
        fprintf(stderr, "PHASE68 invalid guest memory access 0x%llx\n",(unsigned long long)ea);
        abort();
    }
}
uint32_t vm_read32(uint64_t ea) {
    check_ea(ea);
    const uint8_t *p=vm_base+ea;
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
void vm_write32(uint64_t ea, uint32_t v) {
    check_ea(ea);
    uint8_t *p=vm_base+ea;
    p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16);
    p[2]=(uint8_t)(v>>8); p[3]=(uint8_t)v;
}

/* 64-bit big-endian guest-memory operations for the native PowerPC frame
   prologue and epilogue. They are test-only; this is NOT production MMIO. */
static void check_ea64(uint64_t ea) {
    if (!vm_base || ea < 0x10000u || ppu_vm_size < 8 ||
        ea > (uint64_t)ppu_vm_size - 8u) {
        fprintf(stderr, "PHASE68 invalid 64-bit guest memory access 0x%llx\n",
                (unsigned long long)ea);
        abort();
    }
}
uint64_t vm_read64(uint64_t ea) {
    check_ea64(ea);
    uint64_t v=0;
    for(unsigned i=0; i<8; i++) v=(v<<8)|vm_base[ea+i];
    return v;
}
void vm_write64(uint64_t ea, uint64_t v) {
    check_ea64(ea);
    for(unsigned i=0; i<8; i++) vm_base[ea+i]=(uint8_t)(v>>(56-8*i));
}
