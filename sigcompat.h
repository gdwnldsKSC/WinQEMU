/* qemu/sigcompat.h - Windows POSIX signal compatibility */

#ifndef QEMU_SIGCOMPAT_H
#define QEMU_SIGCOMPAT_H

#include "qemu-common.h"

#ifdef _WIN32

/* MSVC <signal.h> is tiny; pthreads4w <signal.h> is more POSIX-like.
   We include both: whichever defines more wins. */
#include <signal.h>
#include <errno.h>

   /* If pthreads4w didn't give us sigset_t, define a simple bitmask type. */
#ifndef QEMU_HAVE_SIGSET_T
typedef unsigned long sigset_t;
#endif

/* Very small siginfo_t that satisfies compatfd.c.
   compatfd.c never inspects anything except the signal number. */
#ifndef QEMU_HAVE_SIGINFO_T
typedef struct qemu_siginfo {
    int si_signo;
} siginfo_t;
#endif

#ifndef SIG_BLOCK
#define SIG_BLOCK   0
#endif

#ifndef SIG_UNBLOCK
#define SIG_UNBLOCK 1
#endif

#ifndef SIG_SETMASK
#define SIG_SETMASK 2
#endif

/* Basic set-ops for our bitmask sigset_t. These are good enough to keep
   upstream code compiling and behaving sensibly inside the process. */
static inline int sigemptyset(sigset_t* set)
{
    *set = 0;
    return 0;
}

static inline int sigfillset(sigset_t* set)
{
    *set = ~0UL;
    return 0;
}

static inline int sigaddset(sigset_t* set, int sig)
{
    if (sig <= 0 || sig >= 8 * (int)sizeof(*set)) {
        errno = EINVAL;
        return -1;
    }
    *set |= (1UL << sig);
    return 0;
}

static inline int sigdelset(sigset_t* set, int sig)
{
    if (sig <= 0 || sig >= 8 * (int)sizeof(*set)) {
        errno = EINVAL;
        return -1;
    }
    *set &= ~(1UL << sig);
    return 0;
}

static inline int sigismember(const sigset_t* set, int sig)
{
    if (sig <= 0 || sig >= 8 * (int)sizeof(*set)) {
        return 0;
    }
    return (*set & (1UL << sig)) != 0;
}

/*
 * sigprocmask stub:
 *  - We don't try to integrate with the CRT or Windows signals.
 *  - For QEMU on Windows, nothing relies on real Unix signal masks anyway.
 */
static inline int sigprocmask(int how,
    const sigset_t* set,
    sigset_t* oldset)
{
    if (oldset) {
        *oldset = 0;
    }
    /* Pretend success. */
    (void)how;
    (void)set;
    return 0;
}

/*
 * sigwait stub:
 *  - We never actually see Unix signals on Windows.
 *  - Make this fail with ENOSYS so the compat thread in compatfd.c
 *    just bails out cleanly.
 */
static inline int sigwait(const sigset_t* set, int* sig)
{
    (void)set;
    (void)sig;
    errno = ENOSYS;
    /* POSIX sigwait() returns 0 on success, or an error number on failure.
       Returning ENOSYS here both satisfies the API and matches the old
       "fail with ENOSYS" behaviour. */
    return ENOSYS;
}

/*
 * sigwaitinfo stub:
 *  - We *never* actually see Unix signals on Windows.
 *  - Make this fail with ENOSYS so the compat thread in compatfd.c
 *    just bails out cleanly.
 */
static inline int sigwaitinfo(const sigset_t* set, siginfo_t* info)
{
    (void)set;
    (void)info;
    errno = ENOSYS;
    return -1;
}

#endif /* _WIN32 */

#endif /* QEMU_SIGCOMPAT_H */
