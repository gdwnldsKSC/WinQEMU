/*
 * QEMU System Emulator
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
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
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <zlib.h>

/* Needed early for CONFIG_BSD etc. */
#include "config-host.h"

#ifndef _WIN32
#include <sys/times.h>
#include <sys/wait.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#if defined(__NetBSD__)
#include <net/if_tap.h>
#endif
#ifdef __linux__
#include <linux/if_tun.h>
#endif
#include <arpa/inet.h>
#include <dirent.h>
#include <netdb.h>
#include <sys/select.h>
#ifdef CONFIG_BSD
#include <sys/stat.h>
#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <libutil.h>
#else
#include <util.h>
#endif
#elif defined (__GLIBC__) && defined (__FreeBSD_kernel__)
#include <freebsd/stdlib.h>
#else
#ifdef __linux__
#include <pty.h>
#include <malloc.h>
#include <linux/rtc.h>
#endif
#endif
#endif

#ifdef _WIN32
#include <WinSock2.h>
#include <windows.h>
#include <malloc.h>
#include <sys/timeb.h>
#include <mmsystem.h>
#define getopt_long_only getopt_long
#define memalign(align, size) malloc(size)
#endif

#include "qemu-common.h"
#include "hw/hw.h"
#include "net.h"
#include "monitor.h"
#include "sysemu.h"
#include "qemu-timer.h"
#include "qemu-char.h"
#include "block.h"
#include "audio/audio.h"
#include "migration.h"
#include "qemu_socket.h"

/* point to the block driver where the snapshots are managed */
static BlockDriverState *bs_snapshots;

#define SELF_ANNOUNCE_ROUNDS 5
#define ETH_P_EXPERIMENTAL 0x01F1 /* just a number */
//#define ETH_P_EXPERIMENTAL 0x0012 /* make it the size of the packet */
#define EXPERIMENTAL_MAGIC 0xf1f23f4f

static int announce_self_create(uint8_t *buf, 
				uint8_t *mac_addr)
{
    uint32_t magic = EXPERIMENTAL_MAGIC;
    uint16_t proto = htons(ETH_P_EXPERIMENTAL);

    /* FIXME: should we send a different packet (arp/rarp/ping)? */

    memset(buf, 0, 64);
    memset(buf, 0xff, 6);         /* h_dst */
    memcpy(buf + 6, mac_addr, 6); /* h_src */
    memcpy(buf + 12, &proto, 2);  /* h_proto */
    memcpy(buf + 14, &magic, 4);  /* magic */

    return 64; /* len */
}

static void qemu_announce_self_once(void *opaque)
{
    int i, len;
    VLANState *vlan;
    VLANClientState *vc;
    uint8_t buf[256];
    static int count = SELF_ANNOUNCE_ROUNDS;
    QEMUTimer *timer = *(QEMUTimer **)opaque;

    for (i = 0; i < MAX_NICS; i++) {
        if (!nd_table[i].used)
            continue;
        len = announce_self_create(buf, nd_table[i].macaddr);
        vlan = nd_table[i].vlan;
	for(vc = vlan->first_client; vc != NULL; vc = vc->next) {
            vc->receive(vc, buf, len);
        }
    }
    if (count--) {
	    qemu_mod_timer(timer, qemu_get_clock(rt_clock) + 100);
    } else {
	    qemu_del_timer(timer);
	    qemu_free_timer(timer);
    }
}

void qemu_announce_self(void)
{
	static QEMUTimer *timer;
	timer = qemu_new_timer(rt_clock, qemu_announce_self_once, &timer);
	qemu_announce_self_once(&timer);
}

/***********************************************************/
/* savevm/loadvm support */

#define IO_BUF_SIZE 32768

struct QEMUFile {
    QEMUFilePutBufferFunc *put_buffer;
    QEMUFileGetBufferFunc *get_buffer;
    QEMUFileCloseFunc *close;
    QEMUFileRateLimit *rate_limit;
    QEMUFileSetRateLimit *set_rate_limit;
    void *opaque;
    int is_write;

    int64_t buf_offset; /* start of buffer when writing, end of buffer
                           when reading */
    int buf_index;
    int buf_size; /* 0 when writing */
    uint8_t buf[IO_BUF_SIZE];

    int has_error;
};

typedef struct QEMUFileStdio
{
    FILE *stdio_file;
    QEMUFile *file;
} QEMUFileStdio;

typedef struct QEMUFileSocket
{
    int fd;
    QEMUFile *file;
} QEMUFileSocket;

static int socket_get_buffer(void *opaque, uint8_t *buf, int64_t pos, int size)
{
    QEMUFileSocket *s = opaque;
    ssize_t len;

    do {
        len = recv(s->fd, (void *)buf, size, 0);
    } while (len == -1 && socket_error() == EINTR);

    if (len == -1)
        len = -socket_error();

    return len;
}

static int socket_close(void *opaque)
{
    QEMUFileSocket *s = opaque;
    qemu_free(s);
    return 0;
}

static int stdio_put_buffer(void *opaque, const uint8_t *buf, int64_t pos, int size)
{
    QEMUFileStdio *s = opaque;
    return fwrite(buf, 1, size, s->stdio_file);
}

static int stdio_get_buffer(void *opaque, uint8_t *buf, int64_t pos, int size)
{
    QEMUFileStdio *s = opaque;
    FILE *fp = s->stdio_file;
    int bytes;

    do {
        clearerr(fp);
        bytes = fread(buf, 1, size, fp);
    } while ((bytes == 0) && ferror(fp) && (errno == EINTR));
    return bytes;
}

static int stdio_pclose(void *opaque)
{
    QEMUFileStdio *s = opaque;
    pclose(s->stdio_file);
    qemu_free(s);
    return 0;
}

static int stdio_fclose(void *opaque)
{
    QEMUFileStdio *s = opaque;
    fclose(s->stdio_file);
    qemu_free(s);
    return 0;
}

