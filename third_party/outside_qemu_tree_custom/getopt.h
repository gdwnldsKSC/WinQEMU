/*
 * getopt.h - GNU-style getopt/getopt_long for MSVC (WinQEMU port).
 *
 * Adapted from third_party/parity.runtime/getopt.h (parity, Markus Duft,
 * LGPL), itself based on my_getopt by Benjamin Sittler (MIT license, see
 * original notice below).  The parity-internal pcrt scaffolding has been
 * replaced with plain C linkage guards so this header stands alone on the
 * include path and satisfies qemu's <getopt.h> includes (qemu-common.h,
 * qemu-img.c, qemu-io.c, ...) with full long-option support.
 */

#ifndef WINQEMU_GETOPT_H
#define WINQEMU_GETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

struct option {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

extern int optind, opterr, optopt;
extern char *optarg;

#undef no_argument
#undef required_argument
#undef optional_argument

#define no_argument			0
#define required_argument	1
#define optional_argument	2

extern int getopt(int argc, char * argv[], const char *opts);
extern int getopt_reset(void);
extern int getopt_long(int argc, char * argv[], const char *shortopts, const struct option *longopts, int *longind);
extern int getopt_long_only(int argc, char * argv[], const char *shortopts, const struct option *longopts, int *longind);
extern int _getopt_internal(int argc, char * argv[], const char *shortopts, const struct option *longopts, int *longind, int long_only);

#ifdef __cplusplus
}
#endif

#endif /* WINQEMU_GETOPT_H */

/* original copyright notice: */
/*
 *  my_getopt.h - interface to my re-implementation of getopt.
 *  Copyright 1997, 2000, 2001, 2002, 2006, Benjamin Sittler
 *
 *  Permission is hereby granted, free of charge, to any person
 *  obtaining a copy of this software and associated documentation
 *  files (the "Software"), to deal in the Software without
 *  restriction, including without limitation the rights to use, copy,
 *  modify, merge, publish, distribute, sublicense, and/or sell copies
 *  of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */
