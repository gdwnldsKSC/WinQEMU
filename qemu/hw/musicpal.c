/*
 * Marvell MV88W8618 / Freecom MusicPal emulation.
 *
 * Copyright (c) 2008 Jan Kiszka
 *
 * This code is licenced under the GNU GPL v2.
 */

#include "sysbus.h"
#include "arm-misc.h"
#include "devices.h"
#include "net.h"
#include "sysemu.h"
#include "boards.h"
#include "pc.h"
#include "qemu-timer.h"
#include "block.h"
#include "flash.h"
#include "console.h"
#include "i2c.h"

#define MP_MISC_BASE            0x80002000
#define MP_MISC_SIZE            0x00001000

#define MP_ETH_BASE             0x80008000
#define MP_ETH_SIZE             0x00001000

#define MP_WLAN_BASE            0x8000C000
#define MP_WLAN_SIZE            0x00000800

#define MP_UART1_BASE           0x8000C840
#define MP_UART2_BASE           0x8000C940

#define MP_GPIO_BASE            0x8000D000
#define MP_GPIO_SIZE            0x00001000

#define MP_FLASHCFG_BASE        0x90006000
#define MP_FLASHCFG_SIZE        0x00001000

#define MP_AUDIO_BASE           0x90007000

#define MP_PIC_BASE             0x90008000
#define MP_PIC_SIZE             0x00001000

#define MP_PIT_BASE             0x90009000
#define MP_PIT_SIZE             0x00001000

#define MP_LCD_BASE             0x9000c000
#define MP_LCD_SIZE             0x00001000

#define MP_SRAM_BASE            0xC0000000
#define MP_SRAM_SIZE            0x00020000

#define MP_RAM_DEFAULT_SIZE     32*1024*1024
#define MP_FLASH_SIZE_MAX       32*1024*1024

#define MP_TIMER1_IRQ           4
#define MP_TIMER2_IRQ           5
#define MP_TIMER3_IRQ           6
#define MP_TIMER4_IRQ           7
#define MP_EHCI_IRQ             8
#define MP_ETH_IRQ              9
#define MP_UART1_IRQ            11
#define MP_UART2_IRQ            11
#define MP_GPIO_IRQ             12
#define MP_RTC_IRQ              28
#define MP_AUDIO_IRQ            30

/* Wolfson 8750 I2C address */
#define MP_WM_ADDR              0x34

/* Ethernet register offsets */
#define MP_ETH_SMIR             0x010
#define MP_ETH_PCXR             0x408
#define MP_ETH_SDCMR            0x448
#define MP_ETH_ICR              0x450
#define MP_ETH_IMR              0x458
#define MP_ETH_FRDP0            0x480
#define MP_ETH_FRDP1            0x484
#define MP_ETH_FRDP2            0x488
#define MP_ETH_FRDP3            0x48C
#define MP_ETH_CRDP0            0x4A0
#define MP_ETH_CRDP1            0x4A4
#define MP_ETH_CRDP2            0x4A8
#define MP_ETH_CRDP3            0x4AC
#define MP_ETH_CTDP0            0x4E0
#define MP_ETH_CTDP1            0x4E4
#define MP_ETH_CTDP2            0x4E8
#define MP_ETH_CTDP3            0x4EC

/* MII PHY access */
#define MP_ETH_SMIR_DATA        0x0000FFFF
#define MP_ETH_SMIR_ADDR        0x03FF0000
#define MP_ETH_SMIR_OPCODE      (1 << 26) /* Read value */
#define MP_ETH_SMIR_RDVALID     (1 << 27)

/* PHY registers */
#define MP_ETH_PHY1_BMSR        0x00210000
#define MP_ETH_PHY1_PHYSID1     0x00410000
#define MP_ETH_PHY1_PHYSID2     0x00610000

#define MP_PHY_BMSR_LINK        0x0004
#define MP_PHY_BMSR_AUTONEG     0x0008

#define MP_PHY_88E3015          0x01410E20

/* TX descriptor status */
#define MP_ETH_TX_OWN           (1 << 31)

/* RX descriptor status */
#define MP_ETH_RX_OWN           (1 << 31)

/* Interrupt cause/mask bits */
#define MP_ETH_IRQ_RX_BIT       0
#define MP_ETH_IRQ_RX           (1 << MP_ETH_IRQ_RX_BIT)
#define MP_ETH_IRQ_TXHI_BIT     2
#define MP_ETH_IRQ_TXLO_BIT     3

/* Port config bits */
#define MP_ETH_PCXR_2BSM_BIT    28 /* 2-byte incoming suffix */

/* SDMA command bits */
#define MP_ETH_CMD_TXHI         (1 << 23)
#define MP_ETH_CMD_TXLO         (1 << 22)

typedef struct mv88w8618_tx_desc {
    uint32_t cmdstat;
    uint16_t res;
    uint16_t bytes;
    uint32_t buffer;
    uint32_t next;
} mv88w8618_tx_desc;

typedef struct mv88w8618_rx_desc {
    uint32_t cmdstat;
    uint16_t bytes;
    uint16_t buffer_size;
    uint32_t buffer;
    uint32_t next;
} mv88w8618_rx_desc;

typedef struct mv88w8618_eth_state {
    SysBusDevice busdev;
    qemu_irq irq;
    uint32_t smir;
    uint32_t icr;
    uint32_t imr;
    int mmio_index;
    int vlan_header;
    uint32_t tx_queue[2];
    uint32_t rx_queue[4];
    uint32_t frx_queue[4];
    uint32_t cur_rx[4];
    VLANClientState *vc;
} mv88w8618_eth_state;

static void eth_rx_desc_put(uint32_t addr, mv88w8618_rx_desc *desc)
{
    cpu_to_le32s(&desc->cmdstat);
    cpu_to_le16s(&desc->bytes);
    cpu_to_le16s(&desc->buffer_size);
    cpu_to_le32s(&desc->buffer);
    cpu_to_le32s(&desc->next);
    cpu_physical_memory_write(addr, (void *)desc, sizeof(*desc));
}

