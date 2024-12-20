/*
 *  APIC support
 *
 *  Copyright (c) 2004-2005 Fabrice Bellard
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>
 */

/*
 * WinQEMU GPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL is available it will apply instead, WinQEMU elects to use only the 
 * General Public License version 3 (GPLv3) at this time for any software where a choice of 
 * GPL license versions is made available with the language indicating that GPLv3 or any later
 * version may be used, or where a choice of which version of the GPL is applied is otherwise unspecified.
 * 
 * Please contact Yan Wen (celestialwy@gmail.com) if you need additional information or have any questions.
 */
 
#include "hw.h"
#include "pc.h"
#include "apic.h"
#include "pci.h"
#include "msix.h"
#include "qemu-timer.h"
#include "host-utils.h"
#include "kvm.h"

//#define DEBUG_APIC
//#define DEBUG_COALESCING

#ifdef DEBUG_APIC
#define DPRINTF(fmt, ...)                                       \
    do { printf("apic: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...)
#endif

#ifdef DEBUG_COALESCING
#define DPRINTF_C(fmt, ...)                                     \
    do { printf("apic: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF_C(fmt, ...)
#endif

/* APIC Local Vector Table */
#define APIC_LVT_TIMER   0
#define APIC_LVT_THERMAL 1
#define APIC_LVT_PERFORM 2
#define APIC_LVT_LINT0   3
#define APIC_LVT_LINT1   4
#define APIC_LVT_ERROR   5
#define APIC_LVT_NB      6

/* APIC delivery modes */
#define APIC_DM_FIXED	0
#define APIC_DM_LOWPRI	1
#define APIC_DM_SMI	2
#define APIC_DM_NMI	4
#define APIC_DM_INIT	5
#define APIC_DM_SIPI	6
#define APIC_DM_EXTINT	7

/* APIC destination mode */
#define APIC_DESTMODE_FLAT	0xf
#define APIC_DESTMODE_CLUSTER	1

#define APIC_TRIGGER_EDGE  0
#define APIC_TRIGGER_LEVEL 1

#define	APIC_LVT_TIMER_PERIODIC		(1<<17)
#define	APIC_LVT_MASKED			(1<<16)
#define	APIC_LVT_LEVEL_TRIGGER		(1<<15)
#define	APIC_LVT_REMOTE_IRR		(1<<14)
#define	APIC_INPUT_POLARITY		(1<<13)
#define	APIC_SEND_PENDING		(1<<12)

#define ESR_ILLEGAL_ADDRESS (1 << 7)

#define APIC_SV_ENABLE (1 << 8)

#define MAX_APICS 255
#define MAX_APIC_WORDS 8

/* Intel APIC constants: from include/asm/msidef.h */
#define MSI_DATA_VECTOR_SHIFT		0
#define MSI_DATA_VECTOR_MASK		0x000000ff
#define MSI_DATA_DELIVERY_MODE_SHIFT	8
#define MSI_DATA_TRIGGER_SHIFT		15
#define MSI_DATA_LEVEL_SHIFT		14
#define MSI_ADDR_DEST_MODE_SHIFT	2
#define MSI_ADDR_DEST_ID_SHIFT		12
#define	MSI_ADDR_DEST_ID_MASK		0x00ffff0

#define MSI_ADDR_BASE                   0xfee00000
#define MSI_ADDR_SIZE                   0x100000

typedef struct APICState {
    CPUState *cpu_env;
    uint32_t apicbase;
    uint8_t id;
    uint8_t arb_id;
    uint8_t tpr;
    uint32_t spurious_vec;
    uint8_t log_dest;
    uint8_t dest_mode;
    uint32_t isr[8];  /* in service register */
    uint32_t tmr[8];  /* trigger mode register */
    uint32_t irr[8]; /* interrupt request register */
    uint32_t lvt[APIC_LVT_NB];
    uint32_t esr; /* error register */
    uint32_t icr[2];

    uint32_t divide_conf;
    int count_shift;
    uint32_t initial_count;
    int64_t initial_count_load_time, next_time;
    uint32_t idx;
    QEMUTimer *timer;
    int sipi_vector;
    int wait_for_sipi;
} APICState;

static int apic_io_memory;
static APICState *local_apics[MAX_APICS + 1];
static int last_apic_idx = 0;
static int apic_irq_delivered;


static void apic_set_irq(APICState *s, int vector_num, int trigger_mode);
static void apic_update_irq(APICState *s);
static void apic_get_delivery_bitmask(uint32_t *deliver_bitmask,
                                      uint8_t dest, uint8_t dest_mode);

/* Find first bit starting from msb */
static int fls_bit(uint32_t value)
{
    return 31 - clz32(value);
}

/* Find first bit starting from lsb */
static int ffs_bit(uint32_t value)
{
    return ctz32(value);
}

static inline void set_bit(uint32_t *tab, int index)
{
    int i, mask;
    i = index >> 5;
    mask = 1 << (index & 0x1f);
    tab[i] |= mask;
}

static inline void reset_bit(uint32_t *tab, int index)
{
    int i, mask;
    i = index >> 5;
    mask = 1 << (index & 0x1f);
    tab[i] &= ~mask;
}

static inline int get_bit(uint32_t *tab, int index)
{
    int i, mask;
    i = index >> 5;
    mask = 1 << (index & 0x1f);
    return !!(tab[i] & mask);
}

static void apic_local_deliver(CPUState *env, int vector)
{
    APICState *s = env->apic_state;
    uint32_t lvt = s->lvt[vector];
    int trigger_mode;

    DPRINTF("%s: vector %d delivery mode %d\n", __func__, vector,
            (lvt >> 8) & 7);
    if (lvt & APIC_LVT_MASKED)
        return;

    switch ((lvt >> 8) & 7) {
    case APIC_DM_SMI:
        cpu_interrupt(env, CPU_INTERRUPT_SMI);
        break;

    case APIC_DM_NMI:
        cpu_interrupt(env, CPU_INTERRUPT_NMI);
        break;

    case APIC_DM_EXTINT:
        cpu_interrupt(env, CPU_INTERRUPT_HARD);
        break;

    case APIC_DM_FIXED:
        trigger_mode = APIC_TRIGGER_EDGE;
        if ((vector == APIC_LVT_LINT0 || vector == APIC_LVT_LINT1) &&
            (lvt & APIC_LVT_LEVEL_TRIGGER))
            trigger_mode = APIC_TRIGGER_LEVEL;
        apic_set_irq(s, lvt & 0xff, trigger_mode);
    }
}

void apic_deliver_pic_intr(CPUState *env, int level)
{
    if (level)
        apic_local_deliver(env, APIC_LVT_LINT0);
    else {
        APICState *s = env->apic_state;
        uint32_t lvt = s->lvt[APIC_LVT_LINT0];

        switch ((lvt >> 8) & 7) {
        case APIC_DM_FIXED:
            if (!(lvt & APIC_LVT_LEVEL_TRIGGER))
                break;
            reset_bit(s->irr, lvt & 0xff);
            /* fall through */
        case APIC_DM_EXTINT:
            cpu_reset_interrupt(env, CPU_INTERRUPT_HARD);
            break;
        }
    }
}

#define foreach_apic(apic, deliver_bitmask, code) \
{\
    int __i, __j, __mask;\
    for(__i = 0; __i < MAX_APIC_WORDS; __i++) {\
        __mask = deliver_bitmask[__i];\
        if (__mask) {\
            for(__j = 0; __j < 32; __j++) {\
                if (__mask & (1 << __j)) {\
                    apic = local_apics[__i * 32 + __j];\
                    if (apic) {\
                        code;\
                    }\
                }\
            }\
        }\
    }\
}

static void apic_bus_deliver(const uint32_t *deliver_bitmask,
                             uint8_t delivery_mode,
                             uint8_t vector_num, uint8_t polarity,
                             uint8_t trigger_mode)
{
    APICState *apic_iter;

    switch (delivery_mode) {
        case APIC_DM_LOWPRI:
            /* XXX: search for focus processor, arbitration */
            {
                int i, d;
                d = -1;
                for(i = 0; i < MAX_APIC_WORDS; i++) {
                    if (deliver_bitmask[i]) {
                        d = i * 32 + ffs_bit(deliver_bitmask[i]);
                        break;
                    }
                }
                if (d >= 0) {
                    apic_iter = local_apics[d];
                    if (apic_iter) {
                        apic_set_irq(apic_iter, vector_num, trigger_mode);
                    }
                }
            }
            return;

        case APIC_DM_FIXED:
            break;

        case APIC_DM_SMI:
            foreach_apic(apic_iter, deliver_bitmask,
                cpu_interrupt(apic_iter->cpu_env, CPU_INTERRUPT_SMI) );
            return;

        case APIC_DM_NMI:
            foreach_apic(apic_iter, deliver_bitmask,
                cpu_interrupt(apic_iter->cpu_env, CPU_INTERRUPT_NMI) );
            return;

        case APIC_DM_INIT:
            /* normal INIT IPI sent to processors */
            foreach_apic(apic_iter, deliver_bitmask,
                         cpu_interrupt(apic_iter->cpu_env, CPU_INTERRUPT_INIT) );
            return;

        case APIC_DM_EXTINT:
            /* handled in I/O APIC code */
            break;

        default:
            return;
    }

    foreach_apic(apic_iter, deliver_bitmask,
                 apic_set_irq(apic_iter, vector_num, trigger_mode) );
}

void apic_deliver_irq(uint8_t dest, uint8_t dest_mode,
                      uint8_t delivery_mode, uint8_t vector_num,
                      uint8_t polarity, uint8_t trigger_mode)
{
    uint32_t deliver_bitmask[MAX_APIC_WORDS];

    DPRINTF("%s: dest %d dest_mode %d delivery_mode %d vector %d"
            " polarity %d trigger_mode %d\n", __func__, dest, dest_mode,
            delivery_mode, vector_num, polarity, trigger_mode);
    apic_get_delivery_bitmask(deliver_bitmask, dest, dest_mode);
    apic_bus_deliver(deliver_bitmask, delivery_mode, vector_num, polarity,
                     trigger_mode);
}

void cpu_set_apic_base(CPUState *env, uint64_t val)
{
    APICState *s = env->apic_state;

    DPRINTF("cpu_set_apic_base: %016" PRIx64 "\n", val);
    if (!s)
        return;
    s->apicbase = (val & 0xfffff000) |
        (s->apicbase & (MSR_IA32_APICBASE_BSP | MSR_IA32_APICBASE_ENABLE));
    /* if disabled, cannot be enabled again */
    if (!(val & MSR_IA32_APICBASE_ENABLE)) {
        s->apicbase &= ~MSR_IA32_APICBASE_ENABLE;
        env->cpuid_features &= ~CPUID_APIC;
        s->spurious_vec &= ~APIC_SV_ENABLE;
    }
}

uint64_t cpu_get_apic_base(CPUState *env)
{
    APICState *s = env->apic_state;

    DPRINTF("cpu_get_apic_base: %016" PRIx64 "\n",
            s ? (uint64_t)s->apicbase: 0);
    return s ? s->apicbase : 0;
}

void cpu_set_apic_tpr(CPUX86State *env, uint8_t val)
{
    APICState *s = env->apic_state;
    if (!s)
        return;
    s->tpr = (val & 0x0f) << 4;
    apic_update_irq(s);
}

uint8_t cpu_get_apic_tpr(CPUX86State *env)
{
    APICState *s = env->apic_state;
    return s ? s->tpr >> 4 : 0;
}

/* return -1 if no bit is set */
static int get_highest_priority_int(uint32_t *tab)
{
    int i;
    for(i = 7; i >= 0; i--) {
        if (tab[i] != 0) {
            return i * 32 + fls_bit(tab[i]);
        }
    }
    return -1;
}

static int apic_get_ppr(APICState *s)
{
    int tpr, isrv, ppr;

    tpr = (s->tpr >> 4);
    isrv = get_highest_priority_int(s->isr);
    if (isrv < 0)
        isrv = 0;
    isrv >>= 4;
    if (tpr >= isrv)
        ppr = s->tpr;
    else
        ppr = isrv << 4;
    return ppr;
}

static int apic_get_arb_pri(APICState *s)
{
    /* XXX: arbitration */
    return 0;
}

/* signal the CPU if an irq is pending */
static void apic_update_irq(APICState *s)
{
    int irrv, ppr;
    if (!(s->spurious_vec & APIC_SV_ENABLE))
        return;
    irrv = get_highest_priority_int(s->irr);
    if (irrv < 0)
        return;
    ppr = apic_get_ppr(s);
    if (ppr && (irrv & 0xf0) <= (ppr & 0xf0))
        return;
    cpu_interrupt(s->cpu_env, CPU_INTERRUPT_HARD);
}

void apic_reset_irq_delivered(void)
{
    DPRINTF_C("%s: old coalescing %d\n", __func__, apic_irq_delivered);
    apic_irq_delivered = 0;
}

int apic_get_irq_delivered(void)
{
    DPRINTF_C("%s: returning coalescing %d\n", __func__, apic_irq_delivered);
    return apic_irq_delivered;
}

static void apic_set_irq(APICState *s, int vector_num, int trigger_mode)
{
    apic_irq_delivered += !get_bit(s->irr, vector_num);
    DPRINTF_C("%s: coalescing %d\n", __func__, apic_irq_delivered);

    set_bit(s->irr, vector_num);
    if (trigger_mode)
        set_bit(s->tmr, vector_num);
    else
        reset_bit(s->tmr, vector_num);
    apic_update_irq(s);
}

static void apic_eoi(APICState *s)
{
    int isrv;
    isrv = get_highest_priority_int(s->isr);
    if (isrv < 0)
        return;
    reset_bit(s->isr, isrv);
    /* XXX: send the EOI packet to the APIC bus to allow the I/O APIC to
            set the remote IRR bit for level triggered interrupts. */
    apic_update_irq(s);
}

static int apic_find_dest(uint8_t dest)
{
    APICState *apic = local_apics[dest];
    int i;

    if (apic && apic->id == dest)
        return dest;  /* shortcut in case apic->id == apic->idx */

    for (i = 0; i < MAX_APICS; i++) {
        apic = local_apics[i];
	if (apic && apic->id == dest)
            return i;
    }

    return -1;
}

static void apic_get_delivery_bitmask(uint32_t *deliver_bitmask,
                                      uint8_t dest, uint8_t dest_mode)
{
    APICState *apic_iter;
    int i;

    if (dest_mode == 0) {
        if (dest == 0xff) {
            memset(deliver_bitmask, 0xff, MAX_APIC_WORDS * sizeof(uint32_t));
        } else {
            int idx = apic_find_dest(dest);
            memset(deliver_bitmask, 0x00, MAX_APIC_WORDS * sizeof(uint32_t));
            if (idx >= 0)
                set_bit(deliver_bitmask, idx);
        }
    } else {
        /* XXX: cluster mode */
        memset(deliver_bitmask, 0x00, MAX_APIC_WORDS * sizeof(uint32_t));
        for(i = 0; i < MAX_APICS; i++) {
            apic_iter = local_apics[i];
            if (apic_iter) {
                if (apic_iter->dest_mode == 0xf) {
                    if (dest & apic_iter->log_dest)
                        set_bit(deliver_bitmask, i);
                } else if (apic_iter->dest_mode == 0x0) {
                    if ((dest & 0xf0) == (apic_iter->log_dest & 0xf0) &&
                        (dest & apic_iter->log_dest & 0x0f)) {
                        set_bit(deliver_bitmask, i);
                    }
                }
            }
        }
    }
}


void apic_init_reset(CPUState *env)
{
    APICState *s = env->apic_state;
    int i;

    if (!s)
        return;

    s->tpr = 0;
    s->spurious_vec = 0xff;
    s->log_dest = 0;
    s->dest_mode = 0xf;
    memset(s->isr, 0, sizeof(s->isr));
    memset(s->tmr, 0, sizeof(s->tmr));
    memset(s->irr, 0, sizeof(s->irr));
    for(i = 0; i < APIC_LVT_NB; i++)
        s->lvt[i] = 1 << 16; /* mask LVT */
    s->esr = 0;
    memset(s->icr, 0, sizeof(s->icr));
    s->divide_conf = 0;
    s->count_shift = 0;
    s->initial_count = 0;
    s->initial_count_load_time = 0;
    s->next_time = 0;
    s->wait_for_sipi = 1;

    env->halted = !(s->apicbase & MSR_IA32_APICBASE_BSP);
}

static void apic_startup(APICState *s, int vector_num)
{
    s->sipi_vector = vector_num;
    cpu_interrupt(s->cpu_env, CPU_INTERRUPT_SIPI);
}

void apic_sipi(CPUState *env)
{
    APICState *s = env->apic_state;

    cpu_reset_interrupt(env, CPU_INTERRUPT_SIPI);

    if (!s->wait_for_sipi)
        return;

    env->eip = 0;
    cpu_x86_load_seg_cache(env, R_CS, s->sipi_vector << 8, s->sipi_vector << 12,
                           env->segs[R_CS].limit, env->segs[R_CS].flags);
    env->halted = 0;
    s->wait_for_sipi = 0;
}

static void apic_deliver(APICState *s, uint8_t dest, uint8_t dest_mode,
                         uint8_t delivery_mode, uint8_t vector_num,
                         uint8_t polarity, uint8_t trigger_mode)
{
    uint32_t deliver_bitmask[MAX_APIC_WORDS];
    int dest_shorthand = (s->icr[0] >> 18) & 3;
    APICState *apic_iter;

    switch (dest_shorthand) {
    case 0:
        apic_get_delivery_bitmask(deliver_bitmask, dest, dest_mode);
        break;
    case 1:
        memset(deliver_bitmask, 0x00, sizeof(deliver_bitmask));
        set_bit(deliver_bitmask, s->idx);
        break;
    case 2:
        memset(deliver_bitmask, 0xff, sizeof(deliver_bitmask));
        break;
    case 3:
        memset(deliver_bitmask, 0xff, sizeof(deliver_bitmask));
        reset_bit(deliver_bitmask, s->idx);
        break;
    }

    switch (delivery_mode) {
        case APIC_DM_INIT:
            {
                int trig_mode = (s->icr[0] >> 15) & 1;
                int level = (s->icr[0] >> 14) & 1;
                if (level == 0 && trig_mode == 1) {
                    foreach_apic(apic_iter, deliver_bitmask,
                                 apic_iter->arb_id = apic_iter->id );
                    return;
                }
            }
            break;

        case APIC_DM_SIPI:
            foreach_apic(apic_iter, deliver_bitmask,
                         apic_startup(apic_iter, vector_num) );
            return;
    }

    apic_bus_deliver(deliver_bitmask, delivery_mode, vector_num, polarity,
                     trigger_mode);
}

int apic_get_interrupt(CPUState *env)
{
    APICState *s = env->apic_state;
    int intno;

    /* if the APIC is installed or enabled, we let the 8259 handle the
       IRQs */
    if (!s)
        return -1;
    if (!(s->spurious_vec & APIC_SV_ENABLE))
        return -1;

    /* XXX: spurious IRQ handling */
    intno = get_highest_priority_int(s->irr);
    if (intno < 0)
        return -1;
    if (s->tpr && intno <= s->tpr)
        return s->spurious_vec & 0xff;
    reset_bit(s->irr, intno);
    set_bit(s->isr, intno);
    apic_update_irq(s);
    return intno;
}

int apic_accept_pic_intr(CPUState *env)
{
    APICState *s = env->apic_state;
    uint32_t lvt0;

    if (!s)
        return -1;

    lvt0 = s->lvt[APIC_LVT_LINT0];

    if ((s->apicbase & MSR_IA32_APICBASE_ENABLE) == 0 ||
        (lvt0 & APIC_LVT_MASKED) == 0)
        return 1;

    return 0;
}

static uint32_t apic_get_current_count(APICState *s)
{
    int64_t d;
    uint32_t val;
    d = (qemu_get_clock(vm_clock) - s->initial_count_load_time) >>
        s->count_shift;
    if (s->lvt[APIC_LVT_TIMER] & APIC_LVT_TIMER_PERIODIC) {
        /* periodic */
        val = s->initial_count - (d % ((uint64_t)s->initial_count + 1));
    } else {
        if (d >= s->initial_count)
            val = 0;
        else
            val = s->initial_count - d;
    }
    return val;
}

static void apic_timer_update(APICState *s, int64_t current_time)
{
    int64_t next_time, d;

    if (!(s->lvt[APIC_LVT_TIMER] & APIC_LVT_MASKED)) {
        d = (current_time - s->initial_count_load_time) >>
            s->count_shift;
        if (s->lvt[APIC_LVT_TIMER] & APIC_LVT_TIMER_PERIODIC) {
            if (!s->initial_count)
                goto no_timer;
            d = ((d / ((uint64_t)s->initial_count + 1)) + 1) * ((uint64_t)s->initial_count + 1);
        } else {
            if (d >= s->initial_count)
                goto no_timer;
            d = (uint64_t)s->initial_count + 1;
        }
        next_time = s->initial_count_load_time + (d << s->count_shift);
        qemu_mod_timer(s->timer, next_time);
        s->next_time = next_time;
    } else {
    no_timer:
        qemu_del_timer(s->timer);
    }
}

static void apic_timer(void *opaque)
{
    APICState *s = opaque;

    apic_local_deliver(s->cpu_env, APIC_LVT_TIMER);
    apic_timer_update(s, s->next_time);
}

static uint32_t apic_mem_readb(void *opaque, target_phys_addr_t addr)
{
    return 0;
}

static uint32_t apic_mem_readw(void *opaque, target_phys_addr_t addr)
{
    return 0;
}

static void apic_mem_writeb(void *opaque, target_phys_addr_t addr, uint32_t val)
{
}

static void apic_mem_writew(void *opaque, target_phys_addr_t addr, uint32_t val)
{
}

static uint32_t apic_mem_readl(void *opaque, target_phys_addr_t addr)
{
    CPUState *env;
    APICState *s;
    uint32_t val;
    int index;

    env = cpu_single_env;
    if (!env)
        return 0;
    s = env->apic_state;

    index = (addr >> 4) & 0xff;
    switch(index) {
    case 0x02: /* id */
        val = s->id << 24;
        break;
    case 0x03: /* version */
        val = 0x11 | ((APIC_LVT_NB - 1) << 16); /* version 0x11 */
        break;
    case 0x08:
        val = s->tpr;
        break;
    case 0x09:
        val = apic_get_arb_pri(s);
        break;
    case 0x0a:
        /* ppr */
        val = apic_get_ppr(s);
        break;
    case 0x0b:
        val = 0;
        break;
    case 0x0d:
        val = s->log_dest << 24;
        break;
    case 0x0e:
        val = s->dest_mode << 28;
        break;
    case 0x0f:
        val = s->spurious_vec;
        break;
#ifndef _MSC_VER
    case 0x10 ... 0x17:
#else
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
#endif
        val = s->isr[index & 7];
        break;
#ifndef _MSC_VER
    case 0x18 ... 0x1f:
#else
	case 0x18:
	case 0x19:
	case 0x1a:
	case 0x1b:
	case 0x1c:
	case 0x1d:
	case 0x1e:
	case 0x1f:
#endif
        val = s->tmr[index & 7];
        break;
#ifndef _MSC_VER
    case 0x20 ... 0x27:
#else
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
#endif
        val = s->irr[index & 7];
        break;
    case 0x28:
        val = s->esr;
        break;
    case 0x30:
    case 0x31:
        val = s->icr[index & 1];
        break;
#ifndef _MSC_VER
    case 0x32 ... 0x37:
#else
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
#endif
        val = s->lvt[index - 0x32];
        break;
    case 0x38:
        val = s->initial_count;
        break;
    case 0x39:
        val = apic_get_current_count(s);
        break;
    case 0x3e:
        val = s->divide_conf;
        break;
    default:
        s->esr |= ESR_ILLEGAL_ADDRESS;
        val = 0;
        break;
    }
    DPRINTF("read: " TARGET_FMT_plx " = %08x\n", addr, val);
    return val;
}

static void apic_send_msi(target_phys_addr_t addr, uint32 data)
{
    uint8_t dest = (addr & MSI_ADDR_DEST_ID_MASK) >> MSI_ADDR_DEST_ID_SHIFT;
    uint8_t vector = (data & MSI_DATA_VECTOR_MASK) >> MSI_DATA_VECTOR_SHIFT;
    uint8_t dest_mode = (addr >> MSI_ADDR_DEST_MODE_SHIFT) & 0x1;
    uint8_t trigger_mode = (data >> MSI_DATA_TRIGGER_SHIFT) & 0x1;
    uint8_t delivery = (data >> MSI_DATA_DELIVERY_MODE_SHIFT) & 0x7;
    /* XXX: Ignore redirection hint. */
    apic_deliver_irq(dest, dest_mode, delivery, vector, 0, trigger_mode);
}

static void apic_mem_writel(void *opaque, target_phys_addr_t addr, uint32_t val)
{
    CPUState *env;
    APICState *s;
    int index = (addr >> 4) & 0xff;
    if (addr > 0xfff || !index) {
        /* MSI and MMIO APIC are at the same memory location,
         * but actually not on the global bus: MSI is on PCI bus
         * APIC is connected directly to the CPU.
         * Mapping them on the global bus happens to work because
         * MSI registers are reserved in APIC MMIO and vice versa. */
        apic_send_msi(addr, val);
        return;
    }

    env = cpu_single_env;
    if (!env)
        return;
    s = env->apic_state;

    DPRINTF("write: " TARGET_FMT_plx " = %08x\n", addr, val);

    switch(index) {
    case 0x02:
        s->id = (val >> 24);
        break;
    case 0x03:
        break;
    case 0x08:
        s->tpr = val;
        apic_update_irq(s);
        break;
    case 0x09:
    case 0x0a:
        break;
    case 0x0b: /* EOI */
        apic_eoi(s);
        break;
    case 0x0d:
        s->log_dest = val >> 24;
        break;
    case 0x0e:
        s->dest_mode = val >> 28;
        break;
    case 0x0f:
        s->spurious_vec = val & 0x1ff;
        apic_update_irq(s);
        break;
#ifndef _MSC_VER
    case 0x10 ... 0x17:
    case 0x18 ... 0x1f:
    case 0x20 ... 0x27:
#else
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:
	case 0x1a:
	case 0x1b:
	case 0x1c:
	case 0x1d:
	case 0x1e:
	case 0x1f:
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
#endif
    case 0x28:
        break;
    case 0x30:
        s->icr[0] = val;
        apic_deliver(s, (s->icr[1] >> 24) & 0xff, (s->icr[0] >> 11) & 1,
                     (s->icr[0] >> 8) & 7, (s->icr[0] & 0xff),
                     (s->icr[0] >> 14) & 1, (s->icr[0] >> 15) & 1);
        break;
    case 0x31:
        s->icr[1] = val;
        break;
#ifndef _MSC_VER
    case 0x32 ... 0x37:
#else
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
#endif
        {
            int n = index - 0x32;
            s->lvt[n] = val;
            if (n == APIC_LVT_TIMER)
                apic_timer_update(s, qemu_get_clock(vm_clock));
        }
        break;
    case 0x38:
        s->initial_count = val;
        s->initial_count_load_time = qemu_get_clock(vm_clock);
        apic_timer_update(s, s->initial_count_load_time);
        break;
    case 0x39:
        break;
    case 0x3e:
        {
            int v;
            s->divide_conf = val & 0xb;
            v = (s->divide_conf & 3) | ((s->divide_conf >> 1) & 4);
            s->count_shift = (v + 1) & 7;
        }
        break;
    default:
        s->esr |= ESR_ILLEGAL_ADDRESS;
        break;
    }
}

/* This function is only used for old state version 1 and 2 */
static int apic_load_old(QEMUFile *f, void *opaque, int version_id)
{
    APICState *s = opaque;
    int i;

    if (version_id > 2)
        return -EINVAL;

    /* XXX: what if the base changes? (registered memory regions) */
    qemu_get_be32s(f, &s->apicbase);
    qemu_get_8s(f, &s->id);
    qemu_get_8s(f, &s->arb_id);
    qemu_get_8s(f, &s->tpr);
    qemu_get_be32s(f, &s->spurious_vec);
    qemu_get_8s(f, &s->log_dest);
    qemu_get_8s(f, &s->dest_mode);
    for (i = 0; i < 8; i++) {
        qemu_get_be32s(f, &s->isr[i]);
        qemu_get_be32s(f, &s->tmr[i]);
        qemu_get_be32s(f, &s->irr[i]);
    }
    for (i = 0; i < APIC_LVT_NB; i++) {
        qemu_get_be32s(f, &s->lvt[i]);
    }
    qemu_get_be32s(f, &s->esr);
    qemu_get_be32s(f, &s->icr[0]);
    qemu_get_be32s(f, &s->icr[1]);
    qemu_get_be32s(f, &s->divide_conf);
    s->count_shift=qemu_get_be32(f);
    qemu_get_be32s(f, &s->initial_count);
    s->initial_count_load_time=qemu_get_be64(f);
    s->next_time=qemu_get_be64(f);

    if (version_id >= 2)
        qemu_get_timer(f, s->timer);
    return 0;
}

static const VMStateDescription vmstate_apic = {
    .name = "apic",
    .version_id = 3,
    .minimum_version_id = 3,
    .minimum_version_id_old = 1,
    .load_state_old = apic_load_old,
    .fields      = (VMStateField []) {
        VMSTATE_UINT32(apicbase, APICState),
        VMSTATE_UINT8(id, APICState),
        VMSTATE_UINT8(arb_id, APICState),
        VMSTATE_UINT8(tpr, APICState),
        VMSTATE_UINT32(spurious_vec, APICState),
        VMSTATE_UINT8(log_dest, APICState),
        VMSTATE_UINT8(dest_mode, APICState),
        VMSTATE_UINT32_ARRAY(isr, APICState, 8),
        VMSTATE_UINT32_ARRAY(tmr, APICState, 8),
        VMSTATE_UINT32_ARRAY(irr, APICState, 8),
        VMSTATE_UINT32_ARRAY(lvt, APICState, APIC_LVT_NB),
        VMSTATE_UINT32(esr, APICState),
        VMSTATE_UINT32_ARRAY(icr, APICState, 2),
        VMSTATE_UINT32(divide_conf, APICState),
        VMSTATE_INT32(count_shift, APICState),
        VMSTATE_UINT32(initial_count, APICState),
        VMSTATE_INT64(initial_count_load_time, APICState),
        VMSTATE_INT64(next_time, APICState),
        VMSTATE_TIMER(timer, APICState),
        VMSTATE_END_OF_LIST()
    }
};

static void apic_reset(void *opaque)
{
    APICState *s = opaque;
    int bsp;

    bsp = cpu_is_bsp(s->cpu_env);
    s->apicbase = 0xfee00000 |
        (bsp ? MSR_IA32_APICBASE_BSP : 0) | MSR_IA32_APICBASE_ENABLE;

    cpu_reset(s->cpu_env);
    apic_init_reset(s->cpu_env);

    if (bsp) {
        /*
         * LINT0 delivery mode on CPU #0 is set to ExtInt at initialization
         * time typically by BIOS, so PIC interrupt can be delivered to the
         * processor when local APIC is enabled.
         */
        s->lvt[APIC_LVT_LINT0] = 0x700;
    }
}

static CPUReadMemoryFunc * const apic_mem_read[3] = {
    apic_mem_readb,
    apic_mem_readw,
    apic_mem_readl,
};

static CPUWriteMemoryFunc * const apic_mem_write[3] = {
    apic_mem_writeb,
    apic_mem_writew,
    apic_mem_writel,
};

int apic_init(CPUState *env)
{
    APICState *s;

    if (last_apic_idx >= MAX_APICS)
        return -1;
    s = qemu_mallocz(sizeof(APICState));
    env->apic_state = s;
    s->idx = last_apic_idx++;
    s->id = env->cpuid_apic_id;
    s->cpu_env = env;

    msix_supported = 1;

    /* XXX: mapping more APICs at the same memory location */
    if (apic_io_memory == 0) {
        /* NOTE: the APIC is directly connected to the CPU - it is not
           on the global memory bus. */
        apic_io_memory = cpu_register_io_memory(apic_mem_read,
                                                apic_mem_write, NULL);
        /* XXX: what if the base changes? */
        cpu_register_physical_memory(MSI_ADDR_BASE, MSI_ADDR_SIZE,
                                     apic_io_memory);
    }
    s->timer = qemu_new_timer(vm_clock, apic_timer, s);

    vmstate_register(s->idx, &vmstate_apic, s);
    qemu_register_reset(apic_reset, s);

    local_apics[s->idx] = s;
    return 0;
}