QEMUFile *qemu_popen(FILE *stdio_file, const char *mode)
{
    QEMUFileStdio *s;

    if (stdio_file == NULL || mode == NULL || (mode[0] != 'r' && mode[0] != 'w') || mode[1] != 0) {
        fprintf(stderr, "qemu_popen: Argument validity check failed\n");
        return NULL;
    }

    s = qemu_mallocz(sizeof(QEMUFileStdio));

    s->stdio_file = stdio_file;

    if(mode[0] == 'r') {
        s->file = qemu_fopen_ops(s, NULL, stdio_get_buffer, stdio_pclose, NULL, NULL);
    } else {
        s->file = qemu_fopen_ops(s, stdio_put_buffer, NULL, stdio_pclose, NULL, NULL);
    }
    return s->file;
}

QEMUFile *qemu_popen_cmd(const char *command, const char *mode)
{
    FILE *popen_file;

    popen_file = popen(command, mode);
    if(popen_file == NULL) {
        return NULL;
    }

    return qemu_popen(popen_file, mode);
}

int qemu_stdio_fd(QEMUFile *f)
{
    QEMUFileStdio *p;
    int fd;

    p = (QEMUFileStdio *)f->opaque;
    fd = fileno(p->stdio_file);

    return fd;
}

QEMUFile *qemu_fdopen(int fd, const char *mode)
{
    QEMUFileStdio *s;

    if (mode == NULL ||
	(mode[0] != 'r' && mode[0] != 'w') ||
	mode[1] != 'b' || mode[2] != 0) {
        fprintf(stderr, "qemu_fdopen: Argument validity check failed\n");
        return NULL;
    }

    s = qemu_mallocz(sizeof(QEMUFileStdio));
    s->stdio_file = fdopen(fd, mode);
    if (!s->stdio_file)
        goto fail;

    if(mode[0] == 'r') {
        s->file = qemu_fopen_ops(s, NULL, stdio_get_buffer, stdio_fclose, NULL, NULL);
    } else {
        s->file = qemu_fopen_ops(s, stdio_put_buffer, NULL, stdio_fclose, NULL, NULL);
    }
    return s->file;

fail:
    qemu_free(s);
    return NULL;
}

QEMUFile *qemu_fopen_socket(int fd)
{
    QEMUFileSocket *s = qemu_mallocz(sizeof(QEMUFileSocket));

    s->fd = fd;
    s->file = qemu_fopen_ops(s, NULL, socket_get_buffer, socket_close, NULL, NULL);
    return s->file;
}

static int file_put_buffer(void *opaque, const uint8_t *buf,
                            int64_t pos, int size)
{
    QEMUFileStdio *s = opaque;
    fseek(s->stdio_file, pos, SEEK_SET);
    fwrite(buf, 1, size, s->stdio_file);
    return size;
}

static int file_get_buffer(void *opaque, uint8_t *buf, int64_t pos, int size)
{
    QEMUFileStdio *s = opaque;
    fseek(s->stdio_file, pos, SEEK_SET);
    return fread(buf, 1, size, s->stdio_file);
}

QEMUFile *qemu_fopen(const char *filename, const char *mode)
{
    QEMUFileStdio *s;

    if (mode == NULL ||
	(mode[0] != 'r' && mode[0] != 'w') ||
	mode[1] != 'b' || mode[2] != 0) {
        fprintf(stderr, "qemu_fdopen: Argument validity check failed\n");
        return NULL;
    }

    s = qemu_mallocz(sizeof(QEMUFileStdio));

    s->stdio_file = fopen(filename, mode);
    if (!s->stdio_file)
        goto fail;

    if(mode[0] == 'w') {
        s->file = qemu_fopen_ops(s, file_put_buffer, NULL, stdio_fclose, NULL, NULL);
    } else {
        s->file = qemu_fopen_ops(s, NULL, file_get_buffer, stdio_fclose, NULL, NULL);
    }
    return s->file;
fail:
    qemu_free(s);
    return NULL;
}

static int block_put_buffer(void *opaque, const uint8_t *buf,
                           int64_t pos, int size)
{
    bdrv_save_vmstate(opaque, buf, pos, size);
    return size;
}

static int block_get_buffer(void *opaque, uint8_t *buf, int64_t pos, int size)
{
    return bdrv_load_vmstate(opaque, buf, pos, size);
}

static int bdrv_fclose(void *opaque)
{
    return 0;
}

static QEMUFile *qemu_fopen_bdrv(BlockDriverState *bs, int is_writable)
{
    if (is_writable)
        return qemu_fopen_ops(bs, block_put_buffer, NULL, bdrv_fclose, NULL, NULL);
    return qemu_fopen_ops(bs, NULL, block_get_buffer, bdrv_fclose, NULL, NULL);
}

QEMUFile *qemu_fopen_ops(void *opaque, QEMUFilePutBufferFunc *put_buffer,
                         QEMUFileGetBufferFunc *get_buffer,
                         QEMUFileCloseFunc *close,
                         QEMUFileRateLimit *rate_limit,
                         QEMUFileSetRateLimit *set_rate_limit)
{
    QEMUFile *f;

    f = qemu_mallocz(sizeof(QEMUFile));

    f->opaque = opaque;
    f->put_buffer = put_buffer;
    f->get_buffer = get_buffer;
    f->close = close;
    f->rate_limit = rate_limit;
    f->set_rate_limit = set_rate_limit;
    f->is_write = 0;

    return f;
}

int qemu_file_has_error(QEMUFile *f)
{
    return f->has_error;
}

void qemu_file_set_error(QEMUFile *f)
{
    f->has_error = 1;
}

void qemu_fflush(QEMUFile *f)
{
    if (!f->put_buffer)
        return;

    if (f->is_write && f->buf_index > 0) {
        int len;

        len = f->put_buffer(f->opaque, f->buf, f->buf_offset, f->buf_index);
        if (len > 0)
            f->buf_offset += f->buf_index;
        else
            f->has_error = 1;
        f->buf_index = 0;
    }
}

