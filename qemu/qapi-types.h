/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * schema-defined QAPI types
 *
 * Copyright IBM, Corp. 2011
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 *
 */

#ifndef QAPI_TYPES_H
#define QAPI_TYPES_H

#include <stdbool.h>
#include <stdint.h>


extern const char *ErrorClass_lookup[];
typedef enum ErrorClass
{
    ERROR_CLASS_GENERIC_ERROR = 0,
    ERROR_CLASS_COMMAND_NOT_FOUND = 1,
    ERROR_CLASS_DEVICE_ENCRYPTED = 2,
    ERROR_CLASS_DEVICE_NOT_ACTIVE = 3,
    ERROR_CLASS_DEVICE_NOT_FOUND = 4,
    ERROR_CLASS_K_V_M_MISSING_CAP = 5,
    ERROR_CLASS_MAX = 6,
} ErrorClass;

typedef struct ErrorClassList
{
    ErrorClass value;
    struct ErrorClassList *next;
} ErrorClassList;

typedef struct NameInfo NameInfo;

typedef struct NameInfoList
{
    NameInfo *value;
    struct NameInfoList *next;
} NameInfoList;

typedef struct VersionInfo VersionInfo;

typedef struct VersionInfoList
{
    VersionInfo *value;
    struct VersionInfoList *next;
} VersionInfoList;

typedef struct KvmInfo KvmInfo;

typedef struct KvmInfoList
{
    KvmInfo *value;
    struct KvmInfoList *next;
} KvmInfoList;

extern const char *RunState_lookup[];
typedef enum RunState
{
    RUN_STATE_DEBUG = 0,
    RUN_STATE_INMIGRATE = 1,
    RUN_STATE_INTERNAL_ERROR = 2,
    RUN_STATE_IO_ERROR = 3,
    RUN_STATE_PAUSED = 4,
    RUN_STATE_POSTMIGRATE = 5,
    RUN_STATE_PRELAUNCH = 6,
    RUN_STATE_FINISH_MIGRATE = 7,
    RUN_STATE_RESTORE_VM = 8,
    RUN_STATE_RUNNING = 9,
    RUN_STATE_SAVE_VM = 10,
    RUN_STATE_SHUTDOWN = 11,
    RUN_STATE_SUSPENDED = 12,
    RUN_STATE_WATCHDOG = 13,
    RUN_STATE_MAX = 14,
} RunState;

typedef struct RunStateList
{
    RunState value;
    struct RunStateList *next;
} RunStateList;

typedef struct SnapshotInfo SnapshotInfo;

typedef struct SnapshotInfoList
{
    SnapshotInfo *value;
    struct SnapshotInfoList *next;
} SnapshotInfoList;

typedef struct ImageInfo ImageInfo;

typedef struct ImageInfoList
{
    ImageInfo *value;
    struct ImageInfoList *next;
} ImageInfoList;

typedef struct StatusInfo StatusInfo;

typedef struct StatusInfoList
{
    StatusInfo *value;
    struct StatusInfoList *next;
} StatusInfoList;

typedef struct UuidInfo UuidInfo;

typedef struct UuidInfoList
{
    UuidInfo *value;
    struct UuidInfoList *next;
} UuidInfoList;

typedef struct ChardevInfo ChardevInfo;

typedef struct ChardevInfoList
{
    ChardevInfo *value;
    struct ChardevInfoList *next;
} ChardevInfoList;

extern const char *DataFormat_lookup[];
typedef enum DataFormat
{
    DATA_FORMAT_UTF8 = 0,
    DATA_FORMAT_BASE64 = 1,
    DATA_FORMAT_MAX = 2,
} DataFormat;

typedef struct DataFormatList
{
    DataFormat value;
    struct DataFormatList *next;
} DataFormatList;

typedef struct CommandInfo CommandInfo;

typedef struct CommandInfoList
{
    CommandInfo *value;
    struct CommandInfoList *next;
} CommandInfoList;

typedef struct EventInfo EventInfo;

typedef struct EventInfoList
{
    EventInfo *value;
    struct EventInfoList *next;
} EventInfoList;

typedef struct MigrationStats MigrationStats;

typedef struct MigrationStatsList
{
    MigrationStats *value;
    struct MigrationStatsList *next;
} MigrationStatsList;

typedef struct XBZRLECacheStats XBZRLECacheStats;

typedef struct XBZRLECacheStatsList
{
    XBZRLECacheStats *value;
    struct XBZRLECacheStatsList *next;
} XBZRLECacheStatsList;

typedef struct MigrationInfo MigrationInfo;

typedef struct MigrationInfoList
{
    MigrationInfo *value;
    struct MigrationInfoList *next;
} MigrationInfoList;

extern const char *MigrationCapability_lookup[];
typedef enum MigrationCapability
{
    MIGRATION_CAPABILITY_XBZRLE = 0,
    MIGRATION_CAPABILITY_MAX = 1,
} MigrationCapability;

typedef struct MigrationCapabilityList
{
    MigrationCapability value;
    struct MigrationCapabilityList *next;
} MigrationCapabilityList;

typedef struct MigrationCapabilityStatus MigrationCapabilityStatus;

typedef struct MigrationCapabilityStatusList
{
    MigrationCapabilityStatus *value;
    struct MigrationCapabilityStatusList *next;
} MigrationCapabilityStatusList;

typedef struct MouseInfo MouseInfo;

typedef struct MouseInfoList
{
    MouseInfo *value;
    struct MouseInfoList *next;
} MouseInfoList;

typedef struct CpuInfo CpuInfo;

typedef struct CpuInfoList
{
    CpuInfo *value;
    struct CpuInfoList *next;
} CpuInfoList;

typedef struct BlockDeviceInfo BlockDeviceInfo;

typedef struct BlockDeviceInfoList
{
    BlockDeviceInfo *value;
    struct BlockDeviceInfoList *next;
} BlockDeviceInfoList;

extern const char *BlockDeviceIoStatus_lookup[];
typedef enum BlockDeviceIoStatus
{
    BLOCK_DEVICE_IO_STATUS_OK = 0,
    BLOCK_DEVICE_IO_STATUS_FAILED = 1,
    BLOCK_DEVICE_IO_STATUS_NOSPACE = 2,
    BLOCK_DEVICE_IO_STATUS_MAX = 3,
} BlockDeviceIoStatus;

typedef struct BlockDeviceIoStatusList
{
    BlockDeviceIoStatus value;
    struct BlockDeviceIoStatusList *next;
} BlockDeviceIoStatusList;

