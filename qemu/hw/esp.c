/*
 * QEMU ESP/NCR53C9x emulation
 *
 * Copyright (c) 2005-2006 Fabrice Bellard
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

#include "sysbus.h"
 /* WinQEMU GPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL is available it will apply instead, WinQEMU elects to use only the 
 * General Public License version 3 (GPLv3) at this time for any software where a choice of 
 * GPL license versions is made available with the language indicating that GPLv3 or any later
 * version may be used, or where a choice of which version of the GPL is applied is otherwise unspecified.
 * 
 * Please contact Yan Wen (celestialwy@gmail.com) if you need additional information or have any questions.
 */
#include "scsi.h"
#include "esp.h"

/* debug ESP card */
//#define DEBUG_ESP

/*
 * On Sparc32, this is the ESP (NCR53C90) part of chip STP2000 (Master I/O),
 * also produced as NCR89C100. See
 * http://www.ibiblio.org/pub/historic-linux/early-ports/Sparc/NCR/NCR89C100.txt
 * and
 * http://www.ibiblio.org/pub/historic-linux/early-ports/Sparc/NCR/NCR53C9X.txt
 */

#ifdef DEBUG_ESP
#define DPRINTF(fmt, ...)                                       \
    do { printf("ESP: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while (0)
#endif

#define ESP_ERROR(fmt, ...)                                             \
    do { printf("ESP ERROR: %s: " fmt, __func__ , ## __VA_ARGS__); } while (0)

#define ESP_REGS 16
#define TI_BUFSZ 16

typedef struct ESPState ESPState;

struct ESPState {
    SysBusDevice busdev;
    uint32_t it_shift;
    qemu_irq irq;
    uint8_t rregs[ESP_REGS];
    uint8_t wregs[ESP_REGS];
    int32_t ti_size;
    uint32_t ti_rptr, ti_wptr;
    uint8_t ti_buf[TI_BUFSZ];
    uint32_t sense;
    uint32_t dma;
    SCSIBus bus;
    SCSIDevice *current_dev;
    uint8_t cmdbuf[TI_BUFSZ];
    uint32_t cmdlen;
    uint32_t do_cmd;

    /* The amount of data left in the current DMA transfer.  */
    uint32_t dma_left;
    /* The size of the current DMA transfer.  Zero if no transfer is in
       progress.  */
    uint32_t dma_counter;
    uint8_t *async_buf;
    uint32_t async_len;

    ESPDMAMemoryReadWriteFunc dma_memory_read;
    ESPDMAMemoryReadWriteFunc dma_memory_write;
    void *dma_opaque;
};

#define ESP_TCLO   0x0
#define ESP_TCMID  0x1
#define ESP_FIFO   0x2
#define ESP_CMD    0x3
#define ESP_RSTAT  0x4
#define ESP_WBUSID 0x4
#define ESP_RINTR  0x5
#define ESP_WSEL   0x5
#define ESP_RSEQ   0x6
#define ESP_WSYNTP 0x6
#define ESP_RFLAGS 0x7
#define ESP_WSYNO  0x7
#define ESP_CFG1   0x8
#define ESP_RRES1  0x9
#define ESP_WCCF   0x9
#define ESP_RRES2  0xa
#define ESP_WTEST  0xa
#define ESP_CFG2   0xb
#define ESP_CFG3   0xc
#define ESP_RES3   0xd
#define ESP_TCHI   0xe
#define ESP_RES4   0xf

#define CMD_DMA 0x80
#define CMD_CMD 0x7f

#define CMD_NOP      0x00
#define CMD_FLUSH    0x01
#define CMD_RESET    0x02
#define CMD_BUSRESET 0x03
#define CMD_TI       0x10
#define CMD_ICCS     0x11
#define CMD_MSGACC   0x12
#define CMD_PAD      0x18
#define CMD_SATN     0x1a
#define CMD_SEL      0x41
#define CMD_SELATN   0x42
#define CMD_SELATNS  0x43
#define CMD_ENSEL    0x44

#define STAT_DO 0x00
#define STAT_DI 0x01
#define STAT_CD 0x02
#define STAT_ST 0x03
#define STAT_MO 0x06
#define STAT_MI 0x07
#define STAT_PIO_MASK 0x06

#define STAT_TC 0x10
#define STAT_PE 0x20
#define STAT_GE 0x40
#define STAT_INT 0x80

#define BUSID_DID 0x07

#define INTR_FC 0x08
#define INTR_BS 0x10
#define INTR_DC 0x20
#define INTR_RST 0x80

#define SEQ_0 0x0
#define SEQ_CD 0x4

#define CFG1_RESREPT 0x40

#define TCHI_FAS100A 0x4

static void esp_raise_irq(ESPState *s)
{
    if (!(s->rregs[ESP_RSTAT] & STAT_INT)) {
        s->rregs[ESP_RSTAT] |= STAT_INT;
        qemu_irq_raise(s->irq);
        DPRINTF("Raise IRQ\n");
    }
}

static void esp_lower_irq(ESPState *s)
{
    if (s->rregs[ESP_RSTAT] & STAT_INT) {
        s->rregs[ESP_RSTAT] &= ~STAT_INT;
        qemu_irq_lower(s->irq);
        DPRINTF("Lower IRQ\n");
    }
}

static uint32_t get_cmd(ESPState *s, uint8_t *buf)
{
    uint32_t dmalen;
    int target;

    target = s->wregs[ESP_WBUSID] & BUSID_DID;
    if (s->dma) {
        dmalen = s->rregs[ESP_TCLO] | (s->rregs[ESP_TCMID] << 8);
        s->dma_memory_read(s->dma_opaque, buf, dmalen);
    } else {
        dmalen = s->ti_size;
        memcpy(buf, s->ti_buf, dmalen);
        buf[0] = 0;
    }
    DPRINTF("get_cmd: len %d target %d\n", dmalen, target);

    s->ti_size = 0;
    s->ti_rptr = 0;
    s->ti_wptr = 0;

    if (s->current_dev) {
        /* Started a new command before the old one finished.  Cancel it.  */
        s->current_dev->info->cancel_io(s->current_dev, 0);
        s->async_len = 0;
    }

    if (target >= ESP_MAX_DEVS || !s->bus.devs[target]) {
        // No such drive
        s->rregs[ESP_RSTAT] = 0;
        s->rregs[ESP_RINTR] = INTR_DC;
        s->rregs[ESP_RSEQ] = SEQ_0;
        esp_raise_irq(s);
        return 0;
    }
    s->current_dev = s->bus.devs[target];
    return dmalen;
}

static void do_busid_cmd(ESPState *s, uint8_t *buf, uint8_t busid)
{
    int32_t datalen;
    int lun;

    DPRINTF("do_busid_cmd: busid 0x%x\n", busid);
    lun = busid & 7;
    datalen = s->current_dev->info->send_command(s->current_dev, 0, buf, lun);
    s->ti_size = datalen;
    if (datalen != 0) {
        s->rregs[ESP_RSTAT] = STAT_TC;
        s->dma_left = 0;
        s->dma_counter = 0;
        if (datalen > 0) {
            s->rregs[ESP_RSTAT] |= STAT_DI;
            s->current_dev->info->read_data(s->current_dev, 0);
        } else {
            s->rregs[ESP_RSTAT] |= STAT_DO;
            s->current_dev->info->write_data(s->current_dev, 0);
        }
    }
    s->rregs[ESP_RINTR] = INTR_BS | INTR_FC;
    s->rregs[ESP_RSEQ] = SEQ_CD;
    esp_raise_irq(s);
}

static void do_cmd(ESPState *s, uint8_t *buf)
{
    uint8_t busid = buf[0];

    do_busid_cmd(s, &buf[1], busid);
}

static void handle_satn(ESPState *s)
{
    uint8_t buf[32];
    int len;

    len = get_cmd(s, buf);
    if (len)
        do_cmd(s, buf);
}

static void handle_s_without_atn(ESPState *s)
{
    uint8_t buf[32];
    int len;

    len = get_cmd(s, buf);
    if (len) {
        do_busid_cmd(s, buf, 0);
    }
}

static void handle_satn_stop(ESPState *s)
{
    s->cmdlen = get_cmd(s, s->cmdbuf);
    if (s->cmdlen) {
        DPRINTF("Set ATN & Stop: cmdlen %d\n", s->cmdlen);
        s->do_cmd = 1;
        s->rregs[ESP_RSTAT] = STAT_TC | STAT_CD;
        s->rregs[ESP_RINTR] = INTR_BS | INTR_FC;
        s->rregs[ESP_RSEQ] = SEQ_CD;
        esp_raise_irq(s);
    }
}

static void write_response(ESPState *s)
{
    DPRINTF("Transfer status (sense=%d)\n", s->sense);
    s->ti_buf[0] = s->sense;
    s->ti_buf[1] = 0;
    if (s->dma) {
        s->dma_memory_write(s->dma_opaque, s->ti_buf, 2);
        s->rregs[ESP_RSTAT] = STAT_TC | STAT_ST;
        s->rregs[ESP_RINTR] = INTR_BS | INTR_FC;
        s->rregs[ESP_RSEQ] = SEQ_CD;
    } else {
        s->ti_size = 2;
        s->ti_rptr = 0;
        s->ti_wptr = 0;
        s->rregs[ESP_RFLAGS] = 2;
    }
    esp_raise_irq(s);
}

static void esp_dma_done(ESPState *s)
{
    s->rregs[ESP_RSTAT] |= STAT_TC;
    s->rregs[ESP_RINTR] = INTR_BS;
    s->rregs[ESP_RSEQ] = 0;
    s->rregs[ESP_RFLAGS] = 0;
    s->rregs[ESP_TCLO] = 0;
    s->rregs[ESP_TCMID] = 0;
    esp_raise_irq(s);
}

static void esp_do_dma(ESPState *s)
{
    uint32_t len;
    int to_device;

    to_device = (s->ti_size < 0);
    len = s->dma_left;
    if (s->do_cmd) {
        DPRINTF("command len %d + %d\n", s->cmdlen, len);
        s->dma_memory_read(s->dma_opaque, &s->cmdbuf[s->cmdlen], len);
        s->ti_size = 0;
        s->cmdlen = 0;
        s->do_cmd = 0;
        do_cmd(s, s->cmdbuf);
        return;
    }
    if (s->async_len == 0) {
        /* Defer until data is available.  */
        return;
    }
    if (len > s->async_len) {
        len = s->async_len;
    }
    if (to_device) {
        s->dma_memory_read(s->dma_opaque, s->async_buf, len);
    } else {
        s->dma_memory_write(s->dma_opaque, s->async_buf, len);
    }
    s->dma_left -= len;
    s->async_buf += len;
    s->async_len -= len;
    if (to_device)
        s->ti_size += len;
    else
        s->ti_size -= len;
    if (s->async_len == 0) {
        if (to_device) {
            // ti_size is negative
            s->current_dev->info->write_data(s->current_dev, 0);
        } else {
            s->current_dev->info->read_data(s->current_dev, 0);
            /* If there is still data to be read from the device then
               complete the DMA operation immediately.  Otherwise defer
               until the scsi layer has completed.  */
            if (s->dma_left == 0 && s->ti_size > 0) {
                esp_dma_done(s);
            }
        }
    } else {
        /* Partially filled a scsi buffer. Complete immediately.  */
        esp_dma_done(s);
    }
}

static void esp_command_complete(SCSIBus *bus, int reason, uint32_t tag,
                                 uint32_t arg)
{
    ESPState *s = DO_UPCAST(ESPState, busdev.qdev, bus->qbus.parent);

    if (reason == SCSI_REASON_DONE) {
        DPRINTF("SCSI Command complete\n");
        if (s->ti_size != 0)
            DPRINTF("SCSI command completed unexpectedly\n");
        s->ti_size = 0;
        s->dma_left = 0;
        s->async_len = 0;
        if (arg)
            DPRINTF("Command failed\n");
        s->sense = arg;
        s->rregs[ESP_RSTAT] = STAT_ST;
        esp_dma_done(s);
        s->current_dev = NULL;
    } else {
        DPRINTF("transfer %d/%d\n", s->dma_left, s->ti_size);
        s->async_len = arg;
        s->async_buf = s->current_dev->info->get_buf(s->current_dev, 0);
        if (s->dma_left) {
            esp_do_dma(s);
        } else if (s->dma_counter != 0 && s->ti_size <= 0) {
            /* If this was the last part of a DMA transfer then the
               completion interrupt is deferred to here.  */
            esp_dma_done(s);
        }
    }
}

static void handle_ti(ESPState *s)
{
    uint32_t dmalen, minlen;

    dmalen = s->rregs[ESP_TCLO] | (s->rregs[ESP_TCMID] << 8);
    if (dmalen==0) {
      dmalen=0x10000;
    }
    s->dma_counter = dmalen;

    if (s->do_cmd)
        minlen = (dmalen < 32) ? dmalen : 32;
    else if (s->ti_size < 0)
        minlen = (dmalen < -s->ti_size) ? dmalen : -s->ti_size;
    else
        minlen = (dmalen < s->ti_size) ? dmalen : s->ti_size;
    DPRINTF("Transfer Information len %d\n", minlen);
    if (s->dma) {
        s->dma_left = minlen;
        s->rregs[ESP_RSTAT] &= ~STAT_TC;
        esp_do_dma(s);
    } else if (s->do_cmd) {
        DPRINTF("command len %d\n", s->cmdlen);
        s->ti_size = 0;
        s->cmdlen = 0;
        s->do_cmd = 0;
        do_cmd(s, s->cmdbuf);
        return;
    }
}

static void esp_reset(DeviceState *d)
{
    ESPState *s = container_of(d, ESPState, busdev.qdev);

    memset(s->rregs, 0, ESP_REGS);
    memset(s->wregs, 0, ESP_REGS);
    s->rregs[ESP_TCHI] = TCHI_FAS100A; // Indicate fas100a
    s->ti_size = 0;
    s->ti_rptr = 0;
    s->ti_wptr = 0;
    s->dma = 0;
    s->do_cmd = 0;

    s->rregs[ESP_CFG1] = 7;
}

static void parent_esp_reset(void *opaque, int irq, int level)
{
    if (level)
        esp_reset(opaque);
}

static uint32_t esp_mem_readb(void *opaque, target_phys_addr_t addr)
{
    ESPState *s = opaque;
    uint32_t saddr, old_val;

    saddr = addr >> s->it_shift;
    DPRINTF("read reg[%d]: 0x%2.2x\n", saddr, s->rregs[saddr]);
    switch (saddr) {
    case ESP_FIFO:
        if (s->ti_size > 0) {
            s->ti_size--;
            if ((s->rregs[ESP_RSTAT] & STAT_PIO_MASK) == 0) {
                /* Data out.  */
                ESP_ERROR("PIO data read not implemented\n");
                s->rregs[ESP_FIFO] = 0;
            } else {
                s->rregs[ESP_FIFO] = s->ti_buf[s->ti_rptr++];
            }
            esp_raise_irq(s);
        }
        if (s->ti_size == 0) {
            s->ti_rptr = 0;
            s->ti_wptr = 0;
        }
        break;
    case ESP_RINTR:
        /* Clear sequence step, interrupt register and all status bits
           except TC */
        old_val = s->rregs[ESP_RINTR];
        s->rregs[ESP_RINTR] = 0;
        s->rregs[ESP_RSTAT] &= ~STAT_TC;
        s->rregs[ESP_RSEQ] = SEQ_CD;
        esp_lower_irq(s);

        return old_val;
    default:
        break;
    }
    return s->rregs[saddr];
}

static void esp_mem_writeb(void *opaque, target_phys_addr_t addr, uint32_t val)
{
    ESPState *s = opaque;
    uint32_t saddr;

    saddr = addr >> s->it_shift;
    DPRINTF("write reg[%d]: 0x%2.2x -> 0x%2.2x\n", saddr, s->wregs[saddr],
            val);
    switch (saddr) {
    case ESP_TCLO:
    case ESP_TCMID:
        s->rregs[ESP_RSTAT] &= ~STAT_TC;
        break;
    case ESP_FIFO:
        if (s->do_cmd) {
            s->cmdbuf[s->cmdlen++] = val & 0xff;
        } else if (s->ti_size == TI_BUFSZ - 1) {
            ESP_ERROR("fifo overrun\n");
        } else {
            s->ti_size++;
            s->ti_buf[s->ti_wptr++] = val & 0xff;
        }
        break;
    case ESP_CMD:
        s->rregs[saddr] = val;
        if (val & CMD_DMA) {
            s->dma = 1;
            /* Reload DMA counter.  */
            s->rregs[ESP_TCLO] = s->wregs[ESP_TCLO];
            s->rregs[ESP_TCMID] = s->wregs[ESP_TCMID];
        } else {
            s->dma = 0;
        }
        switch(val & CMD_CMD) {
        case CMD_NOP:
            DPRINTF("NOP (%2.2x)\n", val);
            break;
        case CMD_FLUSH:
            DPRINTF("Flush FIFO (%2.2x)\n", val);
            //s->ti_size = 0;
            s->rregs[ESP_RINTR] = INTR_FC;
            s->rregs[ESP_RSEQ] = 0;
            s->rregs[ESP_RFLAGS] = 0;
            break;
        case CMD_RESET:
            DPRINTF("Chip reset (%2.2x)\n", val);
            esp_reset(&s->busdev.qdev);
            break;
        case CMD_BUSRESET:
            DPRINTF("Bus reset (%2.2x)\n", val);
            s->rregs[ESP_RINTR] = INTR_RST;
            if (!(s->wregs[ESP_CFG1] & CFG1_RESREPT)) {
                esp_raise_irq(s);
            }
            break;
        case CMD_TI:
            handle_ti(s);
            break;
        case CMD_ICCS:
            DPRINTF("Initiator Command Complete Sequence (%2.2x)\n", val);
            write_response(s);
            s->rregs[ESP_RINTR] = INTR_FC;
            s->rregs[ESP_RSTAT] |= STAT_MI;
            break;
        case CMD_MSGACC:
            DPRINTF("Message Accepted (%2.2x)\n", val);
            s->rregs[ESP_RINTR] = INTR_DC;
            s->rregs[ESP_RSEQ] = 0;
            s->rregs[ESP_RFLAGS] = 0;
            esp_raise_irq(s);
            break;
        case CMD_PAD:
            DPRINTF("Transfer padding (%2.2x)\n", val);
            s->rregs[ESP_RSTAT] = STAT_TC;
            s->rregs[ESP_RINTR] = INTR_FC;
            s->rregs[ESP_RSEQ] = 0;
            break;
        case CMD_SATN:
            DPRINTF("Set ATN (%2.2x)\n", val);
            break;
        case CMD_SEL:
            DPRINTF("Select without ATN (%2.2x)\n", val);
            handle_s_without_atn(s);
            break;
        case CMD_SELATN:
            DPRINTF("Select with ATN (%2.2x)\n", val);
            handle_satn(s);
            break;
        case CMD_SELATNS:
            DPRINTF("Select with ATN & stop (%2.2x)\n", val);
            handle_satn_stop(s);
            break;
        case CMD_ENSEL:
            DPRINTF("Enable selection (%2.2x)\n", val);
            s->rregs[ESP_RINTR] = 0;
            break;
        default:
            ESP_ERROR("Unhandled ESP command (%2.2x)\n", val);
            break;
        }
        break;
#ifndef _MSC_VER
    case ESP_WBUSID ... ESP_WSYNO:
#else
	case ESP_WBUSID:
	case ESP_WSEL:
	case ESP_WSYNTP:
	case ESP_WSYNO:
#endif
        break;
    case ESP_CFG1:
        s->rregs[saddr] = val;
        break;
#ifndef _MSC_VER
    case ESP_WCCF ... ESP_WTEST:
#else
	case ESP_WCCF:
	case ESP_WTEST:
#endif
        break;
#ifndef _MSC_VER
    case ESP_CFG2 ... ESP_RES4:
#else
	case ESP_CFG2:
	case ESP_CFG3:
	case ESP_RES3:
	case ESP_TCHI:
	case ESP_RES4:
#endif
        s->rregs[saddr] = val;
        break;
    default:
        ESP_ERROR("invalid write of 0x%02x at [0x%x]\n", val, saddr);
        return;
    }
    s->wregs[saddr] = val;
}

static CPUReadMemoryFunc * const esp_mem_read[3] = {
    esp_mem_readb,
    NULL,
    NULL,
};

static CPUWriteMemoryFunc * const esp_mem_write[3] = {
    esp_mem_writeb,
    NULL,
    esp_mem_writeb,
};

static const VMStateDescription vmstate_esp = {
    .name ="esp",
    .version_id = 3,
    .minimum_version_id = 3,
    .minimum_version_id_old = 3,
    .fields      = (VMStateField []) {
        VMSTATE_BUFFER(rregs, ESPState),
        VMSTATE_BUFFER(wregs, ESPState),
        VMSTATE_INT32(ti_size, ESPState),
        VMSTATE_UINT32(ti_rptr, ESPState),
        VMSTATE_UINT32(ti_wptr, ESPState),
        VMSTATE_BUFFER(ti_buf, ESPState),
        VMSTATE_UINT32(sense, ESPState),
        VMSTATE_UINT32(dma, ESPState),
        VMSTATE_BUFFER(cmdbuf, ESPState),
        VMSTATE_UINT32(cmdlen, ESPState),
        VMSTATE_UINT32(do_cmd, ESPState),
        VMSTATE_UINT32(dma_left, ESPState),
        VMSTATE_END_OF_LIST()
    }
};

void esp_init(target_phys_addr_t espaddr, int it_shift,
              ESPDMAMemoryReadWriteFunc dma_memory_read,
              ESPDMAMemoryReadWriteFunc dma_memory_write,
              void *dma_opaque, qemu_irq irq, qemu_irq *reset)
{
    DeviceState *dev;
    SysBusDevice *s;
    ESPState *esp;

    dev = qdev_create(NULL, "esp");
    esp = DO_UPCAST(ESPState, busdev.qdev, dev);
    esp->dma_memory_read = dma_memory_read;
    esp->dma_memory_write = dma_memory_write;
    esp->dma_opaque = dma_opaque;
    esp->it_shift = it_shift;
    qdev_init_nofail(dev);
    s = sysbus_from_qdev(dev);
    sysbus_connect_irq(s, 0, irq);
    sysbus_mmio_map(s, 0, espaddr);
    *reset = qdev_get_gpio_in(dev, 0);
}

static int esp_init1(SysBusDevice *dev)
{
    ESPState *s = FROM_SYSBUS(ESPState, dev);
    int esp_io_memory;

    sysbus_init_irq(dev, &s->irq);
    assert(s->it_shift != -1);

    esp_io_memory = cpu_register_io_memory(esp_mem_read, esp_mem_write, s);
    sysbus_init_mmio(dev, ESP_REGS << s->it_shift, esp_io_memory);

    qdev_init_gpio_in(&dev->qdev, parent_esp_reset, 1);

    scsi_bus_new(&s->bus, &dev->qdev, 0, ESP_MAX_DEVS, esp_command_complete);
    scsi_bus_legacy_handle_cmdline(&s->bus);
    return 0;
}

static SysBusDeviceInfo esp_info = {
    .init = esp_init1,
    .qdev.name  = "esp",
    .qdev.size  = sizeof(ESPState),
    .qdev.vmsd  = &vmstate_esp,
    .qdev.reset = esp_reset,
    .qdev.props = (Property[]) {
        {.name = NULL}
    }
};

static void esp_register_devices(void)
{
    sysbus_register_withprop(&esp_info);
}

device_init(esp_register_devices);