static void qemu_fill_buffer(QEMUFile *f)
{
    int len;

    if (!f->get_buffer)
        return;

    if (f->is_write)
        abort();

    len = f->get_buffer(f->opaque, f->buf, f->buf_offset, IO_BUF_SIZE);
    if (len > 0) {
        f->buf_index = 0;
        f->buf_size = len;
        f->buf_offset += len;
    } else if (len != -EAGAIN)
        f->has_error = 1;
}

int qemu_fclose(QEMUFile *f)
{
    int ret = 0;
    qemu_fflush(f);
    if (f->close)
        ret = f->close(f->opaque);
    qemu_free(f);
    return ret;
}

void qemu_file_put_notify(QEMUFile *f)
{
    f->put_buffer(f->opaque, NULL, 0, 0);
}

void qemu_put_buffer(QEMUFile *f, const uint8_t *buf, int size)
{
    int l;

    if (!f->has_error && f->is_write == 0 && f->buf_index > 0) {
        fprintf(stderr,
                "Attempted to write to buffer while read buffer is not empty\n");
        abort();
    }

    while (!f->has_error && size > 0) {
        l = IO_BUF_SIZE - f->buf_index;
        if (l > size)
            l = size;
        memcpy(f->buf + f->buf_index, buf, l);
        f->is_write = 1;
        f->buf_index += l;
        buf += l;
        size -= l;
        if (f->buf_index >= IO_BUF_SIZE)
            qemu_fflush(f);
    }
}

void qemu_put_byte(QEMUFile *f, int v)
{
    if (!f->has_error && f->is_write == 0 && f->buf_index > 0) {
        fprintf(stderr,
                "Attempted to write to buffer while read buffer is not empty\n");
        abort();
    }

    f->buf[f->buf_index++] = v;
    f->is_write = 1;
    if (f->buf_index >= IO_BUF_SIZE)
        qemu_fflush(f);
}

int qemu_get_buffer(QEMUFile *f, uint8_t *buf, int size1)
{
    int size, l;

    if (f->is_write)
        abort();

    size = size1;
    while (size > 0) {
        l = f->buf_size - f->buf_index;
        if (l == 0) {
            qemu_fill_buffer(f);
            l = f->buf_size - f->buf_index;
            if (l == 0)
                break;
        }
        if (l > size)
            l = size;
        memcpy(buf, f->buf + f->buf_index, l);
        f->buf_index += l;
        buf += l;
        size -= l;
    }
    return size1 - size;
}

int qemu_get_byte(QEMUFile *f)
{
    if (f->is_write)
        abort();

    if (f->buf_index >= f->buf_size) {
        qemu_fill_buffer(f);
        if (f->buf_index >= f->buf_size)
            return 0;
    }
    return f->buf[f->buf_index++];
}

int64_t qemu_ftell(QEMUFile *f)
{
    return f->buf_offset - f->buf_size + f->buf_index;
}

int64_t qemu_fseek(QEMUFile *f, int64_t pos, int whence)
{
    if (whence == SEEK_SET) {
        /* nothing to do */
    } else if (whence == SEEK_CUR) {
        pos += qemu_ftell(f);
    } else {
        /* SEEK_END not supported */
        return -1;
    }
    if (f->put_buffer) {
        qemu_fflush(f);
        f->buf_offset = pos;
    } else {
        f->buf_offset = pos;
        f->buf_index = 0;
        f->buf_size = 0;
    }
    return pos;
}

int qemu_file_rate_limit(QEMUFile *f)
{
    if (f->rate_limit)
        return f->rate_limit(f->opaque);

    return 0;
}

size_t qemu_file_set_rate_limit(QEMUFile *f, size_t new_rate)
{
    /* any failed or completed migration keeps its state to allow probing of
     * migration data, but has no associated file anymore */
    if (f && f->set_rate_limit)
        return f->set_rate_limit(f->opaque, new_rate);

    return 0;
}

void qemu_put_be16(QEMUFile *f, unsigned int v)
{
    qemu_put_byte(f, v >> 8);
    qemu_put_byte(f, v);
}

void qemu_put_be32(QEMUFile *f, unsigned int v)
{
    qemu_put_byte(f, v >> 24);
    qemu_put_byte(f, v >> 16);
    qemu_put_byte(f, v >> 8);
    qemu_put_byte(f, v);
}

void qemu_put_be64(QEMUFile *f, uint64_t v)
{
    qemu_put_be32(f, v >> 32);
    qemu_put_be32(f, v);
}

unsigned int qemu_get_be16(QEMUFile *f)
{
    unsigned int v;
    v = qemu_get_byte(f) << 8;
    v |= qemu_get_byte(f);
    return v;
}

unsigned int qemu_get_be32(QEMUFile *f)
{
    unsigned int v;
    v = qemu_get_byte(f) << 24;
    v |= qemu_get_byte(f) << 16;
    v |= qemu_get_byte(f) << 8;
    v |= qemu_get_byte(f);
    return v;
}

uint64_t qemu_get_be64(QEMUFile *f)
{
    uint64_t v;
    v = (uint64_t)qemu_get_be32(f) << 32;
    v |= qemu_get_be32(f);
    return v;
}

/* 8 bit int */

static int get_int8(QEMUFile *f, void *pv, size_t size)
{
    int8_t *v = pv;
    qemu_get_s8s(f, v);
    return 0;
}

static void put_int8(QEMUFile *f, const void *pv, size_t size)
{
    const int8_t *v = pv;
    qemu_put_s8s(f, v);
}

const VMStateInfo vmstate_info_int8 = {
    .name = "int8",
    .get  = get_int8,
    .put  = put_int8,
};

/* 16 bit int */

static int get_int16(QEMUFile *f, void *pv, size_t size)
{
    int16_t *v = pv;
    qemu_get_sbe16s(f, v);
    return 0;
}

static void put_int16(QEMUFile *f, const void *pv, size_t size)
{
    const int16_t *v = pv;
    qemu_put_sbe16s(f, v);
}