static void eth_rx_desc_get(uint32_t addr, mv88w8618_rx_desc *desc)
{
    cpu_physical_memory_read(addr, (void *)desc, sizeof(*desc));
    le32_to_cpus(&desc->cmdstat);
    le16_to_cpus(&desc->bytes);
    le16_to_cpus(&desc->buffer_size);
    le32_to_cpus(&desc->buffer);
    le32_to_cpus(&desc->next);
}

static int eth_can_receive(VLANClientState *vc)
{
    return 1;
}

static ssize_t eth_receive(VLANClientState *vc, const uint8_t *buf, size_t size)
{
    mv88w8618_eth_state *s = vc->opaque;
    uint32_t desc_addr;
    mv88w8618_rx_desc desc;
    int i;

    for (i = 0; i < 4; i++) {
        desc_addr = s->cur_rx[i];
        if (!desc_addr)
            continue;
        do {
            eth_rx_desc_get(desc_addr, &desc);
            if ((desc.cmdstat & MP_ETH_RX_OWN) && desc.buffer_size >= size) {
                cpu_physical_memory_write(desc.buffer + s->vlan_header,
                                          buf, size);
                desc.bytes = size + s->vlan_header;
                desc.cmdstat &= ~MP_ETH_RX_OWN;
                s->cur_rx[i] = desc.next;

                s->icr |= MP_ETH_IRQ_RX;
                if (s->icr & s->imr)
                    qemu_irq_raise(s->irq);
                eth_rx_desc_put(desc_addr, &desc);
                return size;
            }
            desc_addr = desc.next;
        } while (desc_addr != s->rx_queue[i]);
    }
    return size;
}

static void eth_tx_desc_put(uint32_t addr, mv88w8618_tx_desc *desc)
{
    cpu_to_le32s(&desc->cmdstat);
    cpu_to_le16s(&desc->res);
    cpu_to_le16s(&desc->bytes);
    cpu_to_le32s(&desc->buffer);
    cpu_to_le32s(&desc->next);
    cpu_physical_memory_write(addr, (void *)desc, sizeof(*desc));
}

static void eth_tx_desc_get(uint32_t addr, mv88w8618_tx_desc *desc)
{
    cpu_physical_memory_read(addr, (void *)desc, sizeof(*desc));
    le32_to_cpus(&desc->cmdstat);
    le16_to_cpus(&desc->res);
    le16_to_cpus(&desc->bytes);
    le32_to_cpus(&desc->buffer);
    le32_to_cpus(&desc->next);
}

static void eth_send(mv88w8618_eth_state *s, int queue_index)
{
    uint32_t desc_addr = s->tx_queue[queue_index];
    mv88w8618_tx_desc desc;
    uint8_t buf[2048];
    int len;


    do {
        eth_tx_desc_get(desc_addr, &desc);
        if (desc.cmdstat & MP_ETH_TX_OWN) {
            len = desc.bytes;
            if (len < 2048) {
                cpu_physical_memory_read(desc.buffer, buf, len);
                qemu_send_packet(s->vc, buf, len);
            }
            desc.cmdstat &= ~MP_ETH_TX_OWN;
            s->icr |= 1 << (MP_ETH_IRQ_TXLO_BIT - queue_index);
            eth_tx_desc_put(desc_addr, &desc);
        }
        desc_addr = desc.next;
    } while (desc_addr != s->tx_queue[queue_index]);
}

static uint32_t mv88w8618_eth_read(void *opaque, target_phys_addr_t offset)
{
    mv88w8618_eth_state *s = opaque;

    switch (offset) {
    case MP_ETH_SMIR:
        if (s->smir & MP_ETH_SMIR_OPCODE) {
            switch (s->smir & MP_ETH_SMIR_ADDR) {
            case MP_ETH_PHY1_BMSR:
                return MP_PHY_BMSR_LINK | MP_PHY_BMSR_AUTONEG |
                       MP_ETH_SMIR_RDVALID;
            case MP_ETH_PHY1_PHYSID1:
                return (MP_PHY_88E3015 >> 16) | MP_ETH_SMIR_RDVALID;
            case MP_ETH_PHY1_PHYSID2:
                return (MP_PHY_88E3015 & 0xFFFF) | MP_ETH_SMIR_RDVALID;
            default:
                return MP_ETH_SMIR_RDVALID;
            }
        }
        return 0;

    case MP_ETH_ICR:
        return s->icr;

    case MP_ETH_IMR:
        return s->imr;

    case MP_ETH_FRDP0 ... MP_ETH_FRDP3:
        return s->frx_queue[(offset - MP_ETH_FRDP0)/4];

    case MP_ETH_CRDP0 ... MP_ETH_CRDP3:
        return s->rx_queue[(offset - MP_ETH_CRDP0)/4];

    case MP_ETH_CTDP0 ... MP_ETH_CTDP3:
        return s->tx_queue[(offset - MP_ETH_CTDP0)/4];

    default:
        return 0;
    }
}

static void mv88w8618_eth_write(void *opaque, target_phys_addr_t offset,
                                uint32_t value)
{
    mv88w8618_eth_state *s = opaque;

    switch (offset) {
    case MP_ETH_SMIR:
        s->smir = value;
        break;

    case MP_ETH_PCXR:
        s->vlan_header = ((value >> MP_ETH_PCXR_2BSM_BIT) & 1) * 2;
        break;

    case MP_ETH_SDCMR:
        if (value & MP_ETH_CMD_TXHI)
            eth_send(s, 1);
        if (value & MP_ETH_CMD_TXLO)
            eth_send(s, 0);
        if (value & (MP_ETH_CMD_TXHI | MP_ETH_CMD_TXLO) && s->icr & s->imr)
            qemu_irq_raise(s->irq);
        break;

    case MP_ETH_ICR:
        s->icr &= value;
        break;

    case MP_ETH_IMR:
        s->imr = value;
        if (s->icr & s->imr)
            qemu_irq_raise(s->irq);
        break;

    case MP_ETH_FRDP0 ... MP_ETH_FRDP3:
        s->frx_queue[(offset - MP_ETH_FRDP0)/4] = value;
        break;

    case MP_ETH_CRDP0 ... MP_ETH_CRDP3:
        s->rx_queue[(offset - MP_ETH_CRDP0)/4] =
            s->cur_rx[(offset - MP_ETH_CRDP0)/4] = value;
        break;

    case MP_ETH_CTDP0 ... MP_ETH_CTDP3:
        s->tx_queue[(offset - MP_ETH_CTDP0)/4] = value;
        break;
    }
}

