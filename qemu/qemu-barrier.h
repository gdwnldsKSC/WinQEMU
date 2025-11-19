#ifndef __QEMU_BARRIER_H
#define __QEMU_BARRIER_H 1

#ifndef _MSC_VER
/* FIXME: arch dependant, x86 version */
#define smp_wmb()   asm volatile("" ::: "memory")

/* Compiler barrier */
#define barrier()   asm volatile("" ::: "memory")
#else
/* FIXME: arch dependant, x86 version */
#define smp_wmb()   _ReadWriteBarrier()

/* Compiler barrier */
#define barrier()   _ReadWriteBarrier()
#endif

#endif
