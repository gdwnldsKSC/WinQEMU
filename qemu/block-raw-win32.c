/*
 * Block driver for RAW files (win32)
 *
 * Copyright (c) 2006 Fabrice Bellard
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

/*
 * WinQEMU GPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL is available it will apply instead, WinQEMU elects to use only the
 * General Public License version 3 (GPLv3) at this time for any software where a choice of
 * GPL license versions is made available with the language indicating that GPLv3 or any later
 * version may be used, or where a choice of which version of the GPL is applied is otherwise unspecified.
 *
 * Please contact Yan Wen (celestialwy@gmail.com) if you need additional information or have any questions.
 */

#include "qemu-common.h"
#include "qemu-timer.h"
#include "block_int.h"
#include <assert.h>
#include <windows.h>
#include <winioctl.h>

#define FTYPE_FILE 0
#define FTYPE_CD     1
#define FTYPE_HARDDISK 2

typedef struct BDRVRawState {
	HANDLE hfile;
	int type;
	char drive_path[16]; /* format: "d:\" */
} BDRVRawState;

int qemu_ftruncate64(int fd, int64_t length)
{
	LARGE_INTEGER li;
	LONG high;
	HANDLE h;
	BOOL res;

	if ((GetVersion() & 0x80000000UL) && (length >> 32) != 0)
		return -1;

	h = (HANDLE)_get_osfhandle(fd);

	/* get current position, ftruncate do not change position */
	li.HighPart = 0;
	li.LowPart = SetFilePointer(h, 0, &li.HighPart, FILE_CURRENT);
	if (li.LowPart == 0xffffffffUL && GetLastError() != NO_ERROR)
		return -1;

	high = length >> 32;
	if (!SetFilePointer(h, (DWORD)length, &high, FILE_BEGIN))
		return -1;
	res = SetEndOfFile(h);

	/* back to old position */
	SetFilePointer(h, li.LowPart, &li.HighPart, FILE_BEGIN);
	return res ? 0 : -1;
}

static int set_sparse(int fd)
{
	DWORD returned;
	return (int)DeviceIoControl((HANDLE)_get_osfhandle(fd), FSCTL_SET_SPARSE,
		NULL, 0, NULL, 0, &returned, NULL);
}

static int raw_open(BlockDriverState *bs, const char *filename, int flags)
{
	BDRVRawState *s = bs->opaque;
	int access_flags, create_flags;
	DWORD overlapped;

	s->type = FTYPE_FILE;

	if ((flags & BDRV_O_ACCESS) == O_RDWR) {
		access_flags = GENERIC_READ | GENERIC_WRITE;
	}
	else {
		access_flags = GENERIC_READ;
	}
	if (flags & BDRV_O_CREAT) {
		create_flags = CREATE_ALWAYS;
	}
	else {
		create_flags = OPEN_EXISTING;
	}
	overlapped = FILE_ATTRIBUTE_NORMAL;
	if ((flags & BDRV_O_NOCACHE))
		overlapped |= FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH;
	else if (!(flags & BDRV_O_CACHE_WB))
		overlapped |= FILE_FLAG_WRITE_THROUGH;
	s->hfile = CreateFile(filename, access_flags,
		FILE_SHARE_READ, NULL,
		create_flags, overlapped, NULL);
	if (s->hfile == INVALID_HANDLE_VALUE) {
		int err = GetLastError();

		if (err == ERROR_ACCESS_DENIED)
			return -EACCES;
		return -1;
	}
	return 0;
}

static int raw_read(BlockDriverState *bs, int64_t sector_num,
	uint8_t *buf, int nb_sectors)
{
	BDRVRawState *s = bs->opaque;
	OVERLAPPED ov;
	DWORD ret_count;
	int ret;
	int64_t offset = sector_num * 512;
	int count = nb_sectors * 512;

	memset(&ov, 0, sizeof(ov));
	ov.Offset = offset;
	ov.OffsetHigh = offset >> 32;
	ret = ReadFile(s->hfile, buf, count, &ret_count, &ov);
	if (!ret)
		return ret_count;
	if (ret_count == count)
		ret_count = 0;
	return ret_count;
}

static int raw_write(BlockDriverState *bs, int64_t sector_num,
	const uint8_t *buf, int nb_sectors)
{
	BDRVRawState *s = bs->opaque;
	OVERLAPPED ov;
	DWORD ret_count;
	int ret;
	int64_t offset = sector_num * 512;
	int count = nb_sectors * 512;

	memset(&ov, 0, sizeof(ov));
	ov.Offset = offset;
	ov.OffsetHigh = offset >> 32;
	ret = WriteFile(s->hfile, buf, count, &ret_count, &ov);
	if (!ret)
		return ret_count;
	if (ret_count == count)
		ret_count = 0;
	return ret_count;
}

