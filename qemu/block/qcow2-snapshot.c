/*
 * Block driver for the QCOW version 2 format
 *
 * Copyright (c) 2004-2006 Fabrice Bellard
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

#include "qemu-common.h"
#include "block_int.h"
#include "block/qcow2.h"

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif

#ifndef _MSC_VER
typedef struct __attribute__((packed)) QCowSnapshotHeader {
#else
typedef struct QCowSnapshotHeader {
#endif
    /* header is 8 byte aligned */
    uint64_t l1_table_offset;

    uint32_t l1_size;
    uint16_t id_str_size;
    uint16_t name_size;

    uint32_t date_sec;
    uint32_t date_nsec;

    uint64_t vm_clock_nsec;

    uint32_t vm_state_size;
    uint32_t extra_data_size; /* for extension */
    /* extra data follows */
    /* id_str follows */
    /* name follows  */
} QCowSnapshotHeader;

#ifdef _MSC_VER
#pragma pack (pop)
#endif

void qcow2_free_snapshots(BlockDriverState *bs)
{
    BDRVQcowState *s = bs->opaque;
    int i;

    for(i = 0; i < s->nb_snapshots; i++) {
        qemu_free(s->snapshots[i].name);
        qemu_free(s->snapshots[i].id_str);
    }
    qemu_free(s->snapshots);
    s->snapshots = NULL;
    s->nb_snapshots = 0;
}

int qcow2_read_snapshots(BlockDriverState *bs)
{
    BDRVQcowState *s = bs->opaque;
    QCowSnapshotHeader h;
    QCowSnapshot *sn;
    int i, id_str_size, name_size;
    int64_t offset;
    uint32_t extra_data_size;

    if (!s->nb_snapshots) {
        s->snapshots = NULL;
        s->snapshots_size = 0;
        return 0;
    }

    offset = s->snapshots_offset;
    s->snapshots = qemu_mallocz(s->nb_snapshots * sizeof(QCowSnapshot));
    for(i = 0; i < s->nb_snapshots; i++) {
        offset = align_offset(offset, 8);
        if (bdrv_pread(bs->file, offset, &h, sizeof(h)) != sizeof(h))
            goto fail;
        offset += sizeof(h);
        sn = s->snapshots + i;
        sn->l1_table_offset = be64_to_cpu(h.l1_table_offset);
        sn->l1_size = be32_to_cpu(h.l1_size);
        sn->vm_state_size = be32_to_cpu(h.vm_state_size);
        sn->date_sec = be32_to_cpu(h.date_sec);
        sn->date_nsec = be32_to_cpu(h.date_nsec);
        sn->vm_clock_nsec = be64_to_cpu(h.vm_clock_nsec);
        extra_data_size = be32_to_cpu(h.extra_data_size);

        id_str_size = be16_to_cpu(h.id_str_size);
        name_size = be16_to_cpu(h.name_size);

        offset += extra_data_size;

        sn->id_str = qemu_malloc(id_str_size + 1);
        if (bdrv_pread(bs->file, offset, sn->id_str, id_str_size) != id_str_size)
            goto fail;
        offset += id_str_size;
        sn->id_str[id_str_size] = '\0';

        sn->name = qemu_malloc(name_size + 1);
        if (bdrv_pread(bs->file, offset, sn->name, name_size) != name_size)
            goto fail;
        offset += name_size;
        sn->name[name_size] = '\0';
    }
    s->snapshots_size = offset - s->snapshots_offset;
    return 0;
 fail:
    qcow2_free_snapshots(bs);
    return -1;
}

