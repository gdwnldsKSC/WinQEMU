/*
 * QEMU PC keyboard emulation
 *
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "hw.h"
#include "isa.h"
#include "pc.h"
#include "ps2.h"
#include "sysemu.h"

/* debug PC keyboard */
//#define DEBUG_KBD
#ifdef DEBUG_KBD
#define DPRINTF(fmt, ...)                                       \
    do { printf("KBD: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...)
#endif

/*	Keyboard Controller Commands */
#define KBD_CCMD_READ_MODE	0x20	/* Read mode bits */
#define KBD_CCMD_WRITE_MODE	0x60	/* Write mode bits */
#define KBD_CCMD_GET_VERSION	0xA1	/* Get controller version */
#define KBD_CCMD_MOUSE_DISABLE	0xA7	/* Disable mouse interface */
#define KBD_CCMD_MOUSE_ENABLE	0xA8	/* Enable mouse interface */
#define KBD_CCMD_TEST_MOUSE	0xA9	/* Mouse interface test */
#define KBD_CCMD_SELF_TEST	0xAA	/* Controller self test */
#define KBD_CCMD_KBD_TEST	0xAB	/* Keyboard interface test */
#define KBD_CCMD_KBD_DISABLE	0xAD	/* Keyboard interface disable */
#define KBD_CCMD_KBD_ENABLE	0xAE	/* Keyboard interface enable */
#define KBD_CCMD_READ_INPORT    0xC0    /* read input port */
#define KBD_CCMD_READ_OUTPORT	0xD0    /* read output port */
#define KBD_CCMD_WRITE_OUTPORT	0xD1    /* write output port */
#define KBD_CCMD_WRITE_OBUF	0xD2
#define KBD_CCMD_WRITE_AUX_OBUF	0xD3    /* Write to output buffer as if
					   initiated by the auxiliary device */
#define KBD_CCMD_WRITE_MOUSE	0xD4	/* Write the following byte to the mouse */
#define KBD_CCMD_DISABLE_A20    0xDD    /* HP vectra only ? */
#define KBD_CCMD_ENABLE_A20     0xDF    /* HP vectra only ? */
#define KBD_CCMD_RESET	        0xFE

/* Keyboard Commands */
#define KBD_CMD_SET_LEDS	0xED	/* Set keyboard leds */
#define KBD_CMD_ECHO     	0xEE
#define KBD_CMD_GET_ID 	        0xF2	/* get keyboard ID */
#define KBD_CMD_SET_RATE	0xF3	/* Set typematic rate */
#define KBD_CMD_ENABLE		0xF4	/* Enable scanning */
#define KBD_CMD_RESET_DISABLE	0xF5	/* reset and disable scanning */
#define KBD_CMD_RESET_ENABLE   	0xF6    /* reset and enable scanning */
#define KBD_CMD_RESET		0xFF	/* Reset */

/* Keyboard Replies */
#define KBD_REPLY_POR		0xAA	/* Power on reset */
#define KBD_REPLY_ACK		0xFA	/* Command ACK */
#define KBD_REPLY_RESEND	0xFE	/* Command NACK, send the cmd again */

/* Status Register Bits */
#define KBD_STAT_OBF 		0x01	/* Keyboard output buffer full */
#define KBD_STAT_IBF 		0x02	/* Keyboard input buffer full */
#define KBD_STAT_SELFTEST	0x04	/* Self test successful */
#define KBD_STAT_CMD		0x08	/* Last write was a command write (0=data) */
#define KBD_STAT_UNLOCKED	0x10	/* Zero if keyboard locked */
#define KBD_STAT_MOUSE_OBF	0x20	/* Mouse output buffer full */
#define KBD_STAT_GTO 		0x40	/* General receive/xmit timeout */
#define KBD_STAT_PERR 		0x80	/* Parity error */

/* Controller Mode Register Bits */
#define KBD_MODE_KBD_INT	0x01	/* Keyboard data generate IRQ1 */
#define KBD_MODE_MOUSE_INT	0x02	/* Mouse data generate IRQ12 */
#define KBD_MODE_SYS 		0x04	/* The system flag (?) */
#define KBD_MODE_NO_KEYLOCK	0x08	/* The keylock doesn't affect the keyboard if set */
#define KBD_MODE_DISABLE_KBD	0x10	/* Disable keyboard interface */
#define KBD_MODE_DISABLE_MOUSE	0x20	/* Disable mouse interface */
#define KBD_MODE_KCC 		0x40	/* Scan code conversion to PC format */
#define KBD_MODE_RFU		0x80

/* Output Port Bits */
#define KBD_OUT_RESET           0x01    /* 1=normal mode, 0=reset */
#define KBD_OUT_A20             0x02    /* x86 only */
#define KBD_OUT_OBF             0x10    /* Keyboard output buffer full */
#define KBD_OUT_MOUSE_OBF       0x20    /* Mouse output buffer full */

/* Mouse Commands */
#define AUX_SET_SCALE11		0xE6	/* Set 1:1 scaling */
#define AUX_SET_SCALE21		0xE7	/* Set 2:1 scaling */
#define AUX_SET_RES		0xE8	/* Set resolution */
#define AUX_GET_SCALE		0xE9	/* Get scaling factor */
#define AUX_SET_STREAM		0xEA	/* Set stream mode */
#define AUX_POLL		0xEB	/* Poll */
#define AUX_RESET_WRAP		0xEC	/* Reset wrap mode */
#define AUX_SET_WRAP		0xEE	/* Set wrap mode */
#define AUX_SET_REMOTE		0xF0	/* Set remote mode */
#define AUX_GET_TYPE		0xF2	/* Get type */
#define AUX_SET_SAMPLE		0xF3	/* Set sample rate */
#define AUX_ENABLE_DEV		0xF4	/* Enable aux device */
#define AUX_DISABLE_DEV		0xF5	/* Disable aux device */
#define AUX_SET_DEFAULT		0xF6
#define AUX_RESET		0xFF	/* Reset aux device */
#define AUX_ACK			0xFA	/* Command byte ACK. */

#define MOUSE_STATUS_REMOTE     0x40
#define MOUSE_STATUS_ENABLED    0x20
#define MOUSE_STATUS_SCALE21    0x10

#define KBD_PENDING_KBD         1
#define KBD_PENDING_AUX         2

typedef struct KBDState {
    uint8_t write_cmd; /* if non zero, write data to port 60 is expected */
    uint8_t status;
    uint8_t mode;
    uint8_t outport;
    /* Bitmask of devices with data available.  */
    uint8_t pending;
    void *kbd;
    void *mouse;

    qemu_irq irq_kbd;
    qemu_irq irq_mouse;
    qemu_irq *a20_out;
    target_phys_addr_t mask;
} KBDState;

/* update irq and KBD_STAT_[MOUSE_]OBF */
/* XXX: not generating the irqs if KBD_MODE_DISABLE_KBD is set may be
   incorrect, but it avoids having to simulate exact delays */
static void kbd_update_irq(KBDState *s)
{
    int irq_kbd_level, irq_mouse_level;

    irq_kbd_level = 0;
    irq_mouse_level = 0;
    s->status &= ~(KBD_STAT_OBF | KBD_STAT_MOUSE_OBF);
    s->outport &= ~(KBD_OUT_OBF | KBD_OUT_MOUSE_OBF);
    if (s->pending) {
        s->status |= KBD_STAT_OBF;
        s->outport |= KBD_OUT_OBF;
        /* kbd data takes priority over aux data.  */
        if (s->pending == KBD_PENDING_AUX) {
            s->status |= KBD_STAT_MOUSE_OBF;
            s->outport |= KBD_OUT_MOUSE_OBF;
            if (s->mode & KBD_MODE_MOUSE_INT)
                irq_mouse_level = 1;
        } else {
            if ((s->mode & KBD_MODE_KBD_INT) &&
                !(s->mode & KBD_MODE_DISABLE_KBD))
                irq_kbd_level = 1;
        }
    }
    qemu_set_irq(s->irq_kbd, irq_kbd_level);
    qemu_set_irq(s->irq_mouse, irq_mouse_level);
}

static void kbd_update_kbd_irq(void *opaque, int level)
{
    KBDState *s = (KBDState *)opaque;

    if (level)
        s->pending |= KBD_PENDING_KBD;
    else
        s->pending &= ~KBD_PENDING_KBD;
    kbd_update_irq(s);
}

static void kbd_update_aux_irq(void *opaque, int level)
{
    KBDState *s = (KBDState *)opaque;

    if (level)
        s->pending |= KBD_PENDING_AUX;
    else
        s->pending &= ~KBD_PENDING_AUX;
    kbd_update_irq(s);
}

static uint32_t kbd_read_status(void *opaque, uint32_t addr)
{
    KBDState *s = opaque;
    int val;
    val = s->status;
    DPRINTF("kbd: read status=0x%02x\n", val);
    return val;
}

static void kbd_queue(KBDState *s, int b, int aux)
{
    if (aux)
        ps2_queue(s->mouse, b);
    else
        ps2_queue(s->kbd, b);
}

static void ioport92_write(void *opaque, uint32_t addr, uint32_t val)
{
    KBDState *s = opaque;

    DPRINTF("kbd: write outport=0x%02x\n", val);
    s->outport = val;
    if (s->a20_out) {
        qemu_set_irq(*s->a20_out, (val >> 1) & 1);
    }
    if (!(val & 1)) {
        qemu_system_reset_request();
    }
}

static uint32_t ioport92_read(void *opaque, uint32_t addr)
{
    KBDState *s = opaque;
    uint32_t ret;

    ret = s->outport;
    DPRINTF("kbd: read outport=0x%02x\n", ret);
    return ret;
}

static void kbd_write_command(void *opaque, uint32_t addr, uint32_t val)
{
    KBDState *s = opaque;

    DPRINTF("kbd: write cmd=0x%02x\n", val);
    switch(val) {
    case KBD_CCMD_READ_MODE:
        kbd_queue(s, s->mode, 0);
        break;
    case KBD_CCMD_WRITE_MODE:
    case KBD_CCMD_WRITE_OBUF:
    case KBD_CCMD_WRITE_AUX_OBUF:
    case KBD_CCMD_WRITE_MOUSE:
    case KBD_CCMD_WRITE_OUTPORT:
        s->write_cmd = val;
        break;
    case KBD_CCMD_MOUSE_DISABLE:
        s->mode |= KBD_MODE_DISABLE_MOUSE;
        break;
    case KBD_CCMD_MOUSE_ENABLE:
        s->mode &= ~KBD_MODE_DISABLE_MOUSE;
        break;
    case KBD_CCMD_TEST_MOUSE:
        kbd_queue(s, 0x00, 0);
        break;
    case KBD_CCMD_SELF_TEST:
        s->status |= KBD_STAT_SELFTEST;
        kbd_queue(s, 0x55, 0);
        break;
    case KBD_CCMD_KBD_TEST:
        kbd_queue(s, 0x00, 0);
        break;
    case KBD_CCMD_KBD_DISABLE:
        s->mode |= KBD_MODE_DISABLE_KBD;
        kbd_update_irq(s);
        break;
    case KBD_CCMD_KBD_ENABLE:
        s->mode &= ~KBD_MODE_DISABLE_KBD;
        kbd_update_irq(s);
        break;
    case KBD_CCMD_READ_INPORT:
        kbd_queue(s, 0x00, 0);
        break;
    case KBD_CCMD_READ_OUTPORT:
        kbd_queue(s, s->outport, 0);
        break;
    case KBD_CCMD_ENABLE_A20:
        if (s->a20_out) {
            qemu_irq_raise(*s->a20_out);
        }
        s->outport |= KBD_OUT_A20;
        break;
    case KBD_CCMD_DISABLE_A20:
        if (s->a20_out) {
            qemu_irq_lower(*s->a20_out);
        }
        s->outport &= ~KBD_OUT_A20;
        break;
    case KBD_CCMD_RESET:
        qemu_system_reset_request();
        break;
    case 0xff:
        /* ignore that - I don't know what is its use */
        break;
    default:
        fprintf(stderr, "qemu: unsupported keyboard cmd=0x%02x\n", val);
        break;
    }
}

static uint32_t kbd_read_data(void *opaque, uint32_t addr)
{
    KBDState *s = opaque;
    uint32_t val;

    if (s->pending == KBD_PENDING_AUX)
        val = ps2_read_data(s->mouse);
    else
        val = ps2_read_data(s->kbd);

    DPRINTF("kbd: read data=0x%02x\n", val);
    return val;
}

static void kbd_write_data(void *opaque, uint32_t addr, uint32_t val)
{
    KBDState *s = opaque;

    DPRINTF("kbd: write data=0x%02x\n", val);

    switch(s->write_cmd) {
    case 0:
        ps2_write_keyboard(s->kbd, val);
        break;
    case KBD_CCMD_WRITE_MODE:
        s->mode = val;
        ps2_keyboard_set_translation(s->kbd, (s->mode & KBD_MODE_KCC) != 0);
        /* ??? */
        kbd_update_irq(s);
        break;
    case KBD_CCMD_WRITE_OBUF:
        kbd_queue(s, val, 0);
        break;
    case KBD_CCMD_WRITE_AUX_OBUF:
        kbd_queue(s, val, 1);
        break;
    case KBD_CCMD_WRITE_OUTPORT:
        ioport92_write(s, 0, val);
        break;
    case KBD_CCMD_WRITE_MOUSE:
        ps2_write_mouse(s->mouse, val);
        break;
    default:
        break;
    }
    s->write_cmd = 0;
}

static void kbd_reset(void *opaque)
{
    KBDState *s = opaque;

    s->mode = KBD_MODE_KBD_INT | KBD_MODE_MOUSE_INT;
    s->status = KBD_STAT_CMD | KBD_STAT_UNLOCKED;
    s->outport = KBD_OUT_RESET | KBD_OUT_A20;
}

static const VMStateDescription vmstate_kbd = {
    .name = "pckbd",
    .version_id = 3,
    .minimum_version_id = 3,
    .minimum_version_id_old = 3,
    .fields      = (VMStateField []) {
        VMSTATE_UINT8(write_cmd, KBDState),
        VMSTATE_UINT8(status, KBDState),
        VMSTATE_UINT8(mode, KBDState),
        VMSTATE_UINT8(pending, KBDState),
        VMSTATE_END_OF_LIST()
    }
};

/* Memory mapped interface */
static uint32_t kbd_mm_readb (void *opaque, target_phys_addr_t addr)
{
    KBDState *s = opaque;

    if (addr & s->mask)
        return kbd_read_status(s, 0) & 0xff;
    else
        return kbd_read_data(s, 0) & 0xff;
}

static void kbd_mm_writeb (void *opaque, target_phys_addr_t addr, uint32_t value)
{
    KBDState *s = opaque;

    if (addr & s->mask)
        kbd_write_command(s, 0, value & 0xff);
    else
        kbd_write_data(s, 0, value & 0xff);
}

static CPUReadMemoryFunc * const kbd_mm_read[] = {
    &kbd_mm_readb,
    &kbd_mm_readb,
    &kbd_mm_readb,
};

static CPUWriteMemoryFunc * const kbd_mm_write[] = {
    &kbd_mm_writeb,
    &kbd_mm_writeb,
    &kbd_mm_writeb,
};

void i8042_mm_init(qemu_irq kbd_irq, qemu_irq mouse_irq,
                   target_phys_addr_t base, ram_addr_t size,
                   target_phys_addr_t mask)
{
    KBDState *s = qemu_mallocz(sizeof(KBDState));
    int s_io_memory;

    s->irq_kbd = kbd_irq;
    s->irq_mouse = mouse_irq;
    s->mask = mask;

    vmstate_register(0, &vmstate_kbd, s);
    s_io_memory = cpu_register_io_memory(kbd_mm_read, kbd_mm_write, s);
    cpu_register_physical_memory(base, size, s_io_memory);

    s->kbd = ps2_kbd_init(kbd_update_kbd_irq, s);
    s->mouse = ps2_mouse_init(kbd_update_aux_irq, s);
    qemu_register_reset(kbd_reset, s);
}

typedef struct ISAKBDState {
    ISADevice dev;
    KBDState  kbd;
} ISAKBDState;

void i8042_isa_mouse_fake_event(void *opaque)
{
    ISADevice *dev = opaque;
    KBDState *s = &(DO_UPCAST(ISAKBDState, dev, dev)->kbd);

    ps2_mouse_fake_event(s->mouse);
}

void i8042_setup_a20_line(ISADevice *dev, qemu_irq *a20_out)
{
    KBDState *s = &(DO_UPCAST(ISAKBDState, dev, dev)->kbd);

    s->a20_out = a20_out;
}

static const VMStateDescription vmstate_kbd_isa = {
    .name = "pckbd",
    .version_id = 3,
    .minimum_version_id = 3,
    .minimum_version_id_old = 3,
    .fields      = (VMStateField []) {
        VMSTATE_STRUCT(kbd, ISAKBDState, 0, vmstate_kbd, KBDState),
        VMSTATE_END_OF_LIST()
    }
};

static int i8042_initfn(ISADevice *dev)
{
    KBDState *s = &(DO_UPCAST(ISAKBDState, dev, dev)->kbd);

    isa_init_irq(dev, &s->irq_kbd, 1);
    isa_init_irq(dev, &s->irq_mouse, 12);

    register_ioport_read(0x60, 1, 1, kbd_read_data, s);
    register_ioport_write(0x60, 1, 1, kbd_write_data, s);
    register_ioport_read(0x64, 1, 1, kbd_read_status, s);
    register_ioport_write(0x64, 1, 1, kbd_write_command, s);
    register_ioport_read(0x92, 1, 1, ioport92_read, s);
    register_ioport_write(0x92, 1, 1, ioport92_write, s);

    s->kbd = ps2_kbd_init(kbd_update_kbd_irq, s);
    s->mouse = ps2_mouse_init(kbd_update_aux_irq, s);
    qemu_register_reset(kbd_reset, s);
    return 0;
}

static ISADeviceInfo i8042_info = {
    .qdev.name     = "i8042",
    .qdev.size     = sizeof(ISAKBDState),
    .qdev.vmsd     = &vmstate_kbd_isa,
    .qdev.no_user  = 1,
    .init          = i8042_initfn,
};

static void i8042_register(void)
{
    isa_qdev_register(&i8042_info);
}
device_init(i8042_register);