typedef struct BlockDirtyInfo BlockDirtyInfo;

typedef struct BlockDirtyInfoList
{
    BlockDirtyInfo *value;
    struct BlockDirtyInfoList *next;
} BlockDirtyInfoList;

typedef struct BlockInfo BlockInfo;

typedef struct BlockInfoList
{
    BlockInfo *value;
    struct BlockInfoList *next;
} BlockInfoList;

typedef struct BlockDeviceStats BlockDeviceStats;

typedef struct BlockDeviceStatsList
{
    BlockDeviceStats *value;
    struct BlockDeviceStatsList *next;
} BlockDeviceStatsList;

typedef struct BlockStats BlockStats;

typedef struct BlockStatsList
{
    BlockStats *value;
    struct BlockStatsList *next;
} BlockStatsList;

typedef struct VncClientInfo VncClientInfo;

typedef struct VncClientInfoList
{
    VncClientInfo *value;
    struct VncClientInfoList *next;
} VncClientInfoList;

typedef struct VncInfo VncInfo;

typedef struct VncInfoList
{
    VncInfo *value;
    struct VncInfoList *next;
} VncInfoList;

typedef struct SpiceChannel SpiceChannel;

typedef struct SpiceChannelList
{
    SpiceChannel *value;
    struct SpiceChannelList *next;
} SpiceChannelList;

extern const char *SpiceQueryMouseMode_lookup[];
typedef enum SpiceQueryMouseMode
{
    SPICE_QUERY_MOUSE_MODE_CLIENT = 0,
    SPICE_QUERY_MOUSE_MODE_SERVER = 1,
    SPICE_QUERY_MOUSE_MODE_UNKNOWN = 2,
    SPICE_QUERY_MOUSE_MODE_MAX = 3,
} SpiceQueryMouseMode;

typedef struct SpiceQueryMouseModeList
{
    SpiceQueryMouseMode value;
    struct SpiceQueryMouseModeList *next;
} SpiceQueryMouseModeList;

typedef struct SpiceInfo SpiceInfo;

typedef struct SpiceInfoList
{
    SpiceInfo *value;
    struct SpiceInfoList *next;
} SpiceInfoList;

typedef struct BalloonInfo BalloonInfo;

typedef struct BalloonInfoList
{
    BalloonInfo *value;
    struct BalloonInfoList *next;
} BalloonInfoList;

typedef struct PciMemoryRange PciMemoryRange;

typedef struct PciMemoryRangeList
{
    PciMemoryRange *value;
    struct PciMemoryRangeList *next;
} PciMemoryRangeList;

typedef struct PciMemoryRegion PciMemoryRegion;

typedef struct PciMemoryRegionList
{
    PciMemoryRegion *value;
    struct PciMemoryRegionList *next;
} PciMemoryRegionList;

typedef struct PciBridgeInfo PciBridgeInfo;

typedef struct PciBridgeInfoList
{
    PciBridgeInfo *value;
    struct PciBridgeInfoList *next;
} PciBridgeInfoList;

typedef struct PciDeviceInfo PciDeviceInfo;

typedef struct PciDeviceInfoList
{
    PciDeviceInfo *value;
    struct PciDeviceInfoList *next;
} PciDeviceInfoList;

typedef struct PciInfo PciInfo;

typedef struct PciInfoList
{
    PciInfo *value;
    struct PciInfoList *next;
} PciInfoList;

extern const char *BlockdevOnError_lookup[];
typedef enum BlockdevOnError
{
    BLOCKDEV_ON_ERROR_REPORT = 0,
    BLOCKDEV_ON_ERROR_IGNORE = 1,
    BLOCKDEV_ON_ERROR_ENOSPC = 2,
    BLOCKDEV_ON_ERROR_STOP = 3,
    BLOCKDEV_ON_ERROR_MAX = 4,
} BlockdevOnError;

typedef struct BlockdevOnErrorList
{
    BlockdevOnError value;
    struct BlockdevOnErrorList *next;
} BlockdevOnErrorList;

extern const char *MirrorSyncMode_lookup[];
typedef enum MirrorSyncMode
{
    MIRROR_SYNC_MODE_TOP = 0,
    MIRROR_SYNC_MODE_FULL = 1,
    MIRROR_SYNC_MODE_NONE = 2,
    MIRROR_SYNC_MODE_MAX = 3,
} MirrorSyncMode;

typedef struct MirrorSyncModeList
{
    MirrorSyncMode value;
    struct MirrorSyncModeList *next;
} MirrorSyncModeList;

typedef struct BlockJobInfo BlockJobInfo;

typedef struct BlockJobInfoList
{
    BlockJobInfo *value;
    struct BlockJobInfoList *next;
} BlockJobInfoList;

extern const char *NewImageMode_lookup[];
typedef enum NewImageMode
{
    NEW_IMAGE_MODE_EXISTING = 0,
    NEW_IMAGE_MODE_ABSOLUTE_PATHS = 1,
    NEW_IMAGE_MODE_MAX = 2,
} NewImageMode;

typedef struct NewImageModeList
{
    NewImageMode value;
    struct NewImageModeList *next;
} NewImageModeList;

typedef struct BlockdevSnapshot BlockdevSnapshot;

typedef struct BlockdevSnapshotList
{
    BlockdevSnapshot *value;
    struct BlockdevSnapshotList *next;
} BlockdevSnapshotList;

typedef struct BlockdevAction BlockdevAction;

typedef struct BlockdevActionList
{
    BlockdevAction *value;
    struct BlockdevActionList *next;
} BlockdevActionList;

extern const char *BlockdevActionKind_lookup[];
typedef enum BlockdevActionKind
{
    BLOCKDEV_ACTION_KIND_BLOCKDEV_SNAPSHOT_SYNC = 0,
    BLOCKDEV_ACTION_KIND_MAX = 1,
} BlockdevActionKind;

typedef struct ObjectPropertyInfo ObjectPropertyInfo;

typedef struct ObjectPropertyInfoList
{
    ObjectPropertyInfo *value;
    struct ObjectPropertyInfoList *next;
} ObjectPropertyInfoList;

typedef struct ObjectTypeInfo ObjectTypeInfo;

typedef struct ObjectTypeInfoList
{
    ObjectTypeInfo *value;
    struct ObjectTypeInfoList *next;
} ObjectTypeInfoList;

typedef struct DevicePropertyInfo DevicePropertyInfo;

typedef struct DevicePropertyInfoList
{
    DevicePropertyInfo *value;
    struct DevicePropertyInfoList *next;
} DevicePropertyInfoList;

