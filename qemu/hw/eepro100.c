/*
 * QEMU i8255x (PRO100) emulation
 *
 * Copyright (C) 2006-2010 Stefan Weil
 *
 * Portions of the code are copies from grub / etherboot eepro100.c
 * and linux e100.c.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) version 3 or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Tested features (i82559):
 *      PXE boot (i386) ok
 *      Linux networking (i386) ok
 *
 * Untested:
 *      non-i386 platforms
 *      Windows networking
 *
 * References:
 *
 * Intel 8255x 10/100 Mbps Ethernet Controller Family
 * Open Source Software Developer Manual
 *
 * TODO:
 *      * PHY emulation should be separated from nic emulation.
 *        Most nic emulations could share the same phy code.
 *      * i82550 is untested. It is programmed like the i82559.
 *      * i82562 is untested. It is programmed like the i82559.
 *      * Power management (i82558 and later) is not implemented.
 *      * Wake-on-LAN is not implemented.
 */

#include <stdbool.h>            /* bool */
#include <stddef.h>             /* offsetof */
#include "hw.h"
#include "pci.h"
#include "net.h"
#include "eeprom93xx.h"

#define KiB 1024

/* Debug EEPRO100 card. */
#if 0
# define DEBUG_EEPRO100
#endif

#ifdef DEBUG_EEPRO100
#define logout(fmt, ...) fprintf(stderr, "EE100\t%-24s" fmt, __func__, ## __VA_ARGS__)
#else
#define logout(fmt, ...) ((void)0)
#endif

/* Set flags to 0 to disable debug output. */
#define INT     1       /* interrupt related actions */
#define MDI     1       /* mdi related actions */
#define OTHER   1
#define RXTX    1
#define EEPROM  1       /* eeprom related actions */

#define TRACE(flag, command) ((flag) ? (command) : (void)0)

#define missing(text) fprintf(stderr, "eepro100: feature is missing in this emulation: " text "\n")

#define MAX_ETH_FRAME_SIZE 1514

/* This driver supports several different devices which are declared here. */
#define i82550          0x82550
#define i82551          0x82551
#define i82557A         0x82557a
#define i82557B         0x82557b
#define i82557C         0x82557c
#define i82558A         0x82558a
#define i82558B         0x82558b
#define i82559A         0x82559a
#define i82559B         0x82559b
#define i82559C         0x82559c
#define i82559ER        0x82559e
#define i82562          0x82562
#define i82801          0x82801

/* Use 64 word EEPROM. TODO: could be a runtime option. */
#define EEPROM_SIZE     64

#define PCI_MEM_SIZE            (4 * KiB)
#define PCI_IO_SIZE             64
#define PCI_FLASH_SIZE          (128 * KiB)

#define BIT(n) (1 << (n))
#define BITS(n, m) (((0xffffffffU << (31 - n)) >> (31 - n + m)) << m)

/* The SCB accepts the following controls for the Tx and Rx units: */
#define  CU_NOP         0x0000  /* No operation. */
#define  CU_START       0x0010  /* CU start. */
#define  CU_RESUME      0x0020  /* CU resume. */
#define  CU_STATSADDR   0x0040  /* Load dump counters address. */
#define  CU_SHOWSTATS   0x0050  /* Dump statistical counters. */
#define  CU_CMD_BASE    0x0060  /* Load CU base address. */
#define  CU_DUMPSTATS   0x0070  /* Dump and reset statistical counters. */
#define  CU_SRESUME     0x00a0  /* CU static resume. */

#define  RU_NOP         0x0000
#define  RX_START       0x0001
#define  RX_RESUME      0x0002
#define  RU_ABORT       0x0004
#define  RX_ADDR_LOAD   0x0006
#define  RX_RESUMENR    0x0007
#define INT_MASK        0x0100
#define DRVR_INT        0x0200  /* Driver generated interrupt. */

typedef struct {
    PCIDeviceInfo pci;
    uint32_t device;
    uint16_t device_id;
    uint8_t revision;
    uint8_t stats_size;
    bool has_extended_tcb_support;
    bool power_management;
} E100PCIDeviceInfo;

/* Offsets to the various registers.
   All accesses need not be longword aligned. */
enum speedo_offsets {
    SCBStatus = 0,              /* Status Word. */
    SCBAck = 1,
    SCBCmd = 2,                 /* Rx/Command Unit command and status. */
    SCBIntmask = 3,
    SCBPointer = 4,             /* General purpose pointer. */
    SCBPort = 8,                /* Misc. commands and operands.  */
    SCBflash = 12,              /* Flash memory control. */
    SCBeeprom = 14,             /* EEPROM control. */
    SCBCtrlMDI = 16,            /* MDI interface control. */
    SCBEarlyRx = 20,            /* Early receive byte count. */
    SCBFlow = 24,               /* Flow Control. */
    SCBpmdr = 27,               /* Power Management Driver. */
    SCBgctrl = 28,              /* General Control. */
    SCBgstat = 29,              /* General Status. */
};

/* A speedo3 transmit buffer descriptor with two buffers... */
typedef struct {
    uint16_t status;
    uint16_t command;
    uint32_t link;              /* void * */
    uint32_t tbd_array_addr;    /* transmit buffer descriptor array address. */
    uint16_t tcb_bytes;         /* transmit command block byte count (in lower 14 bits */
    uint8_t tx_threshold;       /* transmit threshold */
    uint8_t tbd_count;          /* TBD number */
#if 0
    /* This constitutes two "TBD" entries: hdr and data */
    uint32_t tx_buf_addr0;  /* void *, header of frame to be transmitted.  */
    int32_t  tx_buf_size0;  /* Length of Tx hdr. */
    uint32_t tx_buf_addr1;  /* void *, data to be transmitted.  */
    int32_t  tx_buf_size1;  /* Length of Tx data. */
#endif
} eepro100_tx_t;

/* Receive frame descriptor. */
typedef struct {
    int16_t status;
    uint16_t command;
    uint32_t link;              /* struct RxFD * */
    uint32_t rx_buf_addr;       /* void * */
    uint16_t count;
    uint16_t size;
    char packet[MAX_ETH_FRAME_SIZE + 4];
} eepro100_rx_t;

typedef enum {
    COMMAND_EL = BIT(15),
    COMMAND_S = BIT(14),
    COMMAND_I = BIT(13),
    COMMAND_NC = BIT(4),
    COMMAND_SF = BIT(3),
    COMMAND_CMD = BITS(2, 0),
} scb_command_bit;

typedef enum {
    STATUS_C = BIT(15),
    STATUS_OK = BIT(13),
} scb_status_bit;

typedef struct {
    uint32_t tx_good_frames, tx_max_collisions, tx_late_collisions,
             tx_underruns, tx_lost_crs, tx_deferred, tx_single_collisions,
             tx_multiple_collisions, tx_total_collisions;
    uint32_t rx_good_frames, rx_crc_errors, rx_alignment_errors,
             rx_resource_errors, rx_overrun_errors, rx_cdt_errors,
             rx_short_frame_errors;
    uint32_t fc_xmt_pause, fc_rcv_pause, fc_rcv_unsupported;
    uint16_t xmt_tco_frames, rcv_tco_frames;
    /* TODO: i82559 has six reserved statistics but a total of 24 dwords. */
    uint32_t reserved[4];
} eepro100_stats_t;

typedef enum {
    cu_idle = 0,
    cu_suspended = 1,
    cu_active = 2,
    cu_lpq_active = 2,
    cu_hqp_active = 3
} cu_state_t;

typedef enum {
    ru_idle = 0,
    ru_suspended = 1,
    ru_no_resources = 2,
    ru_ready = 4
} ru_state_t;

typedef struct {
    PCIDevice dev;
    uint8_t mult[8];            /* multicast mask array */
    int mmio_index;
    NICState *nic;
    NICConf conf;
    uint8_t scb_stat;           /* SCB stat/ack byte */
    uint8_t int_stat;           /* PCI interrupt status */
    /* region must not be saved by nic_save. */
    uint32_t region[3];         /* PCI region addresses */
    uint16_t mdimem[32];
    eeprom_t *eeprom;
    uint32_t device;            /* device variant */
    uint32_t pointer;
    /* (cu_base + cu_offset) address the next command block in the command block list. */
    uint32_t cu_base;           /* CU base address */
    uint32_t cu_offset;         /* CU address offset */
    /* (ru_base + ru_offset) address the RFD in the Receive Frame Area. */
    uint32_t ru_base;           /* RU base address */
    uint32_t ru_offset;         /* RU address offset */
    uint32_t statsaddr;         /* pointer to eepro100_stats_t */

    /* Temporary status information (no need to save these values),
     * used while processing CU commands. */
    eepro100_tx_t tx;           /* transmit buffer descriptor */
    uint32_t cb_address;        /* = cu_base + cu_offset */

    /* Statistical counters. Also used for wake-up packet (i82559). */
    eepro100_stats_t statistics;

    /* Configuration bytes. */
    uint8_t configuration[22];

    /* Data in mem is always in the byte order of the controller (le). */
    uint8_t mem[PCI_MEM_SIZE];
    /* vmstate for each particular nic */
    VMStateDescription *vmstate;

    /* Quasi static device properties (no need to save them). */
    uint16_t stats_size;
    bool has_extended_tcb_support;
} EEPRO100State;