const VMStateInfo vmstate_info_int16 = {
    .name = "int16",
    .get  = get_int16,
    .put  = put_int16,
};

/* 32 bit int */

static int get_int32(QEMUFile *f, void *pv, size_t size)
{
    int32_t *v = pv;
    qemu_get_sbe32s(f, v);
    return 0;
}

static void put_int32(QEMUFile *f, const void *pv, size_t size)
{
    const int32_t *v = pv;
    qemu_put_sbe32s(f, v);
}

const VMStateInfo vmstate_info_int32 = {
    .name = "int32",
    .get  = get_int32,
    .put  = put_int32,
};

/* 32 bit int. See that the received value is the same than the one
   in the field */

static int get_int32_equal(QEMUFile *f, void *pv, size_t size)
{
    int32_t *v = pv;
    int32_t v2;
    qemu_get_sbe32s(f, &v2);

    if (*v == v2)
        return 0;
    return -EINVAL;
}

const VMStateInfo vmstate_info_int32_equal = {
    .name = "int32 equal",
    .get  = get_int32_equal,
    .put  = put_int32,
};

/* 32 bit int. See that the received value is the less or the same
   than the one in the field */

static int get_int32_le(QEMUFile *f, void *pv, size_t size)
{
    int32_t *old = pv;
    int32_t new;
    qemu_get_sbe32s(f, &new);

    if (*old <= new)
        return 0;
    return -EINVAL;
}

const VMStateInfo vmstate_info_int32_le = {
    .name = "int32 equal",
    .get  = get_int32_le,
    .put  = put_int32,
};

/* 64 bit int */

static int get_int64(QEMUFile *f, void *pv, size_t size)
{
    int64_t *v = pv;
    qemu_get_sbe64s(f, v);
    return 0;
}

static void put_int64(QEMUFile *f, const void *pv, size_t size)
{
    const int64_t *v = pv;
    qemu_put_sbe64s(f, v);
}

const VMStateInfo vmstate_info_int64 = {
    .name = "int64",
    .get  = get_int64,
    .put  = put_int64,
};

/* 8 bit unsigned int */

static int get_uint8(QEMUFile *f, void *pv, size_t size)
{
    uint8_t *v = pv;
    qemu_get_8s(f, v);
    return 0;
}

static void put_uint8(QEMUFile *f, const void *pv, size_t size)
{
    const uint8_t *v = pv;
    qemu_put_8s(f, v);
}

const VMStateInfo vmstate_info_uint8 = {
    .name = "uint8",
    .get  = get_uint8,
    .put  = put_uint8,
};

/* 16 bit unsigned int */

static int get_uint16(QEMUFile *f, void *pv, size_t size)
{
    uint16_t *v = pv;
    qemu_get_be16s(f, v);
    return 0;
}

static void put_uint16(QEMUFile *f, const void *pv, size_t size)
{
    const uint16_t *v = pv;
    qemu_put_be16s(f, v);
}

const VMStateInfo vmstate_info_uint16 = {
    .name = "uint16",
    .get  = get_uint16,
    .put  = put_uint16,
};

/* 32 bit unsigned int */

static int get_uint32(QEMUFile *f, void *pv, size_t size)
{
    uint32_t *v = pv;
    qemu_get_be32s(f, v);
    return 0;
}

static void put_uint32(QEMUFile *f, const void *pv, size_t size)
{
    const uint32_t *v = pv;
    qemu_put_be32s(f, v);
}

const VMStateInfo vmstate_info_uint32 = {
    .name = "uint32",
    .get  = get_uint32,
    .put  = put_uint32,
};

/* 64 bit unsigned int */

static int get_uint64(QEMUFile *f, void *pv, size_t size)
{
    uint64_t *v = pv;
    qemu_get_be64s(f, v);
    return 0;
}

static void put_uint64(QEMUFile *f, const void *pv, size_t size)
{
    const uint64_t *v = pv;
    qemu_put_be64s(f, v);
}

const VMStateInfo vmstate_info_uint64 = {
    .name = "uint64",
    .get  = get_uint64,
    .put  = put_uint64,
};

/* timers  */

static int get_timer(QEMUFile *f, void *pv, size_t size)
{
    QEMUTimer *v = pv;
    qemu_get_timer(f, v);
    return 0;
}

static void put_timer(QEMUFile *f, const void *pv, size_t size)
{
    QEMUTimer *v = (void *)pv;
    qemu_put_timer(f, v);
}

const VMStateInfo vmstate_info_timer = {
    .name = "timer",
    .get  = get_timer,
    .put  = put_timer,
};

/* uint8_t buffers */

static int get_buffer(QEMUFile *f, void *pv, size_t size)
{
    uint8_t *v = pv;
    qemu_get_buffer(f, v, size);
    return 0;
}

static void put_buffer(QEMUFile *f, const void *pv, size_t size)
{
    uint8_t *v = (void *)pv;
    qemu_put_buffer(f, v, size);
}

const VMStateInfo vmstate_info_buffer = {
    .name = "buffer",
    .get  = get_buffer,
    .put  = put_buffer,
};

typedef struct SaveStateEntry {
    TAILQ_ENTRY(SaveStateEntry) entry;
    char idstr[256];
    int instance_id;
    int version_id;
    int section_id;
    SaveLiveStateHandler *save_live_state;
    SaveStateHandler *save_state;
    LoadStateHandler *load_state;
    const VMStateDescription *vmsd;
    void *opaque;
} SaveStateEntry;

static TAILQ_HEAD(savevm_handlers, SaveStateEntry) savevm_handlers =
    TAILQ_HEAD_INITIALIZER(savevm_handlers);
static int global_section_id;

static int calculate_new_instance_id(const char *idstr)
{
    SaveStateEntry *se;
    int instance_id = 0;

    TAILQ_FOREACH(se, &savevm_handlers, entry) {
        if (strcmp(idstr, se->idstr) == 0
            && instance_id <= se->instance_id) {
            instance_id = se->instance_id + 1;
        }
    }
    return instance_id;
}

