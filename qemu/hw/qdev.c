/*
 *  Dynamic device configuration and creation.
 *
 *  Copyright (c) 2009 CodeSourcery
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 */

/* The theory here is that it should be possible to create a machine without
   knowledge of specific devices.  Historically board init routines have
   passed a bunch of arguments to each device, requiring the board know
   exactly which device it is dealing with.  This file provides an abstract
   API for device configuration and initialization.  Devices will generally
   inherit from a particular bus (e.g. PCI or I2C) rather than
   this API directly.  */

#include "net.h"
#include "qdev.h"
#include "sysemu.h"

struct DeviceProperty {
    const char *name;
    union {
        int i;
        void *ptr;
    } value;
    DeviceProperty *next;
};

struct DeviceType {
    const char *name;
    qdev_initfn init;
    void *opaque;
    int size;
    DeviceType *next;
};

struct ChildBusList {
    const char *name;
    void *ptr;
    ChildBusList *next;
};

static DeviceType *device_type_list;

/* Register a new device type.  */
DeviceType *qdev_register(const char *name, int size, qdev_initfn init,
                          void *opaque)
{
    DeviceType *t;

    assert(size >= sizeof(DeviceState));

    t = qemu_mallocz(sizeof(DeviceType));
    t->next = device_type_list;
    device_type_list = t;
    t->name = qemu_strdup(name);
    t->size = size;
    t->init = init;
    t->opaque = opaque;

    return t;
}

/* Create a new device.  This only initializes the device state structure
   and allows properties to be set.  qdev_init should be called to
   initialize the actual device emulation.  */
DeviceState *qdev_create(void *bus, const char *name)
{
    DeviceType *t;
    DeviceState *dev;

    for (t = device_type_list; t; t = t->next) {
        if (strcmp(t->name, name) == 0) {
            break;
        }
    }
    if (!t) {
        fprintf(stderr, "Unknown device '%s'\n", name);
        exit(1);
    }

    dev = qemu_mallocz(t->size);
    dev->name = name;
    dev->type = t;
    dev->bus = bus;
    return dev;
}

/* Initialize a device.  Device properties should be set before calling
   this function.  IRQs and MMIO regions should be connected/mapped after
   calling this function.  */
void qdev_init(DeviceState *dev)
{
    dev->type->init(dev, dev->type->opaque);
}

static DeviceProperty *create_prop(DeviceState *dev, const char *name)
{
    DeviceProperty *prop;

    /* TODO: Check for duplicate properties.  */
    prop = qemu_mallocz(sizeof(*prop));
    prop->name = qemu_strdup(name);
    prop->next = dev->props;
    dev->props = prop;

    return prop;
}

void qdev_set_prop_int(DeviceState *dev, const char *name, int value)
{
    DeviceProperty *prop;

    prop = create_prop(dev, name);
    prop->value.i = value;
}

void qdev_set_prop_ptr(DeviceState *dev, const char *name, void *value)
{
    DeviceProperty *prop;

    prop = create_prop(dev, name);
    prop->value.ptr = value;
}

void qdev_set_netdev(DeviceState *dev, NICInfo *nd)
{
    assert(!dev->nd);
    dev->nd = nd;
}


qemu_irq qdev_get_irq_sink(DeviceState *dev, int n)
{
    assert(n >= 0 && n < dev->num_irq_sink);
    return dev->irq_sink[n];
}

/* Register device IRQ sinks.  */
void qdev_init_irq_sink(DeviceState *dev, qemu_irq_handler handler, int nirq)
{
    dev->num_irq_sink = nirq;
    dev->irq_sink = qemu_allocate_irqs(handler, dev, nirq);
}

/* Get a character (serial) device interface.  */
CharDriverState *qdev_init_chardev(DeviceState *dev)
{
    static int next_serial;
    static int next_virtconsole;
    /* FIXME: This is a nasty hack that needs to go away.  */
    if (strncmp(dev->name, "virtio", 6) == 0) {
        return virtcon_hds[next_virtconsole++];
    } else {
        return serial_hds[next_serial++];
    }
}

