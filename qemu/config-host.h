/* Automatically generated by configure - do not modify */
// actually, in this case do modify and ignore the above. we do not yet
// have config generation - gdwnldsKSC
#define CONFIG_QEMU_SHAREDIR "D:\Images"
#define HOST_I386 1
#define HOST_LONG_BITS 32
#define CONFIG_WIN32 1
//#define CONFIG_GDBSTUB 1
#define CONFIG_STATIC 1
#define CONFIG_SLIRP 1
#define CONFIG_ADLIB 1
#define QEMU_VERSION "0.12.4"
#define QEMU_PKGVERSION "MS-VisualStudio-2022"
#define CONFIG_UNAME_RELEASE ""
#define CONFIG_AUDIO_DRIVERS &winwave_audio_driver, \

// we must keep an empty space below config_audio_drivers
// we leave CONFIG_BDRV_WHITELIST empty, so anything goes. 
#define CONFIG_BDRV_WHITELIST NULL

#ifdef _MSC_VER

#include <limits.h>
#define inline __inline
#define __func__ __FUNCTION__

#define INT8_MIN		(-128)
#define INT16_MIN		(-32767-1)
#define INT32_MIN		(-2147483647-1)
#define INT64_MIN		(-(int64_t)(9223372036854775807)-1)
#define INT8_MAX		(127)
#define INT16_MAX		(32767)
#define INT32_MAX		(2147483647)
#define INT64_MAX		((int64_t)(9223372036854775807))
#define UINT8_MAX		(255)
#define UINT16_MAX		(65535)
#define UINT32_MAX		(4294967295U)
#define UINT64_MAX		((uint64_t)(18446744073709551615))

// stub out KVM functions
/*
int kvm_init(int smp_cpus);
void kvm_init_vcpu(void *env);
void kvm_sync_vcpus(void);
*/

#endif