typedef struct NetdevNoneOptions NetdevNoneOptions;

typedef struct NetdevNoneOptionsList
{
    NetdevNoneOptions *value;
    struct NetdevNoneOptionsList *next;
} NetdevNoneOptionsList;

typedef struct NetLegacyNicOptions NetLegacyNicOptions;

typedef struct NetLegacyNicOptionsList
{
    NetLegacyNicOptions *value;
    struct NetLegacyNicOptionsList *next;
} NetLegacyNicOptionsList;

typedef struct String String;

typedef struct StringList
{
    String *value;
    struct StringList *next;
} StringList;

typedef struct NetdevUserOptions NetdevUserOptions;

typedef struct NetdevUserOptionsList
{
    NetdevUserOptions *value;
    struct NetdevUserOptionsList *next;
} NetdevUserOptionsList;

typedef struct NetdevTapOptions NetdevTapOptions;

typedef struct NetdevTapOptionsList
{
    NetdevTapOptions *value;
    struct NetdevTapOptionsList *next;
} NetdevTapOptionsList;

typedef struct NetdevSocketOptions NetdevSocketOptions;

typedef struct NetdevSocketOptionsList
{
    NetdevSocketOptions *value;
    struct NetdevSocketOptionsList *next;
} NetdevSocketOptionsList;

typedef struct NetdevVdeOptions NetdevVdeOptions;

typedef struct NetdevVdeOptionsList
{
    NetdevVdeOptions *value;
    struct NetdevVdeOptionsList *next;
} NetdevVdeOptionsList;

typedef struct NetdevDumpOptions NetdevDumpOptions;

typedef struct NetdevDumpOptionsList
{
    NetdevDumpOptions *value;
    struct NetdevDumpOptionsList *next;
} NetdevDumpOptionsList;

typedef struct NetdevBridgeOptions NetdevBridgeOptions;

typedef struct NetdevBridgeOptionsList
{
    NetdevBridgeOptions *value;
    struct NetdevBridgeOptionsList *next;
} NetdevBridgeOptionsList;

typedef struct NetdevHubPortOptions NetdevHubPortOptions;

typedef struct NetdevHubPortOptionsList
{
    NetdevHubPortOptions *value;
    struct NetdevHubPortOptionsList *next;
} NetdevHubPortOptionsList;

typedef struct NetClientOptions NetClientOptions;

typedef struct NetClientOptionsList
{
    NetClientOptions *value;
    struct NetClientOptionsList *next;
} NetClientOptionsList;

extern const char *NetClientOptionsKind_lookup[];
typedef enum NetClientOptionsKind
{
    NET_CLIENT_OPTIONS_KIND_NONE = 0,
    NET_CLIENT_OPTIONS_KIND_NIC = 1,
    NET_CLIENT_OPTIONS_KIND_USER = 2,
    NET_CLIENT_OPTIONS_KIND_TAP = 3,
    NET_CLIENT_OPTIONS_KIND_SOCKET = 4,
    NET_CLIENT_OPTIONS_KIND_VDE = 5,
    NET_CLIENT_OPTIONS_KIND_DUMP = 6,
    NET_CLIENT_OPTIONS_KIND_BRIDGE = 7,
    NET_CLIENT_OPTIONS_KIND_HUBPORT = 8,
    NET_CLIENT_OPTIONS_KIND_MAX = 9,
} NetClientOptionsKind;

typedef struct NetLegacy NetLegacy;

typedef struct NetLegacyList
{
    NetLegacy *value;
    struct NetLegacyList *next;
} NetLegacyList;

typedef struct Netdev Netdev;

typedef struct NetdevList
{
    Netdev *value;
    struct NetdevList *next;
} NetdevList;

typedef struct InetSocketAddress InetSocketAddress;

typedef struct InetSocketAddressList
{
    InetSocketAddress *value;
    struct InetSocketAddressList *next;
} InetSocketAddressList;

typedef struct UnixSocketAddress UnixSocketAddress;

typedef struct UnixSocketAddressList
{
    UnixSocketAddress *value;
    struct UnixSocketAddressList *next;
} UnixSocketAddressList;

typedef struct SocketAddress SocketAddress;

typedef struct SocketAddressList
{
    SocketAddress *value;
    struct SocketAddressList *next;
} SocketAddressList;

extern const char *SocketAddressKind_lookup[];
typedef enum SocketAddressKind
{
    SOCKET_ADDRESS_KIND_INET = 0,
    SOCKET_ADDRESS_KIND_UNIX = 1,
    SOCKET_ADDRESS_KIND_FD = 2,
    SOCKET_ADDRESS_KIND_MAX = 3,
} SocketAddressKind;

typedef struct MachineInfo MachineInfo;

typedef struct MachineInfoList
{
    MachineInfo *value;
    struct MachineInfoList *next;
} MachineInfoList;

typedef struct CpuDefinitionInfo CpuDefinitionInfo;

typedef struct CpuDefinitionInfoList
{
    CpuDefinitionInfo *value;
    struct CpuDefinitionInfoList *next;
} CpuDefinitionInfoList;

typedef struct AddfdInfo AddfdInfo;

typedef struct AddfdInfoList
{
    AddfdInfo *value;
    struct AddfdInfoList *next;
} AddfdInfoList;

typedef struct FdsetFdInfo FdsetFdInfo;

typedef struct FdsetFdInfoList
{
    FdsetFdInfo *value;
    struct FdsetFdInfoList *next;
} FdsetFdInfoList;

typedef struct FdsetInfo FdsetInfo;

typedef struct FdsetInfoList
{
    FdsetInfo *value;
    struct FdsetInfoList *next;
} FdsetInfoList;

extern const char *TargetType_lookup[];
typedef enum TargetType
{
    TARGET_TYPE_ALPHA = 0,
    TARGET_TYPE_ARM = 1,
    TARGET_TYPE_CRIS = 2,
    TARGET_TYPE_I386 = 3,
    TARGET_TYPE_LM32 = 4,
    TARGET_TYPE_M68K = 5,
    TARGET_TYPE_MICROBLAZEEL = 6,
    TARGET_TYPE_MICROBLAZE = 7,
    TARGET_TYPE_MIPS64EL = 8,
    TARGET_TYPE_MIPS64 = 9,
    TARGET_TYPE_MIPSEL = 10,
    TARGET_TYPE_MIPS = 11,
    TARGET_TYPE_OR32 = 12,
    TARGET_TYPE_PPC64 = 13,
    TARGET_TYPE_PPCEMB = 14,
    TARGET_TYPE_PPC = 15,
    TARGET_TYPE_S390X = 16,
    TARGET_TYPE_SH4EB = 17,
    TARGET_TYPE_SH4 = 18,
    TARGET_TYPE_SPARC64 = 19,
    TARGET_TYPE_SPARC = 20,
    TARGET_TYPE_UNICORE32 = 21,
    TARGET_TYPE_X86_64 = 22,
    TARGET_TYPE_XTENSAEB = 23,
    TARGET_TYPE_XTENSA = 24,
    TARGET_TYPE_MAX = 25,
} TargetType;