static CPUReadMemoryFunc * const mv88w8618_eth_readfn[] = {
    mv88w8618_eth_read,
    mv88w8618_eth_read,
    mv88w8618_eth_read
};

static CPUWriteMemoryFunc * const mv88w8618_eth_writefn[] = {
    mv88w8618_eth_write,
    mv88w8618_eth_write,
    mv88w8618_eth_write
};

static void eth_cleanup(VLANClientState *vc)
{
    mv88w8618_eth_state *s = vc->opaque;

    cpu_unregister_io_memory(s->mmio_index);

    qemu_free(s);
}

static void mv88w8618_eth_init(SysBusDevice *dev)
{
    mv88w8618_eth_state *s = FROM_SYSBUS(mv88w8618_eth_state, dev);

    sysbus_init_irq(dev, &s->irq);
    s->vc = qdev_get_vlan_client(&dev->qdev,
                                 eth_can_receive, eth_receive, NULL,
                                 eth_cleanup, s);
    s->mmio_index = cpu_register_io_memory(mv88w8618_eth_readfn,
                                           mv88w8618_eth_writefn, s);
    sysbus_init_mmio(dev, MP_ETH_SIZE, s->mmio_index);
}

/* LCD register offsets */
#define MP_LCD_IRQCTRL          0x180
#define MP_LCD_IRQSTAT          0x184
#define MP_LCD_SPICTRL          0x1ac
#define MP_LCD_INST             0x1bc
#define MP_LCD_DATA             0x1c0

/* Mode magics */
#define MP_LCD_SPI_DATA         0x00100011
#define MP_LCD_SPI_CMD          0x00104011
#define MP_LCD_SPI_INVALID      0x00000000

/* Commmands */
#define MP_LCD_INST_SETPAGE0    0xB0
/* ... */
#define MP_LCD_INST_SETPAGE7    0xB7

#define MP_LCD_TEXTCOLOR        0xe0e0ff /* RRGGBB */

typedef struct musicpal_lcd_state {
    SysBusDevice busdev;
    uint32_t brightness;
    uint32_t mode;
    uint32_t irqctrl;
    int page;
    int page_off;
    DisplayState *ds;
    uint8_t video_ram[128*64/8];
} musicpal_lcd_state;

static uint8_t scale_lcd_color(musicpal_lcd_state *s, uint8_t col)
{
    switch (s->brightness) {
    case 7:
        return col;
    case 0:
        return 0;
    default:
        return (col * s->brightness) / 7;
    }
}

#define SET_LCD_PIXEL(depth, type) \
static inline void glue(set_lcd_pixel, depth) \
        (musicpal_lcd_state *s, int x, int y, type col) \
{ \
    int dx, dy; \
    type *pixel = &((type *) ds_get_data(s->ds))[(y * 128 * 3 + x) * 3]; \
\
    for (dy = 0; dy < 3; dy++, pixel += 127 * 3) \
        for (dx = 0; dx < 3; dx++, pixel++) \
            *pixel = col; \
}
SET_LCD_PIXEL(8, uint8_t)
SET_LCD_PIXEL(16, uint16_t)
SET_LCD_PIXEL(32, uint32_t)

#include "pixel_ops.h"

static void lcd_refresh(void *opaque)
{
    musicpal_lcd_state *s = opaque;
    int x, y, col;

    switch (ds_get_bits_per_pixel(s->ds)) {
    case 0:
        return;
#define LCD_REFRESH(depth, func) \
    case depth: \
        col = func(scale_lcd_color(s, (MP_LCD_TEXTCOLOR >> 16) & 0xff), \
                   scale_lcd_color(s, (MP_LCD_TEXTCOLOR >> 8) & 0xff), \
                   scale_lcd_color(s, MP_LCD_TEXTCOLOR & 0xff)); \
        for (x = 0; x < 128; x++) \
            for (y = 0; y < 64; y++) \
                if (s->video_ram[x + (y/8)*128] & (1 << (y % 8))) \
                    glue(set_lcd_pixel, depth)(s, x, y, col); \
                else \
                    glue(set_lcd_pixel, depth)(s, x, y, 0); \
        break;
    LCD_REFRESH(8, rgb_to_pixel8)
    LCD_REFRESH(16, rgb_to_pixel16)
    LCD_REFRESH(32, (is_surface_bgr(s->ds->surface) ?
                     rgb_to_pixel32bgr : rgb_to_pixel32))
    default:
        hw_error("unsupported colour depth %i\n",
                  ds_get_bits_per_pixel(s->ds));
    }

    dpy_update(s->ds, 0, 0, 128*3, 64*3);
}

static void lcd_invalidate(void *opaque)
{
}

static void musicpal_lcd_gpio_brigthness_in(void *opaque, int irq, int level)
{
    musicpal_lcd_state *s = (musicpal_lcd_state *) opaque;
    s->brightness &= ~(1 << irq);
    s->brightness |= level << irq;
}

static uint32_t musicpal_lcd_read(void *opaque, target_phys_addr_t offset)
{
    musicpal_lcd_state *s = opaque;

    switch (offset) {
    case MP_LCD_IRQCTRL:
        return s->irqctrl;

    default:
        return 0;
    }
}

static void musicpal_lcd_write(void *opaque, target_phys_addr_t offset,
                               uint32_t value)
{
    musicpal_lcd_state *s = opaque;

    switch (offset) {
    case MP_LCD_IRQCTRL:
        s->irqctrl = value;
        break;

    case MP_LCD_SPICTRL:
        if (value == MP_LCD_SPI_DATA || value == MP_LCD_SPI_CMD)
            s->mode = value;
        else
            s->mode = MP_LCD_SPI_INVALID;
        break;

    case MP_LCD_INST:
        if (value >= MP_LCD_INST_SETPAGE0 && value <= MP_LCD_INST_SETPAGE7) {
            s->page = value - MP_LCD_INST_SETPAGE0;
            s->page_off = 0;
        }
        break;

    case MP_LCD_DATA:
        if (s->mode == MP_LCD_SPI_CMD) {
            if (value >= MP_LCD_INST_SETPAGE0 &&
                value <= MP_LCD_INST_SETPAGE7) {
                s->page = value - MP_LCD_INST_SETPAGE0;
                s->page_off = 0;
            }
        } else if (s->mode == MP_LCD_SPI_DATA) {
            s->video_ram[s->page*128 + s->page_off] = value;
            s->page_off = (s->page_off + 1) & 127;
        }
        break;
    }
}

