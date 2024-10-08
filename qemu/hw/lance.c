/*
 * QEMU AMD PC-Net II (Am79C970A) emulation
 *
 * Copyright (c) 2004 Antony T Curtis
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

/* This software was written to be compatible with the specification:
 * AMD Am79C970A PCnet-PCI II Ethernet Controller Data-Sheet
 * AMD Publication# 19436  Rev:E  Amendment/0  Issue Date: June 2000
 */

/*
 * On Sparc32, this is the Lance (Am7990) part of chip STP2000 (Master I/O), also
 * produced as NCR89C100. See
 * http://www.ibiblio.org/pub/historic-linux/early-ports/Sparc/NCR/NCR89C100.txt
 * and
 * http://www.ibiblio.org/pub/historic-linux/early-ports/Sparc/NCR/NCR92C990.txt
 */

#include "sysbus.h"
#include "net.h"
#include "qemu-timer.h"
#include "qemu_socket.h"
#include "sun4m.h"

#include "pcnet.h"

typedef struct {
    SysBusDevice busdev;
    PCNetState state;
} SysBusPCNetState;

static void parent_lance_reset(void *opaque, int irq, int level)
{
    SysBusPCNetState *d = opaque;
    if (level)
        pcnet_h_reset(&d->state);
}

static void lance_mem_writew(void *opaque, target_phys_addr_t addr,
                             uint32_t val)
{
    SysBusPCNetState *d = opaque;
#ifdef PCNET_DEBUG_IO
    printf("lance_mem_writew addr=" TARGET_FMT_plx " val=0x%04x\n", addr,
           val & 0xffff);
#endif
    pcnet_ioport_writew(&d->state, addr, val & 0xffff);
}

static uint32_t lance_mem_readw(void *opaque, target_phys_addr_t addr)
{
    SysBusPCNetState *d = opaque;
    uint32_t val;

    val = pcnet_ioport_readw(&d->state, addr);
#ifdef PCNET_DEBUG_IO
    printf("lance_mem_readw addr=" TARGET_FMT_plx " val = 0x%04x\n", addr,
           val & 0xffff);
#endif

    return val & 0xffff;
}

static CPUReadMemoryFunc * const lance_mem_read[3] = {
    NULL,
    lance_mem_readw,
    NULL,
};

static CPUWriteMemoryFunc * const lance_mem_write[3] = {
    NULL,
    lance_mem_writew,
    NULL,
};

static void lance_cleanup(VLANClientState *nc)
{
    PCNetState *d = DO_UPCAST(NICState, nc, nc)->opaque;

    pcnet_common_cleanup(d);
}

static NetClientInfo net_lance_info = {
    .type = NET_CLIENT_TYPE_NIC,
    .size = sizeof(NICState),
    .can_receive = pcnet_can_receive,
    .receive = pcnet_receive,
    .cleanup = lance_cleanup,
};

static const VMStateDescription vmstate_lance = {
    .name = "pcnet",
    .version_id = 3,
    .minimum_version_id = 2,
    .minimum_version_id_old = 2,
    .fields      = (VMStateField []) {
        VMSTATE_STRUCT(state, SysBusPCNetState, 0, vmstate_pcnet, PCNetState),
        VMSTATE_END_OF_LIST()
    }
};

static int lance_init(SysBusDevice *dev)
{
    SysBusPCNetState *d = FROM_SYSBUS(SysBusPCNetState, dev);
    PCNetState *s = &d->state;

    s->mmio_index =
        cpu_register_io_memory(lance_mem_read, lance_mem_write, d);

    qdev_init_gpio_in(&dev->qdev, parent_lance_reset, 1);

    sysbus_init_mmio(dev, 4, s->mmio_index);

    sysbus_init_irq(dev, &s->irq);

    s->phys_mem_read = ledma_memory_read;
    s->phys_mem_write = ledma_memory_write;
    return pcnet_common_init(&dev->qdev, s, &net_lance_info);
}

static void lance_reset(DeviceState *dev)
{
    SysBusPCNetState *d = DO_UPCAST(SysBusPCNetState, busdev.qdev, dev);

    pcnet_h_reset(&d->state);
}

static SysBusDeviceInfo lance_info = {
    .init       = lance_init,
    .qdev.name  = "lance",
    .qdev.size  = sizeof(SysBusPCNetState),
    .qdev.reset = lance_reset,
    .qdev.vmsd  = &vmstate_lance,
    .qdev.props = (Property[]) {
        DEFINE_PROP_PTR("dma", SysBusPCNetState, state.dma_opaque),
        DEFINE_NIC_PROPERTIES(SysBusPCNetState, state.conf),
        DEFINE_PROP_END_OF_LIST(),
    }
};

static void lance_register_devices(void)
{
    sysbus_register_withprop(&lance_info);
}
device_init(lance_register_devices);
