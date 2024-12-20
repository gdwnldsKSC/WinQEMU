#ifndef BSWAP_H
#define BSWAP_H

#include "config-host.h"

#include <inttypes.h>

#ifdef CONFIG_MACHINE_BSWAP_H
#include <sys/endian.h>
#include <sys/types.h>
#include <machine/bswap.h>
#else

#ifdef CONFIG_BYTESWAP_H
#include <byteswap.h>
#elif _MSC_VER // add in byteswap functions from MSVC
#include <stdlib.h>
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)
#define bswap_16(x) _byteswap_ushort(x)

#else

#define bswap_16(x) \
({ \
	uint16_t __x = (x); \
	((uint16_t)( \
		(((uint16_t)(__x) & (uint16_t)0x00ffU) << 8) | \
		(((uint16_t)(__x) & (uint16_t)0xff00U) >> 8) )); \
})

#define bswap_32(x) \
({ \
	uint32_t __x = (x); \
	((uint32_t)( \
		(((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) | \
		(((uint32_t)(__x) & (uint32_t)0x0000ff00UL) <<  8) | \
		(((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >>  8) | \
		(((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24) )); \
})

#define bswap_64(x) \
({ \
	uint64_t __x = (x); \
	((uint64_t)( \
		(uint64_t)(((uint64_t)(__x) & (uint64_t)0x00000000000000ffULL) << 56) | \
		(uint64_t)(((uint64_t)(__x) & (uint64_t)0x000000000000ff00ULL) << 40) | \
		(uint64_t)(((uint64_t)(__x) & (uint64_t)0x0000000000ff0000ULL) << 24) | \
		(uint64_t)(((uint64_t)(__x) & (uint64_t)0x00000000ff000000ULL) <<  8) | \
	        (uint64_t)(((uint64_t)(__x) & (uint64_t)0x000000ff00000000ULL) >>  8) | \
		(uint64_t)(((uint64_t)(__x) & (uint64_t)0x0000ff0000000000ULL) >> 24) | \
		(uint64_t)(((uint64_t)(__x) & (uint64_t)0x00ff000000000000ULL) >> 40) | \
		(uint64_t)(((uint64_t)(__x) & (uint64_t)0xff00000000000000ULL) >> 56) )); \
})

#endif /* !CONFIG_BYTESWAP_H */

static inline uint16_t bswap16(uint16_t x)
{
    return bswap_16(x);
}

static inline uint32_t bswap32(uint32_t x)
{
    return bswap_32(x);
}

static inline uint64_t bswap64(uint64_t x)
{
    return bswap_64(x);
}

#endif /* ! CONFIG_MACHINE_BSWAP_H */

static inline void bswap16s(uint16_t *s)
{
    *s = bswap16(*s);
}

static inline void bswap32s(uint32_t *s)
{
    *s = bswap32(*s);
}

static inline void bswap64s(uint64_t *s)
{
    *s = bswap64(*s);
}

#if defined(HOST_WORDS_BIGENDIAN)
#define be_bswap(v, size) (v)
#define le_bswap(v, size) bswap ## size(v)
#define be_bswaps(v, size)
#define le_bswaps(p, size) *p = bswap ## size(*p);
#else
#define le_bswap(v, size) (v)
#define be_bswap(v, size) bswap ## size(v)
#define le_bswaps(v, size)
#define be_bswaps(p, size) *p = bswap ## size(*p);
#endif

#define CPU_CONVERT(endian, size, type)\
static inline type endian ## size ## _to_cpu(type v)\
{\
    return endian ## _bswap(v, size);\
}\
\
static inline type cpu_to_ ## endian ## size(type v)\
{\
    return endian ## _bswap(v, size);\
}\
\
static inline void endian ## size ## _to_cpus(type *p)\
{\
    endian ## _bswaps(p, size)\
}\
\
static inline void cpu_to_ ## endian ## size ## s(type *p)\
{\
    endian ## _bswaps(p, size)\
}\
\
static inline type endian ## size ## _to_cpup(const type *p)\
{\
    return endian ## size ## _to_cpu(*p);\
}\
\
static inline void cpu_to_ ## endian ## size ## w(type *p, type v)\
{\
     *p = cpu_to_ ## endian ## size(v);\
}

CPU_CONVERT(be, 16, uint16_t)
CPU_CONVERT(be, 32, uint32_t)
CPU_CONVERT(be, 64, uint64_t)

CPU_CONVERT(le, 16, uint16_t)
CPU_CONVERT(le, 32, uint32_t)
CPU_CONVERT(le, 64, uint64_t)

/* unaligned versions (optimized for frequent unaligned accesses)*/

#if defined(__i386__) || defined(_ARCH_PPC)

#define cpu_to_le16wu(p, v) cpu_to_le16w(p, v)
#define cpu_to_le32wu(p, v) cpu_to_le32w(p, v)
#define le16_to_cpupu(p) le16_to_cpup(p)
#define le32_to_cpupu(p) le32_to_cpup(p)
#define be32_to_cpupu(p) be32_to_cpup(p)

#define cpu_to_be16wu(p, v) cpu_to_be16w(p, v)
#define cpu_to_be32wu(p, v) cpu_to_be32w(p, v)

#else

static inline void cpu_to_le16wu(uint16_t *p, uint16_t v)
{
    uint8_t *p1 = (uint8_t *)p;

    p1[0] = v & 0xff;
    p1[1] = v >> 8;
}

static inline void cpu_to_le32wu(uint32_t *p, uint32_t v)
{
    uint8_t *p1 = (uint8_t *)p;

    p1[0] = v & 0xff;
    p1[1] = v >> 8;
    p1[2] = v >> 16;
    p1[3] = v >> 24;
}

static inline uint16_t le16_to_cpupu(const uint16_t *p)
{
    const uint8_t *p1 = (const uint8_t *)p;
    return p1[0] | (p1[1] << 8);
}

static inline uint32_t le32_to_cpupu(const uint32_t *p)
{
    const uint8_t *p1 = (const uint8_t *)p;
    return p1[0] | (p1[1] << 8) | (p1[2] << 16) | (p1[3] << 24);
}

static inline uint32_t be32_to_cpupu(const uint32_t *p)
{
    const uint8_t *p1 = (const uint8_t *)p;
    return p1[3] | (p1[2] << 8) | (p1[1] << 16) | (p1[0] << 24);
}

static inline void cpu_to_be16wu(uint16_t *p, uint16_t v)
{
    uint8_t *p1 = (uint8_t *)p;

    p1[0] = v >> 8;
    p1[1] = v & 0xff;
}

static inline void cpu_to_be32wu(uint32_t *p, uint32_t v)
{
    uint8_t *p1 = (uint8_t *)p;

    p1[0] = v >> 24;
    p1[1] = v >> 16;
    p1[2] = v >> 8;
    p1[3] = v & 0xff;
}

#endif

#ifdef HOST_WORDS_BIGENDIAN
#define cpu_to_32wu cpu_to_be32wu
#define leul_to_cpu(v) glue(glue(le,HOST_LONG_BITS),_to_cpu)(v)
#else
#define cpu_to_32wu cpu_to_le32wu
#define leul_to_cpu(v) (v)
#endif

#undef le_bswap
#undef be_bswap
#undef le_bswaps
#undef be_bswaps

/* len must be one of 1, 2, 4 */
static inline uint32_t qemu_bswap_len(uint32_t value, int len)
{
    return bswap32(value) >> (32 - 8 * len);
}

#endif /* BSWAP_H */