static CPUReadMemoryFunc * const musicpal_lcd_readfn[] = {
    musicpal_lcd_read,
    musicpal_lcd_read,
    musicpal_lcd_read
};

static CPUWriteMemoryFunc * const musicpal_lcd_writefn[] = {
    musicpal_lcd_write,
    musicpal_lcd_write,
    musicpal_lcd_write
};

static void musicpal_lcd_init(SysBusDevice *dev)
{
    musicpal_lcd_state *s = FROM_SYSBUS(musicpal_lcd_state, dev);
    int iomemtype;

    s->brightness = 7;

    iomemtype = cpu_register_io_memory(musicpal_lcd_readfn,
                                       musicpal_lcd_writefn, s);
    sysbus_init_mmio(dev, MP_LCD_SIZE, iomemtype);

    s->ds = graphic_console_init(lcd_refresh, lcd_invalidate,
                                 NULL, NULL, s);
    qemu_console_resize(s->ds, 128*3, 64*3);

    qdev_init_gpio_in(&dev->qdev, musicpal_lcd_gpio_brigthness_in, 3);
}

/* PIC register offsets */
#define MP_PIC_STATUS           0x00
#define MP_PIC_ENABLE_SET       0x08
#define MP_PIC_ENABLE_CLR       0x0C

typedef struct mv88w8618_pic_state
{
    SysBusDevice busdev;
    uint32_t level;
    uint32_t enabled;
    qemu_irq parent_irq;
} mv88w8618_pic_state;

static void mv88w8618_pic_update(mv88w8618_pic_state *s)
{
    qemu_set_irq(s->parent_irq, (s->level & s->enabled));
}

static void mv88w8618_pic_set_irq(void *opaque, int irq, int level)
{
    mv88w8618_pic_state *s = opaque;

    if (level)
        s->level |= 1 << irq;
    else
        s->level &= ~(1 << irq);
    mv88w8618_pic_update(s);
}

static uint32_t mv88w8618_pic_read(void *opaque, target_phys_addr_t offset)
{
    mv88w8618_pic_state *s = opaque;

    switch (offset) {
    case MP_PIC_STATUS:
        return s->level & s->enabled;

    default:
        return 0;
    }
}

static void mv88w8618_pic_write(void *opaque, target_phys_addr_t offset,
                                uint32_t value)
{
    mv88w8618_pic_state *s = opaque;

    switch (offset) {
    case MP_PIC_ENABLE_SET:
        s->enabled |= value;
        break;

    case MP_PIC_ENABLE_CLR:
        s->enabled &= ~value;
        s->level &= ~value;
        break;
    }
    mv88w8618_pic_update(s);
}

static void mv88w8618_pic_reset(void *opaque)
{
    mv88w8618_pic_state *s = opaque;

    s->level = 0;
    s->enabled = 0;
}

static CPUReadMemoryFunc * const mv88w8618_pic_readfn[] = {
    mv88w8618_pic_read,
    mv88w8618_pic_read,
    mv88w8618_pic_read
};

static CPUWriteMemoryFunc * const mv88w8618_pic_writefn[] = {
    mv88w8618_pic_write,
    mv88w8618_pic_write,
    mv88w8618_pic_write
};

static void mv88w8618_pic_init(SysBusDevice *dev)
{
    mv88w8618_pic_state *s = FROM_SYSBUS(mv88w8618_pic_state, dev);
    int iomemtype;

    qdev_init_gpio_in(&dev->qdev, mv88w8618_pic_set_irq, 32);
    sysbus_init_irq(dev, &s->parent_irq);
    iomemtype = cpu_register_io_memory(mv88w8618_pic_readfn,
                                       mv88w8618_pic_writefn, s);
    sysbus_init_mmio(dev, MP_PIC_SIZE, iomemtype);

    qemu_register_reset(mv88w8618_pic_reset, s);
}

/* PIT register offsets */
#define MP_PIT_TIMER1_LENGTH    0x00
/* ... */
#define MP_PIT_TIMER4_LENGTH    0x0C
#define MP_PIT_CONTROL          0x10
#define MP_PIT_TIMER1_VALUE     0x14
/* ... */
#define MP_PIT_TIMER4_VALUE     0x20
#define MP_BOARD_RESET          0x34

/* Magic board reset value (probably some watchdog behind it) */
#define MP_BOARD_RESET_MAGIC    0x10000

typedef struct mv88w8618_timer_state {
    ptimer_state *ptimer;
    uint32_t limit;
    int freq;
    qemu_irq irq;
} mv88w8618_timer_state;

typedef struct mv88w8618_pit_state {
    SysBusDevice busdev;
    mv88w8618_timer_state timer[4];
    uint32_t control;
} mv88w8618_pit_state;

static void mv88w8618_timer_tick(void *opaque)
{
    mv88w8618_timer_state *s = opaque;

    qemu_irq_raise(s->irq);
}

static void mv88w8618_timer_init(SysBusDevice *dev, mv88w8618_timer_state *s,
                                 uint32_t freq)
{
    QEMUBH *bh;

    sysbus_init_irq(dev, &s->irq);
    s->freq = freq;

    bh = qemu_bh_new(mv88w8618_timer_tick, s);
    s->ptimer = ptimer_init(bh);
}

static uint32_t mv88w8618_pit_read(void *opaque, target_phys_addr_t offset)
{
    mv88w8618_pit_state *s = opaque;
    mv88w8618_timer_state *t;

    switch (offset) {
    case MP_PIT_TIMER1_VALUE ... MP_PIT_TIMER4_VALUE:
        t = &s->timer[(offset-MP_PIT_TIMER1_VALUE) >> 2];
        return ptimer_get_count(t->ptimer);

    default:
        return 0;
    }
}

