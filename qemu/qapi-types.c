/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * deallocation functions for schema-defined QAPI types
 *
 * Copyright IBM, Corp. 2011
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *  Michael Roth      <mdroth@linux.vnet.ibm.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 *
 */

#include "qapi/qapi-dealloc-visitor.h"
#include "qapi-types.h"
#include "qapi-visit.h"

const char *ErrorClass_lookup[] = {
    "GenericError",
    "CommandNotFound",
    "DeviceEncrypted",
    "DeviceNotActive",
    "DeviceNotFound",
    "KVMMissingCap",
    "MigrationExpected",
    NULL,
};

const char *RunState_lookup[] = {
    "debug",
    "inmigrate",
    "internal-error",
    "io-error",
    "paused",
    "postmigrate",
    "prelaunch",
    "finish-migrate",
    "restore-vm",
    "running",
    "save-vm",
    "shutdown",
    "suspended",
    "watchdog",
    NULL,
};

const char *MigrationCapability_lookup[] = {
    "xbzrle",
    NULL,
};

const char *BlockDeviceIoStatus_lookup[] = {
    "ok",
    "failed",
    "nospace",
    NULL,
};

const char *SpiceQueryMouseMode_lookup[] = {
    "client",
    "server",
    "unknown",
    NULL,
};

const char *NewImageMode_lookup[] = {
    "existing",
    "absolute-paths",
    NULL,
};

const char *BlockdevActionKind_lookup[] = {
    "blockdev-snapshot-sync",
    NULL,
};

const char *NetClientOptionsKind_lookup[] = {
    "none",
    "nic",
    "user",
    "tap",
    "socket",
    "vde",
    "dump",
    "bridge",
    "hubport",
    NULL,
};

const char *TargetType_lookup[] = {
    "alpha",
    "arm",
    "cris",
    "i386",
    "lm32",
    "m68k",
    "microblazeel",
    "microblaze",
    "mips64el",
    "mips64",
    "mipsel",
    "mips",
    "or32",
    "ppc64",
    "ppcemb",
    "ppc",
    "s390x",
    "sh4eb",
    "sh4",
    "sparc64",
    "sparc",
    "unicore32",
    "x86_64",
    "xtensaeb",
    "xtensa",
    NULL,
};