static void raw_flush(BlockDriverState *bs)
{
	BDRVRawState *s = bs->opaque;
	FlushFileBuffers(s->hfile);
}

static void raw_close(BlockDriverState *bs)
{
	BDRVRawState *s = bs->opaque;
	CloseHandle(s->hfile);
}

static int raw_truncate(BlockDriverState *bs, int64_t offset)
{
	BDRVRawState *s = bs->opaque;
	LONG low, high;

	low = offset;
	high = offset >> 32;
	if (!SetFilePointer(s->hfile, low, &high, FILE_BEGIN))
		return -EIO;
	if (!SetEndOfFile(s->hfile))
		return -EIO;
	return 0;
}

static int64_t raw_getlength(BlockDriverState *bs)
{
	BDRVRawState *s = bs->opaque;
	LARGE_INTEGER l;
	ULARGE_INTEGER available, total, total_free;
	DISK_GEOMETRY_EX dg;
	DWORD count;
	BOOL status;

	switch (s->type) {
	case FTYPE_FILE:
		l.LowPart = GetFileSize(s->hfile, (PDWORD)&l.HighPart);
		if (l.LowPart == 0xffffffffUL && GetLastError() != NO_ERROR)
			return -EIO;
		break;
	case FTYPE_CD:
		if (!GetDiskFreeSpaceEx(s->drive_path, &available, &total, &total_free))
			return -EIO;
		l.QuadPart = total.QuadPart;
		break;
	case FTYPE_HARDDISK:
		status = DeviceIoControl(s->hfile, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
			NULL, 0, &dg, sizeof(dg), &count, NULL);
		if (status != 0) {
			l = dg.DiskSize;
		}
		break;
	default:
		return -EIO;
	}
	return l.QuadPart;
}

static int raw_create(const char *filename, int64_t total_size,
	const char *backing_file, int flags)
{
	int fd;

	if (flags || backing_file)
		return -ENOTSUP;

	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,
		0644);
	if (fd < 0)
		return -EIO;
	set_sparse(fd);
	ftruncate(fd, total_size * 512);
	close(fd);
	return 0;
}

#ifndef _MSC_VER
BlockDriver bdrv_raw = {
	.format_name	= "raw",
	.instance_size	= sizeof(BDRVRawState),
	.bdrv_open		= raw_open,
	.bdrv_close		= raw_close,
	.bdrv_create	= raw_create,
	.bdrv_flush		= raw_flush,
	.bdrv_read		= raw_read,
	.bdrv_write		= raw_write,
	.bdrv_truncate	= raw_truncate,
	.bdrv_getlength	= raw_getlength,
};

#else
BlockDriver bdrv_raw = {
	"raw",					//const char *format_name;
	sizeof(BDRVRawState),	//int instance_size;
	NULL,					//int (*bdrv_probe)(const uint8_t *buf, int buf_size, const char *filename);
	raw_open,				//int (*bdrv_open)(BlockDriverState *bs, const char *filename, int flags);
	raw_read,				//int (*bdrv_read)(BlockDriverState *bs, int64_t sector_num, uint8_t *buf, int nb_sectors);
	raw_write,				//int (*bdrv_write)(BlockDriverState *bs, int64_t sector_num,	const uint8_t *buf, int nb_sectors);
	raw_close,				//void (*bdrv_close)(BlockDriverState *bs);
	raw_create,					//int (*bdrv_create)(const char *filename, int64_t total_sectors, const char *backing_file, int flags);
	raw_flush,					//void (*bdrv_flush)(BlockDriverState *bs);
	NULL,					//int (*bdrv_is_allocated)(BlockDriverState *bs, int64_t sector_num, int nb_sectors, int *pnum);
	NULL,					//int (*bdrv_set_key)(BlockDriverState *bs, const char *key);
	NULL,					//int (*bdrv_make_empty)(BlockDriverState *bs);
	NULL,
	NULL,
	NULL,
	0,


	NULL,					//const char *protocol_name; --- some of the below is wrong, must_fix comments
	raw_truncate,					//int (*bdrv_pread)(BlockDriverState *bs, int64_t offset, uint8_t *buf, int count);
	raw_getlength,					//int (*bdrv_pwrite)(BlockDriverState *bs, int64_t offset, const uint8_t *buf, int count);
	NULL,					//int (*bdrv_truncate)(BlockDriverState *bs, int64_t offset);
	NULL,			//int64_t (*bdrv_getlength)(BlockDriverState *bs);
	NULL,					//int (*bdrv_write_compressed)(BlockDriverState *bs, int64_t sector_num, const uint8_t *buf, int nb_sectors);

	NULL,					//int (*bdrv_snapshot_create)(BlockDriverState *bs, QEMUSnapshotInfo *sn_info);
	NULL,					//int (*bdrv_snapshot_goto)(BlockDriverState *bs, const char *snapshot_id);
	NULL,					//int (*bdrv_snapshot_delete)(BlockDriverState *bs, const char *snapshot_id);
	NULL,					//int (*bdrv_snapshot_list)(BlockDriverState *bs, QEMUSnapshotInfo **psn_info);
	NULL,					//int (*bdrv_get_info)(BlockDriverState *bs, BlockDriverInfo *bdi);

	/* removable device specific */
	NULL,					//int (*bdrv_is_inserted)(BlockDriverState *bs);
	NULL,					//int (*bdrv_media_changed)(BlockDriverState *bs);
	NULL,					//int (*bdrv_eject)(BlockDriverState *bs, int eject_flag);
	NULL,					//int (*bdrv_set_locked)(BlockDriverState *bs, int locked);

	/* to control generic scsi devices */
	NULL,					//int (*bdrv_ioctl)(BlockDriverState *bs, unsigned long int req, void *buf);

	NULL,					//BlockDriverAIOCB *free_aiocb;
	NULL					//struct BlockDriver *next;
};
#endif