static void mv88w8618_pit_write(void *opaque, target_phys_addr_t offset,
                                uint32_t value)
{
    mv88w8618_pit_state *s = opaque;
    mv88w8618_timer_state *t;
    int i;

    switch (offset) {
    case MP_PIT_TIMER1_LENGTH ... MP_PIT_TIMER4_LENGTH:
        t = &s->timer[offset >> 2];
        t->limit = value;
        ptimer_set_limit(t->ptimer, t->limit, 1);
        break;

    case MP_PIT_CONTROL:
        for (i = 0; i < 4; i++) {
            if (value & 0xf) {
                t = &s->timer[i];
                ptimer_set_limit(t->ptimer, t->limit, 0);
                ptimer_set_freq(t->ptimer, t->freq);
                ptimer_run(t->ptimer, 0);
            }
            value >>= 4;
        }
        break;

    case MP_BOARD_RESET:
        if (value == MP_BOARD_RESET_MAGIC)
            qemu_system_reset_request();
        break;
    }
}

static CPUReadMemoryFunc * const mv88w8618_pit_readfn[] = {
    mv88w8618_pit_read,
    mv88w8618_pit_read,
    mv88w8618_pit_read
};

static CPUWriteMemoryFunc * const mv88w8618_pit_writefn[] = {
    mv88w8618_pit_write,
    mv88w8618_pit_write,
    mv88w8618_pit_write
};

static void mv88w8618_pit_init(SysBusDevice *dev)
{
    int iomemtype;
    mv88w8618_pit_state *s = FROM_SYSBUS(mv88w8618_pit_state, dev);
    int i;

    /* Letting them all run at 1 MHz is likely just a pragmatic
     * simplification. */
    for (i = 0; i < 4; i++) {
        mv88w8618_timer_init(dev, &s->timer[i], 1000000);
    }

    iomemtype = cpu_register_io_memory(mv88w8618_pit_readfn,
                                       mv88w8618_pit_writefn, s);
    sysbus_init_mmio(dev, MP_PIT_SIZE, iomemtype);
}

/* Flash config register offsets */
#define MP_FLASHCFG_CFGR0    0x04

typedef struct mv88w8618_flashcfg_state {
    SysBusDevice busdev;
    uint32_t cfgr0;
} mv88w8618_flashcfg_state;

static uint32_t mv88w8618_flashcfg_read(void *opaque,
                                        target_phys_addr_t offset)
{
    mv88w8618_flashcfg_state *s = opaque;

    switch (offset) {
    case MP_FLASHCFG_CFGR0:
        return s->cfgr0;

    default:
        return 0;
    }
}

static void mv88w8618_flashcfg_write(void *opaque, target_phys_addr_t offset,
                                     uint32_t value)
{
    mv88w8618_flashcfg_state *s = opaque;

    switch (offset) {
    case MP_FLASHCFG_CFGR0:
        s->cfgr0 = value;
        break;
    }
}

static CPUReadMemoryFunc * const mv88w8618_flashcfg_readfn[] = {
    mv88w8618_flashcfg_read,
    mv88w8618_flashcfg_read,
    mv88w8618_flashcfg_read
};

static CPUWriteMemoryFunc * const mv88w8618_flashcfg_writefn[] = {
    mv88w8618_flashcfg_write,
    mv88w8618_flashcfg_write,
    mv88w8618_flashcfg_write
};

static void mv88w8618_flashcfg_init(SysBusDevice *dev)
{
    int iomemtype;
    mv88w8618_flashcfg_state *s = FROM_SYSBUS(mv88w8618_flashcfg_state, dev);

    s->cfgr0 = 0xfffe4285; /* Default as set by U-Boot for 8 MB flash */
    iomemtype = cpu_register_io_memory(mv88w8618_flashcfg_readfn,
                       mv88w8618_flashcfg_writefn, s);
    sysbus_init_mmio(dev, MP_FLASHCFG_SIZE, iomemtype);
}

/* Misc register offsets */
#define MP_MISC_BOARD_REVISION  0x18

#define MP_BOARD_REVISION       0x31

static uint32_t musicpal_misc_read(void *opaque, target_phys_addr_t offset)
{
    switch (offset) {
    case MP_MISC_BOARD_REVISION:
        return MP_BOARD_REVISION;

    default:
        return 0;
    }
}

static void musicpal_misc_write(void *opaque, target_phys_addr_t offset,
                                uint32_t value)
{
}

static CPUReadMemoryFunc * const musicpal_misc_readfn[] = {
    musicpal_misc_read,
    musicpal_misc_read,
    musicpal_misc_read,
};

static CPUWriteMemoryFunc * const musicpal_misc_writefn[] = {
    musicpal_misc_write,
    musicpal_misc_write,
    musicpal_misc_write,
};

static void musicpal_misc_init(void)
{
    int iomemtype;

    iomemtype = cpu_register_io_memory(musicpal_misc_readfn,
                                       musicpal_misc_writefn, NULL);
    cpu_register_physical_memory(MP_MISC_BASE, MP_MISC_SIZE, iomemtype);
}

/* WLAN register offsets */
#define MP_WLAN_MAGIC1          0x11c
#define MP_WLAN_MAGIC2          0x124

static uint32_t mv88w8618_wlan_read(void *opaque, target_phys_addr_t offset)
{
    switch (offset) {
    /* Workaround to allow loading the binary-only wlandrv.ko crap
     * from the original Freecom firmware. */
    case MP_WLAN_MAGIC1:
        return ~3;
    case MP_WLAN_MAGIC2:
        return -1;

    default:
        return 0;
    }
}

static void mv88w8618_wlan_write(void *opaque, target_phys_addr_t offset,
                                 uint32_t value)
{
}

static CPUReadMemoryFunc * const mv88w8618_wlan_readfn[] = {
    mv88w8618_wlan_read,
    mv88w8618_wlan_read,
    mv88w8618_wlan_read,
};

static CPUWriteMemoryFunc * const mv88w8618_wlan_writefn[] = {
    mv88w8618_wlan_write,
    mv88w8618_wlan_write,
    mv88w8618_wlan_write,
};

