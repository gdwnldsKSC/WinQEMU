/*
 * QEMU ESCC (Z8030/Z8530/Z85C30/SCC/ESCC) serial port emulation
 *
 * Copyright (c) 2003-2005 Fabrice Bellard
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
#include "sysbus.h"
#include "escc.h"
#include "qemu-char.h"
#include "console.h"

/* debug serial */
//#define DEBUG_SERIAL

/* debug keyboard */
//#define DEBUG_KBD

/* debug mouse */
//#define DEBUG_MOUSE

/*
 * Chipset docs:
 * "Z80C30/Z85C30/Z80230/Z85230/Z85233 SCC/ESCC User Manual",
 * http://www.zilog.com/docs/serial/scc_escc_um.pdf
 *
 * On Sparc32 this is the serial port, mouse and keyboard part of chip STP2001
 * (Slave I/O), also produced as NCR89C105. See
 * http://www.ibiblio.org/pub/historic-linux/early-ports/Sparc/NCR/NCR89C105.txt
 *
 * The serial ports implement full AMD AM8530 or Zilog Z8530 chips,
 * mouse and keyboard ports don't implement all functions and they are
 * only asynchronous. There is no DMA.
 *
 * Z85C30 is also used on PowerMacs. There are some small differences
 * between Sparc version (sunzilog) and PowerMac (pmac):
 *  Offset between control and data registers
 *  There is some kind of lockup bug, but we can ignore it
 *  CTS is inverted
 *  DMA on pmac using DBDMA chip
 *  pmac can do IRDA and faster rates, sunzilog can only do 38400
 *  pmac baud rate generator clock is 3.6864 MHz, sunzilog 4.9152 MHz
 */

