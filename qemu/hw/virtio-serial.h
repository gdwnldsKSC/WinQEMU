/*
 * Virtio Serial / Console Support
 *
 * Copyright IBM, Corp. 2008
 * Copyright Red Hat, Inc. 2009, 2010
 *
 * Authors:
 *  Christian Ehrhardt <ehrhardt@linux.vnet.ibm.com>
 *  Amit Shah <amit.shah@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 */
#ifndef _QEMU_VIRTIO_SERIAL_H
#define _QEMU_VIRTIO_SERIAL_H

#include <stdbool.h>
#include "qdev.h"
#include "virtio.h"

/* == Interface shared between the guest kernel and qemu == */

/* The Virtio ID for virtio console / serial ports */
#define VIRTIO_ID_CONSOLE		3

/* Features supported */
#define VIRTIO_CONSOLE_F_MULTIPORT	1

#define VIRTIO_CONSOLE_BAD_ID           (~(uint32_t)0)

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif
struct virtio_console_config {
    /*
     * These two fields are used by VIRTIO_CONSOLE_F_SIZE which
     * isn't implemented here yet
     */
    uint16_t cols;
    uint16_t rows;

    uint32_t max_nr_ports;
} 
#ifndef _MSC_VER
__attribute__((packed));
#else
;
#endif

#ifdef _MSC_VER
#pragma pack (pop)
#endif

struct virtio_console_control {
    uint32_t id;		/* Port number */
    uint16_t event;		/* The kind of control event (see below) */
    uint16_t value;		/* Extra information for the key */
};

/* Some events for the internal messages (control packets) */
#define VIRTIO_CONSOLE_DEVICE_READY	0
#define VIRTIO_CONSOLE_PORT_ADD		1
#define VIRTIO_CONSOLE_PORT_REMOVE	2
#define VIRTIO_CONSOLE_PORT_READY	3
#define VIRTIO_CONSOLE_CONSOLE_PORT	4
#define VIRTIO_CONSOLE_RESIZE		5
#define VIRTIO_CONSOLE_PORT_OPEN	6
#define VIRTIO_CONSOLE_PORT_NAME	7

/* == In-qemu interface == */

typedef struct VirtIOSerial VirtIOSerial;
typedef struct VirtIOSerialBus VirtIOSerialBus;
typedef struct VirtIOSerialPort VirtIOSerialPort;
typedef struct VirtIOSerialPortInfo VirtIOSerialPortInfo;

typedef struct VirtIOSerialDevice {
    DeviceState qdev;
    VirtIOSerialPortInfo *info;
} VirtIOSerialDevice;

/*
 * This is the state that's shared between all the ports.  Some of the
 * state is configurable via command-line options. Some of it can be
 * set by individual devices in their initfn routines. Some of the
 * state is set by the generic qdev device init routine.
 */
struct VirtIOSerialPort {
    DeviceState dev;
    VirtIOSerialPortInfo *info;

    QTAILQ_ENTRY(VirtIOSerialPort) next;

    /*
     * This field gives us the virtio device as well as the qdev bus
     * that we are associated with
     */
    VirtIOSerial *vser;

    VirtQueue *ivq, *ovq;

    /*
     * This name is sent to the guest and exported via sysfs.
     * The guest could create symlinks based on this information.
     * The name is in the reverse fqdn format, like org.qemu.console.0
     */
    char *name;

    /*
     * This id helps identify ports between the guest and the host.
     * The guest sends a "header" with this id with each data packet
     * that it sends and the host can then find out which associated
     * device to send out this data to
     */
    uint32_t id;

    /* Identify if this is a port that binds with hvc in the guest */
    uint8_t is_console;

    /* Is the corresponding guest device open? */
    bool guest_connected;
    /* Is this device open for IO on the host? */
    bool host_connected;
    /* Do apps not want to receive data? */
    bool throttled;
};

struct VirtIOSerialPortInfo {
    DeviceInfo qdev;
    /*
     * The per-port (or per-app) init function that's called when a
     * new device is found on the bus.
     */
    int (*init)(VirtIOSerialDevice *dev);
    /*
     * Per-port exit function that's called when a port gets
     * hot-unplugged or removed.
     */
    int (*exit)(VirtIOSerialDevice *dev);

    /* Callbacks for guest events */
        /* Guest opened device. */
    void (*guest_open)(VirtIOSerialPort *port);
        /* Guest closed device. */
    void (*guest_close)(VirtIOSerialPort *port);

        /* Guest is now ready to accept data (virtqueues set up). */
    void (*guest_ready)(VirtIOSerialPort *port);

    /*
     * Guest wrote some data to the port. This data is handed over to
     * the app via this callback.  The app is supposed to consume all
     * the data that is presented to it.
     */
    void (*have_data)(VirtIOSerialPort *port, const uint8_t *buf, size_t len);
};

/* Interface to the virtio-serial bus */

/*
 * Individual ports/apps should call this function to register the port
 * with the virtio-serial bus
 */
void virtio_serial_port_qdev_register(VirtIOSerialPortInfo *info);

/*
 * Open a connection to the port
 *   Returns 0 on success (always).
 */
int virtio_serial_open(VirtIOSerialPort *port);

/*
 * Close the connection to the port
 *   Returns 0 on success (always).
 */
int virtio_serial_close(VirtIOSerialPort *port);

/*
 * Send data to Guest
 */
ssize_t virtio_serial_write(VirtIOSerialPort *port, const uint8_t *buf,
                            size_t size);

/*
 * Query whether a guest is ready to receive data.
 */
size_t virtio_serial_guest_ready(VirtIOSerialPort *port);

/*
 * Flow control: Ports can signal to the virtio-serial core to stop
 * sending data or re-start sending data, depending on the 'throttle'
 * value here.
 */
void virtio_serial_throttle_port(VirtIOSerialPort *port, bool throttle);

#endif
