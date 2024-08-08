#ifndef SYSEMU_H
#define SYSEMU_H
/* Misc. things related to the system emulator.  */

#include "qemu-common.h"
#include "qemu-option.h"
#include "qemu-queue.h"
#include "qemu-timer.h"
#include "qdict.h"

#ifdef _WIN32
#include <windows.h>
#endif

/* vl.c */
extern const char *bios_name;

#define QEMU_FILE_TYPE_BIOS   0
#define QEMU_FILE_TYPE_KEYMAP 1
char *qemu_find_file(int type, const char *name);

extern int vm_running;
extern const char *qemu_name;
extern uint8_t qemu_uuid[];
int qemu_uuid_parse(const char *str, uint8_t *uuid);
#define UUID_FMT "%02hhx%02hhx%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"

typedef struct vm_change_state_entry VMChangeStateEntry;
typedef void VMChangeStateHandler(void *opaque, int running, int reason);

VMChangeStateEntry *qemu_add_vm_change_state_handler(VMChangeStateHandler *cb,
                                                     void *opaque);
void qemu_del_vm_change_state_handler(VMChangeStateEntry *e);

void vm_start(void);
void vm_stop(int reason);

uint64_t ram_bytes_remaining(void);
uint64_t ram_bytes_transferred(void);
uint64_t ram_bytes_total(void);

int64_t cpu_get_ticks(void);
void cpu_enable_ticks(void);
void cpu_disable_ticks(void);

void qemu_system_reset_request(void);
void qemu_system_shutdown_request(void);
void qemu_system_powerdown_request(void);
int qemu_shutdown_requested(void);
int qemu_reset_requested(void);
int qemu_powerdown_requested(void);
extern qemu_irq qemu_system_powerdown;
void qemu_system_reset(void);

void do_savevm(Monitor *mon, const QDict *qdict);
int load_vmstate(Monitor *mon, const char *name);
void do_delvm(Monitor *mon, const QDict *qdict);
void do_info_snapshots(Monitor *mon);

void qemu_announce_self(void);

void main_loop_wait(int timeout);

int qemu_savevm_state_begin(QEMUFile *f);
int qemu_savevm_state_iterate(QEMUFile *f);
int qemu_savevm_state_complete(QEMUFile *f);
int qemu_savevm_state(QEMUFile *f);
int qemu_loadvm_state(QEMUFile *f);