/*
 * Modifications:
 *  2006-Aug-10  Igor Kovalenko :   Renamed KBDQueue to SERIOQueue, implemented
 *                                  serial mouse queue.
 *                                  Implemented serial mouse protocol.
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
 
#ifdef DEBUG_SERIAL
#define SER_DPRINTF(fmt, ...)                                   \
    do { printf("SER: " fmt , ## __VA_ARGS__); } while (0)
#else
#define SER_DPRINTF(fmt, ...)
#endif
#ifdef DEBUG_KBD
#define KBD_DPRINTF(fmt, ...)                                   \
    do { printf("KBD: " fmt , ## __VA_ARGS__); } while (0)
#else
#define KBD_DPRINTF(fmt, ...)
#endif
#ifdef DEBUG_MOUSE
#define MS_DPRINTF(fmt, ...)                                    \
    do { printf("MSC: " fmt , ## __VA_ARGS__); } while (0)
#else
#define MS_DPRINTF(fmt, ...)
#endif

typedef enum {
    chn_a, chn_b,
} ChnID;

#define CHN_C(s) ((s)->chn == chn_b? 'b' : 'a')

typedef enum {
    ser, kbd, mouse,
} ChnType;

#define SERIO_QUEUE_SIZE 256

typedef struct {
    uint8_t data[SERIO_QUEUE_SIZE];
    int rptr, wptr, count;
} SERIOQueue;

#define SERIAL_REGS 16
typedef struct ChannelState {
    qemu_irq irq;
    uint32_t reg;
    uint32_t rxint, txint, rxint_under_svc, txint_under_svc;
    ChnID chn; // this channel, A (base+4) or B (base+0)
    ChnType type;
    struct ChannelState *otherchn;
    uint8_t rx, tx, wregs[SERIAL_REGS], rregs[SERIAL_REGS];
    SERIOQueue queue;
    CharDriverState *chr;
    int e0_mode, led_mode, caps_lock_mode, num_lock_mode;
    int disabled;
    int clock;
    uint32_t vmstate_dummy;
} ChannelState;

struct SerialState {
    SysBusDevice busdev;
    struct ChannelState chn[2];
    uint32_t it_shift;
    int mmio_index;
    uint32_t disabled;
    uint32_t frequency;
};

#define SERIAL_CTRL 0
#define SERIAL_DATA 1

#define W_CMD     0
#define CMD_PTR_MASK   0x07
#define CMD_CMD_MASK   0x38
#define CMD_HI         0x08
#define CMD_CLR_TXINT  0x28
#define CMD_CLR_IUS    0x38
#define W_INTR    1
#define INTR_INTALL    0x01
#define INTR_TXINT     0x02
#define INTR_RXMODEMSK 0x18
#define INTR_RXINT1ST  0x08
#define INTR_RXINTALL  0x10
#define W_IVEC    2
#define W_RXCTRL  3
#define RXCTRL_RXEN    0x01
#define W_TXCTRL1 4
#define TXCTRL1_PAREN  0x01
#define TXCTRL1_PAREV  0x02
#define TXCTRL1_1STOP  0x04
#define TXCTRL1_1HSTOP 0x08
#define TXCTRL1_2STOP  0x0c
#define TXCTRL1_STPMSK 0x0c
#define TXCTRL1_CLK1X  0x00
#define TXCTRL1_CLK16X 0x40
#define TXCTRL1_CLK32X 0x80
#define TXCTRL1_CLK64X 0xc0
#define TXCTRL1_CLKMSK 0xc0
#define W_TXCTRL2 5
#define TXCTRL2_TXEN   0x08
#define TXCTRL2_BITMSK 0x60
#define TXCTRL2_5BITS  0x00
#define TXCTRL2_7BITS  0x20
#define TXCTRL2_6BITS  0x40
#define TXCTRL2_8BITS  0x60
#define W_SYNC1   6
#define W_SYNC2   7
#define W_TXBUF   8
#define W_MINTR   9
#define MINTR_STATUSHI 0x10
#define MINTR_RST_MASK 0xc0
#define MINTR_RST_B    0x40
#define MINTR_RST_A    0x80
#define MINTR_RST_ALL  0xc0
#define W_MISC1  10
#define W_CLOCK  11
#define CLOCK_TRXC     0x08
#define W_BRGLO  12
#define W_BRGHI  13
#define W_MISC2  14
#define MISC2_PLLDIS   0x30
#define W_EXTINT 15
#define EXTINT_DCD     0x08
#define EXTINT_SYNCINT 0x10
#define EXTINT_CTSINT  0x20
#define EXTINT_TXUNDRN 0x40
#define EXTINT_BRKINT  0x80

#define R_STATUS  0
#define STATUS_RXAV    0x01
#define STATUS_ZERO    0x02
#define STATUS_TXEMPTY 0x04
#define STATUS_DCD     0x08
#define STATUS_SYNC    0x10
#define STATUS_CTS     0x20
#define STATUS_TXUNDRN 0x40
#define STATUS_BRK     0x80
#define R_SPEC    1
#define SPEC_ALLSENT   0x01
#define SPEC_BITS8     0x06
#define R_IVEC    2
#define IVEC_TXINTB    0x00
#define IVEC_LONOINT   0x06
#define IVEC_LORXINTA  0x0c
#define IVEC_LORXINTB  0x04
#define IVEC_LOTXINTA  0x08
#define IVEC_HINOINT   0x60
#define IVEC_HIRXINTA  0x30
#define IVEC_HIRXINTB  0x20
#define IVEC_HITXINTA  0x10
#define R_INTR    3
#define INTR_EXTINTB   0x01
#define INTR_TXINTB    0x02
#define INTR_RXINTB    0x04
#define INTR_EXTINTA   0x08
#define INTR_TXINTA    0x10
#define INTR_RXINTA    0x20
#define R_IPEN    4
#define R_TXCTRL1 5
#define R_TXCTRL2 6
#define R_BC      7
#define R_RXBUF   8
#define R_RXCTRL  9
#define R_MISC   10
#define R_MISC1  11
#define R_BRGLO  12
#define R_BRGHI  13
#define R_MISC1I 14
#define R_EXTINT 15

static void handle_kbd_command(ChannelState *s, int val);
static int serial_can_receive(void *opaque);
static void serial_receive_byte(ChannelState *s, int ch);

static void clear_queue(void *opaque)
{
    ChannelState *s = opaque;
    SERIOQueue *q = &s->queue;
    q->rptr = q->wptr = q->count = 0;
}

static void put_queue(void *opaque, int b)
{
    ChannelState *s = opaque;
    SERIOQueue *q = &s->queue;

    SER_DPRINTF("channel %c put: 0x%02x\n", CHN_C(s), b);
    if (q->count >= SERIO_QUEUE_SIZE)
        return;
    q->data[q->wptr] = b;
    if (++q->wptr == SERIO_QUEUE_SIZE)
        q->wptr = 0;
    q->count++;
    serial_receive_byte(s, 0);
}

static uint32_t get_queue(void *opaque)
{
    ChannelState *s = opaque;
    SERIOQueue *q = &s->queue;
    int val;

    if (q->count == 0) {
        return 0;
    } else {
        val = q->data[q->rptr];
        if (++q->rptr == SERIO_QUEUE_SIZE)
            q->rptr = 0;
        q->count--;
    }
    SER_DPRINTF("channel %c get 0x%02x\n", CHN_C(s), val);
    if (q->count > 0)
        serial_receive_byte(s, 0);
    return val;
}

static int escc_update_irq_chn(ChannelState *s)
{
    if ((((s->wregs[W_INTR] & INTR_TXINT) && s->txint == 1) ||
         // tx ints enabled, pending
         ((((s->wregs[W_INTR] & INTR_RXMODEMSK) == INTR_RXINT1ST) ||
           ((s->wregs[W_INTR] & INTR_RXMODEMSK) == INTR_RXINTALL)) &&
          s->rxint == 1) || // rx ints enabled, pending
         ((s->wregs[W_EXTINT] & EXTINT_BRKINT) &&
          (s->rregs[R_STATUS] & STATUS_BRK)))) { // break int e&p
        return 1;
    }
    return 0;
}

static void escc_update_irq(ChannelState *s)
{
    int irq;

    irq = escc_update_irq_chn(s);
    irq |= escc_update_irq_chn(s->otherchn);

    SER_DPRINTF("IRQ = %d\n", irq);
    qemu_set_irq(s->irq, irq);
}

static void escc_reset_chn(ChannelState *s)
{
    int i;

    s->reg = 0;
    for (i = 0; i < SERIAL_REGS; i++) {
        s->rregs[i] = 0;
        s->wregs[i] = 0;
    }
    s->wregs[W_TXCTRL1] = TXCTRL1_1STOP; // 1X divisor, 1 stop bit, no parity
    s->wregs[W_MINTR] = MINTR_RST_ALL;
    s->wregs[W_CLOCK] = CLOCK_TRXC; // Synch mode tx clock = TRxC
    s->wregs[W_MISC2] = MISC2_PLLDIS; // PLL disabled
    s->wregs[W_EXTINT] = EXTINT_DCD | EXTINT_SYNCINT | EXTINT_CTSINT |
        EXTINT_TXUNDRN | EXTINT_BRKINT; // Enable most interrupts
    if (s->disabled)
        s->rregs[R_STATUS] = STATUS_TXEMPTY | STATUS_DCD | STATUS_SYNC |
            STATUS_CTS | STATUS_TXUNDRN;
    else
        s->rregs[R_STATUS] = STATUS_TXEMPTY | STATUS_TXUNDRN;
    s->rregs[R_SPEC] = SPEC_BITS8 | SPEC_ALLSENT;

    s->rx = s->tx = 0;
    s->rxint = s->txint = 0;
    s->rxint_under_svc = s->txint_under_svc = 0;
    s->e0_mode = s->led_mode = s->caps_lock_mode = s->num_lock_mode = 0;
    clear_queue(s);
}

static void escc_reset(DeviceState *d)
{
    SerialState *s = container_of(d, SerialState, busdev.qdev);

    escc_reset_chn(&s->chn[0]);
    escc_reset_chn(&s->chn[1]);
}

static inline void set_rxint(ChannelState *s)
{
    s->rxint = 1;
    if (!s->txint_under_svc) {
        s->rxint_under_svc = 1;
        if (s->chn == chn_a) {
            if (s->wregs[W_MINTR] & MINTR_STATUSHI)
                s->otherchn->rregs[R_IVEC] = IVEC_HIRXINTA;
            else
                s->otherchn->rregs[R_IVEC] = IVEC_LORXINTA;
        } else {
            if (s->wregs[W_MINTR] & MINTR_STATUSHI)
                s->rregs[R_IVEC] = IVEC_HIRXINTB;
            else
                s->rregs[R_IVEC] = IVEC_LORXINTB;
        }
    }
    if (s->chn == chn_a)
        s->rregs[R_INTR] |= INTR_RXINTA;
    else
        s->otherchn->rregs[R_INTR] |= INTR_RXINTB;
    escc_update_irq(s);
}

static inline void set_txint(ChannelState *s)
{
    s->txint = 1;
    if (!s->rxint_under_svc) {
        s->txint_under_svc = 1;
        if (s->chn == chn_a) {
            if (s->wregs[W_MINTR] & MINTR_STATUSHI)
                s->otherchn->rregs[R_IVEC] = IVEC_HITXINTA;
            else
                s->otherchn->rregs[R_IVEC] = IVEC_LOTXINTA;
        } else {
            s->rregs[R_IVEC] = IVEC_TXINTB;
        }
    }
    if (s->chn == chn_a)
        s->rregs[R_INTR] |= INTR_TXINTA;
    else
        s->otherchn->rregs[R_INTR] |= INTR_TXINTB;
    escc_update_irq(s);
}

static inline void clr_rxint(ChannelState *s)
{
    s->rxint = 0;
    s->rxint_under_svc = 0;
    if (s->chn == chn_a) {
        if (s->wregs[W_MINTR] & MINTR_STATUSHI)
            s->otherchn->rregs[R_IVEC] = IVEC_HINOINT;
        else
            s->otherchn->rregs[R_IVEC] = IVEC_LONOINT;
        s->rregs[R_INTR] &= ~INTR_RXINTA;
    } else {
        if (s->wregs[W_MINTR] & MINTR_STATUSHI)
            s->rregs[R_IVEC] = IVEC_HINOINT;
        else
            s->rregs[R_IVEC] = IVEC_LONOINT;
        s->otherchn->rregs[R_INTR] &= ~INTR_RXINTB;
    }
    if (s->txint)
        set_txint(s);
    escc_update_irq(s);
}

static inline void clr_txint(ChannelState *s)
{
    s->txint = 0;
    s->txint_under_svc = 0;
    if (s->chn == chn_a) {
        if (s->wregs[W_MINTR] & MINTR_STATUSHI)
            s->otherchn->rregs[R_IVEC] = IVEC_HINOINT;
        else
            s->otherchn->rregs[R_IVEC] = IVEC_LONOINT;
        s->rregs[R_INTR] &= ~INTR_TXINTA;
    } else {
        if (s->wregs[W_MINTR] & MINTR_STATUSHI)
            s->rregs[R_IVEC] = IVEC_HINOINT;
        else
            s->rregs[R_IVEC] = IVEC_LONOINT;
        s->otherchn->rregs[R_INTR] &= ~INTR_TXINTB;
    }
    if (s->rxint)
        set_rxint(s);
    escc_update_irq(s);
}

static void escc_update_parameters(ChannelState *s)
{
    int speed, parity, data_bits, stop_bits;
    QEMUSerialSetParams ssp;

    if (!s->chr || s->type != ser)
        return;

    if (s->wregs[W_TXCTRL1] & TXCTRL1_PAREN) {
        if (s->wregs[W_TXCTRL1] & TXCTRL1_PAREV)
            parity = 'E';
        else
            parity = 'O';
    } else {
        parity = 'N';
    }
    if ((s->wregs[W_TXCTRL1] & TXCTRL1_STPMSK) == TXCTRL1_2STOP)
        stop_bits = 2;
    else
        stop_bits = 1;
    switch (s->wregs[W_TXCTRL2] & TXCTRL2_BITMSK) {
    case TXCTRL2_5BITS:
        data_bits = 5;
        break;
    case TXCTRL2_7BITS:
        data_bits = 7;
        break;
    case TXCTRL2_6BITS:
        data_bits = 6;
        break;
    default:
    case TXCTRL2_8BITS:
        data_bits = 8;
        break;
    }
    speed = s->clock / ((s->wregs[W_BRGLO] | (s->wregs[W_BRGHI] << 8)) + 2);
    switch (s->wregs[W_TXCTRL1] & TXCTRL1_CLKMSK) {
    case TXCTRL1_CLK1X:
        break;
    case TXCTRL1_CLK16X:
        speed /= 16;
        break;
    case TXCTRL1_CLK32X:
        speed /= 32;
        break;
    default:
    case TXCTRL1_CLK64X:
        speed /= 64;
        break;
    }
    ssp.speed = speed;
    ssp.parity = parity;
    ssp.data_bits = data_bits;
    ssp.stop_bits = stop_bits;
    SER_DPRINTF("channel %c: speed=%d parity=%c data=%d stop=%d\n", CHN_C(s),
                speed, parity, data_bits, stop_bits);
    qemu_chr_ioctl(s->chr, CHR_IOCTL_SERIAL_SET_PARAMS, &ssp);
}

static void escc_mem_writeb(void *opaque, target_phys_addr_t addr, uint32_t val)
{
    SerialState *serial = opaque;
    ChannelState *s;
    uint32_t saddr;
    int newreg, channel;

    val &= 0xff;
    saddr = (addr >> serial->it_shift) & 1;
    channel = (addr >> (serial->it_shift + 1)) & 1;
    s = &serial->chn[channel];
    switch (saddr) {
    case SERIAL_CTRL:
        SER_DPRINTF("Write channel %c, reg[%d] = %2.2x\n", CHN_C(s), s->reg,
                    val & 0xff);
        newreg = 0;
        switch (s->reg) {
        case W_CMD:
            newreg = val & CMD_PTR_MASK;
            val &= CMD_CMD_MASK;
            switch (val) {
            case CMD_HI:
                newreg |= CMD_HI;
                break;
            case CMD_CLR_TXINT:
                clr_txint(s);
                break;
            case CMD_CLR_IUS:
                if (s->rxint_under_svc)
                    clr_rxint(s);
                else if (s->txint_under_svc)
                    clr_txint(s);
                break;
            default:
                break;
            }
            break;
#ifndef _MSC_VER
        case W_INTR ... W_RXCTRL:
        case W_SYNC1 ... W_TXBUF:
        case W_MISC1 ... W_CLOCK:
        case W_MISC2 ... W_EXTINT:
#else
		case W_INTR:
		case W_IVEC:
		case W_RXCTRL:

		case W_SYNC1:
		case W_SYNC2:
		case W_TXBUF:
		
		case W_MISC1:
		case W_CLOCK:
	
		case W_MISC2:
		case W_EXTINT:

#endif
            s->wregs[s->reg] = val;
            break;
        case W_TXCTRL1:
        case W_TXCTRL2:
            s->wregs[s->reg] = val;
            escc_update_parameters(s);
            break;
        case W_BRGLO:
        case W_BRGHI:
            s->wregs[s->reg] = val;
            s->rregs[s->reg] = val;
            escc_update_parameters(s);
            break;
        case W_MINTR:
            switch (val & MINTR_RST_MASK) {
            case 0:
            default:
                break;
            case MINTR_RST_B:
                escc_reset_chn(&serial->chn[0]);
                return;
            case MINTR_RST_A:
                escc_reset_chn(&serial->chn[1]);
                return;
            case MINTR_RST_ALL:
                escc_reset(&serial->busdev.qdev);
                return;
            }
            break;
        default:
            break;
        }
        if (s->reg == 0)
            s->reg = newreg;
        else
            s->reg = 0;
        break;
    case SERIAL_DATA:
        SER_DPRINTF("Write channel %c, ch %d\n", CHN_C(s), val);
        s->tx = val;
        if (s->wregs[W_TXCTRL2] & TXCTRL2_TXEN) { // tx enabled
            if (s->chr)
                qemu_chr_write(s->chr, &s->tx, 1);
            else if (s->type == kbd && !s->disabled) {
                handle_kbd_command(s, val);
            }
        }
        s->rregs[R_STATUS] |= STATUS_TXEMPTY; // Tx buffer empty
        s->rregs[R_SPEC] |= SPEC_ALLSENT; // All sent
        set_txint(s);
        break;
    default:
        break;
    }
}

static uint32_t escc_mem_readb(void *opaque, target_phys_addr_t addr)
{
    SerialState *serial = opaque;
    ChannelState *s;
    uint32_t saddr;
    uint32_t ret;
    int channel;

    saddr = (addr >> serial->it_shift) & 1;
    channel = (addr >> (serial->it_shift + 1)) & 1;
    s = &serial->chn[channel];
    switch (saddr) {
    case SERIAL_CTRL:
        SER_DPRINTF("Read channel %c, reg[%d] = %2.2x\n", CHN_C(s), s->reg,
                    s->rregs[s->reg]);
        ret = s->rregs[s->reg];
        s->reg = 0;
        return ret;
    case SERIAL_DATA:
        s->rregs[R_STATUS] &= ~STATUS_RXAV;
        clr_rxint(s);
        if (s->type == kbd || s->type == mouse)
            ret = get_queue(s);
        else
            ret = s->rx;
        SER_DPRINTF("Read channel %c, ch %d\n", CHN_C(s), ret);
        if (s->chr)
            qemu_chr_accept_input(s->chr);
        return ret;
    default:
        break;
    }
    return 0;
}

static int serial_can_receive(void *opaque)
{
    ChannelState *s = opaque;
    int ret;

    if (((s->wregs[W_RXCTRL] & RXCTRL_RXEN) == 0) // Rx not enabled
        || ((s->rregs[R_STATUS] & STATUS_RXAV) == STATUS_RXAV))
        // char already available
        ret = 0;
    else
        ret = 1;
    return ret;
}

static void serial_receive_byte(ChannelState *s, int ch)
{
    SER_DPRINTF("channel %c put ch %d\n", CHN_C(s), ch);
    s->rregs[R_STATUS] |= STATUS_RXAV;
    s->rx = ch;
    set_rxint(s);
}

static void serial_receive_break(ChannelState *s)
{
    s->rregs[R_STATUS] |= STATUS_BRK;
    escc_update_irq(s);
}

static void serial_receive1(void *opaque, const uint8_t *buf, int size)
{
    ChannelState *s = opaque;
    serial_receive_byte(s, buf[0]);
}

static void serial_event(void *opaque, int event)
{
    ChannelState *s = opaque;
    if (event == CHR_EVENT_BREAK)
        serial_receive_break(s);
}

static CPUReadMemoryFunc * const escc_mem_read[3] = {
    escc_mem_readb,
    NULL,
    NULL,
};

static CPUWriteMemoryFunc * const escc_mem_write[3] = {
    escc_mem_writeb,
    NULL,
    NULL,
};

static const VMStateDescription vmstate_escc_chn = {
    .name ="escc_chn",
    .version_id = 2,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField []) {
        VMSTATE_UINT32(vmstate_dummy, ChannelState),
        VMSTATE_UINT32(reg, ChannelState),
        VMSTATE_UINT32(rxint, ChannelState),
        VMSTATE_UINT32(txint, ChannelState),
        VMSTATE_UINT32(rxint_under_svc, ChannelState),
        VMSTATE_UINT32(txint_under_svc, ChannelState),
        VMSTATE_UINT8(rx, ChannelState),
        VMSTATE_UINT8(tx, ChannelState),
        VMSTATE_BUFFER(wregs, ChannelState),
        VMSTATE_BUFFER(rregs, ChannelState),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription vmstate_escc = {
    .name ="escc",
    .version_id = 2,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField []) {
        VMSTATE_STRUCT_ARRAY(chn, SerialState, 2, 2, vmstate_escc_chn,
                             ChannelState),
        VMSTATE_END_OF_LIST()
    }
};

int escc_init(target_phys_addr_t base, qemu_irq irqA, qemu_irq irqB,
              CharDriverState *chrA, CharDriverState *chrB,
              int clock, int it_shift)
{
    DeviceState *dev;
    SysBusDevice *s;
    SerialState *d;

    dev = qdev_create(NULL, "escc");
    qdev_prop_set_uint32(dev, "disabled", 0);
    qdev_prop_set_uint32(dev, "frequency", clock);
    qdev_prop_set_uint32(dev, "it_shift", it_shift);
    qdev_prop_set_chr(dev, "chrB", chrB);
    qdev_prop_set_chr(dev, "chrA", chrA);
    qdev_prop_set_uint32(dev, "chnBtype", ser);
    qdev_prop_set_uint32(dev, "chnAtype", ser);
    qdev_init_nofail(dev);
    s = sysbus_from_qdev(dev);
    sysbus_connect_irq(s, 0, irqB);
    sysbus_connect_irq(s, 1, irqA);
    if (base) {
        sysbus_mmio_map(s, 0, base);
    }

    d = FROM_SYSBUS(SerialState, s);
    return d->mmio_index;
}

static const uint8_t keycodes[128] = {
    127, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 43, 53,
    54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 89, 76, 77, 78,
    79, 80, 81, 82, 83, 84, 85, 86, 87, 42, 99, 88, 100, 101, 102, 103,
    104, 105, 106, 107, 108, 109, 110, 47, 19, 121, 119, 5, 6, 8, 10, 12,
    14, 16, 17, 18, 7, 98, 23, 68, 69, 70, 71, 91, 92, 93, 125, 112,
    113, 114, 94, 50, 0, 0, 124, 9, 11, 0, 0, 0, 0, 0, 0, 0,
    90, 0, 46, 22, 13, 111, 52, 20, 96, 24, 28, 74, 27, 123, 44, 66,
    0, 45, 2, 4, 48, 0, 0, 21, 0, 0, 0, 0, 0, 120, 122, 67,
};

static const uint8_t e0_keycodes[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 90, 76, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 109, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 68, 69, 70, 0, 91, 0, 93, 0, 112,
    113, 114, 94, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 3, 25, 26, 49, 52, 72, 73, 97, 99, 111, 118, 120, 122, 67, 0,
};

static void sunkbd_event(void *opaque, int ch)
{
    ChannelState *s = opaque;
    int release = ch & 0x80;

    KBD_DPRINTF("Untranslated keycode %2.2x (%s)\n", ch, release? "release" :
                "press");
    switch (ch) {
    case 58: // Caps lock press
        s->caps_lock_mode ^= 1;
        if (s->caps_lock_mode == 2)
            return; // Drop second press
        break;
    case 69: // Num lock press
        s->num_lock_mode ^= 1;
        if (s->num_lock_mode == 2)
            return; // Drop second press
        break;
    case 186: // Caps lock release
        s->caps_lock_mode ^= 2;
        if (s->caps_lock_mode == 3)
            return; // Drop first release
        break;
    case 197: // Num lock release
        s->num_lock_mode ^= 2;
        if (s->num_lock_mode == 3)
            return; // Drop first release
        break;
    case 0xe0:
        s->e0_mode = 1;
        return;
    default:
        break;
    }
    if (s->e0_mode) {
        s->e0_mode = 0;
        ch = e0_keycodes[ch & 0x7f];
    } else {
        ch = keycodes[ch & 0x7f];
    }
    KBD_DPRINTF("Translated keycode %2.2x\n", ch);
    put_queue(s, ch | release);
}

static void handle_kbd_command(ChannelState *s, int val)
{
    KBD_DPRINTF("Command %d\n", val);
    if (s->led_mode) { // Ignore led byte
        s->led_mode = 0;
        return;
    }
    switch (val) {
    case 1: // Reset, return type code
        clear_queue(s);
        put_queue(s, 0xff);
        put_queue(s, 4); // Type 4
        put_queue(s, 0x7f);
        break;
    case 0xe: // Set leds
        s->led_mode = 1;
        break;
    case 7: // Query layout
    case 0xf:
        clear_queue(s);
        put_queue(s, 0xfe);
        put_queue(s, 0); // XXX, layout?
        break;
    default:
        break;
    }
}

static void sunmouse_event(void *opaque,
                               int dx, int dy, int dz, int buttons_state)
{
    ChannelState *s = opaque;
    int ch;

    MS_DPRINTF("dx=%d dy=%d buttons=%01x\n", dx, dy, buttons_state);

    ch = 0x80 | 0x7; /* protocol start byte, no buttons pressed */

    if (buttons_state & MOUSE_EVENT_LBUTTON)
        ch ^= 0x4;
    if (buttons_state & MOUSE_EVENT_MBUTTON)
        ch ^= 0x2;
    if (buttons_state & MOUSE_EVENT_RBUTTON)
        ch ^= 0x1;

    put_queue(s, ch);

    ch = dx;

    if (ch > 127)
        ch = 127;
    else if (ch < -127)
        ch = -127;

    put_queue(s, ch & 0xff);

    ch = -dy;

    if (ch > 127)
        ch = 127;
    else if (ch < -127)
        ch = -127;

    put_queue(s, ch & 0xff);

    // MSC protocol specify two extra motion bytes

    put_queue(s, 0);
    put_queue(s, 0);
}