/* Word indices in EEPROM. */
typedef enum {
    EEPROM_CNFG_MDIX  = 0x03,
    EEPROM_ID         = 0x05,
    EEPROM_PHY_ID     = 0x06,
    EEPROM_VENDOR_ID  = 0x0c,
    EEPROM_CONFIG_ASF = 0x0d,
    EEPROM_DEVICE_ID  = 0x23,
    EEPROM_SMBUS_ADDR = 0x90,
} EEPROMOffset;

/* Bit values for EEPROM ID word. */
typedef enum {
    EEPROM_ID_MDM = BIT(0),     /* Modem */
    EEPROM_ID_STB = BIT(1),     /* Standby Enable */
    EEPROM_ID_WMR = BIT(2),     /* ??? */
    EEPROM_ID_WOL = BIT(5),     /* Wake on LAN */
    EEPROM_ID_DPD = BIT(6),     /* Deep Power Down */
    EEPROM_ID_ALT = BIT(7),     /* */
    /* BITS(10, 8) device revision */
    EEPROM_ID_BD = BIT(11),     /* boot disable */
    EEPROM_ID_ID = BIT(13),     /* id bit */
    /* BITS(15, 14) signature */
    EEPROM_ID_VALID = BIT(14),  /* signature for valid eeprom */
} eeprom_id_bit;