void qemu_errors_to_file(FILE *fp);
void qemu_errors_to_mon(Monitor *mon);
void qemu_errors_to_previous(void);
#ifndef _MSC_VER
void qemu_error(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
#else
void qemu_error(_Printf_format_string_ const char* fmt, ...);
#endif
#ifdef _WIN32
/* Polling handling */

/* return TRUE if no sleep should be done afterwards */
typedef int PollingFunc(void *opaque);

int qemu_add_polling_cb(PollingFunc *func, void *opaque);
void qemu_del_polling_cb(PollingFunc *func, void *opaque);

/* Wait objects handling */
typedef void WaitObjectFunc(void *opaque);

int qemu_add_wait_object(HANDLE handle, WaitObjectFunc *func, void *opaque);
void qemu_del_wait_object(HANDLE handle, WaitObjectFunc *func, void *opaque);
#endif

/* TAP win32 */
int tap_win32_init(VLANState *vlan, const char *model,
                   const char *name, const char *ifname);

/* SLIRP */
void do_info_slirp(Monitor *mon);

typedef enum DisplayType
{
    DT_DEFAULT,
    DT_CURSES,
    DT_SDL,
    DT_VNC,
    DT_NOGRAPHIC,
} DisplayType;

extern int autostart;
extern int bios_size;

typedef enum {
    VGA_NONE, VGA_STD, VGA_CIRRUS, VGA_VMWARE, VGA_XENFB
} VGAInterfaceType;

extern int vga_interface_type;
#define cirrus_vga_enabled (vga_interface_type == VGA_CIRRUS)
#define std_vga_enabled (vga_interface_type == VGA_STD)
#define xenfb_enabled (vga_interface_type == VGA_XENFB)
#define vmsvga_enabled (vga_interface_type == VGA_VMWARE)

extern int graphic_width;
extern int graphic_height;
extern int graphic_depth;
extern uint8_t irq0override;
extern DisplayType display_type;
extern const char *keyboard_layout;
extern int win2k_install_hack;
extern int rtc_td_hack;
extern int alt_grab;
extern int usb_enabled;
extern int smp_cpus;
extern int max_cpus;
extern int cursor_hide;
extern int graphic_rotate;
extern int no_quit;
extern int semihosting_enabled;
extern int old_param;
extern int boot_menu;
extern QEMUClock *rtc_clock;

#define MAX_NODES 64
extern int nb_numa_nodes;
extern uint64_t node_mem[MAX_NODES];
extern uint64_t node_cpumask[MAX_NODES];

#define MAX_OPTION_ROMS 16
extern const char *option_rom[MAX_OPTION_ROMS];
extern int nb_option_roms;

#ifdef NEED_CPU_H
#if defined(TARGET_SPARC) || defined(TARGET_PPC)
#define MAX_PROM_ENVS 128
extern const char *prom_envs[MAX_PROM_ENVS];
extern unsigned int nb_prom_envs;
#endif
#endif

typedef enum {
    IF_NONE,
    IF_IDE, IF_SCSI, IF_FLOPPY, IF_PFLASH, IF_MTD, IF_SD, IF_VIRTIO, IF_XEN,
    IF_COUNT
} BlockInterfaceType;

typedef enum {
    BLOCK_ERR_REPORT, BLOCK_ERR_IGNORE, BLOCK_ERR_STOP_ENOSPC,
    BLOCK_ERR_STOP_ANY
} BlockInterfaceErrorAction;

#define BLOCK_SERIAL_STRLEN 20

typedef struct DriveInfo {
    BlockDriverState *bdrv;
    char *id;
    const char *devaddr;
    BlockInterfaceType type;
    int bus;
    int unit;
    QemuOpts *opts;
    BlockInterfaceErrorAction onerror;
    char serial[BLOCK_SERIAL_STRLEN + 1];
    QTAILQ_ENTRY(DriveInfo) next;
} DriveInfo;

#define MAX_IDE_DEVS	2
#define MAX_SCSI_DEVS	7
#define MAX_DRIVES 32

extern QTAILQ_HEAD(drivelist, DriveInfo) drives;
extern QTAILQ_HEAD(driveoptlist, DriveOpt) driveopts;

extern DriveInfo *drive_get(BlockInterfaceType type, int bus, int unit);
extern DriveInfo *drive_get_by_id(const char *id);
extern int drive_get_max_bus(BlockInterfaceType type);
extern void drive_uninit(DriveInfo *dinfo);
extern const char *drive_get_serial(BlockDriverState *bdrv);
extern BlockInterfaceErrorAction drive_get_onerror(BlockDriverState *bdrv);

BlockDriverState *qdev_init_bdrv(DeviceState *dev, BlockInterfaceType type);

extern QemuOpts *drive_add(const char *file, const char *fmt, ...);
extern DriveInfo *drive_init(QemuOpts *arg, void *machine, int *fatal_error);

/* device-hotplug */

typedef int (dev_match_fn)(void *dev_private, void *arg);

DriveInfo *add_init_drive(const char *opts);
void destroy_nic(dev_match_fn *match_fn, void *arg);

/* pci-hotplug */
void pci_device_hot_add(Monitor *mon, const QDict *qdict);
void drive_hot_add(Monitor *mon, const QDict *qdict);
void pci_device_hot_remove(Monitor *mon, const char *pci_addr);
void do_pci_device_hot_remove(Monitor *mon, const QDict *qdict);
void pci_device_hot_remove_success(PCIDevice *dev);

/* serial ports */

#define MAX_SERIAL_PORTS 4

extern CharDriverState *serial_hds[MAX_SERIAL_PORTS];

/* parallel ports */

#define MAX_PARALLEL_PORTS 3

extern CharDriverState *parallel_hds[MAX_PARALLEL_PORTS];

/* virtio consoles */

#define MAX_VIRTIO_CONSOLES 1

extern CharDriverState *virtcon_hds[MAX_VIRTIO_CONSOLES];

#define TFR(expr) do { if ((expr) != -1) break; } while (errno == EINTR)

#ifdef HAS_AUDIO
struct soundhw {
    const char *name;
    const char *descr;
    int enabled;
    int isa;
    union {
        int (*init_isa) (qemu_irq *pic);
        int (*init_pci) (PCIBus *bus);
    } init;
};

extern struct soundhw soundhw[];
#endif

void do_usb_add(Monitor *mon, const QDict *qdict);
void do_usb_del(Monitor *mon, const QDict *qdict);
void usb_info(Monitor *mon);

void register_devices(void);

#endif
