/*
 * WinQEMU GPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL is available it will apply instead, WinQEMU elects to use only the 
 * General Public License version 3 (GPLv3) at this time for any software where a choice of 
 * GPL license versions is made available with the language indicating that GPLv3 or any later
 * version may be used, or where a choice of which version of the GPL is applied is otherwise unspecified.
 * 
 * Please contact Yan Wen (celestialwy@gmail.com) if you need additional information or have any questions.
 */
 
/* Common header file that is included by all of qemu.  */
#ifndef QEMU_COMMON_H
#define QEMU_COMMON_H

#include "config-host.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define WINVER 0x0501  /* needed for ipv6 bits */
#include <windows.h>
#endif

#ifndef _MSC_VER
#define QEMU_NORETURN __attribute__ ((__noreturn__))
#ifdef CONFIG_GCC_ATTRIBUTE_WARN_UNUSED_RESULT
#define QEMU_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#endif
#else
#define QEMU_NORETURN __declspec(noreturn)
#define QEMU_WARN_UNUSED_RESULT
#endif

#define QEMU_BUILD_BUG_ON(x) typedef char __build_bug_on__##__LINE__[(x)?-1:1];

typedef struct QEMUTimer QEMUTimer;
typedef struct QEMUFile QEMUFile;
typedef struct QEMUBH QEMUBH;

/* Hack around the mess dyngen-exec.h causes: We need QEMU_NORETURN in files that
   cannot include the following headers without conflicts. This condition has
   to be removed once dyngen is gone. */
#ifndef __DYNGEN_EXEC_H__

/* we put basic includes here to avoid repeating them in device drivers */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#ifndef ENOMEDIUM
#define ENOMEDIUM ENODEV
#endif
#if !defined(ENOTSUP)
#define ENOTSUP 4096
#endif

#ifndef CONFIG_IOVEC
#define CONFIG_IOVEC
struct iovec {
    void *iov_base;
    size_t iov_len;
};
/*
 * Use the same value as Linux for now.
 */
#define IOV_MAX		1024
#else
#include <sys/uio.h>
#endif

#ifdef _WIN32
#define fsync _commit
#define lseek _lseeki64
extern int qemu_ftruncate64(int, int64_t);
#define ftruncate qemu_ftruncate64

static inline char *realpath(const char *path, char *resolved_path)
{
    _fullpath(resolved_path, path, _MAX_PATH);
    return resolved_path;
}

#define PRId64 "I64d"
#define PRIx64 "I64x"
#define PRIu64 "I64u"
#define PRIo64 "I64o"
#endif

// Add posix supported commands native to windows aliased to standard posix format
#ifdef _MSC_VER
#define popen			_popen
#define pclose			_pclose
#define getpid			_getpid
#define getcwd			_getcwd
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#ifdef _MSC_VER
// this type isn't provided in MSVC 17/VS 2022.... 
typedef signed int ssize_t;

/*these are for block devices, and otherwise missing posix attributes / defines for posix properties
 * throwing these here enables us to more closely use upstream files instead of deviating every file
 * that requires them */
#define S_IWUSR 00200

#if !defined(S_ISDIR) && defined(S_IFMT) && defined(S_IFDIR)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

// more posixyness
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH

int gettimeofday(struct timeval* tp, struct timezone* tzp);
#endif

// setenv implementation to avoid upstream modification

int setenv(const char* name, const char* value, int overwrite);

#endif // end MSVC sections

/* FIXME: Remove NEED_CPU_H.  */
#ifndef NEED_CPU_H

#include <setjmp.h>
#include "osdep.h"
#include "bswap.h"

#else

#include "cpu.h"

#endif /* !defined(NEED_CPU_H) */

/* bottom halves */
typedef void QEMUBHFunc(void *opaque);

void async_context_push(void);
void async_context_pop(void);
int get_async_context_id(void);

QEMUBH *qemu_bh_new(QEMUBHFunc *cb, void *opaque);
void qemu_bh_schedule(QEMUBH *bh);
/* Bottom halfs that are scheduled from a bottom half handler are instantly
 * invoked.  This can create an infinite loop if a bottom half handler
 * schedules itself.  qemu_bh_schedule_idle() avoids this infinite loop by
 * ensuring that the bottom half isn't executed until the next main loop
 * iteration.
 */
void qemu_bh_schedule_idle(QEMUBH *bh);
void qemu_bh_cancel(QEMUBH *bh);
void qemu_bh_delete(QEMUBH *bh);
int qemu_bh_poll(void);
void qemu_bh_update_timeout(int *timeout);

uint64_t muldiv64(uint64_t a, uint32_t b, uint32_t c);

void qemu_get_timedate(struct tm *tm, int offset);
int qemu_timedate_diff(struct tm *tm);

/* cutils.c */
void pstrcpy(char *buf, int buf_size, const char *str);
char *pstrcat(char *buf, int buf_size, const char *s);
int strstart(const char *str, const char *val, const char **ptr);
int stristart(const char *str, const char *val, const char **ptr);
int qemu_strnlen(const char *s, int max_len);
time_t mktimegm(struct tm *tm);
int qemu_fls(int i);
int qemu_fdatasync(int fd);
int fcntl_setfl(int fd, int flag);

/* path.c */
void init_paths(const char *prefix);
const char *path(const char *pathname);