typedef struct TargetTypeList
{
    TargetType value;
    struct TargetTypeList *next;
} TargetTypeList;

typedef struct TargetInfo TargetInfo;

typedef struct TargetInfoList
{
    TargetInfo *value;
    struct TargetInfoList *next;
} TargetInfoList;

extern const char *QKeyCode_lookup[];
typedef enum QKeyCode
{
    Q_KEY_CODE_SHIFT = 0,
    Q_KEY_CODE_SHIFT_R = 1,
    Q_KEY_CODE_ALT = 2,
    Q_KEY_CODE_ALT_R = 3,
    Q_KEY_CODE_ALTGR = 4,
    Q_KEY_CODE_ALTGR_R = 5,
    Q_KEY_CODE_CTRL = 6,
    Q_KEY_CODE_CTRL_R = 7,
    Q_KEY_CODE_MENU = 8,
    Q_KEY_CODE_ESC = 9,
    Q_KEY_CODE_1 = 10,
    Q_KEY_CODE_2 = 11,
    Q_KEY_CODE_3 = 12,
    Q_KEY_CODE_4 = 13,
    Q_KEY_CODE_5 = 14,
    Q_KEY_CODE_6 = 15,
    Q_KEY_CODE_7 = 16,
    Q_KEY_CODE_8 = 17,
    Q_KEY_CODE_9 = 18,
    Q_KEY_CODE_0 = 19,
    Q_KEY_CODE_MINUS = 20,
    Q_KEY_CODE_EQUAL = 21,
    Q_KEY_CODE_BACKSPACE = 22,
    Q_KEY_CODE_TAB = 23,
    Q_KEY_CODE_Q = 24,
    Q_KEY_CODE_W = 25,
    Q_KEY_CODE_E = 26,
    Q_KEY_CODE_R = 27,
    Q_KEY_CODE_T = 28,
    Q_KEY_CODE_Y = 29,
    Q_KEY_CODE_U = 30,
    Q_KEY_CODE_I = 31,
    Q_KEY_CODE_O = 32,
    Q_KEY_CODE_P = 33,
    Q_KEY_CODE_BRACKET_LEFT = 34,
    Q_KEY_CODE_BRACKET_RIGHT = 35,
    Q_KEY_CODE_RET = 36,
    Q_KEY_CODE_A = 37,
    Q_KEY_CODE_S = 38,
    Q_KEY_CODE_D = 39,
    Q_KEY_CODE_F = 40,
    Q_KEY_CODE_G = 41,
    Q_KEY_CODE_H = 42,
    Q_KEY_CODE_J = 43,
    Q_KEY_CODE_K = 44,
    Q_KEY_CODE_L = 45,
    Q_KEY_CODE_SEMICOLON = 46,
    Q_KEY_CODE_APOSTROPHE = 47,
    Q_KEY_CODE_GRAVE_ACCENT = 48,
    Q_KEY_CODE_BACKSLASH = 49,
    Q_KEY_CODE_Z = 50,
    Q_KEY_CODE_X = 51,
    Q_KEY_CODE_C = 52,
    Q_KEY_CODE_V = 53,
    Q_KEY_CODE_B = 54,
    Q_KEY_CODE_N = 55,
    Q_KEY_CODE_M = 56,
    Q_KEY_CODE_COMMA = 57,
    Q_KEY_CODE_DOT = 58,
    Q_KEY_CODE_SLASH = 59,
    Q_KEY_CODE_ASTERISK = 60,
    Q_KEY_CODE_SPC = 61,
    Q_KEY_CODE_CAPS_LOCK = 62,
    Q_KEY_CODE_F1 = 63,
    Q_KEY_CODE_F2 = 64,
    Q_KEY_CODE_F3 = 65,
    Q_KEY_CODE_F4 = 66,
    Q_KEY_CODE_F5 = 67,
    Q_KEY_CODE_F6 = 68,
    Q_KEY_CODE_F7 = 69,
    Q_KEY_CODE_F8 = 70,
    Q_KEY_CODE_F9 = 71,
    Q_KEY_CODE_F10 = 72,
    Q_KEY_CODE_NUM_LOCK = 73,
    Q_KEY_CODE_SCROLL_LOCK = 74,
    Q_KEY_CODE_KP_DIVIDE = 75,
    Q_KEY_CODE_KP_MULTIPLY = 76,
    Q_KEY_CODE_KP_SUBTRACT = 77,
    Q_KEY_CODE_KP_ADD = 78,
    Q_KEY_CODE_KP_ENTER = 79,
    Q_KEY_CODE_KP_DECIMAL = 80,
    Q_KEY_CODE_SYSRQ = 81,
    Q_KEY_CODE_KP_0 = 82,
    Q_KEY_CODE_KP_1 = 83,
    Q_KEY_CODE_KP_2 = 84,
    Q_KEY_CODE_KP_3 = 85,
    Q_KEY_CODE_KP_4 = 86,
    Q_KEY_CODE_KP_5 = 87,
    Q_KEY_CODE_KP_6 = 88,
    Q_KEY_CODE_KP_7 = 89,
    Q_KEY_CODE_KP_8 = 90,
    Q_KEY_CODE_KP_9 = 91,
    Q_KEY_CODE_LESS = 92,
    Q_KEY_CODE_F11 = 93,
    Q_KEY_CODE_F12 = 94,
    Q_KEY_CODE_PRINT = 95,
    Q_KEY_CODE_HOME = 96,
    Q_KEY_CODE_PGUP = 97,
    Q_KEY_CODE_PGDN = 98,
    Q_KEY_CODE_END = 99,
    Q_KEY_CODE_LEFT = 100,
    Q_KEY_CODE_UP = 101,
    Q_KEY_CODE_DOWN = 102,
    Q_KEY_CODE_RIGHT = 103,
    Q_KEY_CODE_INSERT = 104,
    Q_KEY_CODE_DELETE = 105,
    Q_KEY_CODE_STOP = 106,
    Q_KEY_CODE_AGAIN = 107,
    Q_KEY_CODE_PROPS = 108,
    Q_KEY_CODE_UNDO = 109,
    Q_KEY_CODE_FRONT = 110,
    Q_KEY_CODE_COPY = 111,
    Q_KEY_CODE_OPEN = 112,
    Q_KEY_CODE_PASTE = 113,
    Q_KEY_CODE_FIND = 114,
    Q_KEY_CODE_CUT = 115,
    Q_KEY_CODE_LF = 116,
    Q_KEY_CODE_HELP = 117,
    Q_KEY_CODE_META_L = 118,
    Q_KEY_CODE_META_R = 119,
    Q_KEY_CODE_COMPOSE = 120,
    Q_KEY_CODE_MAX = 121,
} QKeyCode;