/***********************************************/
/* host device */

static int find_cdrom(char *cdrom_name, int cdrom_name_size)
{
	char drives[256], *pdrv = drives;
	UINT type;

	memset(drives, 0, sizeof(drives));
	GetLogicalDriveStrings(sizeof(drives), drives);
	while (pdrv[0] != '\0') {
		type = GetDriveType(pdrv);
		switch (type) {
		case DRIVE_CDROM:
			snprintf(cdrom_name, cdrom_name_size, "\\\\.\\%c:", pdrv[0]);
			return 0;
			break;
		}
		pdrv += lstrlen(pdrv) + 1;
	}
	return -1;
}

static int find_device_type(BlockDriverState *bs, const char *filename)
{
	BDRVRawState *s = bs->opaque;
	UINT type;
	const char *p;

	if (strstart(filename, "\\\\.\\", &p) ||
		strstart(filename, "//./", &p)) {
		if (stristart(p, "PhysicalDrive", NULL))
			return FTYPE_HARDDISK;
		snprintf(s->drive_path, sizeof(s->drive_path), "%c:\\", p[0]);
		type = GetDriveType(s->drive_path);
		switch (type) {
		case DRIVE_REMOVABLE:
		case DRIVE_FIXED:
			return FTYPE_HARDDISK;
		case DRIVE_CDROM:
			return FTYPE_CD;
		default:
			return FTYPE_FILE;
		}
	}
	else {
		return FTYPE_FILE;
	}
}

static int hdev_open(BlockDriverState *bs, const char *filename, int flags)
{
	BDRVRawState *s = bs->opaque;
	int access_flags, create_flags;
	DWORD overlapped;
	char device_name[64];

	if (strstart(filename, "/dev/cdrom", NULL)) {
		if (find_cdrom(device_name, sizeof(device_name)) < 0)
			return -ENOENT;
		filename = device_name;
	}
	else {
		/* transform drive letters into device name */
		if (((filename[0] >= 'a' && filename[0] <= 'z') ||
			(filename[0] >= 'A' && filename[0] <= 'Z')) &&
			filename[1] == ':' && filename[2] == '\0') {
			snprintf(device_name, sizeof(device_name), "\\\\.\\%c:", filename[0]);
			filename = device_name;
		}
	}
	s->type = find_device_type(bs, filename);

	if ((flags & BDRV_O_ACCESS) == O_RDWR) {
		access_flags = GENERIC_READ | GENERIC_WRITE;
	}
	else {
		access_flags = GENERIC_READ;
	}
	create_flags = OPEN_EXISTING;

	overlapped = FILE_ATTRIBUTE_NORMAL;
	if ((flags & BDRV_O_NOCACHE))
		overlapped |= FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH;
	else if (!(flags & BDRV_O_CACHE_WB))
		overlapped |= FILE_FLAG_WRITE_THROUGH;
	s->hfile = CreateFile(filename, access_flags,
		FILE_SHARE_READ, NULL,
		create_flags, overlapped, NULL);
	if (s->hfile == INVALID_HANDLE_VALUE) {
		int err = GetLastError();

		if (err == ERROR_ACCESS_DENIED)
			return -EACCES;
		return -1;
	}
	return 0;
}