void *qdev_get_bus(DeviceState *dev)
{
    return dev->bus;
}

static DeviceProperty *find_prop(DeviceState *dev, const char *name)
{
    DeviceProperty *prop;

    for (prop = dev->props; prop; prop = prop->next) {
        if (strcmp(prop->name, name) == 0) {
            return prop;
        }
    }
    return NULL;
}

uint64_t qdev_get_prop_int(DeviceState *dev, const char *name, uint64_t def)
{
    DeviceProperty *prop;

    prop = find_prop(dev, name);
    if (!prop)
        return def;

    return prop->value.i;
}

void *qdev_get_prop_ptr(DeviceState *dev, const char *name)
{
    DeviceProperty *prop;

    prop = find_prop(dev, name);
    assert(prop);
    return prop->value.ptr;
}

void qdev_init_gpio_in(DeviceState *dev, qemu_irq_handler handler, int n)
{
    assert(dev->num_gpio_in == 0);
    dev->num_gpio_in = n;
    dev->gpio_in = qemu_allocate_irqs(handler, dev, n);
}

void qdev_init_gpio_out(DeviceState *dev, qemu_irq *pins, int n)
{
    assert(dev->num_gpio_out == 0);
    dev->num_gpio_out = n;
    dev->gpio_out = pins;
}

qemu_irq qdev_get_gpio_in(DeviceState *dev, int n)
{
    assert(n >= 0 && n < dev->num_gpio_in);
    return dev->gpio_in[n];
}

void qdev_connect_gpio_out(DeviceState * dev, int n, qemu_irq pin)
{
    assert(n >= 0 && n < dev->num_gpio_out);
    dev->gpio_out[n] = pin;
}

VLANClientState *qdev_get_vlan_client(DeviceState *dev,
                                      IOReadHandler *fd_read,
                                      IOCanRWHandler *fd_can_read,
                                      NetCleanup *cleanup,
                                      void *opaque)
{
    NICInfo *nd = dev->nd;
    assert(nd);
    return qemu_new_vlan_client(nd->vlan, nd->model, nd->name,
                                fd_read, fd_can_read, cleanup, opaque);
}


void qdev_get_macaddr(DeviceState *dev, uint8_t *macaddr)
{
    memcpy(macaddr, dev->nd->macaddr, 6);
}

static int next_block_unit[IF_COUNT];

/* Get a block device.  This should only be used for single-drive devices
   (e.g. SD/Floppy/MTD).  Multi-disk devices (scsi/ide) should use the
   appropriate bus.  */
BlockDriverState *qdev_init_bdrv(DeviceState *dev, BlockInterfaceType type)
{
    int unit = next_block_unit[type]++;
    int index;

    index = drive_get_index(type, 0, unit);
    if (index == -1) {
        return NULL;
    }
    return drives_table[index].bdrv;
}

void *qdev_get_child_bus(DeviceState *dev, const char *name)
{
    ChildBusList *bus;

    for (bus = dev->child_bus; bus; bus = bus->next) {
        if (strcmp(name, bus->name) == 0) {
            return bus->ptr;
        }
    }
    return NULL;
}

void qdev_attach_child_bus(DeviceState *dev, const char *name, void *bus)
{
    ChildBusList *p;

    assert(!qdev_get_child_bus(dev, name));
    p = qemu_mallocz(sizeof(*p));
    p->name = qemu_strdup(name);
    p->ptr = bus;
    p->next = dev->child_bus;
    dev->child_bus = p;
}

static int next_scsi_bus;

/* Create a scsi bus, and attach devices to it.  */
/* TODO: Actually create a scsi bus for hotplug to use.  */
void scsi_bus_new(DeviceState *host, SCSIAttachFn attach)
{
   int bus = next_scsi_bus++;
   int unit;
   int index;

   for (unit = 0; unit < MAX_SCSI_DEVS; unit++) {
       index = drive_get_index(IF_SCSI, bus, unit);
       if (index == -1) {
           continue;
       }
       attach(host, drives_table[index].bdrv, unit);
   }
}