/* add at the end of the file a new list of snapshots */
static int qcow_write_snapshots(BlockDriverState *bs)
{
    BDRVQcowState *s = bs->opaque;
    QCowSnapshot *sn;
    QCowSnapshotHeader h;
    int i, name_size, id_str_size, snapshots_size;
    uint64_t data64;
    uint32_t data32;
    int64_t offset, snapshots_offset;

    /* compute the size of the snapshots */
    offset = 0;
    for(i = 0; i < s->nb_snapshots; i++) {
        sn = s->snapshots + i;
        offset = align_offset(offset, 8);
        offset += sizeof(h);
        offset += strlen(sn->id_str);
        offset += strlen(sn->name);
    }
    snapshots_size = offset;

    snapshots_offset = qcow2_alloc_clusters(bs, snapshots_size);
    offset = snapshots_offset;
    if (offset < 0) {
        return offset;
    }

    for(i = 0; i < s->nb_snapshots; i++) {
        sn = s->snapshots + i;
        memset(&h, 0, sizeof(h));
        h.l1_table_offset = cpu_to_be64(sn->l1_table_offset);
        h.l1_size = cpu_to_be32(sn->l1_size);
        h.vm_state_size = cpu_to_be32(sn->vm_state_size);
        h.date_sec = cpu_to_be32(sn->date_sec);
        h.date_nsec = cpu_to_be32(sn->date_nsec);
        h.vm_clock_nsec = cpu_to_be64(sn->vm_clock_nsec);

        id_str_size = strlen(sn->id_str);
        name_size = strlen(sn->name);
        h.id_str_size = cpu_to_be16(id_str_size);
        h.name_size = cpu_to_be16(name_size);
        offset = align_offset(offset, 8);
        if (bdrv_pwrite(bs->file, offset, &h, sizeof(h)) != sizeof(h))
            goto fail;
        offset += sizeof(h);
        if (bdrv_pwrite(bs->file, offset, sn->id_str, id_str_size) != id_str_size)
            goto fail;
        offset += id_str_size;
        if (bdrv_pwrite(bs->file, offset, sn->name, name_size) != name_size)
            goto fail;
        offset += name_size;
    }

    /* update the various header fields */
    data64 = cpu_to_be64(snapshots_offset);
    if (bdrv_pwrite(bs->file, offsetof(QCowHeader, snapshots_offset),
                    &data64, sizeof(data64)) != sizeof(data64))
        goto fail;
    data32 = cpu_to_be32(s->nb_snapshots);
    if (bdrv_pwrite(bs->file, offsetof(QCowHeader, nb_snapshots),
                    &data32, sizeof(data32)) != sizeof(data32))
        goto fail;

    /* free the old snapshot table */
    qcow2_free_clusters(bs, s->snapshots_offset, s->snapshots_size);
    s->snapshots_offset = snapshots_offset;
    s->snapshots_size = snapshots_size;
    return 0;
 fail:
    return -1;
}

static void find_new_snapshot_id(BlockDriverState *bs,
                                 char *id_str, int id_str_size)
{
    BDRVQcowState *s = bs->opaque;
    QCowSnapshot *sn;
    int i, id, id_max = 0;

    for(i = 0; i < s->nb_snapshots; i++) {
        sn = s->snapshots + i;
        id = strtoul(sn->id_str, NULL, 10);
        if (id > id_max)
            id_max = id;
    }
    snprintf(id_str, id_str_size, "%d", id_max + 1);
}

static int find_snapshot_by_id(BlockDriverState *bs, const char *id_str)
{
    BDRVQcowState *s = bs->opaque;
    int i;

    for(i = 0; i < s->nb_snapshots; i++) {
        if (!strcmp(s->snapshots[i].id_str, id_str))
            return i;
    }
    return -1;
}

static int find_snapshot_by_id_or_name(BlockDriverState *bs, const char *name)
{
    BDRVQcowState *s = bs->opaque;
    int i, ret;

    ret = find_snapshot_by_id(bs, name);
    if (ret >= 0)
        return ret;
    for(i = 0; i < s->nb_snapshots; i++) {
        if (!strcmp(s->snapshots[i].name, name))
            return i;
    }
    return -1;
}

/* if no id is provided, a new one is constructed */
int qcow2_snapshot_create(BlockDriverState *bs, QEMUSnapshotInfo *sn_info)
{
    BDRVQcowState *s = bs->opaque;
    QCowSnapshot *snapshots1, sn1, *sn = &sn1;
    int i, ret;
    uint64_t *l1_table = NULL;
    int64_t l1_table_offset;

    memset(sn, 0, sizeof(*sn));

    if (sn_info->id_str[0] == '\0') {
        /* compute a new id */
        find_new_snapshot_id(bs, sn_info->id_str, sizeof(sn_info->id_str));
    }

    /* check that the ID is unique */
    if (find_snapshot_by_id(bs, sn_info->id_str) >= 0)
        return -ENOENT;

    sn->id_str = qemu_strdup(sn_info->id_str);
    if (!sn->id_str)
        goto fail;
    sn->name = qemu_strdup(sn_info->name);
    if (!sn->name)
        goto fail;
    sn->vm_state_size = sn_info->vm_state_size;
    sn->date_sec = sn_info->date_sec;
    sn->date_nsec = sn_info->date_nsec;
    sn->vm_clock_nsec = sn_info->vm_clock_nsec;

    ret = qcow2_update_snapshot_refcount(bs, s->l1_table_offset, s->l1_size, 1);
    if (ret < 0)
        goto fail;

    /* create the L1 table of the snapshot */
    l1_table_offset = qcow2_alloc_clusters(bs, s->l1_size * sizeof(uint64_t));
    if (l1_table_offset < 0) {
        goto fail;
    }

    sn->l1_table_offset = l1_table_offset;
    sn->l1_size = s->l1_size;

    if (s->l1_size != 0) {
        l1_table = qemu_malloc(s->l1_size * sizeof(uint64_t));
    } else {
        l1_table = NULL;
    }

    for(i = 0; i < s->l1_size; i++) {
        l1_table[i] = cpu_to_be64(s->l1_table[i]);
    }
    if (bdrv_pwrite(bs->file, sn->l1_table_offset,
                    l1_table, s->l1_size * sizeof(uint64_t)) !=
        (s->l1_size * sizeof(uint64_t)))
        goto fail;
    qemu_free(l1_table);
    l1_table = NULL;

    snapshots1 = qemu_malloc((s->nb_snapshots + 1) * sizeof(QCowSnapshot));
    if (s->snapshots) {
        memcpy(snapshots1, s->snapshots, s->nb_snapshots * sizeof(QCowSnapshot));
        qemu_free(s->snapshots);
    }
    s->snapshots = snapshots1;
    s->snapshots[s->nb_snapshots++] = *sn;

    if (qcow_write_snapshots(bs) < 0)
        goto fail;
#ifdef DEBUG_ALLOC
    qcow2_check_refcounts(bs);
#endif
    return 0;
 fail:
    qemu_free(sn->name);
    qemu_free(l1_table);
    return -1;
}

