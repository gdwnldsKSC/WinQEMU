
DEFHEADING(Standard options:)

DEF("help", 0, QEMU_OPTION_h,
"-h or -help     display this help and exit\n", QEMU_ARCH_ALL)

DEF("version", 0, QEMU_OPTION_version,
"-version        display version information and exit\n", QEMU_ARCH_ALL)

DEF("M", HAS_ARG, QEMU_OPTION_M,
"-M machine      select emulated machine (-M ? for list)\n", QEMU_ARCH_ALL)

DEF("cpu", HAS_ARG, QEMU_OPTION_cpu,
"-cpu cpu        select CPU (-cpu ? for list)\n", QEMU_ARCH_ALL)

DEF("smp", HAS_ARG, QEMU_OPTION_smp,
"-smp n[,maxcpus=cpus][,cores=cores][,threads=threads][,sockets=sockets]\n"
"                set the number of CPUs to 'n' [default=1]\n"
"                maxcpus= maximum number of total cpus, including\n"
"                offline CPUs for hotplug, etc\n"
"                cores= number of CPU cores on one socket\n"
"                threads= number of threads on one CPU core\n"
"                sockets= number of discrete sockets in the system\n",
QEMU_ARCH_ALL)

DEF("numa", HAS_ARG, QEMU_OPTION_numa,
"-numa node[,mem=size][,cpus=cpu[-cpu]][,nodeid=node]\n", QEMU_ARCH_ALL)

DEF("fda", HAS_ARG, QEMU_OPTION_fda,
"-fda/-fdb file  use 'file' as floppy disk 0/1 image\n", QEMU_ARCH_ALL)
DEF("fdb", HAS_ARG, QEMU_OPTION_fdb, "", QEMU_ARCH_ALL)

DEF("hda", HAS_ARG, QEMU_OPTION_hda,
"-hda/-hdb file  use 'file' as IDE hard disk 0/1 image\n", QEMU_ARCH_ALL)
DEF("hdb", HAS_ARG, QEMU_OPTION_hdb, "", QEMU_ARCH_ALL)
DEF("hdc", HAS_ARG, QEMU_OPTION_hdc,
"-hdc/-hdd file  use 'file' as IDE hard disk 2/3 image\n", QEMU_ARCH_ALL)
DEF("hdd", HAS_ARG, QEMU_OPTION_hdd, "", QEMU_ARCH_ALL)

DEF("cdrom", HAS_ARG, QEMU_OPTION_cdrom,
"-cdrom file     use 'file' as IDE cdrom image (cdrom is ide1 master)\n",
QEMU_ARCH_ALL)

DEF("drive", HAS_ARG, QEMU_OPTION_drive,
"-drive [file=file][,if=type][,bus=n][,unit=m][,media=d][,index=i]\n"
"       [,cyls=c,heads=h,secs=s[,trans=t]][,snapshot=on|off]\n"
"       [,cache=writethrough|writeback|unsafe|none][,format=f]\n"
"       [,serial=s][,addr=A][,id=name][,aio=threads|native]\n"
"       [,readonly=on|off]\n"
"                use 'file' as a drive image\n", QEMU_ARCH_ALL)

DEF("set", HAS_ARG, QEMU_OPTION_set,
"-set group.id.arg=value\n"
"                set <arg> parameter for item <id> of type <group>\n"
"                i.e. -set drive.$id.file=/path/to/image\n", QEMU_ARCH_ALL)

DEF("global", HAS_ARG, QEMU_OPTION_global,
"-global driver.property=value\n"
"                set a global default for a driver property\n",
QEMU_ARCH_ALL)

DEF("mtdblock", HAS_ARG, QEMU_OPTION_mtdblock,
"-mtdblock file  use 'file' as on-board Flash memory image\n",
QEMU_ARCH_ALL)

DEF("sd", HAS_ARG, QEMU_OPTION_sd,
"-sd file        use 'file' as SecureDigital card image\n", QEMU_ARCH_ALL)

DEF("pflash", HAS_ARG, QEMU_OPTION_pflash,
"-pflash file    use 'file' as a parallel flash image\n", QEMU_ARCH_ALL)

DEF("boot", HAS_ARG, QEMU_OPTION_boot,
"-boot [order=drives][,once=drives][,menu=on|off]\n"
"                'drives': floppy (a), hard disk (c), CD-ROM (d), network (n)\n",
QEMU_ARCH_ALL)

DEF("snapshot", 0, QEMU_OPTION_snapshot,
"-snapshot       write to temporary files instead of disk image files\n",
QEMU_ARCH_ALL)

DEF("m", HAS_ARG, QEMU_OPTION_m,
"-m megs         set virtual RAM size to megs MB [default="
stringify(DEFAULT_RAM_SIZE) "]\n", QEMU_ARCH_ALL)

DEF("mem-path", HAS_ARG, QEMU_OPTION_mempath,
"-mem-path FILE  provide backing storage for guest RAM\n", QEMU_ARCH_ALL)

#ifdef MAP_POPULATE
DEF("mem-prealloc", 0, QEMU_OPTION_mem_prealloc,
"-mem-prealloc   preallocate guest memory (use with -mem-path)\n",
QEMU_ARCH_ALL)
#endif

DEF("k", HAS_ARG, QEMU_OPTION_k,
"-k language     use keyboard layout (for example 'fr' for French)\n",
QEMU_ARCH_ALL)


DEF("audio-help", 0, QEMU_OPTION_audio_help,
"-audio-help     print list of audio drivers and their options\n",
QEMU_ARCH_ALL)

DEF("soundhw", HAS_ARG, QEMU_OPTION_soundhw,
"-soundhw c1,... enable audio support\n"
"                and only specified sound cards (comma separated list)\n"
"                use -soundhw ? to get the list of supported cards\n"
"                use -soundhw all to enable all of them\n", QEMU_ARCH_ALL)


DEF("usb", 0, QEMU_OPTION_usb,
"-usb            enable the USB driver (will be the default soon)\n",
QEMU_ARCH_ALL)

DEF("usbdevice", HAS_ARG, QEMU_OPTION_usbdevice,
"-usbdevice name add the host or guest USB device 'name'\n",
QEMU_ARCH_ALL)

DEF("device", HAS_ARG, QEMU_OPTION_device,
"-device driver[,prop[=value][,...]]\n"
"                add device (based on driver)\n"
"                prop=value,... sets driver properties\n"
"                use -device ? to print all possible drivers\n"
"                use -device driver,? to print all possible properties\n",
QEMU_ARCH_ALL)

#ifdef CONFIG_LINUX
DEFHEADING(File system options:)

DEF("fsdev", HAS_ARG, QEMU_OPTION_fsdev,
"-fsdev local,id=id,path=path\n",
QEMU_ARCH_ALL)

#endif

#ifdef CONFIG_LINUX
DEFHEADING(Virtual File system pass-through options:)

DEF("virtfs", HAS_ARG, QEMU_OPTION_virtfs,
"-virtfs local,path=path,mount_tag=tag\n",
QEMU_ARCH_ALL)

#endif

DEFHEADING()

DEF("name", HAS_ARG, QEMU_OPTION_name,
"-name string1[,process=string2]\n"
"                set the name of the guest\n"
"                string1 sets the window title and string2 the process name (on Linux)\n",
QEMU_ARCH_ALL)

DEF("uuid", HAS_ARG, QEMU_OPTION_uuid,
"-uuid %08x-%04x-%04x-%04x-%012x\n"
"                specify machine UUID\n", QEMU_ARCH_ALL)


DEFHEADING()

DEFHEADING(Display options:)


DEF("nographic", 0, QEMU_OPTION_nographic,
"-nographic      disable graphical output and redirect serial I/Os to console\n",
QEMU_ARCH_ALL)

#ifdef CONFIG_CURSES
DEF("curses", 0, QEMU_OPTION_curses,
"-curses         use a curses/ncurses interface instead of SDL\n",
QEMU_ARCH_ALL)
#endif

#ifdef CONFIG_SDL
DEF("no-frame", 0, QEMU_OPTION_no_frame,
"-no-frame       open SDL window without a frame and window decorations\n",
QEMU_ARCH_ALL)
#endif

#ifdef CONFIG_SDL
DEF("alt-grab", 0, QEMU_OPTION_alt_grab,
"-alt-grab       use Ctrl-Alt-Shift to grab mouse (instead of Ctrl-Alt)\n",
QEMU_ARCH_ALL)
#endif

#ifdef CONFIG_SDL
DEF("ctrl-grab", 0, QEMU_OPTION_ctrl_grab,
"-ctrl-grab      use Right-Ctrl to grab mouse (instead of Ctrl-Alt)\n",
QEMU_ARCH_ALL)
#endif

#ifdef CONFIG_SDL
DEF("no-quit", 0, QEMU_OPTION_no_quit,
"-no-quit        disable SDL window close capability\n", QEMU_ARCH_ALL)
#endif

#ifdef CONFIG_SDL
DEF("sdl", 0, QEMU_OPTION_sdl,
"-sdl            enable SDL\n", QEMU_ARCH_ALL)
#endif

DEF("portrait", 0, QEMU_OPTION_portrait,
"-portrait       rotate graphical output 90 deg left (only PXA LCD)\n",
QEMU_ARCH_ALL)

DEF("vga", HAS_ARG, QEMU_OPTION_vga,
"-vga [std|cirrus|vmware|xenfb|none]\n"
"                select video card type\n", QEMU_ARCH_ALL)

DEF("full-screen", 0, QEMU_OPTION_full_screen,
"-full-screen    start in full screen\n", QEMU_ARCH_ALL)

DEF("g", 1, QEMU_OPTION_g ,
"-g WxH[xDEPTH]  Set the initial graphical resolution and depth\n",
QEMU_ARCH_PPC | QEMU_ARCH_SPARC)

DEF("vnc", HAS_ARG, QEMU_OPTION_vnc ,
"-vnc display    start a VNC server on display\n", QEMU_ARCH_ALL)


DEFHEADING()

DEFHEADING(i386 target only:)

DEF("win2k-hack", 0, QEMU_OPTION_win2k_hack,
"-win2k-hack     use it when installing Windows 2000 to avoid a disk full bug\n",
QEMU_ARCH_I386)

DEF("rtc-td-hack", 0, QEMU_OPTION_rtc_td_hack, "", QEMU_ARCH_I386)

DEF("no-fd-bootchk", 0, QEMU_OPTION_no_fd_bootchk,
"-no-fd-bootchk  disable boot signature checking for floppy disks\n",
QEMU_ARCH_I386)

DEF("no-acpi", 0, QEMU_OPTION_no_acpi,
"-no-acpi        disable ACPI\n", QEMU_ARCH_I386)

DEF("no-hpet", 0, QEMU_OPTION_no_hpet,
"-no-hpet        disable HPET\n", QEMU_ARCH_I386)

DEF("balloon", HAS_ARG, QEMU_OPTION_balloon,
"-balloon none   disable balloon device\n"
"-balloon virtio[,addr=str]\n"
"                enable virtio balloon device (default)\n", QEMU_ARCH_ALL)

DEF("acpitable", HAS_ARG, QEMU_OPTION_acpitable,
"-acpitable [sig=str][,rev=n][,oem_id=str][,oem_table_id=str][,oem_rev=n][,asl_compiler_id=str][,asl_compiler_rev=n][,data=file1[:file2]...]\n"
"                ACPI table description\n", QEMU_ARCH_I386)

DEF("smbios", HAS_ARG, QEMU_OPTION_smbios,
"-smbios file=binary\n"
"                load SMBIOS entry from binary file\n"
"-smbios type=0[,vendor=str][,version=str][,date=str][,release=%d.%d]\n"
"                specify SMBIOS type 0 fields\n"
"-smbios type=1[,manufacturer=str][,product=str][,version=str][,serial=str]\n"
"              [,uuid=uuid][,sku=str][,family=str]\n"
"                specify SMBIOS type 1 fields\n", QEMU_ARCH_I386)

DEFHEADING()

DEFHEADING(Network options : )

// BEGIN HEAVILY MODIFIED SECTION

#ifdef CONFIG_SLIRP
DEF("tftp", HAS_ARG, QEMU_OPTION_tftp, "", QEMU_ARCH_ALL)
DEF("bootp", HAS_ARG, QEMU_OPTION_bootp, "", QEMU_ARCH_ALL)
DEF("redir", HAS_ARG, QEMU_OPTION_redir, "", QEMU_ARCH_ALL)
#ifndef _WIN32
DEF("smb", HAS_ARG, QEMU_OPTION_smb, "", QEMU_ARCH_ALL)
#endif
#endif

#ifdef CONFIG_SLIRP
#ifdef _WIN32
#define SLIRP_OPTIONS \
"-net user[,vlan=n][,name=str][,net=addr[/mask]][,host=addr][,restrict=y|n]\n" \
"         [,hostname=host][,dhcpstart=addr][,dns=addr][,tftp=dir][,bootfile=f]\n" \
"         [,hostfwd=rule][,guestfwd=rule]" \
"                connect the user mode network stack to VLAN 'n', configure its\n" \
"                DHCP server and enabled optional services\n"
#else
#define SLIRP_OPTIONS \
"-net user[,vlan=n][,name=str][,net=addr[/mask]][,host=addr][,restrict=y|n]\n" \
"         [,hostname=host][,dhcpstart=addr][,dns=addr][,tftp=dir][,bootfile=f]\n" \
"         [,hostfwd=rule][,guestfwd=rule][,smb=dir[,smbserver=addr]]" \
"                connect the user mode network stack to VLAN 'n', configure its\n" \
"                DHCP server and enabled optional services\n"
#endif
#else
#define SLIRP_OPTIONS ""
#endif

// TAP options
#ifdef _WIN32
#define TAP_OPTIONS \
"-net tap[,vlan=n][,name=str],ifname=name\n" \
"                connect the host TAP network interface to VLAN 'n'\n"
#else // Non-Windows TAP options - we don't really need it, but having it as reference will help future updates
#define TAP_OPTIONS \
"-net tap[,vlan=n][,name=str][,fd=h][,ifname=name][,script=file][,downscript=dfile][,sndbuf=nbytes][,vnet_hdr=on|off]\n" \
"                connect the host TAP network interface to VLAN 'n' and use the\n" \
"                network scripts 'file' (default=%s)\n" \
"                and 'dfile' (default=%s);\n" \
"                use '[down]script=no' to disable script execution;\n" \
"                use 'fd=h' to connect to an already opened TAP interface\n" \
"                use 'sndbuf=nbytes' to limit the size of the send buffer; the\n" \
"                default of 'sndbuf=1048576' can be disabled using 'sndbuf=0'\n" \
"                use vnet_hdr=off to avoid enabling the IFF_VNET_HDR tap flag\n"
"                use vnet_hdr=on to make the lack of IFF_VNET_HDR support an error condition\n" \
"                use vhost=on to enable experimental in kernel accelerator\n" \
"                use 'vhostfd=h' to connect to an already opened vhost net device\n"
#endif

#ifdef CONFIG_VDE 
#define VDE_OPTIONS \
"-net vde[,vlan=n][,name=str][,sock=socketpath][,port=n][,group=groupname][,mode=octalmode]\n" \
"                connect the vlan 'n' to port 'n' of a vde switch running\n" \
"                on host and listening for incoming connections on 'socketpath'.\n" \
"                Use group 'groupname' and mode 'octalmode' to change default\n" \
"                ownership and permissions for communication port.\n"
#else
#define VDE_OPTIONS ""
#endif


// Put it all together... 
// DEFHEADING(Network options : ) we probably don't need this one again.... 

DEF("net", HAS_ARG, QEMU_OPTION_net,
	"-net nic[,vlan=n][,macaddr=mac][,model=type][,name=str][,addr=str][,vectors=v]\n"
	"                create a new Network Interface Card and connect it to VLAN 'n'\n" \
	SLIRP_OPTIONS \
	TAP_OPTIONS \
	"-net socket[,vlan=n][,name=str][,fd=h][,listen=[host]:port][,connect=host:port]\n" \
	"                connect the vlan 'n' to another VLAN using a socket connection\n" \
	"-net socket[,vlan=n][,name=str][,fd=h][,mcast=maddr:port]\n" \
	"                connect the vlan 'n' to multicast maddr and port\n" \
	VDE_OPTIONS \
	"-net dump[,vlan=n][,file=f][,len=n]\n" \
	"                dump traffic on vlan 'n' to file 'f' (max n bytes per packet)\n" \
	"-net none       use it alone to have zero network devices; if no -net option\n" \
	"                is provided, the default is '-net nic -net user'\n", QEMU_ARCH_ALL)

	// netdev section
