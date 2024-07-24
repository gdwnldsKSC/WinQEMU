/*
 * WinQEMU GPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL is available it will apply instead, WinQEMU elects to use only the 
 * General Public License version 3 (GPLv3) at this time for any software where a choice of 
 * GPL license versions is made available with the language indicating that GPLv3 or any later
 * version may be used, or where a choice of which version of the GPL is applied is otherwise unspecified.
 * 
 * Please contact Yan Wen (celestialwy@gmail.com) if you need additional information or have any questions.
 */
 
#ifndef QEMU_OSDEP_H
#define QEMU_OSDEP_H

#include <stdarg.h>
#include <stddef.h>
#ifdef __OpenBSD__
#include <sys/types.h>
#include <sys/signal.h>
#endif

#ifndef _WIN32
#include <sys/time.h>
#endif

#ifndef glue
#define xglue(x, y) x ## y
#define glue(x, y) xglue(x, y)
#define stringify(s)	tostring(s)
#define tostring(s)	#s
#endif

#ifndef likely
#if __GNUC__ < 3
#define __builtin_expect(x, n) (x)
#endif

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x)   __builtin_expect(!!(x), 0)
#endif

#ifdef CONFIG_NEED_OFFSETOF
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *) 0)->MEMBER)
#endif
#ifndef container_of
// MSVC modficiation ..... similar result, no GCC syntax
#ifndef _MSC_VER
#define container_of(ptr, type, member) ({                      \
        const typeof(((type *) 0)->member) *__mptr = (ptr);     \
        (type *) ((char *) __mptr - offsetof(type, member));})
#else
#define container_of(ptr, type, member) \
    ((type*)((char*)(ptr) - offsetof(type, member)))
#endif
#endif

#define typeof_field(type, field) typeof(((type *)0)->field)
#define type_check(t1,t2) ((t1*)0 - (t2*)0)

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef always_inline
#if !((__GNUC__ < 3) || defined(__APPLE__))
#ifdef __OPTIMIZE__
#define inline __attribute__ (( always_inline )) __inline__
#endif
#endif
#else
#define inline always_inline
#endif

#ifdef __i386__

#ifdef _MSC_VER
#define REGPARM //__fastcall
#else
#define REGPARM __attribute((regparm(3)))
#endif

#else
#define REGPARM
#endif

#define qemu_printf printf

#if defined (__GNUC__) && defined (__GNUC_MINOR__)
# define QEMU_GNUC_PREREQ(maj, min) \
         ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
# define QEMU_GNUC_PREREQ(maj, min) 0
#endif

void *qemu_memalign(size_t alignment, size_t size);
void *qemu_vmalloc(size_t size);
void qemu_vfree(void *ptr);

int qemu_create_pidfile(const char *filename);

#ifdef _WIN32
#ifndef _MSC_VER
int ffs(int i);
#else
__inline int ffs(int x)
{
	int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}
#endif


typedef struct {
    long tv_sec;
    long tv_usec;
} qemu_timeval;
int qemu_gettimeofday(qemu_timeval *tp);
#else
typedef struct timeval qemu_timeval;
#define qemu_gettimeofday(tp) gettimeofday(tp, NULL);
#endif /* !_WIN32 */

#endif