static void mv88w8618_wlan_init(SysBusDevice *dev)
{
    int iomemtype;

    iomemtype = cpu_register_io_memory(mv88w8618_wlan_readfn,
                                       mv88w8618_wlan_writefn, NULL);
    sysbus_init_mmio(dev, MP_WLAN_SIZE, iomemtype);
}

/* GPIO register offsets */
#define MP_GPIO_OE_LO           0x008
#define MP_GPIO_OUT_LO          0x00c
#define MP_GPIO_IN_LO           0x010
#define MP_GPIO_ISR_LO          0x020
#define MP_GPIO_OE_HI           0x508
#define MP_GPIO_OUT_HI          0x50c
#define MP_GPIO_IN_HI           0x510
#define MP_GPIO_ISR_HI          0x520

/* GPIO bits & masks */
#define MP_GPIO_LCD_BRIGHTNESS  0x00070000
#define MP_GPIO_I2C_DATA_BIT    29
#define MP_GPIO_I2C_DATA        (1 << MP_GPIO_I2C_DATA_BIT)
#define MP_GPIO_I2C_CLOCK_BIT   30

/* LCD brightness bits in GPIO_OE_HI */
#define MP_OE_LCD_BRIGHTNESS    0x0007

typedef struct musicpal_gpio_state {
    SysBusDevice busdev;
    uint32_t lcd_brightness;
    uint32_t out_state;
    uint32_t in_state;
    uint32_t isr;
    uint32_t i2c_read_data;
    uint32_t key_released;
    uint32_t keys_event;    /* store the received key event */
    qemu_irq irq;
    qemu_irq out[5];
} musicpal_gpio_state;

static void musicpal_gpio_brightness_update(musicpal_gpio_state *s) {
    int i;
    uint32_t brightness;

    /* compute brightness ratio */
    switch (s->lcd_brightness) {
    case 0x00000007:
        brightness = 0;
        break;

    case 0x00020000:
        brightness = 1;
        break;

    case 0x00020001:
        brightness = 2;
        break;

    case 0x00040000:
        brightness = 3;
        break;

    case 0x00010006:
        brightness = 4;
        break;

    case 0x00020005:
        brightness = 5;
        break;

    case 0x00040003:
        brightness = 6;
        break;

    case 0x00030004:
    default:
        brightness = 7;
    }

    /* set lcd brightness GPIOs  */
    for (i = 0; i <= 2; i++)
        qemu_set_irq(s->out[i], (brightness >> i) & 1);
}

static void musicpal_gpio_keys_update(musicpal_gpio_state *s)
{
        int gpio_mask = 0;

        /* transform the key state for GPIO usage */
        gpio_mask |= (s->keys_event & 15) << 8;
        gpio_mask |= ((s->keys_event >> 4) & 15) << 19;

        /* update GPIO state */
        if (s->key_released) {
            s->in_state |= gpio_mask;
        } else {
            s->in_state &= ~gpio_mask;
            s->isr = gpio_mask;
            qemu_irq_raise(s->irq);
        }
}

static void musicpal_gpio_irq(void *opaque, int irq, int level)
{
    musicpal_gpio_state *s = (musicpal_gpio_state *) opaque;

    if (irq == 10) {
        s->i2c_read_data = level;
    }

    /* receives keys bits */
    if (irq <= 7) {
        s->keys_event &= ~(1 << irq);
        s->keys_event |= level << irq;
        return;
    }

    /* receives key press/release */
    if (irq == 8) {
        s->key_released = level;
        return;
    }

    /* a key has been transmited */
    if (irq == 9 && level == 1)
        musicpal_gpio_keys_update(s);
}

static uint32_t musicpal_gpio_read(void *opaque, target_phys_addr_t offset)
{
    musicpal_gpio_state *s = (musicpal_gpio_state *) opaque;

    switch (offset) {
    case MP_GPIO_OE_HI: /* used for LCD brightness control */
        return s->lcd_brightness & MP_OE_LCD_BRIGHTNESS;

    case MP_GPIO_OUT_LO:
        return s->out_state & 0xFFFF;
    case MP_GPIO_OUT_HI:
        return s->out_state >> 16;

    case MP_GPIO_IN_LO:
        return s->in_state & 0xFFFF;
    case MP_GPIO_IN_HI:
        /* Update received I2C data */
        s->in_state = (s->in_state & ~MP_GPIO_I2C_DATA) |
                        (s->i2c_read_data << MP_GPIO_I2C_DATA_BIT);
        return s->in_state >> 16;

    case MP_GPIO_ISR_LO:
        return s->isr & 0xFFFF;
    case MP_GPIO_ISR_HI:
        return s->isr >> 16;

    default:
        return 0;
    }
}

static void musicpal_gpio_write(void *opaque, target_phys_addr_t offset,
                                uint32_t value)
{
    musicpal_gpio_state *s = (musicpal_gpio_state *) opaque;
    switch (offset) {
    case MP_GPIO_OE_HI: /* used for LCD brightness control */
        s->lcd_brightness = (s->lcd_brightness & MP_GPIO_LCD_BRIGHTNESS) |
                         (value & MP_OE_LCD_BRIGHTNESS);
        musicpal_gpio_brightness_update(s);
        break;

    case MP_GPIO_OUT_LO:
        s->out_state = (s->out_state & 0xFFFF0000) | (value & 0xFFFF);
        break;
    case MP_GPIO_OUT_HI:
        s->out_state = (s->out_state & 0xFFFF) | (value << 16);
        s->lcd_brightness = (s->lcd_brightness & 0xFFFF) |
                            (s->out_state & MP_GPIO_LCD_BRIGHTNESS);
        musicpal_gpio_brightness_update(s);
        qemu_set_irq(s->out[3], (s->out_state >> MP_GPIO_I2C_DATA_BIT) & 1);
        qemu_set_irq(s->out[4], (s->out_state >> MP_GPIO_I2C_CLOCK_BIT) & 1);
        break;

    }
}

static CPUReadMemoryFunc * const musicpal_gpio_readfn[] = {
    musicpal_gpio_read,
    musicpal_gpio_read,
    musicpal_gpio_read,
};

static CPUWriteMemoryFunc * const musicpal_gpio_writefn[] = {
    musicpal_gpio_write,
    musicpal_gpio_write,
    musicpal_gpio_write,
};