#ifdef CONFIG_SLIRP
#define NETDEV_SLIRP \
"user|"
#else
#define NETDEV_SLIRP ""
#endif
#ifdef CONFIG_VDE
#define NETDEV_VDE \
"vde|"
#else
#define NETDEV_VDE ""
#endif

DEF("netdev", HAS_ARG, QEMU_OPTION_netdev,
	"-netdev [" \
	NETDEV_SLIRP \
	"tap|" \
	NETDEV_VDE \
	"socket],id=str[,option][,option][,...]\n", QEMU_ARCH_ALL)

DEFHEADING()

DEFHEADING(Character device options : )

#ifdef _WIN32
#define CHARDEV_WIN32 \
	"-chardev console,id=id\n" \
	"-chardev serial,id=id,path=path\n"
#else
#define CHARDEV_WIN32 \
	"-chardev pty,id=id\n" \
	"-chardev stdio,id=id\n"
#endif

#ifdef CONFIG_BRLAPI
#define CHARDEV_BRLAPI \
	"-chardev braille,id=id\n"
#else
#define CHARDEV_BRLAPI ""
#endif

#if defined(__linux__) || defined(__sun__) || defined(__FreeBSD__) \
|| defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
#define CHARDEV_TTY \
	"-chardev tty,id=id,path=path\n"
#else
#define CHARDEV_TTY ""
#endif

#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
#define CHARDEV_PARPORT \
	"-chardev parport,id=id,path=path\n"
#else
#define CHARDEV_PARPORT ""
#endif

DEF("chardev", HAS_ARG, QEMU_OPTION_chardev,
	"-chardev null,id=id[,mux=on|off]\n" \
	"-chardev socket,id=id[,host=host],port=host[,to=to][,ipv4][,ipv6][,nodelay]\n" \
	"         [,server][,nowait][,telnet][,mux=on|off] (tcp)\n" \
	"-chardev socket,id=id,path=path[,server][,nowait][,telnet],[mux=on|off] (unix)\n" \
	"-chardev udp,id=id[,host=host],port=port[,localaddr=localaddr]\n" \
	"         [,localport=localport][,ipv4][,ipv6][,mux=on|off]\n" \
	"-chardev msmouse,id=id[,mux=on|off]\n" \
	"-chardev vc,id=id[[,width=width][,height=height]][[,cols=cols][,rows=rows]]\n" \
	"         [,mux=on|off]\n" \
	"-chardev file,id=id,path=path[,mux=on|off]\n" \
	"-chardev pipe,id=id,path=path[,mux=on|off]\n" \
	CHARDEV_WIN32 \
	CHARDEV_BRLAPI \
	CHARDEV_TTY \
	CHARDEV_PARPORT, QEMU_ARCH_ALL
)

// END HEAVILY MODIFIED SECTION 

DEFHEADING()

DEFHEADING(Bluetooth(R) options:)

DEF("bt", HAS_ARG, QEMU_OPTION_bt, \
"-bt hci,null    dumb bluetooth HCI - doesn't respond to commands\n" \
"-bt hci,host[:id]\n" \
"                use host's HCI with the given name\n" \
"-bt hci[,vlan=n]\n" \
"                emulate a standard HCI in virtual scatternet 'n'\n" \
"-bt vhci[,vlan=n]\n" \
"                add host computer to virtual scatternet 'n' using VHCI\n" \
"-bt device:dev[,vlan=n]\n" \
"                emulate a bluetooth device 'dev' in scatternet 'n'\n",
QEMU_ARCH_ALL)

DEFHEADING()

DEFHEADING(Linux/Multiboot boot specific:)

DEF("kernel", HAS_ARG, QEMU_OPTION_kernel, \
"-kernel bzImage use 'bzImage' as kernel image\n", QEMU_ARCH_ALL)

DEF("append", HAS_ARG, QEMU_OPTION_append, \
"-append cmdline use 'cmdline' as kernel command line\n", QEMU_ARCH_ALL)

DEF("initrd", HAS_ARG, QEMU_OPTION_initrd, \
"-initrd file    use 'file' as initial ram disk\n", QEMU_ARCH_ALL)


DEFHEADING()

DEFHEADING(Debug/Expert options:)


DEF("serial", HAS_ARG, QEMU_OPTION_serial, \
"-serial dev     redirect the serial port to char device 'dev'\n",
QEMU_ARCH_ALL)