/* TODO: Individual devices generally have very little idea about the rest
   of the system, so instance_id should be removed/replaced.
   Meanwhile pass -1 as instance_id if you do not already have a clearly
   distinguishing id for all instances of your device class. */
int register_savevm_live(const char *idstr,
                         int instance_id,
                         int version_id,
                         SaveLiveStateHandler *save_live_state,
                         SaveStateHandler *save_state,
                         LoadStateHandler *load_state,
                         void *opaque)
{
    SaveStateEntry *se;

    se = qemu_malloc(sizeof(SaveStateEntry));
    pstrcpy(se->idstr, sizeof(se->idstr), idstr);
    se->version_id = version_id;
    se->section_id = global_section_id++;
    se->save_live_state = save_live_state;
    se->save_state = save_state;
    se->load_state = load_state;
    se->opaque = opaque;
    se->vmsd = NULL;

    if (instance_id == -1) {
        se->instance_id = calculate_new_instance_id(idstr);
    } else {
        se->instance_id = instance_id;
    }
    /* add at the end of list */
    TAILQ_INSERT_TAIL(&savevm_handlers, se, entry);
    return 0;
}

int register_savevm(const char *idstr,
                    int instance_id,
                    int version_id,
                    SaveStateHandler *save_state,
                    LoadStateHandler *load_state,
                    void *opaque)
{
    return register_savevm_live(idstr, instance_id, version_id,
                                NULL, save_state, load_state, opaque);
}

void unregister_savevm(const char *idstr, void *opaque)
{
    SaveStateEntry *se, *new_se;

    TAILQ_FOREACH_SAFE(se, &savevm_handlers, entry, new_se) {
        if (strcmp(se->idstr, idstr) == 0 && se->opaque == opaque) {
            TAILQ_REMOVE(&savevm_handlers, se, entry);
            qemu_free(se);
        }
    }
}

int vmstate_register(int instance_id, const VMStateDescription *vmsd,
                     void *opaque)
{
    SaveStateEntry *se;

    se = qemu_malloc(sizeof(SaveStateEntry));
    pstrcpy(se->idstr, sizeof(se->idstr), vmsd->name);
    se->version_id = vmsd->version_id;
    se->section_id = global_section_id++;
    se->save_live_state = NULL;
    se->save_state = NULL;
    se->load_state = NULL;
    se->opaque = opaque;
    se->vmsd = vmsd;

    if (instance_id == -1) {
        se->instance_id = calculate_new_instance_id(vmsd->name);
    } else {
        se->instance_id = instance_id;
    }
    /* add at the end of list */
    TAILQ_INSERT_TAIL(&savevm_handlers, se, entry);
    return 0;
}

void vmstate_unregister(const char *idstr,  void *opaque)
{
    unregister_savevm(idstr, opaque);
}

int vmstate_load_state(QEMUFile *f, const VMStateDescription *vmsd,
                       void *opaque, int version_id)
{
    VMStateField *field = vmsd->fields;

    if (version_id > vmsd->version_id) {
        return -EINVAL;
    }
    if (version_id < vmsd->minimum_version_id_old) {
        return -EINVAL;
    }
    if  (version_id < vmsd->minimum_version_id) {
        return vmsd->load_state_old(f, opaque, version_id);
    }
    while(field->name) {
        if (field->version_id <= version_id) {
            void *base_addr = (char *)opaque + field->offset;
            int ret, i, n_elems = 1;

            if (field->flags & VMS_ARRAY) {
                n_elems = field->num;
            } else if (field->flags & VMS_VARRAY) {
                n_elems = *(size_t *)((char *)opaque+field->num_offset);
            }
            if (field->flags & VMS_POINTER) {
                base_addr = *(void **)base_addr;
            }
            for (i = 0; i < n_elems; i++) {
                void *addr = (char *)base_addr + field->size * i;

                if (field->flags & VMS_STRUCT) {
                    ret = vmstate_load_state(f, field->vmsd, addr, field->vmsd->version_id);
                } else {
                    ret = field->info->get(f, addr, field->size);

                }
                if (ret < 0) {
                    return ret;
                }
            }
        }
        field++;
    }
    if (vmsd->run_after_load)
        return vmsd->run_after_load(opaque);
    return 0;
}

void vmstate_save_state(QEMUFile *f, const VMStateDescription *vmsd,
                        const void *opaque)
{
    VMStateField *field = vmsd->fields;

    while(field->name) {
        const void *base_addr = (char *)opaque + field->offset;
        int i, n_elems = 1;

        if (field->flags & VMS_ARRAY) {
            n_elems = field->num;
        } else if (field->flags & VMS_VARRAY) {
            n_elems = *(size_t *)((char *)opaque+field->num_offset);
        }
        if (field->flags & VMS_POINTER) {
            base_addr = *(void **)base_addr;
        }
        for (i = 0; i < n_elems; i++) {
            const void *addr = (char *)base_addr + field->size * i;

            if (field->flags & VMS_STRUCT) {
                vmstate_save_state(f, field->vmsd, addr);
            } else {
                field->info->put(f, addr, field->size);
            }
        }
        field++;
    }
}

static int vmstate_load(QEMUFile *f, SaveStateEntry *se, int version_id)
{
    if (!se->vmsd) {         /* Old style */
        return se->load_state(f, se->opaque, version_id);
    }
    return vmstate_load_state(f, se->vmsd, se->opaque, version_id);
}

static void vmstate_save(QEMUFile *f, SaveStateEntry *se)
{
    if (!se->vmsd) {         /* Old style */
        se->save_state(f, se->opaque);
        return;
    }
    vmstate_save_state(f,se->vmsd, se->opaque);
}

#define QEMU_VM_FILE_MAGIC           0x5145564d
#define QEMU_VM_FILE_VERSION_COMPAT  0x00000002
#define QEMU_VM_FILE_VERSION         0x00000003