static void musicpal_gpio_reset(musicpal_gpio_state *s)
{
    s->in_state = 0xffffffff;
    s->i2c_read_data = 1;
    s->key_released = 0;
    s->keys_event = 0;
    s->isr = 0;
}

static void musicpal_gpio_init(SysBusDevice *dev)
{
    musicpal_gpio_state *s = FROM_SYSBUS(musicpal_gpio_state, dev);
    int iomemtype;

    sysbus_init_irq(dev, &s->irq);

    iomemtype = cpu_register_io_memory(musicpal_gpio_readfn,
                                       musicpal_gpio_writefn, s);
    sysbus_init_mmio(dev, MP_GPIO_SIZE, iomemtype);

    musicpal_gpio_reset(s);

    /* 3 brightness out + 2 lcd (data and clock ) */
    qdev_init_gpio_out(&dev->qdev, s->out, 5);
    /* 10 gpio button input + 1 I2C data input */
    qdev_init_gpio_in(&dev->qdev, musicpal_gpio_irq, 11);
}

/* Keyboard codes & masks */
#define KEY_RELEASED            0x80
#define KEY_CODE                0x7f

#define KEYCODE_TAB             0x0f
#define KEYCODE_ENTER           0x1c
#define KEYCODE_F               0x21
#define KEYCODE_M               0x32

#define KEYCODE_EXTENDED        0xe0
#define KEYCODE_UP              0x48
#define KEYCODE_DOWN            0x50
#define KEYCODE_LEFT            0x4b
#define KEYCODE_RIGHT           0x4d

#define MP_KEY_WHEEL_VOL       (1)
#define MP_KEY_WHEEL_VOL_INV   (1 << 1)
#define MP_KEY_WHEEL_NAV       (1 << 2)
#define MP_KEY_WHEEL_NAV_INV   (1 << 3)
#define MP_KEY_BTN_FAVORITS    (1 << 4)
#define MP_KEY_BTN_MENU        (1 << 5)
#define MP_KEY_BTN_VOLUME      (1 << 6)
#define MP_KEY_BTN_NAVIGATION  (1 << 7)

typedef struct musicpal_key_state {
    SysBusDevice busdev;
    uint32_t kbd_extended;
    uint32_t keys_state;
    qemu_irq out[10];
} musicpal_key_state;

static void musicpal_key_event(void *opaque, int keycode)
{
    musicpal_key_state *s = (musicpal_key_state *) opaque;
    uint32_t event = 0;
    int i;

    if (keycode == KEYCODE_EXTENDED) {
        s->kbd_extended = 1;
        return;
    }

    if (s->kbd_extended)
        switch (keycode & KEY_CODE) {
        case KEYCODE_UP:
            event = MP_KEY_WHEEL_NAV | MP_KEY_WHEEL_NAV_INV;
            break;

        case KEYCODE_DOWN:
            event = MP_KEY_WHEEL_NAV;
            break;

        case KEYCODE_LEFT:
            event = MP_KEY_WHEEL_VOL | MP_KEY_WHEEL_VOL_INV;
            break;

        case KEYCODE_RIGHT:
            event = MP_KEY_WHEEL_VOL;
            break;
        }
    else {
        switch (keycode & KEY_CODE) {
        case KEYCODE_F:
            event = MP_KEY_BTN_FAVORITS;
            break;

        case KEYCODE_TAB:
            event = MP_KEY_BTN_VOLUME;
            break;

        case KEYCODE_ENTER:
            event = MP_KEY_BTN_NAVIGATION;
            break;

        case KEYCODE_M:
            event = MP_KEY_BTN_MENU;
            break;
        }
        /* Do not repeat already pressed buttons */
        if (!(keycode & KEY_RELEASED) && !(s->keys_state & event))
            event = 0;
    }

    if (event) {

        /* transmit key event on GPIOS */
        for (i = 0; i <= 7; i++)
            qemu_set_irq(s->out[i], (event >> i) & 1);

        /* handle key press/release */
        if (keycode & KEY_RELEASED) {
            s->keys_state |= event;
            qemu_irq_raise(s->out[8]);
        } else {
            s->keys_state &= ~event;
            qemu_irq_lower(s->out[8]);
        }

        /* signal that a key event occured */
        qemu_irq_pulse(s->out[9]);
    }

    s->kbd_extended = 0;
}

static void musicpal_key_init(SysBusDevice *dev)
{
    musicpal_key_state *s = FROM_SYSBUS(musicpal_key_state, dev);

    sysbus_init_mmio(dev, 0x0, 0);

    s->kbd_extended = 0;
    s->keys_state = 0;

    /* 8 key event GPIO + 1 key press/release + 1 strobe */
    qdev_init_gpio_out(&dev->qdev, s->out, 10);

    qemu_add_kbd_event_handler(musicpal_key_event, s);
}

static struct arm_boot_info musicpal_binfo = {
    .loader_start = 0x0,
    .board_id = 0x20e,
};