DEF("parallel", HAS_ARG, QEMU_OPTION_parallel, \
"-parallel dev   redirect the parallel port to char device 'dev'\n",
QEMU_ARCH_ALL)

DEF("monitor", HAS_ARG, QEMU_OPTION_monitor, \
"-monitor dev    redirect the monitor to char device 'dev'\n",
QEMU_ARCH_ALL)
DEF("qmp", HAS_ARG, QEMU_OPTION_qmp, \
"-qmp dev        like -monitor but opens in 'control' mode\n",
QEMU_ARCH_ALL)

DEF("mon", HAS_ARG, QEMU_OPTION_mon, \
"-mon chardev=[name][,mode=readline|control][,default]\n", QEMU_ARCH_ALL)

DEF("debugcon", HAS_ARG, QEMU_OPTION_debugcon, \
"-debugcon dev   redirect the debug console to char device 'dev'\n",
QEMU_ARCH_ALL)

DEF("pidfile", HAS_ARG, QEMU_OPTION_pidfile, \
"-pidfile file   write PID to 'file'\n", QEMU_ARCH_ALL)

DEF("singlestep", 0, QEMU_OPTION_singlestep, \
"-singlestep     always run in singlestep mode\n", QEMU_ARCH_ALL)

DEF("S", 0, QEMU_OPTION_S, \
"-S              freeze CPU at startup (use 'c' to start execution)\n",
QEMU_ARCH_ALL)

DEF("gdb", HAS_ARG, QEMU_OPTION_gdb, \
"-gdb dev        wait for gdb connection on 'dev'\n", QEMU_ARCH_ALL)

DEF("s", 0, QEMU_OPTION_s, \
"-s              shorthand for -gdb tcp::" DEFAULT_GDBSTUB_PORT "\n",
QEMU_ARCH_ALL)

DEF("d", HAS_ARG, QEMU_OPTION_d, \
"-d item1,...    output log to /tmp/qemu.log (use -d ? for a list of log items)\n",
QEMU_ARCH_ALL)

DEF("hdachs", HAS_ARG, QEMU_OPTION_hdachs, \
"-hdachs c,h,s[,t]\n" \
"                force hard disk 0 physical geometry and the optional BIOS\n" \
"                translation (t=none or lba) (usually qemu can guess them)\n",
QEMU_ARCH_ALL)

DEF("L", HAS_ARG, QEMU_OPTION_L, \
"-L path         set the directory for the BIOS, VGA BIOS and keymaps\n",
QEMU_ARCH_ALL)

DEF("bios", HAS_ARG, QEMU_OPTION_bios, \
"-bios file      set the filename for the BIOS\n", QEMU_ARCH_ALL)

DEF("enable-kvm", 0, QEMU_OPTION_enable_kvm, \
"-enable-kvm     enable KVM full virtualization support\n", QEMU_ARCH_ALL)

DEF("xen-domid", HAS_ARG, QEMU_OPTION_xen_domid,
"-xen-domid id   specify xen guest domain id\n", QEMU_ARCH_ALL)
DEF("xen-create", 0, QEMU_OPTION_xen_create,
"-xen-create     create domain using xen hypercalls, bypassing xend\n"
"                warning: should not be used when xend is in use\n",
QEMU_ARCH_ALL)
DEF("xen-attach", 0, QEMU_OPTION_xen_attach,
"-xen-attach     attach to existing xen domain\n"
"                xend will use this when starting qemu\n",
QEMU_ARCH_ALL)

DEF("no-reboot", 0, QEMU_OPTION_no_reboot, \
"-no-reboot      exit instead of rebooting\n", QEMU_ARCH_ALL)

DEF("no-shutdown", 0, QEMU_OPTION_no_shutdown, \
"-no-shutdown    stop before shutdown\n", QEMU_ARCH_ALL)

DEF("loadvm", HAS_ARG, QEMU_OPTION_loadvm, \
"-loadvm [tag|id]\n" \
"                start right away with a saved state (loadvm in monitor)\n",
QEMU_ARCH_ALL)

#ifndef _WIN32
DEF("daemonize", 0, QEMU_OPTION_daemonize, \
"-daemonize      daemonize QEMU after initializing\n", QEMU_ARCH_ALL)
#endif