#define QEMU_VM_EOF                  0x00
#define QEMU_VM_SECTION_START        0x01
#define QEMU_VM_SECTION_PART         0x02
#define QEMU_VM_SECTION_END          0x03
#define QEMU_VM_SECTION_FULL         0x04

int qemu_savevm_state_begin(QEMUFile *f)
{
    SaveStateEntry *se;

    qemu_put_be32(f, QEMU_VM_FILE_MAGIC);
    qemu_put_be32(f, QEMU_VM_FILE_VERSION);

    TAILQ_FOREACH(se, &savevm_handlers, entry) {
        int len;

        if (se->save_live_state == NULL)
            continue;

        /* Section type */
        qemu_put_byte(f, QEMU_VM_SECTION_START);
        qemu_put_be32(f, se->section_id);

        /* ID string */
        len = strlen(se->idstr);
        qemu_put_byte(f, len);
        qemu_put_buffer(f, (uint8_t *)se->idstr, len);

        qemu_put_be32(f, se->instance_id);
        qemu_put_be32(f, se->version_id);

        se->save_live_state(f, QEMU_VM_SECTION_START, se->opaque);
    }

    if (qemu_file_has_error(f))
        return -EIO;

    return 0;
}

int qemu_savevm_state_iterate(QEMUFile *f)
{
    SaveStateEntry *se;
    int ret = 1;

    TAILQ_FOREACH(se, &savevm_handlers, entry) {
        if (se->save_live_state == NULL)
            continue;

        /* Section type */
        qemu_put_byte(f, QEMU_VM_SECTION_PART);
        qemu_put_be32(f, se->section_id);

        ret &= !!se->save_live_state(f, QEMU_VM_SECTION_PART, se->opaque);
    }

    if (ret)
        return 1;

    if (qemu_file_has_error(f))
        return -EIO;

    return 0;
}

int qemu_savevm_state_complete(QEMUFile *f)
{
    SaveStateEntry *se;

    TAILQ_FOREACH(se, &savevm_handlers, entry) {
        if (se->save_live_state == NULL)
            continue;

        /* Section type */
        qemu_put_byte(f, QEMU_VM_SECTION_END);
        qemu_put_be32(f, se->section_id);

        se->save_live_state(f, QEMU_VM_SECTION_END, se->opaque);
    }

    TAILQ_FOREACH(se, &savevm_handlers, entry) {
        int len;

	if (se->save_state == NULL && se->vmsd == NULL)
	    continue;

        /* Section type */
        qemu_put_byte(f, QEMU_VM_SECTION_FULL);
        qemu_put_be32(f, se->section_id);

        /* ID string */
        len = strlen(se->idstr);
        qemu_put_byte(f, len);
        qemu_put_buffer(f, (uint8_t *)se->idstr, len);

        qemu_put_be32(f, se->instance_id);
        qemu_put_be32(f, se->version_id);

        vmstate_save(f, se);
    }

    qemu_put_byte(f, QEMU_VM_EOF);

    if (qemu_file_has_error(f))
        return -EIO;

    return 0;
}

int qemu_savevm_state(QEMUFile *f)
{
    int saved_vm_running;
    int ret;

    saved_vm_running = vm_running;
    vm_stop(0);

    bdrv_flush_all();

    ret = qemu_savevm_state_begin(f);
    if (ret < 0)
        goto out;

    do {
        ret = qemu_savevm_state_iterate(f);
        if (ret < 0)
            goto out;
    } while (ret == 0);

    ret = qemu_savevm_state_complete(f);

out:
    if (qemu_file_has_error(f))
        ret = -EIO;

    if (!ret && saved_vm_running)
        vm_start();

    return ret;
}

static SaveStateEntry *find_se(const char *idstr, int instance_id)
{
    SaveStateEntry *se;

    TAILQ_FOREACH(se, &savevm_handlers, entry) {
        if (!strcmp(se->idstr, idstr) &&
            instance_id == se->instance_id)
            return se;
    }
    return NULL;
}

typedef struct LoadStateEntry {
    LIST_ENTRY(LoadStateEntry) entry;
    SaveStateEntry *se;
    int section_id;
    int version_id;
} LoadStateEntry;

static int qemu_loadvm_state_v2(QEMUFile *f)
{
    SaveStateEntry *se;
    int len, ret, instance_id, record_len, version_id;
    int64_t total_len, end_pos, cur_pos;
    char idstr[256];

    total_len = qemu_get_be64(f);
    end_pos = total_len + qemu_ftell(f);
    for(;;) {
        if (qemu_ftell(f) >= end_pos)
            break;
        len = qemu_get_byte(f);
        qemu_get_buffer(f, (uint8_t *)idstr, len);
        idstr[len] = '\0';
        instance_id = qemu_get_be32(f);
        version_id = qemu_get_be32(f);
        record_len = qemu_get_be32(f);
        cur_pos = qemu_ftell(f);
        se = find_se(idstr, instance_id);
        if (!se) {
            fprintf(stderr, "qemu: warning: instance 0x%x of device '%s' not present in current VM\n",
                    instance_id, idstr);
        } else {
            ret = vmstate_load(f, se, version_id);
            if (ret < 0) {
                fprintf(stderr, "qemu: warning: error while loading state for instance 0x%x of device '%s'\n",
                        instance_id, idstr);
                return ret;
            }
        }
        /* always seek to exact end of record */
        qemu_fseek(f, cur_pos + record_len, SEEK_SET);
    }

    if (qemu_file_has_error(f))
        return -EIO;

    return 0;
}

int qemu_loadvm_state(QEMUFile *f)
{
    LIST_HEAD(, LoadStateEntry) loadvm_handlers =
        LIST_HEAD_INITIALIZER(loadvm_handlers);
    LoadStateEntry *le, *new_le;
    uint8_t section_type;
    unsigned int v;
    int ret;

    v = qemu_get_be32(f);
    if (v != QEMU_VM_FILE_MAGIC)
        return -EINVAL;

    v = qemu_get_be32(f);
    if (v == QEMU_VM_FILE_VERSION_COMPAT)
        return qemu_loadvm_state_v2(f);
    if (v != QEMU_VM_FILE_VERSION)
        return -ENOTSUP;

    while ((section_type = qemu_get_byte(f)) != QEMU_VM_EOF) {
        uint32_t instance_id, version_id, section_id;
        SaveStateEntry *se;
        char idstr[257];
        int len;

        switch (section_type) {
        case QEMU_VM_SECTION_START:
        case QEMU_VM_SECTION_FULL:
            /* Read section start */
            section_id = qemu_get_be32(f);
            len = qemu_get_byte(f);
            qemu_get_buffer(f, (uint8_t *)idstr, len);
            idstr[len] = 0;
            instance_id = qemu_get_be32(f);
            version_id = qemu_get_be32(f);

            /* Find savevm section */
            se = find_se(idstr, instance_id);
            if (se == NULL) {
                fprintf(stderr, "Unknown savevm section or instance '%s' %d\n", idstr, instance_id);
                ret = -EINVAL;
                goto out;
            }

            /* Validate version */
            if (version_id > se->version_id) {
                fprintf(stderr, "savevm: unsupported version %d for '%s' v%d\n",
                        version_id, idstr, se->version_id);
                ret = -EINVAL;
                goto out;
            }

            /* Add entry */
            le = qemu_mallocz(sizeof(*le));

            le->se = se;
            le->section_id = section_id;
            le->version_id = version_id;
            LIST_INSERT_HEAD(&loadvm_handlers, le, entry);

            ret = vmstate_load(f, le->se, le->version_id);
            if (ret < 0) {
                fprintf(stderr, "qemu: warning: error while loading state for instance 0x%x of device '%s'\n",
                        instance_id, idstr);
                goto out;
            }
            break;
        case QEMU_VM_SECTION_PART:
        case QEMU_VM_SECTION_END:
            section_id = qemu_get_be32(f);

            LIST_FOREACH(le, &loadvm_handlers, entry) {
                if (le->section_id == section_id) {
                    break;
                }
            }
            if (le == NULL) {
                fprintf(stderr, "Unknown savevm section %d\n", section_id);
                ret = -EINVAL;
                goto out;
            }

            ret = vmstate_load(f, le->se, le->version_id);
            if (ret < 0) {
                fprintf(stderr, "qemu: warning: error while loading state section id %d\n",
                        section_id);
                goto out;
            }
            break;
        default:
            fprintf(stderr, "Unknown savevm section type %d\n", section_type);
            ret = -EINVAL;
            goto out;
        }
    }

    ret = 0;

out:
    LIST_FOREACH_SAFE(le, &loadvm_handlers, entry, new_le) {
        LIST_REMOVE(le, entry);
        qemu_free(le);
    }

    if (qemu_file_has_error(f))
        ret = -EIO;

    return ret;
}

/* device can contain snapshots */
static int bdrv_can_snapshot(BlockDriverState *bs)
{
    return (bs &&
            !bdrv_is_removable(bs) &&
            !bdrv_is_read_only(bs));
}

/* device must be snapshots in order to have a reliable snapshot */
static int bdrv_has_snapshot(BlockDriverState *bs)
{
    return (bs &&
            !bdrv_is_removable(bs) &&
            !bdrv_is_read_only(bs));
}

static BlockDriverState *get_bs_snapshots(void)
{
    BlockDriverState *bs;
    DriveInfo *dinfo;

    if (bs_snapshots)
        return bs_snapshots;
    TAILQ_FOREACH(dinfo, &drives, next) {
        bs = dinfo->bdrv;
        if (bdrv_can_snapshot(bs))
            goto ok;
    }
    return NULL;
 ok:
    bs_snapshots = bs;
    return bs;
}

static int bdrv_snapshot_find(BlockDriverState *bs, QEMUSnapshotInfo *sn_info,
                              const char *name)
{
    QEMUSnapshotInfo *sn_tab, *sn;
    int nb_sns, i, ret;

    ret = -ENOENT;
    nb_sns = bdrv_snapshot_list(bs, &sn_tab);
    if (nb_sns < 0)
        return ret;
    for(i = 0; i < nb_sns; i++) {
        sn = &sn_tab[i];
        if (!strcmp(sn->id_str, name) || !strcmp(sn->name, name)) {
            *sn_info = *sn;
            ret = 0;
            break;
        }
    }
    qemu_free(sn_tab);
    return ret;
}

void do_savevm(Monitor *mon, const QDict *qdict)
{
    DriveInfo *dinfo;
    BlockDriverState *bs, *bs1;
    QEMUSnapshotInfo sn1, *sn = &sn1, old_sn1, *old_sn = &old_sn1;
    int must_delete, ret;
    QEMUFile *f;
    int saved_vm_running;
    uint32_t vm_state_size;
#ifdef _WIN32
    struct _timeb tb;
#else
    struct timeval tv;
#endif
    const char *name = qdict_get_try_str(qdict, "name");

    bs = get_bs_snapshots();
    if (!bs) {
        monitor_printf(mon, "No block device can accept snapshots\n");
        return;
    }

    /* ??? Should this occur after vm_stop?  */
    qemu_aio_flush();

    saved_vm_running = vm_running;
    vm_stop(0);

    must_delete = 0;
    if (name) {
        ret = bdrv_snapshot_find(bs, old_sn, name);
        if (ret >= 0) {
            must_delete = 1;
        }
    }
    memset(sn, 0, sizeof(*sn));
    if (must_delete) {
        pstrcpy(sn->name, sizeof(sn->name), old_sn->name);
        pstrcpy(sn->id_str, sizeof(sn->id_str), old_sn->id_str);
    } else {
        if (name)
            pstrcpy(sn->name, sizeof(sn->name), name);
    }

    /* fill auxiliary fields */
#ifdef _WIN32
    _ftime(&tb);
    sn->date_sec = tb.time;
    sn->date_nsec = tb.millitm * 1000000;
#else
    gettimeofday(&tv, NULL);
    sn->date_sec = tv.tv_sec;
    sn->date_nsec = tv.tv_usec * 1000;
#endif
    sn->vm_clock_nsec = qemu_get_clock(vm_clock);

    /* save the VM state */
    f = qemu_fopen_bdrv(bs, 1);
    if (!f) {
        monitor_printf(mon, "Could not open VM state file\n");
        goto the_end;
    }
    ret = qemu_savevm_state(f);
    vm_state_size = qemu_ftell(f);
    qemu_fclose(f);
    if (ret < 0) {
        monitor_printf(mon, "Error %d while writing VM\n", ret);
        goto the_end;
    }

    /* create the snapshots */

    TAILQ_FOREACH(dinfo, &drives, next) {
        bs1 = dinfo->bdrv;
        if (bdrv_has_snapshot(bs1)) {
            if (must_delete) {
                ret = bdrv_snapshot_delete(bs1, old_sn->id_str);
                if (ret < 0) {
                    monitor_printf(mon,
                                   "Error while deleting snapshot on '%s'\n",
                                   bdrv_get_device_name(bs1));
                }
            }
            /* Write VM state size only to the image that contains the state */
            sn->vm_state_size = (bs == bs1 ? vm_state_size : 0);
            ret = bdrv_snapshot_create(bs1, sn);
            if (ret < 0) {
                monitor_printf(mon, "Error while creating snapshot on '%s'\n",
                               bdrv_get_device_name(bs1));
            }
        }
    }

 the_end:
    if (saved_vm_running)
        vm_start();
}

