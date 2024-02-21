Currently VS2022! TESTFILES build was created via fully updated
VS2022 17.9.0 Windows 11 26052 with the latest 26058 SDK.

All code that currently compiles and executes matches an upstream tree now, with 
just MSVC compatibiilty fixes so that building from this repo for GCC/Linux should
also still work just fine (but is untested).

Including SDL 1.2.14 (with some later patches after this release tag up to April 2009)
as it appears timeperiod appropriate for the included compiled libraries in the tree 
to enable clean building. Eventually, that and other supporting libraries will be brought
into the project and built alongside the entire solution.

WinQemu buils the dll, WinQemuTest builds the exe to actually use it

Hardcoded to use D:\Images\ for files.

# Building instructions

2/13/24 - Build system now fixed to output directories being correct for debug builds. 
To run debugger, copy the two library files from WinQEMU\Binary Artifacts\Debug to the 
WinQEMU\Debug folder (fmod.dll and SDL.dll). You must launch WinQemuTest as the debug
target as WinQemu is built as a DLL.

Only Debug/Win32 is currently "fixed up" and validated. 

For a working debug environment, add to D:\Images\ (currently hard coded) vgabios-cirrus.bin, 
small.ffs, and bios.bin for a minimal x86 emulated system during debugging. This path can 
be modified in project WinQemu\qemu\inc\config-host.h

Add '-net none -cpu coreduo -m 480 -M pc -vga std -sdl -hda D:\Images\small.ffs'  to the
command arguments part of the WinQemuTest project to reproduce the 'test' environment

vcpkg install pthreads:x64-windows will be required. acquire vcpkg from
https://github.com/microsoft/vcpkg/ 

Powershell 7 is brought in/used by vcpkg but you should probably install it system wide.
Otherwise you will get a pwsh.exe is not recognized as an internal or external command 
when attempting the build (but it will still work for now). 

Instructions here: https://github.com/microsoft/vcpkg?tab=readme-ov-file#getting-started

This will now automatically, after following those steps, build pthreadVC3.dll inside the
debug target folder as well, which is now required to run. 

# Included precompiled images/binaries

Folder TESTFILES contains working bios image and 'small' BSD disk with not much on it
but works with these. VGABIOS project for VGA BIOS, and a SeaBIOS image.

QEMU BIOS - build: 05/08/09
VGABIOS - "current-cvs 17 Dec 2008"

Working startup commandline:

winqemutest.exe -net none -cpu coreduo -m 480 -M pc -vga std -sdl -hda D:\Images\small.ffs

Adding alpha target bits on the side for OpenVMS, Tru64, and AXP Windows NT emulation over time. 