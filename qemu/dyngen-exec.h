/*
 *  dyngen defines for micro operation code
 *
 *  Copyright (c) 2003 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
/*
 * WinQEMU GPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL is available it will apply instead, WinQEMU elects to use only the 
 * General Public License version 3 (GPLv3) at this time for any software where a choice of 
 * GPL license versions is made available with the language indicating that GPLv3 or any later
 * version may be used, or where a choice of which version of the GPL is applied is otherwise unspecified.
 * 
 * Please contact Yan Wen (celestialwy@gmail.com) if you need additional information or have any questions.
 */
#if !defined(__DYNGEN_EXEC_H__)
#define __DYNGEN_EXEC_H__

/* prevent Solaris from trying to typedef FILE in gcc's
   include/floatingpoint.h which will conflict with the
   definition down below */
#ifdef __sun__
#define _FILEDEFED
#endif

/* NOTE: standard headers should be used with special care at this
   point because host CPU registers are used as global variables. Some
   host headers do not allow that. */
#include <stddef.h>
#include <stdint.h>

#ifdef __OpenBSD__
#include <sys/types.h>
#endif

/* XXX: This may be wrong for 64-bit ILP32 hosts.  */
typedef void * host_reg_t;

#ifdef CONFIG_BSD
typedef struct __sFILE FILE;
#else
typedef struct FILE FILE;
#endif
extern int fprintf(FILE *, const char *, ...);
extern int fputs(const char *, FILE *);
extern int printf(const char *, ...);

#if defined(__i386__)
#define AREG0 "ebp"
#elif defined(__x86_64__)
#define AREG0 "r14"
#elif defined(_ARCH_PPC)
#define AREG0 "r27"
#elif defined(__arm__)
#define AREG0 "r7"
#elif defined(__hppa__)
#define AREG0 "r17"
#elif defined(__mips__)
#define AREG0 "s0"
#elif defined(__sparc__)
#ifdef CONFIG_SOLARIS
#define AREG0 "g2"
#else
#ifdef __sparc_v9__
#define AREG0 "g5"
#else
#define AREG0 "g6"
#endif
#endif
#elif defined(__s390__)
#define AREG0 "r10"
#elif defined(__alpha__)
/* Note $15 is the frame pointer, so anything in op-i386.c that would
   require a frame pointer, like alloca, would probably loose.  */
#define AREG0 "$15"
#elif defined(__mc68000)
#define AREG0 "%a5"
#elif defined(__ia64__)
#define AREG0 "r7"
#else
#error unsupported CPU
#endif

#define xglue(x, y) x ## y
#define glue(x, y) xglue(x, y)
#define stringify(s)	tostring(s)
#define tostring(s)	#s

/* The return address may point to the start of the next instruction.
   Subtracting one gets us the call instruction itself.  */
#if defined(__s390__) && !defined(__s390x__)
# define GETPC() ((void*)(((unsigned long)__builtin_return_address(0) & 0x7fffffffUL) - 1))
#elif defined(__arm__)
/* Thumb return addresses have the low bit set, so we need to subtract two.
   This is still safe in ARM mode because instructions are 4 bytes.  */
# define GETPC() ((void *)((unsigned long)__builtin_return_address(0) - 2))
#elif defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
#define GETPC _ReturnAddress
#else
# define GETPC() ((void *)((unsigned long)__builtin_return_address(0) - 1))
#endif

#endif /* !defined(__DYNGEN_EXEC_H__) */