int load_vmstate(Monitor *mon, const char *name)
{
    DriveInfo *dinfo;
    BlockDriverState *bs, *bs1;
    QEMUSnapshotInfo sn;
    QEMUFile *f;
    int ret;

    bs = get_bs_snapshots();
    if (!bs) {
        monitor_printf(mon, "No block device supports snapshots\n");
        return -EINVAL;
    }

    /* Flush all IO requests so they don't interfere with the new state.  */
    qemu_aio_flush();

    TAILQ_FOREACH(dinfo, &drives, next) {
        bs1 = dinfo->bdrv;
        if (bdrv_has_snapshot(bs1)) {
            ret = bdrv_snapshot_goto(bs1, name);
            if (ret < 0) {
                if (bs != bs1)
                    monitor_printf(mon, "Warning: ");
                switch(ret) {
                case -ENOTSUP:
                    monitor_printf(mon,
                                   "Snapshots not supported on device '%s'\n",
                                   bdrv_get_device_name(bs1));
                    break;
                case -ENOENT:
                    monitor_printf(mon, "Could not find snapshot '%s' on "
                                   "device '%s'\n",
                                   name, bdrv_get_device_name(bs1));
                    break;
                default:
                    monitor_printf(mon, "Error %d while activating snapshot on"
                                   " '%s'\n", ret, bdrv_get_device_name(bs1));
                    break;
                }
                /* fatal on snapshot block device */
                if (bs == bs1)
                    return 0;
            }
        }
    }

    /* Don't even try to load empty VM states */
    ret = bdrv_snapshot_find(bs, &sn, name);
    if ((ret >= 0) && (sn.vm_state_size == 0))
        return -EINVAL;

    /* restore the VM state */
    f = qemu_fopen_bdrv(bs, 0);
    if (!f) {
        monitor_printf(mon, "Could not open VM state file\n");
        return -EINVAL;
    }
    ret = qemu_loadvm_state(f);
    qemu_fclose(f);
    if (ret < 0) {
        monitor_printf(mon, "Error %d while loading VM state\n", ret);
        return ret;
    }
    return 0;
}

void do_delvm(Monitor *mon, const QDict *qdict)
{
    DriveInfo *dinfo;
    BlockDriverState *bs, *bs1;
    int ret;
    const char *name = qdict_get_str(qdict, "name");

    bs = get_bs_snapshots();
    if (!bs) {
        monitor_printf(mon, "No block device supports snapshots\n");
        return;
    }

    TAILQ_FOREACH(dinfo, &drives, next) {
        bs1 = dinfo->bdrv;
        if (bdrv_has_snapshot(bs1)) {
            ret = bdrv_snapshot_delete(bs1, name);
            if (ret < 0) {
                if (ret == -ENOTSUP)
                    monitor_printf(mon,
                                   "Snapshots not supported on device '%s'\n",
                                   bdrv_get_device_name(bs1));
                else
                    monitor_printf(mon, "Error %d while deleting snapshot on "
                                   "'%s'\n", ret, bdrv_get_device_name(bs1));
            }
        }
    }
}

void do_info_snapshots(Monitor *mon)
{
    DriveInfo *dinfo;
    BlockDriverState *bs, *bs1;
    QEMUSnapshotInfo *sn_tab, *sn;
    int nb_sns, i;
    char buf[256];

    bs = get_bs_snapshots();
    if (!bs) {
        monitor_printf(mon, "No available block device supports snapshots\n");
        return;
    }
    monitor_printf(mon, "Snapshot devices:");
    TAILQ_FOREACH(dinfo, &drives, next) {
        bs1 = dinfo->bdrv;
        if (bdrv_has_snapshot(bs1)) {
            if (bs == bs1)
                monitor_printf(mon, " %s", bdrv_get_device_name(bs1));
        }
    }
    monitor_printf(mon, "\n");

    nb_sns = bdrv_snapshot_list(bs, &sn_tab);
    if (nb_sns < 0) {
        monitor_printf(mon, "bdrv_snapshot_list: error %d\n", nb_sns);
        return;
    }
    monitor_printf(mon, "Snapshot list (from %s):\n",
                   bdrv_get_device_name(bs));
    monitor_printf(mon, "%s\n", bdrv_snapshot_dump(buf, sizeof(buf), NULL));
    for(i = 0; i < nb_sns; i++) {
        sn = &sn_tab[i];
        monitor_printf(mon, "%s\n", bdrv_snapshot_dump(buf, sizeof(buf), sn));
    }
    qemu_free(sn_tab);
}
