 /**************************************************************************\
 *                                                                          *
 *   pxwc - wgcc's gcc to cl libc wrapper                                   *
 *   Copyright (C) 2006  Markus Duft <markus.duft@salomon.at>               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Lesser General Public             *
 *   License as published by the Free Software Foundation; either           *
 *   version 2.1 of the License, or (at your option) any later version.     *
 *                                                                          *
 *   This library is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with this library; if not, write to the Free Software    *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,             *
 *   MA  02110-1301 USA                                                     *
 *                                                                          *
 \**************************************************************************/

/*
 * DIRENT.H (formerly DIRLIB.H)
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 */
#ifndef _DIRENT_H_
#define _DIRENT_H_

#ifndef ARM

#include <stdio.h>
#include <io.h>

#ifdef __WGCC

/* When this is part of pxwc, we must use some different symbol names, so we don't
 * conflict with other libraries that bring dirent with them */

#ifdef DIR
#undef DIR
#endif

#define dirent __pxwc_dirent

#define opendir __pxwc_opendir
#define readdir __pxwc_readdir
#define closedir __pxwc_closedir
#define rewinddir __pxwc_rewinddir
#define telldir __pxwc_telldir
#define seekdir __pxwc_seekdir

#define _wopendir __pxwc__wopendir
#define _wreaddir __pxwc__wreaddir
#define _wclosedir __pxwc__wclosedir
#define _wrewinddir __pxwc__wrewinddir
#define _wtelldir __pxwc__wtelldir
#define _wseekdir __pxwc__wseekdir

#endif

#ifndef RC_INVOKED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FILENAME_MAX
#define FILENAME_MAX 260
#endif

struct dirent
{
	long		d_ino;		/* Always zero. */
	unsigned short	d_reclen;	/* Always zero. */
	unsigned short	d_namlen;	/* Length of name in d_name. */
	char		d_name[FILENAME_MAX]; /* File name. */
};

/*
 * This is an internal data structure. Good programmers will not use it
 * except as an argument to one of the functions below.
 * dd_stat field is now int (was short in older versions).
 */
typedef struct
{
	/* disk transfer area for this dir */
	struct _finddata_t	dd_dta;

	/* dirent struct to return from dir (NOTE: this makes this thread
	 * safe as long as only one thread uses a particular DIR struct at
	 * a time) */
	struct dirent		dd_dir;

	/* _findnext handle */
	long			dd_handle;

	/*
         * Status of search:
	 *   0 = not started yet (next entry to read is first entry)
	 *  -1 = off the end
	 *   positive = 0 based index of next entry
	 */
	int			dd_stat;

	/* given path for dir with search pattern (struct is extended) */
	char			dd_name[1];
} DIR;

DIR* __cdecl opendir (const char*);
struct dirent* __cdecl readdir (DIR*);
int __cdecl closedir (DIR*);
void __cdecl rewinddir (DIR*);
long __cdecl telldir (DIR*);
void __cdecl seekdir (DIR*, long);


/* wide char versions */

struct _wdirent
{
	long		d_ino;		/* Always zero. */
	unsigned short	d_reclen;	/* Always zero. */
	unsigned short	d_namlen;	/* Length of name in d_name. */
	wchar_t		d_name[FILENAME_MAX]; /* File name. */
};

/*
 * This is an internal data structure. Good programmers will not use it
 * except as an argument to one of the functions below.
 */
typedef struct
{
	/* disk transfer area for this dir */
	struct _wfinddata_t	dd_dta;

	/* dirent struct to return from dir (NOTE: this makes this thread
	 * safe as long as only one thread uses a particular DIR struct at
	 * a time) */
	struct _wdirent		dd_dir;

	/* _findnext handle */
	long			dd_handle;

	/*
         * Status of search:
	 *   0 = not started yet (next entry to read is first entry)
	 *  -1 = off the end
	 *   positive = 0 based index of next entry
	 */
	int			dd_stat;

	/* given path for dir with search pattern (struct is extended) */
	wchar_t			dd_name[1];
} _WDIR;



_WDIR* __cdecl _wopendir (const wchar_t*);
struct _wdirent*  __cdecl _wreaddir (_WDIR*);
int __cdecl _wclosedir (_WDIR*);
void __cdecl _wrewinddir (_WDIR*);
long __cdecl _wtelldir (_WDIR*);
void __cdecl _wseekdir (_WDIR*, long);


#ifdef	__cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif  /* ARM */

#endif	/* Not _DIRENT_H_ */