typedef struct QKeyCodeList
{
    QKeyCode value;
    struct QKeyCodeList *next;
} QKeyCodeList;

typedef struct KeyValue KeyValue;

typedef struct KeyValueList
{
    KeyValue *value;
    struct KeyValueList *next;
} KeyValueList;

extern const char *KeyValueKind_lookup[];
typedef enum KeyValueKind
{
    KEY_VALUE_KIND_NUMBER = 0,
    KEY_VALUE_KIND_QCODE = 1,
    KEY_VALUE_KIND_MAX = 2,
} KeyValueKind;

typedef struct ChardevFile ChardevFile;

typedef struct ChardevFileList
{
    ChardevFile *value;
    struct ChardevFileList *next;
} ChardevFileList;

typedef struct ChardevHostdev ChardevHostdev;

typedef struct ChardevHostdevList
{
    ChardevHostdev *value;
    struct ChardevHostdevList *next;
} ChardevHostdevList;

typedef struct ChardevSocket ChardevSocket;

typedef struct ChardevSocketList
{
    ChardevSocket *value;
    struct ChardevSocketList *next;
} ChardevSocketList;

typedef struct ChardevDummy ChardevDummy;

typedef struct ChardevDummyList
{
    ChardevDummy *value;
    struct ChardevDummyList *next;
} ChardevDummyList;

typedef struct ChardevBackend ChardevBackend;

typedef struct ChardevBackendList
{
    ChardevBackend *value;
    struct ChardevBackendList *next;
} ChardevBackendList;

extern const char *ChardevBackendKind_lookup[];
typedef enum ChardevBackendKind
{
    CHARDEV_BACKEND_KIND_FILE = 0,
    CHARDEV_BACKEND_KIND_SERIAL = 1,
    CHARDEV_BACKEND_KIND_PARALLEL = 2,
    CHARDEV_BACKEND_KIND_SOCKET = 3,
    CHARDEV_BACKEND_KIND_PTY = 4,
    CHARDEV_BACKEND_KIND_NULL = 5,
    CHARDEV_BACKEND_KIND_MAX = 6,
} ChardevBackendKind;

typedef struct ChardevReturn ChardevReturn;

typedef struct ChardevReturnList
{
    ChardevReturn *value;
    struct ChardevReturnList *next;
} ChardevReturnList;

void qapi_free_ErrorClassList(ErrorClassList * obj);

struct NameInfo
{
    bool has_name;
    char * name;
};

void qapi_free_NameInfoList(NameInfoList * obj);
void qapi_free_NameInfo(NameInfo * obj);

struct VersionInfo
{
    struct 
    {
        int64_t major;
        int64_t minor;
        int64_t micro;
    } qemu;
    char * package;
};

void qapi_free_VersionInfoList(VersionInfoList * obj);
void qapi_free_VersionInfo(VersionInfo * obj);

struct KvmInfo
{
    bool enabled;
    bool present;
};

void qapi_free_KvmInfoList(KvmInfoList * obj);
void qapi_free_KvmInfo(KvmInfo * obj);

void qapi_free_RunStateList(RunStateList * obj);

struct SnapshotInfo
{
    char * id;
    char * name;
    int64_t vm_state_size;
    int64_t date_sec;
    int64_t date_nsec;
    int64_t vm_clock_sec;
    int64_t vm_clock_nsec;
};

void qapi_free_SnapshotInfoList(SnapshotInfoList * obj);
void qapi_free_SnapshotInfo(SnapshotInfo * obj);

struct ImageInfo
{
    char * filename;
    char * format;
    bool has_dirty_flag;
    bool dirty_flag;
    bool has_actual_size;
    int64_t actual_size;
    int64_t virtual_size;
    bool has_cluster_size;
    int64_t cluster_size;
    bool has_encrypted;
    bool encrypted;
    bool has_backing_filename;
    char * backing_filename;
    bool has_full_backing_filename;
    char * full_backing_filename;
    bool has_backing_filename_format;
    char * backing_filename_format;
    bool has_snapshots;
    SnapshotInfoList * snapshots;
};

void qapi_free_ImageInfoList(ImageInfoList * obj);
void qapi_free_ImageInfo(ImageInfo * obj);

struct StatusInfo
{
    bool running;
    bool singlestep;
    RunState status;
};

void qapi_free_StatusInfoList(StatusInfoList * obj);
void qapi_free_StatusInfo(StatusInfo * obj);

struct UuidInfo
{
    char * UUID;
};

void qapi_free_UuidInfoList(UuidInfoList * obj);
void qapi_free_UuidInfo(UuidInfo * obj);

struct ChardevInfo
{
    char * label;
    char * filename;
};

void qapi_free_ChardevInfoList(ChardevInfoList * obj);
void qapi_free_ChardevInfo(ChardevInfo * obj);

void qapi_free_DataFormatList(DataFormatList * obj);

struct CommandInfo
{
    char * name;
};

void qapi_free_CommandInfoList(CommandInfoList * obj);
void qapi_free_CommandInfo(CommandInfo * obj);

struct EventInfo
{
    char * name;
};

void qapi_free_EventInfoList(EventInfoList * obj);
void qapi_free_EventInfo(EventInfo * obj);

struct MigrationStats
{
    int64_t transferred;
    int64_t remaining;
    int64_t total;
    int64_t duplicate;
    int64_t normal;
    int64_t normal_bytes;
    int64_t dirty_pages_rate;
};

void qapi_free_MigrationStatsList(MigrationStatsList * obj);
void qapi_free_MigrationStats(MigrationStats * obj);

struct XBZRLECacheStats
{
    int64_t cache_size;
    int64_t bytes;
    int64_t pages;
    int64_t cache_miss;
    int64_t overflow;
};

void qapi_free_XBZRLECacheStatsList(XBZRLECacheStatsList * obj);
void qapi_free_XBZRLECacheStats(XBZRLECacheStats * obj);

struct MigrationInfo
{
    bool has_status;
    char * status;
    bool has_ram;
    MigrationStats * ram;
    bool has_disk;
    MigrationStats * disk;
    bool has_xbzrle_cache;
    XBZRLECacheStats * xbzrle_cache;
    bool has_total_time;
    int64_t total_time;
    bool has_expected_downtime;
    int64_t expected_downtime;
    bool has_downtime;
    int64_t downtime;
};

void qapi_free_MigrationInfoList(MigrationInfoList * obj);
void qapi_free_MigrationInfo(MigrationInfo * obj);

void qapi_free_MigrationCapabilityList(MigrationCapabilityList * obj);

struct MigrationCapabilityStatus
{
    MigrationCapability capability;
    bool state;
};

void qapi_free_MigrationCapabilityStatusList(MigrationCapabilityStatusList * obj);
void qapi_free_MigrationCapabilityStatus(MigrationCapabilityStatus * obj);

struct MouseInfo
{
    char * name;
    int64_t index;
    bool current;
    bool absolute;
};

void qapi_free_MouseInfoList(MouseInfoList * obj);
void qapi_free_MouseInfo(MouseInfo * obj);

struct CpuInfo
{
    int64_t CPU;
    bool current;
    bool halted;
    bool has_pc;
    int64_t pc;
    bool has_nip;
    int64_t nip;
    bool has_npc;
    int64_t npc;
    bool has_PC;
    int64_t PC;
    int64_t thread_id;
};

void qapi_free_CpuInfoList(CpuInfoList * obj);
void qapi_free_CpuInfo(CpuInfo * obj);

struct BlockDeviceInfo
{
    char * file;
    bool ro;
    char * drv;
    bool has_backing_file;
    char * backing_file;
    int64_t backing_file_depth;
    bool encrypted;
    bool encryption_key_missing;
    int64_t bps;
    int64_t bps_rd;
    int64_t bps_wr;
    int64_t iops;
    int64_t iops_rd;
    int64_t iops_wr;
};

void qapi_free_BlockDeviceInfoList(BlockDeviceInfoList * obj);
void qapi_free_BlockDeviceInfo(BlockDeviceInfo * obj);

void qapi_free_BlockDeviceIoStatusList(BlockDeviceIoStatusList * obj);

struct BlockDirtyInfo
{
    int64_t count;
    int64_t granularity;
};

void qapi_free_BlockDirtyInfoList(BlockDirtyInfoList * obj);
void qapi_free_BlockDirtyInfo(BlockDirtyInfo * obj);

struct BlockInfo
{
    char * device;
    char * type;
    bool removable;
    bool locked;
    bool has_inserted;
    BlockDeviceInfo * inserted;
    bool has_tray_open;
    bool tray_open;
    bool has_io_status;
    BlockDeviceIoStatus io_status;
    bool has_dirty;
    BlockDirtyInfo * dirty;
};

void qapi_free_BlockInfoList(BlockInfoList * obj);
void qapi_free_BlockInfo(BlockInfo * obj);

struct BlockDeviceStats
{
    int64_t rd_bytes;
    int64_t wr_bytes;
    int64_t rd_operations;
    int64_t wr_operations;
    int64_t flush_operations;
    int64_t flush_total_time_ns;
    int64_t wr_total_time_ns;
    int64_t rd_total_time_ns;
    int64_t wr_highest_offset;
};

void qapi_free_BlockDeviceStatsList(BlockDeviceStatsList * obj);
void qapi_free_BlockDeviceStats(BlockDeviceStats * obj);

struct BlockStats
{
    bool has_device;
    char * device;
    BlockDeviceStats * stats;
    bool has_parent;
    BlockStats * parent;
};

void qapi_free_BlockStatsList(BlockStatsList * obj);
void qapi_free_BlockStats(BlockStats * obj);

struct VncClientInfo
{
    char * host;
    char * family;
    char * service;
    bool has_x509_dname;
    char * x509_dname;
    bool has_sasl_username;
    char * sasl_username;
};

void qapi_free_VncClientInfoList(VncClientInfoList * obj);
void qapi_free_VncClientInfo(VncClientInfo * obj);

struct VncInfo
{
    bool enabled;
    bool has_host;
    char * host;
    bool has_family;
    char * family;
    bool has_service;
    char * service;
    bool has_auth;
    char * auth;
    bool has_clients;
    VncClientInfoList * clients;
};

void qapi_free_VncInfoList(VncInfoList * obj);
void qapi_free_VncInfo(VncInfo * obj);

struct SpiceChannel
{
    char * host;
    char * family;
    char * port;
    int64_t connection_id;
    int64_t channel_type;
    int64_t channel_id;
    bool tls;
};

void qapi_free_SpiceChannelList(SpiceChannelList * obj);
void qapi_free_SpiceChannel(SpiceChannel * obj);

void qapi_free_SpiceQueryMouseModeList(SpiceQueryMouseModeList * obj);

struct SpiceInfo
{
    bool enabled;
    bool migrated;
    bool has_host;
    char * host;
    bool has_port;
    int64_t port;
    bool has_tls_port;
    int64_t tls_port;
    bool has_auth;
    char * auth;
    bool has_compiled_version;
    char * compiled_version;
    SpiceQueryMouseMode mouse_mode;
    bool has_channels;
    SpiceChannelList * channels;
};

void qapi_free_SpiceInfoList(SpiceInfoList * obj);
void qapi_free_SpiceInfo(SpiceInfo * obj);

struct BalloonInfo
{
    int64_t actual;
};

void qapi_free_BalloonInfoList(BalloonInfoList * obj);
void qapi_free_BalloonInfo(BalloonInfo * obj);

struct PciMemoryRange
{
    int64_t base;
    int64_t limit;
};

void qapi_free_PciMemoryRangeList(PciMemoryRangeList * obj);
void qapi_free_PciMemoryRange(PciMemoryRange * obj);

struct PciMemoryRegion
{
    int64_t bar;
    char * type;
    int64_t address;
    int64_t size;
    bool has_prefetch;
    bool prefetch;
    bool has_mem_type_64;
    bool mem_type_64;
};

void qapi_free_PciMemoryRegionList(PciMemoryRegionList * obj);
void qapi_free_PciMemoryRegion(PciMemoryRegion * obj);

