#include "config-host.h"
#include "config-target.h"
#define CONFIG_QEMU_PREFIX "D:/Images"
#define CONFIG_QEMU_CONFDIR "D:/Images"
#define CONFIG_SOFTMMU 1
#define CONFIG_SDL 1

#define HAS_AUDIO_CHOICE

#define FLOATX80

//#define DEBUG_TCG 1 // TCG debug switch

#ifdef _MSC_VER
#define asm(X)

#define private __private
#endif