/* Default values for MDI (PHY) registers */
static const uint16_t eepro100_mdi_default[] = {
    /* MDI Registers 0 - 6, 7 */
    0x3000, 0x780d, 0x02a8, 0x0154, 0x05e1, 0x0000, 0x0000, 0x0000,
    /* MDI Registers 8 - 15 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    /* MDI Registers 16 - 31 */
    0x0003, 0x0000, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

/* Readonly mask for MDI (PHY) registers */
static const uint16_t eepro100_mdi_mask[] = {
    0x0000, 0xffff, 0xffff, 0xffff, 0xc01f, 0xffff, 0xffff, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0fff, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

/* XXX: optimize */
static void stl_le_phys(target_phys_addr_t addr, uint32_t val)
{
    val = cpu_to_le32(val);
    cpu_physical_memory_write(addr, (const uint8_t *)&val, sizeof(val));
}

#define POLYNOMIAL 0x04c11db6

/* From FreeBSD */
/* XXX: optimize */
static unsigned compute_mcast_idx(const uint8_t * ep)
{
    uint32_t crc;
    int carry, i, j;
    uint8_t b;

    crc = 0xffffffff;
    for (i = 0; i < 6; i++) {
        b = *ep++;
        for (j = 0; j < 8; j++) {
            carry = ((crc & 0x80000000L) ? 1 : 0) ^ (b & 0x01);
            crc <<= 1;
            b >>= 1;
            if (carry) {
                crc = ((crc ^ POLYNOMIAL) | carry);
            }
        }
    }
    return (crc & BITS(7, 2)) >> 2;
}

#if defined(DEBUG_EEPRO100)
static const char *nic_dump(const uint8_t * buf, unsigned size)
{
    static char dump[3 * 16 + 1];
    char *p = &dump[0];
    if (size > 16) {
        size = 16;
    }
    while (size-- > 0) {
        p += sprintf(p, " %02x", *buf++);
    }
    return dump;
}
#endif                          /* DEBUG_EEPRO100 */

enum scb_stat_ack {
    stat_ack_not_ours = 0x00,
    stat_ack_sw_gen = 0x04,
    stat_ack_rnr = 0x10,
    stat_ack_cu_idle = 0x20,
    stat_ack_frame_rx = 0x40,
    stat_ack_cu_cmd_done = 0x80,
    stat_ack_not_present = 0xFF,
    stat_ack_rx = (stat_ack_sw_gen | stat_ack_rnr | stat_ack_frame_rx),
    stat_ack_tx = (stat_ack_cu_idle | stat_ack_cu_cmd_done),
};

static void disable_interrupt(EEPRO100State * s)
{
    if (s->int_stat) {
        TRACE(INT, logout("interrupt disabled\n"));
        qemu_irq_lower(s->dev.irq[0]);
        s->int_stat = 0;
    }
}

static void enable_interrupt(EEPRO100State * s)
{
    if (!s->int_stat) {
        TRACE(INT, logout("interrupt enabled\n"));
        qemu_irq_raise(s->dev.irq[0]);
        s->int_stat = 1;
    }
}

static void eepro100_acknowledge(EEPRO100State * s)
{
    s->scb_stat &= ~s->mem[SCBAck];
    s->mem[SCBAck] = s->scb_stat;
    if (s->scb_stat == 0) {
        disable_interrupt(s);
    }
}

static void eepro100_interrupt(EEPRO100State * s, uint8_t status)
{
    uint8_t mask = ~s->mem[SCBIntmask];
    s->mem[SCBAck] |= status;
    status = s->scb_stat = s->mem[SCBAck];
    status &= (mask | 0x0f);
#if 0
    status &= (~s->mem[SCBIntmask] | 0x0xf);
#endif
    if (status && (mask & 0x01)) {
        /* SCB mask and SCB Bit M do not disable interrupt. */
        enable_interrupt(s);
    } else if (s->int_stat) {
        disable_interrupt(s);
    }
}

static void eepro100_cx_interrupt(EEPRO100State * s)
{
    /* CU completed action command. */
    /* Transmit not ok (82557 only, not in emulation). */
    eepro100_interrupt(s, 0x80);
}

static void eepro100_cna_interrupt(EEPRO100State * s)
{
    /* CU left the active state. */
    eepro100_interrupt(s, 0x20);
}

static void eepro100_fr_interrupt(EEPRO100State * s)
{
    /* RU received a complete frame. */
    eepro100_interrupt(s, 0x40);
}

static void eepro100_rnr_interrupt(EEPRO100State * s)
{
    /* RU is not ready. */
    eepro100_interrupt(s, 0x10);
}

static void eepro100_mdi_interrupt(EEPRO100State * s)
{
    /* MDI completed read or write cycle. */
    eepro100_interrupt(s, 0x08);
}

static void eepro100_swi_interrupt(EEPRO100State * s)
{
    /* Software has requested an interrupt. */
    eepro100_interrupt(s, 0x04);
}

#if 0
static void eepro100_fcp_interrupt(EEPRO100State * s)
{
    /* Flow control pause interrupt (82558 and later). */
    eepro100_interrupt(s, 0x01);
}
#endif

static void e100_pci_reset(EEPRO100State * s, E100PCIDeviceInfo *e100_device)
{
    uint32_t device = s->device;
    uint8_t *pci_conf = s->dev.config;

    TRACE(OTHER, logout("%p\n", s));

    /* PCI Vendor ID */
    pci_config_set_vendor_id(pci_conf, PCI_VENDOR_ID_INTEL);
    /* PCI Device ID */
    pci_config_set_device_id(pci_conf, e100_device->device_id);
    /* PCI Status */
    pci_set_word(pci_conf + PCI_STATUS, PCI_STATUS_DEVSEL_MEDIUM |
                                        PCI_STATUS_FAST_BACK);
    /* PCI Revision ID */
    pci_config_set_revision(pci_conf, e100_device->revision);
    pci_config_set_class(pci_conf, PCI_CLASS_NETWORK_ETHERNET);
    /* PCI Latency Timer */
    pci_set_byte(pci_conf + PCI_LATENCY_TIMER, 0x20);   /* latency timer = 32 clocks */
    /* Capability Pointer is set by PCI framework. */
    /* Interrupt Line */
    /* Interrupt Pin */
    pci_set_byte(pci_conf + PCI_INTERRUPT_PIN, 1);      /* interrupt pin A */
    /* Minimum Grant */
    pci_set_byte(pci_conf + PCI_MIN_GNT, 0x08);
    /* Maximum Latency */
    pci_set_byte(pci_conf + PCI_MAX_LAT, 0x18);

    s->stats_size = e100_device->stats_size;
    s->has_extended_tcb_support = e100_device->has_extended_tcb_support;

    switch (device) {
    case i82550:
    case i82551:
    case i82557A:
    case i82557B:
    case i82557C:
    case i82558A:
    case i82558B:
    case i82559A:
    case i82559B:
    case i82559ER:
    case i82562:
    case i82801:
        break;
    case i82559C:
#if EEPROM_SIZE > 0
        pci_set_word(pci_conf + PCI_SUBSYSTEM_VENDOR_ID, PCI_VENDOR_ID_INTEL);
        pci_set_word(pci_conf + PCI_SUBSYSTEM_ID, 0x0040);
#endif
        break;
    default:
        logout("Device %X is undefined!\n", device);
    }

    /* Standard TxCB. */
    s->configuration[6] |= BIT(4);

    /* Standard statistical counters. */
    s->configuration[6] |= BIT(5);

    if (s->stats_size == 80) {
        /* TODO: check TCO Statistical Counters bit. Documentation not clear. */
        if (s->configuration[6] & BIT(2)) {
            /* TCO statistical counters. */
            assert(s->configuration[6] & BIT(5));
        } else {
            if (s->configuration[6] & BIT(5)) {
                /* No extended statistical counters, i82557 compatible. */
                s->stats_size = 64;
            } else {
                /* i82558 compatible. */
                s->stats_size = 76;
            }
        }
    } else {
        if (s->configuration[6] & BIT(5)) {
            /* No extended statistical counters. */
            s->stats_size = 64;
        }
    }
    assert(s->stats_size > 0 && s->stats_size <= sizeof(s->statistics));

    if (e100_device->power_management) {
        /* Power Management Capabilities */
        int cfg_offset = 0xdc;
        int r = pci_add_capability_at_offset(&s->dev, PCI_CAP_ID_PM,
                                             cfg_offset, PCI_PM_SIZEOF);
        assert(r >= 0);
        pci_set_word(pci_conf + cfg_offset + PCI_PM_PMC, 0x7e21);
#if 0 /* TODO: replace dummy code for power management emulation. */
        /* TODO: Power Management Control / Status. */
        pci_set_word(pci_conf + cfg_offset + PCI_PM_CTRL, 0x0000);
        /* TODO: Ethernet Power Consumption Registers (i82559 and later). */
        pci_set_byte(pci_conf + cfg_offset + PCI_PM_PPB_EXTENSIONS, 0x0000);
#endif
    }

#if EEPROM_SIZE > 0
    if (device == i82557C || device == i82558B || device == i82559C) {
        /*
        TODO: get vendor id from EEPROM for i82557C or later.
        TODO: get device id from EEPROM for i82557C or later.
        TODO: status bit 4 can be disabled by EEPROM for i82558, i82559.
        TODO: header type is determined by EEPROM for i82559.
        TODO: get subsystem id from EEPROM for i82557C or later.
        TODO: get subsystem vendor id from EEPROM for i82557C or later.
        TODO: exp. rom baddr depends on a bit in EEPROM for i82558 or later.
        TODO: capability pointer depends on EEPROM for i82558.
        */
        logout("Get device id and revision from EEPROM!!!\n");
    }
#endif /* EEPROM_SIZE > 0 */
}

static void nic_selective_reset(EEPRO100State * s)
{
    size_t i;
    uint16_t *eeprom_contents = eeprom93xx_data(s->eeprom);
#if 0
    eeprom93xx_reset(s->eeprom);
#endif
    memcpy(eeprom_contents, s->conf.macaddr.a, 6);
    eeprom_contents[EEPROM_ID] = EEPROM_ID_VALID;
    if (s->device == i82557B || s->device == i82557C)
        eeprom_contents[5] = 0x0100;
    eeprom_contents[EEPROM_PHY_ID] = 1;
    uint16_t sum = 0;
    for (i = 0; i < EEPROM_SIZE - 1; i++) {
        sum += eeprom_contents[i];
    }
    eeprom_contents[EEPROM_SIZE - 1] = 0xbaba - sum;
    TRACE(EEPROM, logout("checksum=0x%04x\n", eeprom_contents[EEPROM_SIZE - 1]));

    memset(s->mem, 0, sizeof(s->mem));
    uint32_t val = BIT(21);
    memcpy(&s->mem[SCBCtrlMDI], &val, sizeof(val));

    assert(sizeof(s->mdimem) == sizeof(eepro100_mdi_default));
    memcpy(&s->mdimem[0], &eepro100_mdi_default[0], sizeof(s->mdimem));
}

static void nic_reset(void *opaque)
{
    EEPRO100State *s = opaque;
    TRACE(OTHER, logout("%p\n", s));
    /* TODO: Clearing of multicast table for selective reset, too? */
    memset(&s->mult[0], 0, sizeof(s->mult));
    nic_selective_reset(s);
}

#if defined(DEBUG_EEPRO100)
static const char * const e100_reg[PCI_IO_SIZE / 4] = {
    "Command/Status",
    "General Pointer",
    "Port",
    "EEPROM/Flash Control",
    "MDI Control",
    "Receive DMA Byte Count",
    "Flow Control",
    "General Status/Control"
};

static char *regname(uint32_t addr)
{
    static char buf[32];
    if (addr < PCI_IO_SIZE) {
        const char *r = e100_reg[addr / 4];
        if (r != 0) {
            snprintf(buf, sizeof(buf), "%s+%u", r, addr % 4);
        } else {
            snprintf(buf, sizeof(buf), "0x%02x", addr);
        }
    } else {
        snprintf(buf, sizeof(buf), "??? 0x%08x", addr);
    }
    return buf;
}
#endif                          /* DEBUG_EEPRO100 */

/*****************************************************************************
 *
 * Command emulation.
 *
 ****************************************************************************/

#if 0
static uint16_t eepro100_read_command(EEPRO100State * s)
{
    uint16_t val = 0xffff;
    TRACE(OTHER, logout("val=0x%04x\n", val));
    return val;
}
#endif

/* Commands that can be put in a command list entry. */
enum commands {
    CmdNOp = 0,
    CmdIASetup = 1,
    CmdConfigure = 2,
    CmdMulticastList = 3,
    CmdTx = 4,
    CmdTDR = 5,                 /* load microcode */
    CmdDump = 6,
    CmdDiagnose = 7,

    /* And some extra flags: */
    CmdSuspend = 0x4000,        /* Suspend after completion. */
    CmdIntr = 0x2000,           /* Interrupt after completion. */
    CmdTxFlex = 0x0008,         /* Use "Flexible mode" for CmdTx command. */
};

static cu_state_t get_cu_state(EEPRO100State * s)
{
    return ((s->mem[SCBStatus] & BITS(7, 6)) >> 6);
}

static void set_cu_state(EEPRO100State * s, cu_state_t state)
{
    s->mem[SCBStatus] = (s->mem[SCBStatus] & ~BITS(7, 6)) + (state << 6);
}

static ru_state_t get_ru_state(EEPRO100State * s)
{
    return ((s->mem[SCBStatus] & BITS(5, 2)) >> 2);
}

static void set_ru_state(EEPRO100State * s, ru_state_t state)
{
    s->mem[SCBStatus] = (s->mem[SCBStatus] & ~BITS(5, 2)) + (state << 2);
}

static void dump_statistics(EEPRO100State * s)
{
    /* Dump statistical data. Most data is never changed by the emulation
     * and always 0, so we first just copy the whole block and then those
     * values which really matter.
     * Number of data should check configuration!!!
     */
    cpu_physical_memory_write(s->statsaddr,
                              (uint8_t *) & s->statistics, s->stats_size);
    stl_le_phys(s->statsaddr + 0, s->statistics.tx_good_frames);
    stl_le_phys(s->statsaddr + 36, s->statistics.rx_good_frames);
    stl_le_phys(s->statsaddr + 48, s->statistics.rx_resource_errors);
    stl_le_phys(s->statsaddr + 60, s->statistics.rx_short_frame_errors);
#if 0
    stw_le_phys(s->statsaddr + 76, s->statistics.xmt_tco_frames);
    stw_le_phys(s->statsaddr + 78, s->statistics.rcv_tco_frames);
    missing("CU dump statistical counters");
#endif
}

static void read_cb(EEPRO100State *s)
{
    cpu_physical_memory_read(s->cb_address, (uint8_t *) &s->tx, sizeof(s->tx));
    s->tx.status = le16_to_cpu(s->tx.status);
    s->tx.command = le16_to_cpu(s->tx.command);
    s->tx.link = le32_to_cpu(s->tx.link);
    s->tx.tbd_array_addr = le32_to_cpu(s->tx.tbd_array_addr);
    s->tx.tcb_bytes = le16_to_cpu(s->tx.tcb_bytes);
}

static void tx_command(EEPRO100State *s)
{
    uint32_t tbd_array = le32_to_cpu(s->tx.tbd_array_addr);
    uint16_t tcb_bytes = (le16_to_cpu(s->tx.tcb_bytes) & 0x3fff);
    /* Sends larger than MAX_ETH_FRAME_SIZE are allowed, up to 2600 bytes. */
    uint8_t buf[2600];
    uint16_t size = 0;
    uint32_t tbd_address = s->cb_address + 0x10;
    TRACE(RXTX, logout
        ("transmit, TBD array address 0x%08x, TCB byte count 0x%04x, TBD count %u\n",
         tbd_array, tcb_bytes, s->tx.tbd_count));

    if (tcb_bytes > 2600) {
        logout("TCB byte count too large, using 2600\n");
        tcb_bytes = 2600;
    }
    if (!((tcb_bytes > 0) || (tbd_array != 0xffffffff))) {
        logout
            ("illegal values of TBD array address and TCB byte count!\n");
    }
    assert(tcb_bytes <= sizeof(buf));
    while (size < tcb_bytes) {
        uint32_t tx_buffer_address = ldl_phys(tbd_address);
        uint16_t tx_buffer_size = lduw_phys(tbd_address + 4);
#if 0
        uint16_t tx_buffer_el = lduw_phys(tbd_address + 6);
#endif
        tbd_address += 8;
        TRACE(RXTX, logout
            ("TBD (simplified mode): buffer address 0x%08x, size 0x%04x\n",
             tx_buffer_address, tx_buffer_size));
        tx_buffer_size = MIN(tx_buffer_size, sizeof(buf) - size);
        cpu_physical_memory_read(tx_buffer_address, &buf[size],
                                 tx_buffer_size);
        size += tx_buffer_size;
    }
    if (tbd_array == 0xffffffff) {
        /* Simplified mode. Was already handled by code above. */
    } else {
        /* Flexible mode. */
        uint8_t tbd_count = 0;
        if (s->has_extended_tcb_support && !(s->configuration[6] & BIT(4))) {
            /* Extended Flexible TCB. */
            for (; tbd_count < 2; tbd_count++) {
                uint32_t tx_buffer_address = ldl_phys(tbd_address);
                uint16_t tx_buffer_size = lduw_phys(tbd_address + 4);
                uint16_t tx_buffer_el = lduw_phys(tbd_address + 6);
                tbd_address += 8;
                TRACE(RXTX, logout
                    ("TBD (extended flexible mode): buffer address 0x%08x, size 0x%04x\n",
                     tx_buffer_address, tx_buffer_size));
                tx_buffer_size = MIN(tx_buffer_size, sizeof(buf) - size);
                cpu_physical_memory_read(tx_buffer_address, &buf[size],
                                         tx_buffer_size);
                size += tx_buffer_size;
                if (tx_buffer_el & 1) {
                    break;
                }
            }
        }
        tbd_address = tbd_array;
        for (; tbd_count < s->tx.tbd_count; tbd_count++) {
            uint32_t tx_buffer_address = ldl_phys(tbd_address);
            uint16_t tx_buffer_size = lduw_phys(tbd_address + 4);
            uint16_t tx_buffer_el = lduw_phys(tbd_address + 6);
            tbd_address += 8;
            TRACE(RXTX, logout
                ("TBD (flexible mode): buffer address 0x%08x, size 0x%04x\n",
                 tx_buffer_address, tx_buffer_size));
            tx_buffer_size = MIN(tx_buffer_size, sizeof(buf) - size);
            cpu_physical_memory_read(tx_buffer_address, &buf[size],
                                     tx_buffer_size);
            size += tx_buffer_size;
            if (tx_buffer_el & 1) {
                break;
            }
        }
    }
    TRACE(RXTX, logout("%p sending frame, len=%d,%s\n", s, size, nic_dump(buf, size)));
    qemu_send_packet(&s->nic->nc, buf, size);
    s->statistics.tx_good_frames++;
    /* Transmit with bad status would raise an CX/TNO interrupt.
     * (82557 only). Emulation never has bad status. */
#if 0
    eepro100_cx_interrupt(s);
#endif
}

static void set_multicast_list(EEPRO100State *s)
{
    uint16_t multicast_count = s->tx.tbd_array_addr & BITS(13, 0);
    uint16_t i;
    memset(&s->mult[0], 0, sizeof(s->mult));
    TRACE(OTHER, logout("multicast list, multicast count = %u\n", multicast_count));
    for (i = 0; i < multicast_count; i += 6) {
        uint8_t multicast_addr[6];
        cpu_physical_memory_read(s->cb_address + 10 + i, multicast_addr, 6);
        TRACE(OTHER, logout("multicast entry %s\n", nic_dump(multicast_addr, 6)));
        unsigned mcast_idx = compute_mcast_idx(multicast_addr);
        assert(mcast_idx < 64);
        s->mult[mcast_idx >> 3] |= (1 << (mcast_idx & 7));
    }
}

static void action_command(EEPRO100State *s)
{
    for (;;) {
        bool bit_el;
        bool bit_s;
        bool bit_i;
        bool bit_nc;
        uint16_t ok_status = STATUS_OK;
        s->cb_address = s->cu_base + s->cu_offset;
        read_cb(s);
        bit_el = ((s->tx.command & COMMAND_EL) != 0);
        bit_s = ((s->tx.command & COMMAND_S) != 0);
        bit_i = ((s->tx.command & COMMAND_I) != 0);
        bit_nc = ((s->tx.command & COMMAND_NC) != 0);
#if 0
        bool bit_sf = ((s->tx.command & COMMAND_SF) != 0);
#endif
        s->cu_offset = s->tx.link;
        TRACE(OTHER,
              logout("val=(cu start), status=0x%04x, command=0x%04x, link=0x%08x\n",
                     s->tx.status, s->tx.command, s->tx.link));
        switch (s->tx.command & COMMAND_CMD) {
        case CmdNOp:
            /* Do nothing. */
            break;
        case CmdIASetup:
            cpu_physical_memory_read(s->cb_address + 8, &s->conf.macaddr.a[0], 6);
            TRACE(OTHER, logout("macaddr: %s\n", nic_dump(&s->conf.macaddr.a[0], 6)));
            break;
        case CmdConfigure:
            cpu_physical_memory_read(s->cb_address + 8, &s->configuration[0],
                                     sizeof(s->configuration));
            TRACE(OTHER, logout("configuration: %s\n", nic_dump(&s->configuration[0], 16)));
            break;
        case CmdMulticastList:
            set_multicast_list(s);
            break;
        case CmdTx:
            if (bit_nc) {
                missing("CmdTx: NC = 0");
                ok_status = 0;
                break;
            }
            tx_command(s);
            break;
        case CmdTDR:
            TRACE(OTHER, logout("load microcode\n"));
            /* Starting with offset 8, the command contains
             * 64 dwords microcode which we just ignore here. */
            break;
        case CmdDiagnose:
            TRACE(OTHER, logout("diagnose\n"));
            /* Make sure error flag is not set. */
            s->tx.status = 0;
            break;
        default:
            missing("undefined command");
            ok_status = 0;
            break;
        }
        /* Write new status. */
        stw_phys(s->cb_address, s->tx.status | ok_status | STATUS_C);
        if (bit_i) {
            /* CU completed action. */
            eepro100_cx_interrupt(s);
        }
        if (bit_el) {
            /* CU becomes idle. Terminate command loop. */
            set_cu_state(s, cu_idle);
            eepro100_cna_interrupt(s);
            break;
        } else if (bit_s) {
            /* CU becomes suspended. Terminate command loop. */
            set_cu_state(s, cu_suspended);
            eepro100_cna_interrupt(s);
            break;
        } else {
            /* More entries in list. */
            TRACE(OTHER, logout("CU list with at least one more entry\n"));
        }
    }
    TRACE(OTHER, logout("CU list empty\n"));
    /* List is empty. Now CU is idle or suspended. */
}

static void eepro100_cu_command(EEPRO100State * s, uint8_t val)
{
    cu_state_t cu_state;
    switch (val) {
    case CU_NOP:
        /* No operation. */
        break;
    case CU_START:
        cu_state = get_cu_state(s);
        if (cu_state != cu_idle && cu_state != cu_suspended) {
            /* Intel documentation says that CU must be idle or suspended
             * for the CU start command. */
            logout("unexpected CU state is %u\n", cu_state);
        }
        set_cu_state(s, cu_active);
        s->cu_offset = s->pointer;
        action_command(s);
        break;
    case CU_RESUME:
        if (get_cu_state(s) != cu_suspended) {
            logout("bad CU resume from CU state %u\n", get_cu_state(s));
            /* Workaround for bad Linux eepro100 driver which resumes
             * from idle state. */
#if 0
            missing("cu resume");
#endif
            set_cu_state(s, cu_suspended);
        }
        if (get_cu_state(s) == cu_suspended) {
            TRACE(OTHER, logout("CU resuming\n"));
            set_cu_state(s, cu_active);
            action_command(s);
        }
        break;
    case CU_STATSADDR:
        /* Load dump counters address. */
        s->statsaddr = s->pointer;
        TRACE(OTHER, logout("val=0x%02x (status address)\n", val));
        break;
    case CU_SHOWSTATS:
        /* Dump statistical counters. */
        TRACE(OTHER, logout("val=0x%02x (dump stats)\n", val));
        dump_statistics(s);
        stl_le_phys(s->statsaddr + s->stats_size, 0xa005);
        break;
    case CU_CMD_BASE:
        /* Load CU base. */
        TRACE(OTHER, logout("val=0x%02x (CU base address)\n", val));
        s->cu_base = s->pointer;
        break;
    case CU_DUMPSTATS:
        /* Dump and reset statistical counters. */
        TRACE(OTHER, logout("val=0x%02x (dump stats and reset)\n", val));
        dump_statistics(s);
        stl_le_phys(s->statsaddr + s->stats_size, 0xa007);
        memset(&s->statistics, 0, sizeof(s->statistics));
        break;
    case CU_SRESUME:
        /* CU static resume. */
        missing("CU static resume");
        break;
    default:
        missing("Undefined CU command");
    }
}

static void eepro100_ru_command(EEPRO100State * s, uint8_t val)
{
    switch (val) {
    case RU_NOP:
        /* No operation. */
        break;
    case RX_START:
        /* RU start. */
        if (get_ru_state(s) != ru_idle) {
            logout("RU state is %u, should be %u\n", get_ru_state(s), ru_idle);
#if 0
            assert(!"wrong RU state");
#endif
        }
        set_ru_state(s, ru_ready);
        s->ru_offset = s->pointer;
        TRACE(OTHER, logout("val=0x%02x (rx start)\n", val));
        break;
    case RX_RESUME:
        /* Restart RU. */
        if (get_ru_state(s) != ru_suspended) {
            logout("RU state is %u, should be %u\n", get_ru_state(s),
                   ru_suspended);
#if 0
            assert(!"wrong RU state");
#endif
        }
        set_ru_state(s, ru_ready);
        break;
    case RU_ABORT:
        /* RU abort. */
        if (get_ru_state(s) == ru_ready) {
            eepro100_rnr_interrupt(s);
        }
        set_ru_state(s, ru_idle);
        break;
    case RX_ADDR_LOAD:
        /* Load RU base. */
        TRACE(OTHER, logout("val=0x%02x (RU base address)\n", val));
        s->ru_base = s->pointer;
        break;
    default:
        logout("val=0x%02x (undefined RU command)\n", val);
        missing("Undefined SU command");
    }
}

static void eepro100_write_command(EEPRO100State * s, uint8_t val)
{
    eepro100_ru_command(s, val & 0x0f);
    eepro100_cu_command(s, val & 0xf0);
    if ((val) == 0) {
        TRACE(OTHER, logout("val=0x%02x\n", val));
    }
    /* Clear command byte after command was accepted. */
    s->mem[SCBCmd] = 0;
}

/*****************************************************************************
 *
 * EEPROM emulation.
 *
 ****************************************************************************/

#define EEPROM_CS       0x02
#define EEPROM_SK       0x01
#define EEPROM_DI       0x04
#define EEPROM_DO       0x08

static uint16_t eepro100_read_eeprom(EEPRO100State * s)
{
    uint16_t val;
    memcpy(&val, &s->mem[SCBeeprom], sizeof(val));
    if (eeprom93xx_read(s->eeprom)) {
        val |= EEPROM_DO;
    } else {
        val &= ~EEPROM_DO;
    }
    TRACE(EEPROM, logout("val=0x%04x\n", val));
    return val;
}

static void eepro100_write_eeprom(eeprom_t * eeprom, uint8_t val)
{
    TRACE(EEPROM, logout("val=0x%02x\n", val));

    /* mask unwriteable bits */
#if 0
    val = SET_MASKED(val, 0x31, eeprom->value);
#endif

    int eecs = ((val & EEPROM_CS) != 0);
    int eesk = ((val & EEPROM_SK) != 0);
    int eedi = ((val & EEPROM_DI) != 0);
    eeprom93xx_write(eeprom, eecs, eesk, eedi);
}

static void eepro100_write_pointer(EEPRO100State * s, uint32_t val)
{
    s->pointer = le32_to_cpu(val);
    TRACE(OTHER, logout("val=0x%08x\n", val));
}

/*****************************************************************************
 *
 * MDI emulation.
 *
 ****************************************************************************/

#if defined(DEBUG_EEPRO100)
static const char * const mdi_op_name[] = {
    "opcode 0",
    "write",
    "read",
    "opcode 3"
};

static const char * const mdi_reg_name[] = {
    "Control",
    "Status",
    "PHY Identification (Word 1)",
    "PHY Identification (Word 2)",
    "Auto-Negotiation Advertisement",
    "Auto-Negotiation Link Partner Ability",
    "Auto-Negotiation Expansion"
};

static const char *reg2name(uint8_t reg)
{
    static char buffer[10];
    const char *p = buffer;
    if (reg < ARRAY_SIZE(mdi_reg_name)) {
        p = mdi_reg_name[reg];
    } else {
        snprintf(buffer, sizeof(buffer), "reg=0x%02x", reg);
    }
    return p;
}
#endif                          /* DEBUG_EEPRO100 */

static uint32_t eepro100_read_mdi(EEPRO100State * s)
{
    uint32_t val;
    memcpy(&val, &s->mem[0x10], sizeof(val));

#ifdef DEBUG_EEPRO100
    uint8_t raiseint = (val & BIT(29)) >> 29;
    uint8_t opcode = (val & BITS(27, 26)) >> 26;
    uint8_t phy = (val & BITS(25, 21)) >> 21;
    uint8_t reg = (val & BITS(20, 16)) >> 16;
    uint16_t data = (val & BITS(15, 0));
#endif
    /* Emulation takes no time to finish MDI transaction. */
    val |= BIT(28);
    TRACE(MDI, logout("val=0x%08x (int=%u, %s, phy=%u, %s, data=0x%04x\n",
                      val, raiseint, mdi_op_name[opcode], phy,
                      reg2name(reg), data));
    return val;
}

static void eepro100_write_mdi(EEPRO100State * s, uint32_t val)
{
    uint8_t raiseint = (val & BIT(29)) >> 29;
    uint8_t opcode = (val & BITS(27, 26)) >> 26;
    uint8_t phy = (val & BITS(25, 21)) >> 21;
    uint8_t reg = (val & BITS(20, 16)) >> 16;
    uint16_t data = (val & BITS(15, 0));
    TRACE(MDI, logout("val=0x%08x (int=%u, %s, phy=%u, %s, data=0x%04x\n",
          val, raiseint, mdi_op_name[opcode], phy, reg2name(reg), data));
    if (phy != 1) {
        /* Unsupported PHY address. */
#if 0
        logout("phy must be 1 but is %u\n", phy);
#endif
        data = 0;
    } else if (opcode != 1 && opcode != 2) {
        /* Unsupported opcode. */
        logout("opcode must be 1 or 2 but is %u\n", opcode);
        data = 0;
    } else if (reg > 6) {
        /* Unsupported register. */
        logout("register must be 0...6 but is %u\n", reg);
        data = 0;
    } else {
        TRACE(MDI, logout("val=0x%08x (int=%u, %s, phy=%u, %s, data=0x%04x\n",
                          val, raiseint, mdi_op_name[opcode], phy,
                          reg2name(reg), data));
        if (opcode == 1) {
            /* MDI write */
            switch (reg) {
            case 0:            /* Control Register */
                if (data & 0x8000) {
                    /* Reset status and control registers to default. */
                    s->mdimem[0] = eepro100_mdi_default[0];
                    s->mdimem[1] = eepro100_mdi_default[1];
                    data = s->mdimem[reg];
                } else {
                    /* Restart Auto Configuration = Normal Operation */
                    data &= ~0x0200;
                }
                break;
            case 1:            /* Status Register */
                missing("not writable");
                data = s->mdimem[reg];
                break;
            case 2:            /* PHY Identification Register (Word 1) */
            case 3:            /* PHY Identification Register (Word 2) */
                missing("not implemented");
                break;
            case 4:            /* Auto-Negotiation Advertisement Register */
            case 5:            /* Auto-Negotiation Link Partner Ability Register */
                break;
            case 6:            /* Auto-Negotiation Expansion Register */
            default:
                missing("not implemented");
            }
            s->mdimem[reg] = data;
        } else if (opcode == 2) {
            /* MDI read */
            switch (reg) {
            case 0:            /* Control Register */
                if (data & 0x8000) {
                    /* Reset status and control registers to default. */
                    s->mdimem[0] = eepro100_mdi_default[0];
                    s->mdimem[1] = eepro100_mdi_default[1];
                }
                break;
            case 1:            /* Status Register */
                s->mdimem[reg] |= 0x0020;
                break;
            case 2:            /* PHY Identification Register (Word 1) */
            case 3:            /* PHY Identification Register (Word 2) */
            case 4:            /* Auto-Negotiation Advertisement Register */
                break;
            case 5:            /* Auto-Negotiation Link Partner Ability Register */
                s->mdimem[reg] = 0x41fe;
                break;
            case 6:            /* Auto-Negotiation Expansion Register */
                s->mdimem[reg] = 0x0001;
                break;
            }
            data = s->mdimem[reg];
        }
        /* Emulation takes no time to finish MDI transaction.
         * Set MDI bit in SCB status register. */
        s->mem[SCBAck] |= 0x08;
        val |= BIT(28);
        if (raiseint) {
            eepro100_mdi_interrupt(s);
        }
    }
    val = (val & 0xffff0000) + data;
    memcpy(&s->mem[0x10], &val, sizeof(val));
}

/*****************************************************************************
 *
 * Port emulation.
 *
 ****************************************************************************/

#define PORT_SOFTWARE_RESET     0
#define PORT_SELFTEST           1
#define PORT_SELECTIVE_RESET    2
#define PORT_DUMP               3
#define PORT_SELECTION_MASK     3

typedef struct {
    uint32_t st_sign;           /* Self Test Signature */
    uint32_t st_result;         /* Self Test Results */
} eepro100_selftest_t;

static uint32_t eepro100_read_port(EEPRO100State * s)
{
    return 0;
}

static void eepro100_write_port(EEPRO100State * s, uint32_t val)
{
    val = le32_to_cpu(val);
    uint32_t address = (val & ~PORT_SELECTION_MASK);
    uint8_t selection = (val & PORT_SELECTION_MASK);
    switch (selection) {
    case PORT_SOFTWARE_RESET:
        nic_reset(s);
        break;
    case PORT_SELFTEST:
        TRACE(OTHER, logout("selftest address=0x%08x\n", address));
        eepro100_selftest_t data;
        cpu_physical_memory_read(address, (uint8_t *) & data, sizeof(data));
        data.st_sign = 0xffffffff;
        data.st_result = 0;
        cpu_physical_memory_write(address, (uint8_t *) & data, sizeof(data));
        break;
    case PORT_SELECTIVE_RESET:
        TRACE(OTHER, logout("selective reset, selftest address=0x%08x\n", address));
        nic_selective_reset(s);
        break;
    default:
        logout("val=0x%08x\n", val);
        missing("unknown port selection");
    }
}

/*****************************************************************************
 *
 * General hardware emulation.
 *
 ****************************************************************************/

static uint8_t eepro100_read1(EEPRO100State * s, uint32_t addr)
{
    uint8_t val;
    if (addr <= sizeof(s->mem) - sizeof(val)) {
        memcpy(&val, &s->mem[addr], sizeof(val));
    }

    switch (addr) {
    case SCBStatus:
    case SCBAck:
        TRACE(OTHER, logout("addr=%s val=0x%02x\n", regname(addr), val));
        break;
    case SCBCmd:
        TRACE(OTHER, logout("addr=%s val=0x%02x\n", regname(addr), val));
#if 0
        val = eepro100_read_command(s);
#endif
        break;
    case SCBIntmask:
        TRACE(OTHER, logout("addr=%s val=0x%02x\n", regname(addr), val));
        break;
    case SCBPort + 3:
        TRACE(OTHER, logout("addr=%s val=0x%02x\n", regname(addr), val));
        break;
    case SCBeeprom:
        val = eepro100_read_eeprom(s);
        break;
    case SCBpmdr:       /* Power Management Driver Register */
        val = 0;
        TRACE(OTHER, logout("addr=%s val=0x%02x\n", regname(addr), val));
        break;
    case SCBgstat:      /* General Status Register */
        /* 100 Mbps full duplex, valid link */
        val = 0x07;
        TRACE(OTHER, logout("addr=General Status val=%02x\n", val));
        break;
    default:
        logout("addr=%s val=0x%02x\n", regname(addr), val);
        missing("unknown byte read");
    }
    return val;
}

static uint16_t eepro100_read2(EEPRO100State * s, uint32_t addr)
{
    uint16_t val;
    if (addr <= sizeof(s->mem) - sizeof(val)) {
        memcpy(&val, &s->mem[addr], sizeof(val));
    }

    switch (addr) {
    case SCBStatus:
    case SCBCmd:
        TRACE(OTHER, logout("addr=%s val=0x%04x\n", regname(addr), val));
        break;
    case SCBeeprom:
        val = eepro100_read_eeprom(s);
        TRACE(OTHER, logout("addr=%s val=0x%04x\n", regname(addr), val));
        break;
    default:
        logout("addr=%s val=0x%04x\n", regname(addr), val);
        missing("unknown word read");
    }
    return val;
}

static uint32_t eepro100_read4(EEPRO100State * s, uint32_t addr)
{
    uint32_t val;
    if (addr <= sizeof(s->mem) - sizeof(val)) {
        memcpy(&val, &s->mem[addr], sizeof(val));
    }

    switch (addr) {
    case SCBStatus:
        TRACE(OTHER, logout("addr=%s val=0x%08x\n", regname(addr), val));
        break;
    case SCBPointer:
#if 0
        val = eepro100_read_pointer(s);
#endif
        TRACE(OTHER, logout("addr=%s val=0x%08x\n", regname(addr), val));
        break;
    case SCBPort:
        val = eepro100_read_port(s);
        TRACE(OTHER, logout("addr=%s val=0x%08x\n", regname(addr), val));
        break;
    case SCBCtrlMDI:
        val = eepro100_read_mdi(s);
        break;
    default:
        logout("addr=%s val=0x%08x\n", regname(addr), val);
        missing("unknown longword read");
    }
    return val;
}

static void eepro100_write1(EEPRO100State * s, uint32_t addr, uint8_t val)
{
    /* SCBStatus is readonly. */
    if (addr > SCBStatus && addr <= sizeof(s->mem) - sizeof(val)) {
        memcpy(&s->mem[addr], &val, sizeof(val));
    }

    TRACE(OTHER, logout("addr=%s val=0x%02x\n", regname(addr), val));

    switch (addr) {
    case SCBStatus:
        break;
    case SCBAck:
        eepro100_acknowledge(s);
        break;
    case SCBCmd:
        eepro100_write_command(s, val);
        break;
    case SCBIntmask:
        if (val & BIT(1)) {
            eepro100_swi_interrupt(s);
        }
        eepro100_interrupt(s, 0);
        break;
    case SCBPort + 3:
    case SCBFlow:       /* does not exist on 82557 */
    case SCBFlow + 1:
    case SCBFlow + 2:
    case SCBpmdr:       /* does not exist on 82557 */
        TRACE(OTHER, logout("addr=%s val=0x%02x\n", regname(addr), val));
        break;
    case SCBeeprom:
        eepro100_write_eeprom(s->eeprom, val);
        break;
    default:
        logout("addr=%s val=0x%02x\n", regname(addr), val);
        missing("unknown byte write");
    }
}

static void eepro100_write2(EEPRO100State * s, uint32_t addr, uint16_t val)
{
    /* SCBStatus is readonly. */
    if (addr > SCBStatus && addr <= sizeof(s->mem) - sizeof(val)) {
        memcpy(&s->mem[addr], &val, sizeof(val));
    }

    TRACE(OTHER, logout("addr=%s val=0x%04x\n", regname(addr), val));

    switch (addr) {
    case SCBStatus:
        s->mem[SCBAck] = (val >> 8);
        eepro100_acknowledge(s);
        break;
    case SCBCmd:
        eepro100_write_command(s, val);
        eepro100_write1(s, SCBIntmask, val >> 8);
        break;
    case SCBeeprom:
        eepro100_write_eeprom(s->eeprom, val);
        break;
    default:
        logout("addr=%s val=0x%04x\n", regname(addr), val);
        missing("unknown word write");
    }
}

static void eepro100_write4(EEPRO100State * s, uint32_t addr, uint32_t val)
{
    if (addr <= sizeof(s->mem) - sizeof(val)) {
        memcpy(&s->mem[addr], &val, sizeof(val));
    }

    switch (addr) {
    case SCBPointer:
        eepro100_write_pointer(s, val);
        break;
    case SCBPort:
        TRACE(OTHER, logout("addr=%s val=0x%08x\n", regname(addr), val));
        eepro100_write_port(s, val);
        break;
    case SCBCtrlMDI:
        eepro100_write_mdi(s, val);
        break;
    default:
        logout("addr=%s val=0x%08x\n", regname(addr), val);
        missing("unknown longword write");
    }
}

/*****************************************************************************
 *
 * Port mapped I/O.
 *
 ****************************************************************************/

static uint32_t ioport_read1(void *opaque, uint32_t addr)
{
    EEPRO100State *s = opaque;
#if 0
    logout("addr=%s\n", regname(addr));
#endif
    return eepro100_read1(s, addr - s->region[1]);
}

static uint32_t ioport_read2(void *opaque, uint32_t addr)
{
    EEPRO100State *s = opaque;
    return eepro100_read2(s, addr - s->region[1]);
}

static uint32_t ioport_read4(void *opaque, uint32_t addr)
{
    EEPRO100State *s = opaque;
    return eepro100_read4(s, addr - s->region[1]);
}

static void ioport_write1(void *opaque, uint32_t addr, uint32_t val)
{
    EEPRO100State *s = opaque;
#if 0
    logout("addr=%s val=0x%02x\n", regname(addr), val);
#endif
    eepro100_write1(s, addr - s->region[1], val);
}

static void ioport_write2(void *opaque, uint32_t addr, uint32_t val)
{
    EEPRO100State *s = opaque;
    eepro100_write2(s, addr - s->region[1], val);
}

static void ioport_write4(void *opaque, uint32_t addr, uint32_t val)
{
    EEPRO100State *s = opaque;
    eepro100_write4(s, addr - s->region[1], val);
}

/***********************************************************/
/* PCI EEPRO100 definitions */

static void pci_map(PCIDevice * pci_dev, int region_num,
                    pcibus_t addr, pcibus_t size, int type)
{
    EEPRO100State *s = DO_UPCAST(EEPRO100State, dev, pci_dev);

    TRACE(OTHER, logout("region %d, addr=0x%08"FMT_PCIBUS", "
          "size=0x%08"FMT_PCIBUS", type=%d\n",
          region_num, addr, size, type));

    assert(region_num == 1);
    register_ioport_write(addr, size, 1, ioport_write1, s);
    register_ioport_read(addr, size, 1, ioport_read1, s);
    register_ioport_write(addr, size, 2, ioport_write2, s);
    register_ioport_read(addr, size, 2, ioport_read2, s);
    register_ioport_write(addr, size, 4, ioport_write4, s);
    register_ioport_read(addr, size, 4, ioport_read4, s);

    s->region[region_num] = addr;
}

/*****************************************************************************
 *
 * Memory mapped I/O.
 *
 ****************************************************************************/

static void pci_mmio_writeb(void *opaque, target_phys_addr_t addr, uint32_t val)
{
    EEPRO100State *s = opaque;
#if 0
    logout("addr=%s val=0x%02x\n", regname(addr), val);
#endif
    eepro100_write1(s, addr, val);
}

static void pci_mmio_writew(void *opaque, target_phys_addr_t addr, uint32_t val)
{
    EEPRO100State *s = opaque;
#if 0
    logout("addr=%s val=0x%02x\n", regname(addr), val);
#endif
    eepro100_write2(s, addr, val);
}

static void pci_mmio_writel(void *opaque, target_phys_addr_t addr, uint32_t val)
{
    EEPRO100State *s = opaque;
#if 0
    logout("addr=%s val=0x%02x\n", regname(addr), val);
#endif
    eepro100_write4(s, addr, val);
}

static uint32_t pci_mmio_readb(void *opaque, target_phys_addr_t addr)
{
    EEPRO100State *s = opaque;
#if 0
    logout("addr=%s\n", regname(addr));
#endif
    return eepro100_read1(s, addr);
}

static uint32_t pci_mmio_readw(void *opaque, target_phys_addr_t addr)
{
    EEPRO100State *s = opaque;
#if 0
    logout("addr=%s\n", regname(addr));
#endif
    return eepro100_read2(s, addr);
}

static uint32_t pci_mmio_readl(void *opaque, target_phys_addr_t addr)
{
    EEPRO100State *s = opaque;
#if 0
    logout("addr=%s\n", regname(addr));
#endif
    return eepro100_read4(s, addr);
}

static CPUWriteMemoryFunc * const pci_mmio_write[] = {
    pci_mmio_writeb,
    pci_mmio_writew,
    pci_mmio_writel
};

static CPUReadMemoryFunc * const pci_mmio_read[] = {
    pci_mmio_readb,
    pci_mmio_readw,
    pci_mmio_readl
};

static void pci_mmio_map(PCIDevice * pci_dev, int region_num,
                         pcibus_t addr, pcibus_t size, int type)
{
    EEPRO100State *s = DO_UPCAST(EEPRO100State, dev, pci_dev);

    TRACE(OTHER, logout("region %d, addr=0x%08"FMT_PCIBUS", "
          "size=0x%08"FMT_PCIBUS", type=%d\n",
          region_num, addr, size, type));

    assert(region_num == 0 || region_num == 2);

    /* Map control / status registers and flash. */
    cpu_register_physical_memory(addr, size, s->mmio_index);
    s->region[region_num] = addr;
}

static int nic_can_receive(VLANClientState *nc)
{
    EEPRO100State *s = DO_UPCAST(NICState, nc, nc)->opaque;
    TRACE(RXTX, logout("%p\n", s));
    return get_ru_state(s) == ru_ready;
#if 0
    return !eepro100_buffer_full(s);
#endif
}

static ssize_t nic_receive(VLANClientState *nc, const uint8_t * buf, size_t size)
{
    /* TODO:
     * - Magic packets should set bit 30 in power management driver register.
     * - Interesting packets should set bit 29 in power management driver register.
     */
    EEPRO100State *s = DO_UPCAST(NICState, nc, nc)->opaque;
    uint16_t rfd_status = 0xa000;
    static const uint8_t broadcast_macaddr[6] =
        { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    /* TODO: check multiple IA bit. */
    if (s->configuration[20] & BIT(6)) {
        missing("Multiple IA bit");
        return -1;
    }

    if (s->configuration[8] & 0x80) {
        /* CSMA is disabled. */
        logout("%p received while CSMA is disabled\n", s);
        return -1;
    } else if (size < 64 && (s->configuration[7] & BIT(0))) {
        /* Short frame and configuration byte 7/0 (discard short receive) set:
         * Short frame is discarded */
        logout("%p received short frame (%zu byte)\n", s, size);
        s->statistics.rx_short_frame_errors++;
#if 0
        return -1;
#endif
    } else if ((size > MAX_ETH_FRAME_SIZE + 4) && !(s->configuration[18] & BIT(3))) {
        /* Long frame and configuration byte 18/3 (long receive ok) not set:
         * Long frames are discarded. */
        logout("%p received long frame (%zu byte), ignored\n", s, size);
        return -1;
    } else if (memcmp(buf, s->conf.macaddr.a, 6) == 0) {       /* !!! */
        /* Frame matches individual address. */
        /* TODO: check configuration byte 15/4 (ignore U/L). */
        TRACE(RXTX, logout("%p received frame for me, len=%zu\n", s, size));
    } else if (memcmp(buf, broadcast_macaddr, 6) == 0) {
        /* Broadcast frame. */
        TRACE(RXTX, logout("%p received broadcast, len=%zu\n", s, size));
        rfd_status |= 0x0002;
    } else if (buf[0] & 0x01) {
        /* Multicast frame. */
        TRACE(RXTX, logout("%p received multicast, len=%zu,%s\n", s, size, nic_dump(buf, size)));
        if (s->configuration[21] & BIT(3)) {
          /* Multicast all bit is set, receive all multicast frames. */
        } else {
          unsigned mcast_idx = compute_mcast_idx(buf);
          assert(mcast_idx < 64);
          if (s->mult[mcast_idx >> 3] & (1 << (mcast_idx & 7))) {
            /* Multicast frame is allowed in hash table. */
          } else if (s->configuration[15] & BIT(0)) {
              /* Promiscuous: receive all. */
              rfd_status |= 0x0004;
          } else {
              TRACE(RXTX, logout("%p multicast ignored\n", s));
              return -1;
          }
        }
        /* TODO: Next not for promiscuous mode? */
        rfd_status |= 0x0002;
    } else if (s->configuration[15] & BIT(0)) {
        /* Promiscuous: receive all. */
        TRACE(RXTX, logout("%p received frame in promiscuous mode, len=%zu\n", s, size));
        rfd_status |= 0x0004;
    } else {
        TRACE(RXTX, logout("%p received frame, ignored, len=%zu,%s\n", s, size,
              nic_dump(buf, size)));
        return size;
    }

    if (get_ru_state(s) != ru_ready) {
        /* No resources available. */
        logout("no resources, state=%u\n", get_ru_state(s));
        /* TODO: RNR interrupt only at first failed frame? */
        eepro100_rnr_interrupt(s);
        s->statistics.rx_resource_errors++;
#if 0
        assert(!"no resources");
#endif
        return -1;
    }
    /* !!! */
    eepro100_rx_t rx;
    cpu_physical_memory_read(s->ru_base + s->ru_offset, (uint8_t *) & rx,
                             offsetof(eepro100_rx_t, packet));
    uint16_t rfd_command = le16_to_cpu(rx.command);
    uint16_t rfd_size = le16_to_cpu(rx.size);

    if (size > rfd_size) {
        logout("Receive buffer (%" PRId16 " bytes) too small for data "
            "(%zu bytes); data truncated\n", rfd_size, size);
        size = rfd_size;
    }
    if (size < 64) {
        rfd_status |= 0x0080;
    }
    TRACE(OTHER, logout("command 0x%04x, link 0x%08x, addr 0x%08x, size %u\n",
          rfd_command, rx.link, rx.rx_buf_addr, rfd_size));
    stw_phys(s->ru_base + s->ru_offset + offsetof(eepro100_rx_t, status),
             rfd_status);
    stw_phys(s->ru_base + s->ru_offset + offsetof(eepro100_rx_t, count), size);
    /* Early receive interrupt not supported. */
#if 0
    eepro100_er_interrupt(s);
#endif
    /* Receive CRC Transfer not supported. */
    if (s->configuration[18] & BIT(2)) {
        missing("Receive CRC Transfer");
        return -1;
    }
    /* TODO: check stripping enable bit. */
#if 0
    assert(!(s->configuration[17] & BIT(0)));
#endif
    cpu_physical_memory_write(s->ru_base + s->ru_offset +
                              offsetof(eepro100_rx_t, packet), buf, size);
    s->statistics.rx_good_frames++;
    eepro100_fr_interrupt(s);
    s->ru_offset = le32_to_cpu(rx.link);
    if (rfd_command & COMMAND_EL) {
        /* EL bit is set, so this was the last frame. */
        logout("receive: Running out of frames\n");
        set_ru_state(s, ru_suspended);
    }
    if (rfd_command & COMMAND_S) {
        /* S bit is set. */
        set_ru_state(s, ru_suspended);
    }
    return size;
}

static const VMStateDescription vmstate_eepro100 = {
    .version_id = 3,
    .minimum_version_id = 2,
    .minimum_version_id_old = 2,
    .fields      = (VMStateField []) {
        VMSTATE_PCI_DEVICE(dev, EEPRO100State),
        VMSTATE_UNUSED(32),
        VMSTATE_BUFFER(mult, EEPRO100State),
        VMSTATE_BUFFER(mem, EEPRO100State),
        /* Save all members of struct between scb_stat and mem. */
        VMSTATE_UINT8(scb_stat, EEPRO100State),
        VMSTATE_UINT8(int_stat, EEPRO100State),
        VMSTATE_UNUSED(3*4),
        VMSTATE_MACADDR(conf.macaddr, EEPRO100State),
        VMSTATE_UNUSED(19*4),
        VMSTATE_UINT16_ARRAY(mdimem, EEPRO100State, 32),
        /* The eeprom should be saved and restored by its own routines. */
        VMSTATE_UINT32(device, EEPRO100State),
        /* TODO check device. */
        VMSTATE_UINT32(pointer, EEPRO100State),
        VMSTATE_UINT32(cu_base, EEPRO100State),
        VMSTATE_UINT32(cu_offset, EEPRO100State),
        VMSTATE_UINT32(ru_base, EEPRO100State),
        VMSTATE_UINT32(ru_offset, EEPRO100State),
        VMSTATE_UINT32(statsaddr, EEPRO100State),
        /* Save eepro100_stats_t statistics. */
        VMSTATE_UINT32(statistics.tx_good_frames, EEPRO100State),
        VMSTATE_UINT32(statistics.tx_max_collisions, EEPRO100State),
        VMSTATE_UINT32(statistics.tx_late_collisions, EEPRO100State),
        VMSTATE_UINT32(statistics.tx_underruns, EEPRO100State),
        VMSTATE_UINT32(statistics.tx_lost_crs, EEPRO100State),
        VMSTATE_UINT32(statistics.tx_deferred, EEPRO100State),
        VMSTATE_UINT32(statistics.tx_single_collisions, EEPRO100State),
        VMSTATE_UINT32(statistics.tx_multiple_collisions, EEPRO100State),
        VMSTATE_UINT32(statistics.tx_total_collisions, EEPRO100State),
        VMSTATE_UINT32(statistics.rx_good_frames, EEPRO100State),
        VMSTATE_UINT32(statistics.rx_crc_errors, EEPRO100State),
        VMSTATE_UINT32(statistics.rx_alignment_errors, EEPRO100State),
        VMSTATE_UINT32(statistics.rx_resource_errors, EEPRO100State),
        VMSTATE_UINT32(statistics.rx_overrun_errors, EEPRO100State),
        VMSTATE_UINT32(statistics.rx_cdt_errors, EEPRO100State),
        VMSTATE_UINT32(statistics.rx_short_frame_errors, EEPRO100State),
        VMSTATE_UINT32(statistics.fc_xmt_pause, EEPRO100State),
        VMSTATE_UINT32(statistics.fc_rcv_pause, EEPRO100State),
        VMSTATE_UINT32(statistics.fc_rcv_unsupported, EEPRO100State),
        VMSTATE_UINT16(statistics.xmt_tco_frames, EEPRO100State),
        VMSTATE_UINT16(statistics.rcv_tco_frames, EEPRO100State),
        /* Configuration bytes. */
        VMSTATE_BUFFER(configuration, EEPRO100State),
        VMSTATE_END_OF_LIST()
    }
};

static void nic_cleanup(VLANClientState *nc)
{
    EEPRO100State *s = DO_UPCAST(NICState, nc, nc)->opaque;

    s->nic = NULL;
}

static int pci_nic_uninit(PCIDevice *pci_dev)
{
    EEPRO100State *s = DO_UPCAST(EEPRO100State, dev, pci_dev);

    cpu_unregister_io_memory(s->mmio_index);
    vmstate_unregister(s->vmstate, s);
    eeprom93xx_free(s->eeprom);
    qemu_del_vlan_client(&s->nic->nc);
    return 0;
}

static NetClientInfo net_eepro100_info = {
    .type = NET_CLIENT_TYPE_NIC,
    .size = sizeof(NICState),
    .can_receive = nic_can_receive,
    .receive = nic_receive,
    .cleanup = nic_cleanup,
};

static int e100_nic_init(PCIDevice *pci_dev)
{
    EEPRO100State *s = DO_UPCAST(EEPRO100State, dev, pci_dev);
    E100PCIDeviceInfo *e100_device = DO_UPCAST(E100PCIDeviceInfo, pci.qdev,
                                               pci_dev->qdev.info);

    TRACE(OTHER, logout("\n"));

    s->device = e100_device->device;

    e100_pci_reset(s, e100_device);

    /* Add 64 * 2 EEPROM. i82557 and i82558 support a 64 word EEPROM,
     * i82559 and later support 64 or 256 word EEPROM. */
    s->eeprom = eeprom93xx_new(EEPROM_SIZE);

    /* Handler for memory-mapped I/O */
    s->mmio_index =
        cpu_register_io_memory(pci_mmio_read, pci_mmio_write, s);

    pci_register_bar(&s->dev, 0, PCI_MEM_SIZE,
                           PCI_BASE_ADDRESS_SPACE_MEMORY |
                           PCI_BASE_ADDRESS_MEM_PREFETCH, pci_mmio_map);
    pci_register_bar(&s->dev, 1, PCI_IO_SIZE, PCI_BASE_ADDRESS_SPACE_IO,
                           pci_map);
    pci_register_bar(&s->dev, 2, PCI_FLASH_SIZE, PCI_BASE_ADDRESS_SPACE_MEMORY,
                           pci_mmio_map);

    qemu_macaddr_default_if_unset(&s->conf.macaddr);
    logout("macaddr: %s\n", nic_dump(&s->conf.macaddr.a[0], 6));
    assert(s->region[1] == 0);

    nic_reset(s);

    s->nic = qemu_new_nic(&net_eepro100_info, &s->conf,
                          pci_dev->qdev.info->name, pci_dev->qdev.id, s);

    qemu_format_nic_info_str(&s->nic->nc, s->conf.macaddr.a);
    TRACE(OTHER, logout("%s\n", s->nic->nc.info_str));

    qemu_register_reset(nic_reset, s);

    s->vmstate = qemu_malloc(sizeof(vmstate_eepro100));
    memcpy(s->vmstate, &vmstate_eepro100, sizeof(vmstate_eepro100));
    s->vmstate->name = s->nic->nc.model;
    vmstate_register(-1, s->vmstate, s);

    return 0;
}

static E100PCIDeviceInfo e100_devices[] = {
    {
        .pci.qdev.name = "i82550",
        .pci.qdev.desc = "Intel i82550 Ethernet",
        .device = i82550,
        /* TODO: check device id. */
        .device_id = PCI_DEVICE_ID_INTEL_82551IT,
        /* Revision ID: 0x0c, 0x0d, 0x0e. */
        .revision = 0x0e,
        /* TODO: check size of statistical counters. */
        .stats_size = 80,
        /* TODO: check extended tcb support. */
        .has_extended_tcb_support = true,
        .power_management = true,
    },{
        .pci.qdev.name = "i82551",
        .pci.qdev.desc = "Intel i82551 Ethernet",
        .device = i82551,
        .device_id = PCI_DEVICE_ID_INTEL_82551IT,
        /* Revision ID: 0x0f, 0x10. */
        .revision = 0x0f,
        /* TODO: check size of statistical counters. */
        .stats_size = 80,
        .has_extended_tcb_support = true,
        .power_management = true,
    },{
        .pci.qdev.name = "i82557a",
        .pci.qdev.desc = "Intel i82557A Ethernet",
        .device = i82557A,
        .device_id = PCI_DEVICE_ID_INTEL_82557,
        .revision = 0x01,
        .power_management = false,
    },{
        .pci.qdev.name = "i82557b",
        .pci.qdev.desc = "Intel i82557B Ethernet",
        .device = i82557B,
        .device_id = PCI_DEVICE_ID_INTEL_82557,
        .revision = 0x02,
        .power_management = false,
    },{
        .pci.qdev.name = "i82557c",
        .pci.qdev.desc = "Intel i82557C Ethernet",
        .device = i82557C,
        .device_id = PCI_DEVICE_ID_INTEL_82557,
        .revision = 0x03,
        .power_management = false,
    },{
        .pci.qdev.name = "i82558a",
        .pci.qdev.desc = "Intel i82558A Ethernet",
        .device = i82558A,
        .device_id = PCI_DEVICE_ID_INTEL_82557,
        .revision = 0x04,
        .stats_size = 76,
        .has_extended_tcb_support = true,
        .power_management = true,
    },{
        .pci.qdev.name = "i82558b",
        .pci.qdev.desc = "Intel i82558B Ethernet",
        .device = i82558B,
        .device_id = PCI_DEVICE_ID_INTEL_82557,
        .revision = 0x05,
        .stats_size = 76,
        .has_extended_tcb_support = true,
        .power_management = true,
    },{
        .pci.qdev.name = "i82559a",
        .pci.qdev.desc = "Intel i82559A Ethernet",
        .device = i82559A,
        .device_id = PCI_DEVICE_ID_INTEL_82557,
        .revision = 0x06,
        .stats_size = 80,
        .has_extended_tcb_support = true,
        .power_management = true,
    },{
        .pci.qdev.name = "i82559b",
        .pci.qdev.desc = "Intel i82559B Ethernet",
        .device = i82559B,
        .device_id = PCI_DEVICE_ID_INTEL_82557,
        .revision = 0x07,
        .stats_size = 80,
        .has_extended_tcb_support = true,
        .power_management = true,
    },{
        .pci.qdev.name = "i82559c",
        .pci.qdev.desc = "Intel i82559C Ethernet",
        .device = i82559C,
        .device_id = PCI_DEVICE_ID_INTEL_82557,
#if 0
        .revision = 0x08,
#endif
        /* TODO: Windows wants revision id 0x0c. */
        .revision = 0x0c,
        .stats_size = 80,
        .has_extended_tcb_support = true,
        .power_management = true,
    },{
        .pci.qdev.name = "i82559er",
        .pci.qdev.desc = "Intel i82559ER Ethernet",
        .device = i82559ER,
        .device_id = PCI_DEVICE_ID_INTEL_82551IT,
        .revision = 0x09,
        .stats_size = 80,
        .has_extended_tcb_support = true,
        .power_management = true,
    },{
        .pci.qdev.name = "i82562",
        .pci.qdev.desc = "Intel i82562 Ethernet",
        .device = i82562,
        /* TODO: check device id. */
        .device_id = PCI_DEVICE_ID_INTEL_82551IT,
        /* TODO: wrong revision id. */
        .revision = 0x0e,
        .stats_size = 80,
        .has_extended_tcb_support = true,
        .power_management = true,
    },{
        /* Toshiba Tecra 8200. */
        .pci.qdev.name = "i82801",
        .pci.qdev.desc = "Intel i82801 Ethernet",
        .device = i82801,
        .device_id = 0x2449,
        .revision = 0x03,
        .stats_size = 80,
        .has_extended_tcb_support = true,
        .power_management = true,
    }
};

static Property e100_properties[] = {
    DEFINE_NIC_PROPERTIES(EEPRO100State, conf),
    DEFINE_PROP_END_OF_LIST(),
};

static void eepro100_register_devices(void)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(e100_devices); i++) {
        PCIDeviceInfo *pci_dev = &e100_devices[i].pci;
        switch (e100_devices[i].device_id) {
            case PCI_DEVICE_ID_INTEL_82551IT:
                pci_dev->romfile = "gpxe-eepro100-80861209.rom";
                break;
            case PCI_DEVICE_ID_INTEL_82557:
                pci_dev->romfile = "gpxe-eepro100-80861229.rom";
                break;
            case 0x2449:
                pci_dev->romfile = "gpxe-eepro100-80862449.rom";
                break;
        }
        pci_dev->init = e100_nic_init;
        pci_dev->exit = pci_nic_uninit;
        pci_dev->qdev.props = e100_properties;
        pci_dev->qdev.size = sizeof(EEPRO100State);
        pci_qdev_register(pci_dev);
    }
}

device_init(eepro100_register_devices);
