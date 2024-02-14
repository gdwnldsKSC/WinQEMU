Build is fixed.

All code that currently compiles and executes matches an upstream tree now, with 
just MSVC customizations. 

Including SDL 1.2.14 (with some later patches after this release tag up to April 2009)
as it appears timeperiod appropriate for the included compiled libraries in the tree 
to enable clean building. 

Fixed some include path things

WinQemu buils the dll, WinQemuTest builds the exe to actually use it

Hardcoded to use D:\Images\ for files.

############
2/13/24 - Build system now fixed to output directories being correct for debug builds. 
To run debugger, copy the two library files from WinQEMU\Binary Artifacts\Debug to the 
WinQEMU\Debug folder (fmod.dll and SDL.dll). 

Target debug/win32 is fixed up, anyway. 

As well to D:\Images\ (currently hard coded) vgabios-cirrus.bin, small.ffs, and bios.bin 
for a minimal x86 emulated system during debugging. This path can be modified in project
WinQemu\qemu\inc\config-host.h
############

Folder TESTFILES contains working bios image and 'small' BSD disk with not much on it
but works with these. VGABIOS project for VGA BIOS, and a SeaBIOS image.

QEMU BIOS - build: 05/08/09
VGABIOS - "current-cvs 17 Dec 2008"

Working startup commandline:

winqemutest.exe -net none -cpu coreduo -m 480 -M pc -vga std -sdl -hda D:\Images\small.ffs

Currently VS2013! TESTFILES build was created via fully updated
VS2013 12.0.40629.00 Update 5 on Windows 11 26016.1012

Adding alpha bits now. 