/*
 * ACPI implementation
 *
 * Copyright (c) 2006 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>
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
 

#include "hw.h"
#include "pc.h"
#include "acpi.h"

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif

struct acpi_table_header
{
    char signature [4];    /* ACPI signature (4 ASCII characters) */
    uint32_t length;          /* Length of table, in bytes, including header */
    uint8_t revision;         /* ACPI Specification minor version # */
    uint8_t checksum;         /* To make sum of entire table == 0 */
    char oem_id [6];       /* OEM identification */
    char oem_table_id [8]; /* OEM table identification */
    uint32_t oem_revision;    /* OEM revision number */
    char asl_compiler_id [4]; /* ASL compiler vendor ID */
    uint32_t asl_compiler_revision; /* ASL compiler revision number */
} 
#ifndef _MSC_VER
__attribute__((packed));
#else
;
#endif

#ifdef _MSC_VER
#pragma pack (pop)
#endif

char *acpi_tables;
size_t acpi_tables_len;

static int acpi_checksum(const uint8_t *data, int len)
{
    int sum, i;
    sum = 0;
    for(i = 0; i < len; i++)
        sum += data[i];
    return (-sum) & 0xff;
}

int acpi_table_add(const char *t)
{
    static const char *dfl_id = "QEMUQEMU";
    char buf[1024], *p, *f;
    struct acpi_table_header acpi_hdr;
    unsigned long val;
    size_t off;

    memset(&acpi_hdr, 0, sizeof(acpi_hdr));
  
    if (get_param_value(buf, sizeof(buf), "sig", t)) {
        strncpy(acpi_hdr.signature, buf, 4);
    } else {
        strncpy(acpi_hdr.signature, dfl_id, 4);
    }
    if (get_param_value(buf, sizeof(buf), "rev", t)) {
        val = strtoul(buf, &p, 10);
        if (val > 255 || *p != '\0')
            goto out;
    } else {
        val = 1;
    }
    acpi_hdr.revision = (int8_t)val;

    if (get_param_value(buf, sizeof(buf), "oem_id", t)) {
        strncpy(acpi_hdr.oem_id, buf, 6);
    } else {
        strncpy(acpi_hdr.oem_id, dfl_id, 6);
    }

    if (get_param_value(buf, sizeof(buf), "oem_table_id", t)) {
        strncpy(acpi_hdr.oem_table_id, buf, 8);
    } else {
        strncpy(acpi_hdr.oem_table_id, dfl_id, 8);
    }

    if (get_param_value(buf, sizeof(buf), "oem_rev", t)) {
        val = strtol(buf, &p, 10);
        if(*p != '\0')
            goto out;
    } else {
        val = 1;
    }
    acpi_hdr.oem_revision = cpu_to_le32(val);

    if (get_param_value(buf, sizeof(buf), "asl_compiler_id", t)) {
        strncpy(acpi_hdr.asl_compiler_id, buf, 4);
    } else {
        strncpy(acpi_hdr.asl_compiler_id, dfl_id, 4);
    }

    if (get_param_value(buf, sizeof(buf), "asl_compiler_rev", t)) {
        val = strtol(buf, &p, 10);
        if(*p != '\0')
            goto out;
    } else {
        val = 1;
    }
    acpi_hdr.asl_compiler_revision = cpu_to_le32(val);
    
    if (!get_param_value(buf, sizeof(buf), "data", t)) {
         buf[0] = '\0';
    }

    acpi_hdr.length = sizeof(acpi_hdr);

    f = buf;
    while (buf[0]) {
        struct stat s;
        char *n = strchr(f, ':');
        if (n)
            *n = '\0';
        if(stat(f, &s) < 0) {
            fprintf(stderr, "Can't stat file '%s': %s\n", f, strerror(errno));
            goto out;
        }
        acpi_hdr.length += s.st_size;
        if (!n)
            break;
        *n = ':';
        f = n + 1;
    }

    if (!acpi_tables) {
        acpi_tables_len = sizeof(uint16_t);
        acpi_tables = qemu_mallocz(acpi_tables_len);
    }
    p = acpi_tables + acpi_tables_len;
    acpi_tables_len += sizeof(uint16_t) + acpi_hdr.length;
    acpi_tables = qemu_realloc(acpi_tables, acpi_tables_len);

    acpi_hdr.length = cpu_to_le32(acpi_hdr.length);
    *(uint16_t*)p = acpi_hdr.length;
    p += sizeof(uint16_t);
    memcpy(p, &acpi_hdr, sizeof(acpi_hdr));
    off = sizeof(acpi_hdr);

    f = buf;
    while (buf[0]) {
        struct stat s;
        int fd;
        char *n = strchr(f, ':');
        if (n)
            *n = '\0';
        fd = open(f, O_RDONLY);

        if(fd < 0)
            goto out;
        if(fstat(fd, &s) < 0) {
            close(fd);
            goto out;
        }

        do {
            int r;
            r = read(fd, p + off, s.st_size);
            if (r > 0) {
                off += r;
                s.st_size -= r;
            } else if ((r < 0 && errno != EINTR) || r == 0) {
                close(fd);
                goto out;
            }
        } while(s.st_size);

        close(fd);
        if (!n)
            break;
        f = n + 1;
    }

    ((struct acpi_table_header*)p)->checksum = acpi_checksum((uint8_t*)p, off);
    /* increase number of tables */
    (*(uint16_t*)acpi_tables) =
	    cpu_to_le32(le32_to_cpu(*(uint16_t*)acpi_tables) + 1);
    return 0;
out:
    if (acpi_tables) {
        qemu_free(acpi_tables);
        acpi_tables = NULL;
    }
    return -1;
}
