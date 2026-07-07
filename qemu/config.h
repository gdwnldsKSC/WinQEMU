#include "config-host.h"
//#include "config-target.h"
#define CONFIG_QEMU_PREFIX "D:/Images"
#define CONFIG_QEMU_CONFDIR "D:/Images"
#define CONFIG_SOFTMMU 1
#define CONFIG_SDL 1

#define HAS_AUDIO_CHOICE

#ifdef TARGET_I386
#define TARGET_ARCH "i386"
#define TARGET_I386 1
#define TARGET_TYPE TARGET_TYPE_I386
#define CONFIG_QEMU_LDST_OPTIMIZATION
#elif defined(TARGET_ALPHA)
#define TARGET_ARCH "alpha"
#define TARGET_ALPHA 1
#define TARGET_LONG_BITS 64
#define TARGET_TYPE TARGET_TYPE_ALPHA
#define TARGET_PHYS_ADDR_SPACE_BITS 44
#define CONFIG_QEMU_LDST_OPTIMIZATION
#endif

#define FLOATX80

//#define DEBUG_TCG 1 // TCG debug switch

#ifdef _MSC_VER
#define asm(X)

#define private __private
#endif