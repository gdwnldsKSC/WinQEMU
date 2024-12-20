#include "config-host.h"
#include "config-target.h"

#define CONFIG_QEMU_PREFIX "D:/Images"
#define CONFIG_QEMU_CONFDIR "D:/Images"
#define TARGET_ARCH "i386"
#define TARGET_I386 1
//#define TARGET_ARCH "alpha"
//#define TARGET_ALPHA 1
//#define USE_KQEMU 1 // TODO
#define CONFIG_SOFTMMU 1
#define CONFIG_SDL 1

#define FLOATX80

//#define DEBUG_TCG 1 // TCG debug switch

#ifdef _MSC_VER
#define asm(X)

#define private __private
//#define snprintf _snprintf // not needed MSVC2015
#endif