#define qemu_isalnum(c)		isalnum((unsigned char)(c))
#define qemu_isalpha(c)		isalpha((unsigned char)(c))
#define qemu_iscntrl(c)		iscntrl((unsigned char)(c))
#define qemu_isdigit(c)		isdigit((unsigned char)(c))
#define qemu_isgraph(c)		isgraph((unsigned char)(c))
#define qemu_islower(c)		islower((unsigned char)(c))
#define qemu_isprint(c)		isprint((unsigned char)(c))
#define qemu_ispunct(c)		ispunct((unsigned char)(c))
#define qemu_isspace(c)		isspace((unsigned char)(c))
#define qemu_isupper(c)		isupper((unsigned char)(c))
#define qemu_isxdigit(c)	isxdigit((unsigned char)(c))
#define qemu_tolower(c)		tolower((unsigned char)(c))
#define qemu_toupper(c)		toupper((unsigned char)(c))
#define qemu_isascii(c)		isascii((unsigned char)(c))
#define qemu_toascii(c)		toascii((unsigned char)(c))

void *qemu_malloc(size_t size);
void *qemu_realloc(void *ptr, size_t size);
void *qemu_mallocz(size_t size);
void qemu_free(void *ptr);
char *qemu_strdup(const char *str);
char *qemu_strndup(const char *str, size_t size);

void *get_mmap_addr(unsigned long size);


void qemu_mutex_lock_iothread(void);
void qemu_mutex_unlock_iothread(void);

int qemu_open(const char *name, int flags, ...);
ssize_t qemu_write_full(int fd, const void *buf, size_t count)
    QEMU_WARN_UNUSED_RESULT;
void qemu_set_cloexec(int fd);

#ifndef _WIN32
int qemu_eventfd(int pipefd[2]);
int qemu_pipe(int pipefd[2]);
#endif

/* Error handling.  */

#ifdef _MSC_VER
void QEMU_NORETURN hw_error(const char *fmt, ...);
#else
void QEMU_NORETURN hw_error(const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 1, 2)));
#endif

/* IO callbacks.  */
typedef void IOReadHandler(void *opaque, const uint8_t *buf, int size);
typedef int IOCanReadHandler(void *opaque);
typedef void IOHandler(void *opaque);

struct ParallelIOArg {
    void *buffer;
    int count;
};

typedef int (*DMA_transfer_handler) (void *opaque, int nchan, int pos, int size);

/* A load of opaque types so that device init declarations don't have to
   pull in all the real definitions.  */
typedef struct NICInfo NICInfo;
typedef struct HCIInfo HCIInfo;
typedef struct AudioState AudioState;
typedef struct BlockDriverState BlockDriverState;
typedef struct DisplayState DisplayState;
typedef struct DisplayChangeListener DisplayChangeListener;
typedef struct DisplaySurface DisplaySurface;
typedef struct DisplayAllocator DisplayAllocator;
typedef struct PixelFormat PixelFormat;
typedef struct TextConsole TextConsole;
typedef TextConsole QEMUConsole;
typedef struct CharDriverState CharDriverState;
typedef struct MACAddr MACAddr;
typedef struct VLANState VLANState;
typedef struct VLANClientState VLANClientState;
typedef struct i2c_bus i2c_bus;
typedef struct i2c_slave i2c_slave;
typedef struct SMBusDevice SMBusDevice;
typedef struct PCIHostState PCIHostState;
typedef struct PCIExpressHost PCIExpressHost;
typedef struct PCIBus PCIBus;
typedef struct PCIDevice PCIDevice;
typedef struct SerialState SerialState;
typedef struct IRQState *qemu_irq;
typedef struct PCMCIACardState PCMCIACardState;
typedef struct MouseTransformInfo MouseTransformInfo;
typedef struct uWireSlave uWireSlave;
typedef struct I2SCodec I2SCodec;
typedef struct DeviceState DeviceState;
typedef struct SSIBus SSIBus;
typedef struct EventNotifier EventNotifier;
typedef struct VirtIODevice VirtIODevice;

typedef uint64_t pcibus_t;

void cpu_exec_init_all(unsigned long tb_size);

/* CPU save/load.  */
void cpu_save(QEMUFile *f, void *opaque);
int cpu_load(QEMUFile *f, void *opaque, int version_id);

/* Force QEMU to stop what it's doing and service IO */
void qemu_service_io(void);

/* Force QEMU to process pending events */
void qemu_notify_event(void);

/* Unblock cpu */
void qemu_cpu_kick(void *env);
int qemu_cpu_self(void *env);

/* work queue */
struct qemu_work_item {
    struct qemu_work_item* next;
    void (*func)(void* data);
    void* data;
    int done;
};

#ifdef CONFIG_USER_ONLY
#define qemu_init_vcpu(env) do { } while (0)
#else
void qemu_init_vcpu(void *env);
#endif

typedef struct QEMUIOVector {
    struct iovec *iov;
    int niov;
    int nalloc;
    size_t size;
} QEMUIOVector;

void qemu_iovec_init(QEMUIOVector *qiov, int alloc_hint);
void qemu_iovec_init_external(QEMUIOVector *qiov, struct iovec *iov, int niov);
void qemu_iovec_add(QEMUIOVector *qiov, void *base, size_t len);
void qemu_iovec_concat(QEMUIOVector *dst, QEMUIOVector *src, size_t size);
void qemu_iovec_destroy(QEMUIOVector *qiov);
void qemu_iovec_reset(QEMUIOVector *qiov);
void qemu_iovec_to_buffer(QEMUIOVector *qiov, void *buf);
void qemu_iovec_from_buffer(QEMUIOVector *qiov, const void *buf, size_t count);

struct Monitor;
typedef struct Monitor Monitor;

/* Convert a byte between binary and BCD.  */
static inline uint8_t to_bcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

static inline uint8_t from_bcd(uint8_t val)
{
    return ((val >> 4) * 10) + (val & 0x0f);
}

#include "module.h"

#endif /* dyngen-exec.h hack */

#endif