#if 0
/***********************************************/
/* removable device additional commands */

static int raw_is_inserted(BlockDriverState *bs)
{
	return 1;
}

static int raw_media_changed(BlockDriverState *bs)
{
	return -ENOTSUP;
}

static int raw_eject(BlockDriverState *bs, int eject_flag)
{
	DWORD ret_count;

	if (s->type == FTYPE_FILE)
		return -ENOTSUP;
	if (eject_flag) {
		DeviceIoControl(s->hfile, IOCTL_STORAGE_EJECT_MEDIA,
			NULL, 0, NULL, 0, &lpBytesReturned, NULL);
	} else {
		DeviceIoControl(s->hfile, IOCTL_STORAGE_LOAD_MEDIA,
			NULL, 0, NULL, 0, &lpBytesReturned, NULL);
	}
}

static int raw_set_locked(BlockDriverState *bs, int locked)
{
	return -ENOTSUP;
}
#endif

#ifndef _MSC_VER
BlockDriver bdrv_host_device = {
	.format_name	= "host_device",
	.instance_size	= sizeof(BDRVRawState),
	.bdrv_open		= hdev_open,
	.bdrv_close		= raw_close,
	.bdrv_flush		= raw_flush,

	.bdrv_read		= raw_read,
	.bdrv_write	        = raw_write,
	.bdrv_getlength	= raw_getlength,
};
#else
BlockDriver bdrv_host_device = {
	"host_device",					//const char *format_name;
	sizeof(BDRVRawState),	//int instance_size;
	NULL,					//int (*bdrv_probe)(const uint8_t *buf, int buf_size, const char *filename);
	hdev_open,				//int (*bdrv_open)(BlockDriverState *bs, const char *filename, int flags);
	raw_read,				//int (*bdrv_read)(BlockDriverState *bs, int64_t sector_num, uint8_t *buf, int nb_sectors);
	raw_write,				//int (*bdrv_write)(BlockDriverState *bs, int64_t sector_num,	const uint8_t *buf, int nb_sectors);
	raw_close,				//void (*bdrv_close)(BlockDriverState *bs);
	NULL,					//int (*bdrv_create)(const char *filename, int64_t total_sectors, const char *backing_file, int flags);
	raw_flush,					//void (*bdrv_flush)(BlockDriverState *bs);
	NULL,					//int (*bdrv_is_allocated)(BlockDriverState *bs, int64_t sector_num, int nb_sectors, int *pnum);
	NULL,					//int (*bdrv_set_key)(BlockDriverState *bs, const char *key);
	NULL,					//int (*bdrv_make_empty)(BlockDriverState *bs);
	NULL,
	NULL,
	NULL,
	0,

	NULL,					//const char *protocol_name;
	NULL,					//int (*bdrv_pread)(BlockDriverState *bs, int64_t offset, uint8_t *buf, int count);
	raw_getlength,					//int (*bdrv_pwrite)(BlockDriverState *bs, int64_t offset, const uint8_t *buf, int count);
	NULL,					//int (*bdrv_truncate)(BlockDriverState *bs, int64_t offset);
	NULL,			//int64_t (*bdrv_getlength)(BlockDriverState *bs);
	NULL,					//int (*bdrv_write_compressed)(BlockDriverState *bs, int64_t sector_num, const uint8_t *buf, int nb_sectors);

	NULL,					//int (*bdrv_snapshot_create)(BlockDriverState *bs, QEMUSnapshotInfo *sn_info);
	NULL,					//int (*bdrv_snapshot_goto)(BlockDriverState *bs, const char *snapshot_id);
	NULL,					//int (*bdrv_snapshot_delete)(BlockDriverState *bs, const char *snapshot_id);
	NULL,					//int (*bdrv_snapshot_list)(BlockDriverState *bs, QEMUSnapshotInfo **psn_info);
	NULL,					//int (*bdrv_get_info)(BlockDriverState *bs, BlockDriverInfo *bdi);

	/* removable device specific */
	NULL,					//int (*bdrv_is_inserted)(BlockDriverState *bs);
	NULL,					//int (*bdrv_media_changed)(BlockDriverState *bs);
	NULL,					//int (*bdrv_eject)(BlockDriverState *bs, int eject_flag);
	NULL,					//int (*bdrv_set_locked)(BlockDriverState *bs, int locked);

	/* to control generic scsi devices */
	NULL,					//int (*bdrv_ioctl)(BlockDriverState *bs, unsigned long int req, void *buf);

	NULL,					//BlockDriverAIOCB *free_aiocb;
	NULL					//struct BlockDriver *next;
};
#endif