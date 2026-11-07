#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <errno.h>

/* Shared guest VM state. Real allocation occurs only when init is called. */
uint8_t *vm_base = NULL;
extern uint32_t ppu_vm_size;
static const size_t guest_size = (size_t)0xE0000000u;

int ios_guest_vm_init(void) {
    if (vm_base != NULL) return EALREADY;
    void *p = mmap(NULL, guest_size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) return errno ? errno : ENOMEM;
    vm_base = (uint8_t *)p;
    ppu_vm_size = (uint32_t)guest_size;
    return 0;
}

int ios_guest_vm_shutdown(void) {
    if (vm_base == NULL) return 0;
    if (munmap(vm_base, guest_size) != 0) return errno ? errno : EIO;
    vm_base = NULL;
    ppu_vm_size = 0;
    return 0;
}
