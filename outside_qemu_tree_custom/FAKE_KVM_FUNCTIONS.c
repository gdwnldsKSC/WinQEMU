/* here we stub out KVM functions so that we don't have to try and
 * force KVM support/compilation on MSVC/Windows where it will never
 * possibly function. - 2/26/2024 gdwnldsKSC
 */

#include <inttypes.h>
#include "targphys.h"

/*

future placeholder for cpustate header to be able to stub out more functions

*/

// necessary defines

typedef unsigned long ram_addr_t;

#ifdef _MSC_VER
int kvm_init(int smp_cpus) {
    return 0; 
}

/* can't stub this out without defining CPU state, which is target specific.... 
void kvm_init_vcpu(CPUState *env) {
} */

void kvm_sync_vcpus(void) {
}

int kvm_set_migration_log(int enable) {
    return 0;
}

void kvm_setup_guest_memory(void* start, size_t size) {
}

int kvm_coalesce_mmio_region(target_phys_addr_t start, ram_addr_t size) {
    return 0;
}

int kvm_uncoalesce_mmio_region(target_phys_addr_t start, ram_addr_t size) {
    return 0;
}

void kvm_set_phys_mem(target_phys_addr_t start_addr, ram_addr_t size, ram_addr_t phys_offset) {
}

int kvm_physical_sync_dirty_bitmap(target_phys_addr_t start_addr, target_phys_addr_t end_addr) {
    return 0;
}

int kvm_log_start(target_phys_addr_t phys_addr, ram_addr_t size) {
    return 0;
}

int kvm_log_stop(target_phys_addr_t phys_addr, ram_addr_t size) {
    return 0;
}

#endif