void slavio_serial_ms_kbd_init(target_phys_addr_t base, qemu_irq irq,
                               int disabled, int clock, int it_shift)
{
    DeviceState *dev;
    SysBusDevice *s;

    dev = qdev_create(NULL, "escc");
    qdev_prop_set_uint32(dev, "disabled", disabled);
    qdev_prop_set_uint32(dev, "frequency", clock);
    qdev_prop_set_uint32(dev, "it_shift", it_shift);
    qdev_prop_set_chr(dev, "chrB", NULL);
    qdev_prop_set_chr(dev, "chrA", NULL);
    qdev_prop_set_uint32(dev, "chnBtype", mouse);
    qdev_prop_set_uint32(dev, "chnAtype", kbd);
    qdev_init_nofail(dev);
    s = sysbus_from_qdev(dev);
    sysbus_connect_irq(s, 0, irq);
    sysbus_connect_irq(s, 1, irq);
    sysbus_mmio_map(s, 0, base);
}

static int escc_init1(SysBusDevice *dev)
{
    SerialState *s = FROM_SYSBUS(SerialState, dev);
    int io;
    unsigned int i;

    s->chn[0].disabled = s->disabled;
    s->chn[1].disabled = s->disabled;
    for (i = 0; i < 2; i++) {
        sysbus_init_irq(dev, &s->chn[i].irq);
        s->chn[i].chn = 1 - i;
        s->chn[i].clock = s->frequency / 2;
        if (s->chn[i].chr) {
            qemu_chr_add_handlers(s->chn[i].chr, serial_can_receive,
                                  serial_receive1, serial_event, &s->chn[i]);
        }
    }
    s->chn[0].otherchn = &s->chn[1];
    s->chn[1].otherchn = &s->chn[0];

    io = cpu_register_io_memory(escc_mem_read, escc_mem_write, s);
    sysbus_init_mmio(dev, ESCC_SIZE << s->it_shift, io);
    s->mmio_index = io;

    if (s->chn[0].type == mouse) {
        qemu_add_mouse_event_handler(sunmouse_event, &s->chn[0], 0,
                                     "QEMU Sun Mouse");
    }
    if (s->chn[1].type == kbd) {
        qemu_add_kbd_event_handler(sunkbd_event, &s->chn[1]);
    }

    return 0;
}

static SysBusDeviceInfo escc_info = {
    .init = escc_init1,
    .qdev.name  = "escc",
    .qdev.size  = sizeof(SerialState),
    .qdev.vmsd  = &vmstate_escc,
    .qdev.reset = escc_reset,
    .qdev.props = (Property[]) {
        DEFINE_PROP_UINT32("frequency", SerialState, frequency,   0),
        DEFINE_PROP_UINT32("it_shift",  SerialState, it_shift,    0),
        DEFINE_PROP_UINT32("disabled",  SerialState, disabled,    0),
        DEFINE_PROP_UINT32("disabled",  SerialState, disabled,    0),
        DEFINE_PROP_UINT32("chnBtype",  SerialState, chn[0].type, 0),
        DEFINE_PROP_UINT32("chnAtype",  SerialState, chn[1].type, 0),
        DEFINE_PROP_CHR("chrB", SerialState, chn[0].chr),
        DEFINE_PROP_CHR("chrA", SerialState, chn[1].chr),
        DEFINE_PROP_END_OF_LIST(),
    }
};

static void escc_register_devices(void)
{
    sysbus_register_withprop(&escc_info);
}

device_init(escc_register_devices);