void qapi_free_NameInfoList(NameInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NameInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NameInfo(NameInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NameInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_VersionInfoList(VersionInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_VersionInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_VersionInfo(VersionInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_VersionInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_KvmInfoList(KvmInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_KvmInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_KvmInfo(KvmInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_KvmInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_StatusInfoList(StatusInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_StatusInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_StatusInfo(StatusInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_StatusInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_UuidInfoList(UuidInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_UuidInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_UuidInfo(UuidInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_UuidInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_ChardevInfoList(ChardevInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_ChardevInfo(ChardevInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_CommandInfoList(CommandInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CommandInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_CommandInfo(CommandInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CommandInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_EventInfoList(EventInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_EventInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_EventInfo(EventInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_EventInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_MigrationStatsList(MigrationStatsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MigrationStatsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_MigrationStats(MigrationStats * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MigrationStats(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_XBZRLECacheStatsList(XBZRLECacheStatsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_XBZRLECacheStatsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_XBZRLECacheStats(XBZRLECacheStats * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_XBZRLECacheStats(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_MigrationInfoList(MigrationInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MigrationInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_MigrationInfo(MigrationInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MigrationInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_MigrationCapabilityStatusList(MigrationCapabilityStatusList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MigrationCapabilityStatusList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_MigrationCapabilityStatus(MigrationCapabilityStatus * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MigrationCapabilityStatus(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_MouseInfoList(MouseInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MouseInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_MouseInfo(MouseInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MouseInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_CpuInfoList(CpuInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CpuInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_CpuInfo(CpuInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CpuInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockDeviceInfoList(BlockDeviceInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockDeviceInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockDeviceInfo(BlockDeviceInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockDeviceInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockInfoList(BlockInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockInfo(BlockInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockDeviceStatsList(BlockDeviceStatsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockDeviceStatsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockDeviceStats(BlockDeviceStats * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockDeviceStats(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockStatsList(BlockStatsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockStatsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockStats(BlockStats * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockStats(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_VncClientInfoList(VncClientInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_VncClientInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_VncClientInfo(VncClientInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_VncClientInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_VncInfoList(VncInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_VncInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_VncInfo(VncInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_VncInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_SpiceChannelList(SpiceChannelList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_SpiceChannelList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_SpiceChannel(SpiceChannel * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_SpiceChannel(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_SpiceInfoList(SpiceInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_SpiceInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_SpiceInfo(SpiceInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_SpiceInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BalloonInfoList(BalloonInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BalloonInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BalloonInfo(BalloonInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BalloonInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_PciMemoryRangeList(PciMemoryRangeList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciMemoryRangeList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_PciMemoryRange(PciMemoryRange * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciMemoryRange(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_PciMemoryRegionList(PciMemoryRegionList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciMemoryRegionList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_PciMemoryRegion(PciMemoryRegion * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciMemoryRegion(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_PciBridgeInfoList(PciBridgeInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciBridgeInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_PciBridgeInfo(PciBridgeInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciBridgeInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_PciDeviceInfoList(PciDeviceInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciDeviceInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_PciDeviceInfo(PciDeviceInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciDeviceInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_PciInfoList(PciInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_PciInfo(PciInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockJobInfoList(BlockJobInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockJobInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockJobInfo(BlockJobInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockJobInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockdevSnapshotList(BlockdevSnapshotList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevSnapshotList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockdevSnapshot(BlockdevSnapshot * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevSnapshot(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockdevActionList(BlockdevActionList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevActionList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_BlockdevAction(BlockdevAction * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevAction(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_ObjectPropertyInfoList(ObjectPropertyInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ObjectPropertyInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_ObjectPropertyInfo(ObjectPropertyInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ObjectPropertyInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_ObjectTypeInfoList(ObjectTypeInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ObjectTypeInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_ObjectTypeInfo(ObjectTypeInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ObjectTypeInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_DevicePropertyInfoList(DevicePropertyInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_DevicePropertyInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_DevicePropertyInfo(DevicePropertyInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_DevicePropertyInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevNoneOptionsList(NetdevNoneOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevNoneOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevNoneOptions(NetdevNoneOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevNoneOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetLegacyNicOptionsList(NetLegacyNicOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetLegacyNicOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetLegacyNicOptions(NetLegacyNicOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetLegacyNicOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_StringList(StringList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_StringList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_String(String * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_String(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevUserOptionsList(NetdevUserOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevUserOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevUserOptions(NetdevUserOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevUserOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevTapOptionsList(NetdevTapOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevTapOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevTapOptions(NetdevTapOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevTapOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevSocketOptionsList(NetdevSocketOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevSocketOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevSocketOptions(NetdevSocketOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevSocketOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevVdeOptionsList(NetdevVdeOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevVdeOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevVdeOptions(NetdevVdeOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevVdeOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevDumpOptionsList(NetdevDumpOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevDumpOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevDumpOptions(NetdevDumpOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevDumpOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevBridgeOptionsList(NetdevBridgeOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevBridgeOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevBridgeOptions(NetdevBridgeOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevBridgeOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevHubPortOptionsList(NetdevHubPortOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevHubPortOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevHubPortOptions(NetdevHubPortOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevHubPortOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetClientOptionsList(NetClientOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetClientOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetClientOptions(NetClientOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetClientOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetLegacyList(NetLegacyList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetLegacyList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetLegacy(NetLegacy * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetLegacy(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_NetdevList(NetdevList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_Netdev(Netdev * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_Netdev(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_MachineInfoList(MachineInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MachineInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_MachineInfo(MachineInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MachineInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_CpuDefinitionInfoList(CpuDefinitionInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CpuDefinitionInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_CpuDefinitionInfo(CpuDefinitionInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CpuDefinitionInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_AddfdInfoList(AddfdInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_AddfdInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_AddfdInfo(AddfdInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_AddfdInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_FdsetFdInfoList(FdsetFdInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_FdsetFdInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_FdsetFdInfo(FdsetFdInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_FdsetFdInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_FdsetInfoList(FdsetInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_FdsetInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_FdsetInfo(FdsetInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_FdsetInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_TargetInfoList(TargetInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TargetInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_TargetInfo(TargetInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TargetInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