DEF("option-rom", HAS_ARG, QEMU_OPTION_option_rom, \
"-option-rom rom load a file, rom, into the option ROM space\n",
QEMU_ARCH_ALL)

DEF("clock", HAS_ARG, QEMU_OPTION_clock, \
"-clock          force the use of the given methods for timer alarm.\n" \
"                To see what timers are available use -clock ?\n",
QEMU_ARCH_ALL)

DEF("localtime", 0, QEMU_OPTION_localtime, "", QEMU_ARCH_ALL)
DEF("startdate", HAS_ARG, QEMU_OPTION_startdate, "", QEMU_ARCH_ALL)

DEF("rtc", HAS_ARG, QEMU_OPTION_rtc, \
"-rtc [base=utc|localtime|date][,clock=host|vm][,driftfix=none|slew]\n" \
"                set the RTC base and clock, enable drift fix for clock ticks (x86 only)\n",
QEMU_ARCH_ALL)


DEF("icount", HAS_ARG, QEMU_OPTION_icount, \
"-icount [N|auto]\n" \
"                enable virtual instruction counter with 2^N clock ticks per\n" \
"                instruction\n", QEMU_ARCH_ALL)

DEF("watchdog", HAS_ARG, QEMU_OPTION_watchdog, \
"-watchdog i6300esb|ib700\n" \
"                enable virtual hardware watchdog [default=none]\n",
QEMU_ARCH_ALL)

DEF("watchdog-action", HAS_ARG, QEMU_OPTION_watchdog_action, \
"-watchdog-action reset|shutdown|poweroff|pause|debug|none\n" \
"                action when watchdog fires [default=reset]\n",
QEMU_ARCH_ALL)

DEF("echr", HAS_ARG, QEMU_OPTION_echr, \
"-echr chr       set terminal escape character instead of ctrl-a\n",
QEMU_ARCH_ALL)

DEF("virtioconsole", HAS_ARG, QEMU_OPTION_virtiocon, \
"-virtioconsole c\n" \
"                set virtio console\n", QEMU_ARCH_ALL)

DEF("show-cursor", 0, QEMU_OPTION_show_cursor, \
"-show-cursor    show cursor\n", QEMU_ARCH_ALL)

DEF("tb-size", HAS_ARG, QEMU_OPTION_tb_size, \
"-tb-size n      set TB size\n", QEMU_ARCH_ALL)

DEF("incoming", HAS_ARG, QEMU_OPTION_incoming, \
"-incoming p     prepare for incoming migration, listen on port p\n",
QEMU_ARCH_ALL)

DEF("nodefaults", 0, QEMU_OPTION_nodefaults, \
"-nodefaults     don't create default devices\n", QEMU_ARCH_ALL)

#ifndef _WIN32
DEF("chroot", HAS_ARG, QEMU_OPTION_chroot, \
"-chroot dir     chroot to dir just before starting the VM\n",
QEMU_ARCH_ALL)
#endif

#ifndef _WIN32
DEF("runas", HAS_ARG, QEMU_OPTION_runas, \
"-runas user     change to user id user just before starting the VM\n",
QEMU_ARCH_ALL)
#endif

DEF("prom-env", HAS_ARG, QEMU_OPTION_prom_env,
"-prom-env variable=value\n"
"                set OpenBIOS nvram variables\n",
QEMU_ARCH_PPC | QEMU_ARCH_SPARC)
DEF("semihosting", 0, QEMU_OPTION_semihosting,
"-semihosting    semihosting mode\n", QEMU_ARCH_ARM | QEMU_ARCH_M68K)
DEF("old-param", 0, QEMU_OPTION_old_param,
"-old-param      old param mode\n", QEMU_ARCH_ARM)

DEF("readconfig", HAS_ARG, QEMU_OPTION_readconfig,
"-readconfig <file>\n", QEMU_ARCH_ALL)
DEF("writeconfig", HAS_ARG, QEMU_OPTION_writeconfig,
"-writeconfig <file>\n"
"                read/write config file\n", QEMU_ARCH_ALL)
DEF("nodefconfig", 0, QEMU_OPTION_nodefconfig,
"-nodefconfig\n"
"                do not load default config files at startup\n",
QEMU_ARCH_ALL)

