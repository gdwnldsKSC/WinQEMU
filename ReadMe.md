Currently VS2022! TESTFILES build was created via fully updated
VS2022 17.9.2 Windows 11 26063 with the Windows 10 20348 SDK.

All code that currently compiles and executes matches an upstream tree now, with 
just MSVC compatibiilty fixes so that building from this repo for GCC/Linux should
also still work just fine (but is untested).

Including SDL 1.2.14 (with some later patches after this release tag up to April 2009)
as it appears timeperiod appropriate for the included compiled libraries in the tree 
to enable clean building. Eventually, that and other supporting libraries will be brought
into the project and built alongside the entire solution.

WinQemu buils the dll, WinQemuTest builds the exe to actually use it

Hardcoded to use D:\Images\ for files. - this is not exactly true anymore as we now have
functional parameters for everything. -L specifies where vgabios.bin is, -bios where... 
the main bios is, and -hda where the disk image is. small.ffs is NetBSD 1.2

Check "WinQemuTest.exe -help" for BIOS options, as by default it skips boot menu 
per default configuration from upstream codebase. 

# Building instructions

2/13/24 - Build system now fixed to output directories being correct for debug builds. 
To run debugger, copy the two library files from WinQEMU\Binary Artifacts\Debug to the 
WinQEMU\Debug folder (fmod.dll and SDL.dll). You must launch WinQemuTest as the debug
target as WinQemu is built as a DLL.

Only Debug/Win32 is currently "fixed up" and validated. 

For a working debug environment, add to D:\Images\ (currently hard coded) vgabios-cirrus.bin, 
small.ffs, and bios.bin for a minimal x86 emulated system during debugging. This path can 
be modified in project WinQemu\qemu\inc\config-host.h

Add '-net none -cpu coreduo -m 480 -M pc -vga std -sdl -hda D:\Images\small.ffs -bios D:\Images\bios.bin -L D:\Images'  
to the command arguments part of the WinQemuTest project to reproduce the 'test' environment
or the paths of your choosing now that we can specify arbitrary paths. 

vcpkg install pthreads:x64-windows will be required. acquire vcpkg from
https://github.com/microsoft/vcpkg/ 

Powershell 7 is brought in/used by vcpkg but you should probably install it system wide.
Otherwise you will get a pwsh.exe is not recognized as an internal or external command 
when attempting the build (but it will still work for now). 

Instructions here: https://github.com/microsoft/vcpkg?tab=readme-ov-file#getting-started

This will now automatically, after following those steps, build pthreadVC3.dll inside the
debug target folder as well, which is now required to run. 

From a bash shell (or WSL) run ./hxtool -h < qemu-options.hx > qemu-options.h
As well do the same thing for qemu-monitor.hx > qemu-monitor.h

Rebaselined on upstream vl.c which now (as of a few iterations ago) uses that header
instead of declaring all the enum and help files in source to allow them to remain in 
sync and export of current version documentation easier.

This will need to be re-done every time there is a change/feature added to these functions,
but for ease of use I will be supplying a working version of qemu-options.h in my repository.

qemu-options generated QEMU_OPTIONS_net must be modified to avoid macro expansion issues
in MSVC C Preprocessor currently. 

# Included precompiled images/binaries

Folder TESTFILES contains working bios image and 'small' BSD disk with not much on it
but works with these. VGABIOS project for VGA BIOS, and a SeaBIOS image.

QEMU BIOS - build: 07/27/09
VGABIOS - "current-cvs 17 Dec 2008"

Working startup commandline:

winqemutest.exe -net none -cpu coreduo -m 480 -M pc -vga std -sdl -hda D:\Images\small.ffs -bios D:\Images\bios.bin -L D:\Images

Adding alpha target bits on the side for OpenVMS, Tru64, and AXP Windows NT emulation over time. 