static void musicpal_init(ram_addr_t ram_size,
               const char *boot_device,
               const char *kernel_filename, const char *kernel_cmdline,
               const char *initrd_filename, const char *cpu_model)
{
    CPUState *env;
    qemu_irq *cpu_pic;
    qemu_irq pic[32];
    DeviceState *dev;
    DeviceState *i2c_dev;
    DeviceState *lcd_dev;
    DeviceState *key_dev;
#ifdef HAS_AUDIO
    DeviceState *wm8750_dev;
    SysBusDevice *s;
#endif
    i2c_bus *i2c;
    int i;
    unsigned long flash_size;
    DriveInfo *dinfo;
    ram_addr_t sram_off;

    if (!cpu_model)
        cpu_model = "arm926";

    env = cpu_init(cpu_model);
    if (!env) {
        fprintf(stderr, "Unable to find CPU definition\n");
        exit(1);
    }
    cpu_pic = arm_pic_init_cpu(env);

    /* For now we use a fixed - the original - RAM size */
    cpu_register_physical_memory(0, MP_RAM_DEFAULT_SIZE,
                                 qemu_ram_alloc(MP_RAM_DEFAULT_SIZE));

    sram_off = qemu_ram_alloc(MP_SRAM_SIZE);
    cpu_register_physical_memory(MP_SRAM_BASE, MP_SRAM_SIZE, sram_off);

    dev = sysbus_create_simple("mv88w8618_pic", MP_PIC_BASE,
                               cpu_pic[ARM_PIC_CPU_IRQ]);
    for (i = 0; i < 32; i++) {
        pic[i] = qdev_get_gpio_in(dev, i);
    }
    sysbus_create_varargs("mv88w8618_pit", MP_PIT_BASE, pic[MP_TIMER1_IRQ],
                          pic[MP_TIMER2_IRQ], pic[MP_TIMER3_IRQ],
                          pic[MP_TIMER4_IRQ], NULL);

    if (serial_hds[0])
        serial_mm_init(MP_UART1_BASE, 2, pic[MP_UART1_IRQ], 1825000,
                   serial_hds[0], 1);
    if (serial_hds[1])
        serial_mm_init(MP_UART2_BASE, 2, pic[MP_UART2_IRQ], 1825000,
                   serial_hds[1], 1);

    /* Register flash */
    dinfo = drive_get(IF_PFLASH, 0, 0);
    if (dinfo) {
        flash_size = bdrv_getlength(dinfo->bdrv);
        if (flash_size != 8*1024*1024 && flash_size != 16*1024*1024 &&
            flash_size != 32*1024*1024) {
            fprintf(stderr, "Invalid flash image size\n");
            exit(1);
        }

        /*
         * The original U-Boot accesses the flash at 0xFE000000 instead of
         * 0xFF800000 (if there is 8 MB flash). So remap flash access if the
         * image is smaller than 32 MB.
         */
        pflash_cfi02_register(0-MP_FLASH_SIZE_MAX, qemu_ram_alloc(flash_size),
                              dinfo->bdrv, 0x10000,
                              (flash_size + 0xffff) >> 16,
                              MP_FLASH_SIZE_MAX / flash_size,
                              2, 0x00BF, 0x236D, 0x0000, 0x0000,
                              0x5555, 0x2AAA);
    }
    sysbus_create_simple("mv88w8618_flashcfg", MP_FLASHCFG_BASE, NULL);

    qemu_check_nic_model(&nd_table[0], "mv88w8618");
    dev = qdev_create(NULL, "mv88w8618_eth");
    dev->nd = &nd_table[0];
    qdev_init(dev);
    sysbus_mmio_map(sysbus_from_qdev(dev), 0, MP_ETH_BASE);
    sysbus_connect_irq(sysbus_from_qdev(dev), 0, pic[MP_ETH_IRQ]);

    sysbus_create_simple("mv88w8618_wlan", MP_WLAN_BASE, NULL);

    musicpal_misc_init();

    dev = sysbus_create_simple("musicpal_gpio", MP_GPIO_BASE, pic[MP_GPIO_IRQ]);
    i2c_dev = sysbus_create_simple("bitbang_i2c", 0, NULL);
    i2c = (i2c_bus *)qdev_get_child_bus(i2c_dev, "i2c");

    lcd_dev = sysbus_create_simple("musicpal_lcd", MP_LCD_BASE, NULL);
    key_dev = sysbus_create_simple("musicpal_key", 0, NULL);

    /* I2C read data */
    qdev_connect_gpio_out(i2c_dev, 0, qdev_get_gpio_in(dev, 10));
    /* I2C data */
    qdev_connect_gpio_out(dev, 3, qdev_get_gpio_in(i2c_dev, 0));
    /* I2C clock */
    qdev_connect_gpio_out(dev, 4, qdev_get_gpio_in(i2c_dev, 1));

    for (i = 0; i < 3; i++)
        qdev_connect_gpio_out(dev, i, qdev_get_gpio_in(lcd_dev, i));

    for (i = 0; i < 10; i++)
        qdev_connect_gpio_out(key_dev, i, qdev_get_gpio_in(dev, i));

#ifdef HAS_AUDIO
    wm8750_dev = i2c_create_slave(i2c, "wm8750", MP_WM_ADDR);
    dev = qdev_create(NULL, "mv88w8618_audio");
    s = sysbus_from_qdev(dev);
    qdev_prop_set_ptr(dev, "wm8750", wm8750_dev);
    qdev_init(dev);
    sysbus_mmio_map(s, 0, MP_AUDIO_BASE);
    sysbus_connect_irq(s, 0, pic[MP_AUDIO_IRQ]);
#endif

    musicpal_binfo.ram_size = MP_RAM_DEFAULT_SIZE;
    musicpal_binfo.kernel_filename = kernel_filename;
    musicpal_binfo.kernel_cmdline = kernel_cmdline;
    musicpal_binfo.initrd_filename = initrd_filename;
    arm_load_kernel(env, &musicpal_binfo);
}

static QEMUMachine musicpal_machine = {
    .name = "musicpal",
    .desc = "Marvell 88w8618 / MusicPal (ARM926EJ-S)",
    .init = musicpal_init,
};

static void musicpal_machine_init(void)
{
    qemu_register_machine(&musicpal_machine);
}

machine_init(musicpal_machine_init);

static void musicpal_register_devices(void)
{
    sysbus_register_dev("mv88w8618_pic", sizeof(mv88w8618_pic_state),
                        mv88w8618_pic_init);
    sysbus_register_dev("mv88w8618_pit", sizeof(mv88w8618_pit_state),
                        mv88w8618_pit_init);
    sysbus_register_dev("mv88w8618_flashcfg", sizeof(mv88w8618_flashcfg_state),
                        mv88w8618_flashcfg_init);
    sysbus_register_dev("mv88w8618_eth", sizeof(mv88w8618_eth_state),
                        mv88w8618_eth_init);
    sysbus_register_dev("mv88w8618_wlan", sizeof(SysBusDevice),
                        mv88w8618_wlan_init);
    sysbus_register_dev("musicpal_lcd", sizeof(musicpal_lcd_state),
                        musicpal_lcd_init);
    sysbus_register_dev("musicpal_gpio", sizeof(musicpal_gpio_state),
                        musicpal_gpio_init);
    sysbus_register_dev("musicpal_key", sizeof(musicpal_key_state),
                        musicpal_key_init);
}

device_init(musicpal_register_devices)