struct PciBridgeInfo
{
    struct 
    {
        int64_t number;
        int64_t secondary;
        int64_t subordinate;
        PciMemoryRange * io_range;
        PciMemoryRange * memory_range;
        PciMemoryRange * prefetchable_range;
    } bus;
    bool has_devices;
    PciDeviceInfoList * devices;
};

void qapi_free_PciBridgeInfoList(PciBridgeInfoList * obj);
void qapi_free_PciBridgeInfo(PciBridgeInfo * obj);

struct PciDeviceInfo
{
    int64_t bus;
    int64_t slot;
    int64_t function;
    struct 
    {
        bool has_desc;
        char * desc;
        int64_t class;
    } class_info;
    struct 
    {
        int64_t device;
        int64_t vendor;
    } id;
    bool has_irq;
    int64_t irq;
    char * qdev_id;
    bool has_pci_bridge;
    PciBridgeInfo * pci_bridge;
    PciMemoryRegionList * regions;
};

void qapi_free_PciDeviceInfoList(PciDeviceInfoList * obj);
void qapi_free_PciDeviceInfo(PciDeviceInfo * obj);

struct PciInfo
{
    int64_t bus;
    PciDeviceInfoList * devices;
};

void qapi_free_PciInfoList(PciInfoList * obj);
void qapi_free_PciInfo(PciInfo * obj);

void qapi_free_BlockdevOnErrorList(BlockdevOnErrorList * obj);

void qapi_free_MirrorSyncModeList(MirrorSyncModeList * obj);

struct BlockJobInfo
{
    char * type;
    char * device;
    int64_t len;
    int64_t offset;
    bool busy;
    bool paused;
    int64_t speed;
    BlockDeviceIoStatus io_status;
};

void qapi_free_BlockJobInfoList(BlockJobInfoList * obj);
void qapi_free_BlockJobInfo(BlockJobInfo * obj);

void qapi_free_NewImageModeList(NewImageModeList * obj);

struct BlockdevSnapshot
{
    char * device;
    char * snapshot_file;
    bool has_format;
    char * format;
    bool has_mode;
    NewImageMode mode;
};

void qapi_free_BlockdevSnapshotList(BlockdevSnapshotList * obj);
void qapi_free_BlockdevSnapshot(BlockdevSnapshot * obj);

struct BlockdevAction
{
    BlockdevActionKind kind;
    union {
        void *data;
        BlockdevSnapshot * blockdev_snapshot_sync;
    };
};
void qapi_free_BlockdevActionList(BlockdevActionList * obj);
void qapi_free_BlockdevAction(BlockdevAction * obj);

struct ObjectPropertyInfo
{
    char * name;
    char * type;
};

void qapi_free_ObjectPropertyInfoList(ObjectPropertyInfoList * obj);
void qapi_free_ObjectPropertyInfo(ObjectPropertyInfo * obj);

struct ObjectTypeInfo
{
    char * name;
};

void qapi_free_ObjectTypeInfoList(ObjectTypeInfoList * obj);
void qapi_free_ObjectTypeInfo(ObjectTypeInfo * obj);

struct DevicePropertyInfo
{
    char * name;
    char * type;
};

void qapi_free_DevicePropertyInfoList(DevicePropertyInfoList * obj);
void qapi_free_DevicePropertyInfo(DevicePropertyInfo * obj);

struct NetdevNoneOptions
{
    char qapi_dummy_for_empty_struct;
};

void qapi_free_NetdevNoneOptionsList(NetdevNoneOptionsList * obj);
void qapi_free_NetdevNoneOptions(NetdevNoneOptions * obj);

struct NetLegacyNicOptions
{
    bool has_netdev;
    char * netdev;
    bool has_macaddr;
    char * macaddr;
    bool has_model;
    char * model;
    bool has_addr;
    char * addr;
    bool has_vectors;
    uint32_t vectors;
};

void qapi_free_NetLegacyNicOptionsList(NetLegacyNicOptionsList * obj);
void qapi_free_NetLegacyNicOptions(NetLegacyNicOptions * obj);

struct String
{
    char * str;
};

void qapi_free_StringList(StringList * obj);
void qapi_free_String(String * obj);

struct NetdevUserOptions
{
    bool has_hostname;
    char * hostname;
    bool has_q_restrict;
    bool q_restrict;
    bool has_ip;
    char * ip;
    bool has_net;
    char * net;
    bool has_host;
    char * host;
    bool has_tftp;
    char * tftp;
    bool has_bootfile;
    char * bootfile;
    bool has_dhcpstart;
    char * dhcpstart;
    bool has_dns;
    char * dns;
    bool has_dnssearch;
    StringList * dnssearch;
    bool has_smb;
    char * smb;
    bool has_smbserver;
    char * smbserver;
    bool has_hostfwd;
    StringList * hostfwd;
    bool has_guestfwd;
    StringList * guestfwd;
};

void qapi_free_NetdevUserOptionsList(NetdevUserOptionsList * obj);
void qapi_free_NetdevUserOptions(NetdevUserOptions * obj);

struct NetdevTapOptions
{
    bool has_ifname;
    char * ifname;
    bool has_fd;
    char * fd;
    bool has_fds;
    char * fds;
    bool has_script;
    char * script;
    bool has_downscript;
    char * downscript;
    bool has_helper;
    char * helper;
    bool has_sndbuf;
    uint64_t sndbuf;
    bool has_vnet_hdr;
    bool vnet_hdr;
    bool has_vhost;
    bool vhost;
    bool has_vhostfd;
    char * vhostfd;
    bool has_vhostfds;
    char * vhostfds;
    bool has_vhostforce;
    bool vhostforce;
    bool has_queues;
    uint32_t queues;
};

void qapi_free_NetdevTapOptionsList(NetdevTapOptionsList * obj);
void qapi_free_NetdevTapOptions(NetdevTapOptions * obj);

struct NetdevSocketOptions
{
    bool has_fd;
    char * fd;
    bool has_listen;
    char * listen;
    bool has_connect;
    char * connect;
    bool has_mcast;
    char * mcast;
    bool has_localaddr;
    char * localaddr;
    bool has_udp;
    char * udp;
};

void qapi_free_NetdevSocketOptionsList(NetdevSocketOptionsList * obj);
void qapi_free_NetdevSocketOptions(NetdevSocketOptions * obj);

struct NetdevVdeOptions
{
    bool has_sock;
    char * sock;
    bool has_port;
    uint16_t port;
    bool has_group;
    char * group;
    bool has_mode;
    uint16_t mode;
};