/* copy the snapshot 'snapshot_name' into the current disk image */
int qcow2_snapshot_goto(BlockDriverState *bs, const char *snapshot_id)
{
    BDRVQcowState *s = bs->opaque;
    QCowSnapshot *sn;
    int i, snapshot_index, l1_size2;

    snapshot_index = find_snapshot_by_id_or_name(bs, snapshot_id);
    if (snapshot_index < 0)
        return -ENOENT;
    sn = &s->snapshots[snapshot_index];

    if (qcow2_update_snapshot_refcount(bs, s->l1_table_offset, s->l1_size, -1) < 0)
        goto fail;

    if (qcow2_grow_l1_table(bs, sn->l1_size) < 0)
        goto fail;

    s->l1_size = sn->l1_size;
    l1_size2 = s->l1_size * sizeof(uint64_t);
    /* copy the snapshot l1 table to the current l1 table */
    if (bdrv_pread(bs->file, sn->l1_table_offset,
                   s->l1_table, l1_size2) != l1_size2)
        goto fail;
    if (bdrv_pwrite(bs->file, s->l1_table_offset,
                    s->l1_table, l1_size2) != l1_size2)
        goto fail;
    for(i = 0;i < s->l1_size; i++) {
        be64_to_cpus(&s->l1_table[i]);
    }

    if (qcow2_update_snapshot_refcount(bs, s->l1_table_offset, s->l1_size, 1) < 0)
        goto fail;

#ifdef DEBUG_ALLOC
    qcow2_check_refcounts(bs);
#endif
    return 0;
 fail:
    return -EIO;
}

int qcow2_snapshot_delete(BlockDriverState *bs, const char *snapshot_id)
{
    BDRVQcowState *s = bs->opaque;
    QCowSnapshot *sn;
    int snapshot_index, ret;

    snapshot_index = find_snapshot_by_id_or_name(bs, snapshot_id);
    if (snapshot_index < 0)
        return -ENOENT;
    sn = &s->snapshots[snapshot_index];

    ret = qcow2_update_snapshot_refcount(bs, sn->l1_table_offset, sn->l1_size, -1);
    if (ret < 0)
        return ret;
    /* must update the copied flag on the current cluster offsets */
    ret = qcow2_update_snapshot_refcount(bs, s->l1_table_offset, s->l1_size, 0);
    if (ret < 0)
        return ret;
    qcow2_free_clusters(bs, sn->l1_table_offset, sn->l1_size * sizeof(uint64_t));

    qemu_free(sn->id_str);
    qemu_free(sn->name);
    memmove(sn, sn + 1, (s->nb_snapshots - snapshot_index - 1) * sizeof(*sn));
    s->nb_snapshots--;
    ret = qcow_write_snapshots(bs);
    if (ret < 0) {
        /* XXX: restore snapshot if error ? */
        return ret;
    }
#ifdef DEBUG_ALLOC
    qcow2_check_refcounts(bs);
#endif
    return 0;
}

int qcow2_snapshot_list(BlockDriverState *bs, QEMUSnapshotInfo **psn_tab)
{
    BDRVQcowState *s = bs->opaque;
    QEMUSnapshotInfo *sn_tab, *sn_info;
    QCowSnapshot *sn;
    int i;

    if (!s->nb_snapshots) {
        *psn_tab = NULL;
        return s->nb_snapshots;
    }

    sn_tab = qemu_mallocz(s->nb_snapshots * sizeof(QEMUSnapshotInfo));
    for(i = 0; i < s->nb_snapshots; i++) {
        sn_info = sn_tab + i;
        sn = s->snapshots + i;
        pstrcpy(sn_info->id_str, sizeof(sn_info->id_str),
                sn->id_str);
        pstrcpy(sn_info->name, sizeof(sn_info->name),
                sn->name);
        sn_info->vm_state_size = sn->vm_state_size;
        sn_info->date_sec = sn->date_sec;
        sn_info->date_nsec = sn->date_nsec;
        sn_info->vm_clock_nsec = sn->vm_clock_nsec;
    }
    *psn_tab = sn_tab;
    return s->nb_snapshots;
}

