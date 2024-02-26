/* here we stub out KVM functions so that we don't have to try and
 * force KVM support/compilation on MSVC/Windows where it will never
 * possibly function. - 2/26/2024 gdwnldsKSC
 */
#ifdef _MSC_VER
int kvm_init(int smp_cpus) {
    return 0; 
}

/* can't stub this out without defining CPU state, which is target specific.... 
void kvm_init_vcpu(CPUState *env) {
} */

void kvm_sync_vcpus(void) {
}
#endif