void qapi_free_NetdevVdeOptionsList(NetdevVdeOptionsList * obj);
void qapi_free_NetdevVdeOptions(NetdevVdeOptions * obj);

struct NetdevDumpOptions
{
    bool has_len;
    uint64_t len;
    bool has_file;
    char * file;
};

void qapi_free_NetdevDumpOptionsList(NetdevDumpOptionsList * obj);
void qapi_free_NetdevDumpOptions(NetdevDumpOptions * obj);

struct NetdevBridgeOptions
{
    bool has_br;
    char * br;
    bool has_helper;
    char * helper;
};

void qapi_free_NetdevBridgeOptionsList(NetdevBridgeOptionsList * obj);
void qapi_free_NetdevBridgeOptions(NetdevBridgeOptions * obj);

struct NetdevHubPortOptions
{
    int32_t hubid;
};

void qapi_free_NetdevHubPortOptionsList(NetdevHubPortOptionsList * obj);
void qapi_free_NetdevHubPortOptions(NetdevHubPortOptions * obj);

struct NetClientOptions
{
    NetClientOptionsKind kind;
    union {
        void *data;
        NetdevNoneOptions * none;
        NetLegacyNicOptions * nic;
        NetdevUserOptions * user;
        NetdevTapOptions * tap;
        NetdevSocketOptions * socket;
        NetdevVdeOptions * vde;
        NetdevDumpOptions * dump;
        NetdevBridgeOptions * bridge;
        NetdevHubPortOptions * hubport;
    };
};
void qapi_free_NetClientOptionsList(NetClientOptionsList * obj);
void qapi_free_NetClientOptions(NetClientOptions * obj);

struct NetLegacy
{
    bool has_vlan;
    int32_t vlan;
    bool has_id;
    char * id;
    bool has_name;
    char * name;
    NetClientOptions * opts;
};

void qapi_free_NetLegacyList(NetLegacyList * obj);
void qapi_free_NetLegacy(NetLegacy * obj);

struct Netdev
{
    char * id;
    NetClientOptions * opts;
};

void qapi_free_NetdevList(NetdevList * obj);
void qapi_free_Netdev(Netdev * obj);

struct InetSocketAddress
{
    char * host;
    char * port;
    bool has_to;
    uint16_t to;
    bool has_ipv4;
    bool ipv4;
    bool has_ipv6;
    bool ipv6;
};

void qapi_free_InetSocketAddressList(InetSocketAddressList * obj);
void qapi_free_InetSocketAddress(InetSocketAddress * obj);

struct UnixSocketAddress
{
    char * path;
};

void qapi_free_UnixSocketAddressList(UnixSocketAddressList * obj);
void qapi_free_UnixSocketAddress(UnixSocketAddress * obj);

struct SocketAddress
{
    SocketAddressKind kind;
    union {
        void *data;
        InetSocketAddress * inet;
        UnixSocketAddress * q_unix;
        String * fd;
    };
};
void qapi_free_SocketAddressList(SocketAddressList * obj);
void qapi_free_SocketAddress(SocketAddress * obj);

struct MachineInfo
{
    char * name;
    bool has_alias;
    char * alias;
    bool has_is_default;
    bool is_default;
};

void qapi_free_MachineInfoList(MachineInfoList * obj);
void qapi_free_MachineInfo(MachineInfo * obj);

struct CpuDefinitionInfo
{
    char * name;
};

void qapi_free_CpuDefinitionInfoList(CpuDefinitionInfoList * obj);
void qapi_free_CpuDefinitionInfo(CpuDefinitionInfo * obj);

struct AddfdInfo
{
    int64_t fdset_id;
    int64_t fd;
};

void qapi_free_AddfdInfoList(AddfdInfoList * obj);
void qapi_free_AddfdInfo(AddfdInfo * obj);

struct FdsetFdInfo
{
    int64_t fd;
    bool has_opaque;
    char * opaque;
};

void qapi_free_FdsetFdInfoList(FdsetFdInfoList * obj);
void qapi_free_FdsetFdInfo(FdsetFdInfo * obj);

struct FdsetInfo
{
    int64_t fdset_id;
    FdsetFdInfoList * fds;
};

void qapi_free_FdsetInfoList(FdsetInfoList * obj);
void qapi_free_FdsetInfo(FdsetInfo * obj);

void qapi_free_TargetTypeList(TargetTypeList * obj);

struct TargetInfo
{
    TargetType arch;
};

void qapi_free_TargetInfoList(TargetInfoList * obj);
void qapi_free_TargetInfo(TargetInfo * obj);

void qapi_free_QKeyCodeList(QKeyCodeList * obj);

struct KeyValue
{
    KeyValueKind kind;
    union {
        void *data;
        int64_t number;
        QKeyCode qcode;
    };
};
void qapi_free_KeyValueList(KeyValueList * obj);
void qapi_free_KeyValue(KeyValue * obj);

struct ChardevFile
{
    bool has_in;
    char * in;
    char * out;
};

void qapi_free_ChardevFileList(ChardevFileList * obj);
void qapi_free_ChardevFile(ChardevFile * obj);

struct ChardevHostdev
{
    char * device;
};

void qapi_free_ChardevHostdevList(ChardevHostdevList * obj);
void qapi_free_ChardevHostdev(ChardevHostdev * obj);

struct ChardevSocket
{
    SocketAddress * addr;
    bool has_server;
    bool server;
    bool has_wait;
    bool wait;
    bool has_nodelay;
    bool nodelay;
    bool has_telnet;
    bool telnet;
};

void qapi_free_ChardevSocketList(ChardevSocketList * obj);
void qapi_free_ChardevSocket(ChardevSocket * obj);

struct ChardevDummy
{
    char qapi_dummy_for_empty_struct;
};

void qapi_free_ChardevDummyList(ChardevDummyList * obj);
void qapi_free_ChardevDummy(ChardevDummy * obj);

struct ChardevBackend
{
    ChardevBackendKind kind;
    union {
        void *data;
        ChardevFile * file;
        ChardevHostdev * serial;
        ChardevHostdev * parallel;
        ChardevSocket * socket;
        ChardevDummy * pty;
        ChardevDummy * null;
    };
};
void qapi_free_ChardevBackendList(ChardevBackendList * obj);
void qapi_free_ChardevBackend(ChardevBackend * obj);

struct ChardevReturn
{
    bool has_pty;
    char * pty;
};

void qapi_free_ChardevReturnList(ChardevReturnList * obj);
void qapi_free_ChardevReturn(ChardevReturn * obj);

#endif
