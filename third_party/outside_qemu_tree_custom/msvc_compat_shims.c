#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>

/* GCC builtin replacement: full memory barrier */
void __sync_synchronize(void)
{
    MemoryBarrier();
}

/* GCC builtin replacement: atomic fetch-and-add, return old value */
int __sync_fetch_and_add(int* ptr, int value)
{
    return InterlockedExchangeAdd((volatile LONG*)ptr, (LONG)value);
}

/* GCC builtin replacement: atomic compare-and-swap, return old value */
int __sync_val_compare_and_swap(int* ptr, int oldval, int newval)
{
    return InterlockedCompareExchange((volatile LONG*)ptr, (LONG)newval, (LONG)oldval);
}

/* POSIX writev replacement for MSVC (sequential _write per iovec element) */
struct msvc_iovec {
    void*  iov_base;
    size_t iov_len;
};
int writev(int fd, const struct msvc_iovec* iov, int iovcnt)
{
    int i, ret, total = 0;
    for (i = 0; i < iovcnt; i++) {
        if (iov[i].iov_len == 0) {
            continue;
        }
        ret = _write(fd, iov[i].iov_base, (unsigned int)iov[i].iov_len);
        if (ret < 0) {
            return -1;
        }
        total += ret;
        if ((size_t)ret < iov[i].iov_len) {
            break;
        }
    }
    return total;
}

/* GNU asprintf replacement for MSVC */
int asprintf(char** strp, const char* fmt, ...)
{
    int len;
    va_list ap;

    if (!strp || !fmt) {
        return -1;
    }

    va_start(ap, fmt);
    len = _vscprintf(fmt, ap);  /* count without writing */
    va_end(ap);

    if (len < 0) {
        *strp = NULL;
        return -1;
    }

    *strp = (char*)malloc((size_t)len + 1);
    if (!*strp) {
        return -1;
    }

    va_start(ap, fmt);
    if (vsnprintf(*strp, (size_t)len + 1, fmt, ap) < 0) {
        va_end(ap);
        free(*strp);
        *strp = NULL;
        return -1;
    }
    va_end(ap);

    return len;  /* chars written, not counting terminator */
}

#endif /* _MSC_VER */
