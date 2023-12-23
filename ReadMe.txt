Including SDL 1.2.14 (with some later patches after this release tag up to April 2009)
as it appears timeperiod appropriate for the included compiled libraries in the tree 
to enable clean building. 

Fixed some include path things

WinQemu buils the dll, WinQemuTest builds the exe to actually use it

Hardcoded to use D:\Images\ for files.

Folder TESTFILES contains working bios image and 'small' BSD disk with not much on it
but works with these. VGABIOS project for VGA BIOS, and a SeaBIOS image.

QEMU BIOS - build: 05/08/09
VGABIOS - "current-cvs 17 Dec 2008"

Working startup commandline:

winqemutest.exe -net none -cpu coreduo -m 480 -M pc -vga std -sdl -hda D:\Images\small.ffs

Currently VS2013! TESTFILES build was created via fully updated
VS2013 12.0.40629.00 Update 5 on Windows 11 26016.1012

version 0.10.3:
  - fix AIO cancellations (Avi Kivity)
  - fix live migration error path on incoming
  - avoid SEGV on pci hotplug failure (Chris Wright)
  - fix serial option in -drive
  - support DDIM for option roms (Glauber Costa)
  - avoid fork/exec on pre-2.6.27 kernels with KVM (Jan Kiszka)
  - block-vpc: don't silently create smaller images than requested (Kevin Wolf)
  - Fix non-ACPI timer interrupt routing (Beth Kon)
  - hpet: fix emulation of HPET_TN_SETVAL (Jan Kiszka)
  - kvm: fix cpuid initialization (Jan Kiszka)
  - qcow2: fix corruption on little endian hosts (Kevin Wolf)
  - avoid leaing memory on hot unplug (Mark McLoughlin)
  - fix savevm/migration after hot unplug (Mark McLoughlin)
  - Fix keyboard mapping on newer Xords with non-default keymaps (balrog)
  - Make PCI config status register read-only (Anthony Liguori)
  - Fix crash on resolution change -> screen dump -> vga redraw (Avi Kivity)