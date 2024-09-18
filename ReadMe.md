Currently VS2022! TESTFILES build was created via fully updated
VS2022 17.11 on Windows 11 26257 with the Windows 10 20348 SDK.

All code that currently compiles and executes matches an upstream tree now, with 
just MSVC compatibiilty fixes so that building from this repo for GCC/Linux should
also still work just fine (but is untested).

Including SDL 1.2.14 (with some later patches after this release tag up to April 2009)
as it appears timeperiod appropriate for the included compiled libraries in the tree 
to enable clean building. Eventually, that and other supporting libraries will be brought
into the project and built alongside the entire solution.

WinQemu builds the dll, WinQemuTest builds the exe to actually use it

Hardcoded to use D:\Images\ for files. - this is not exactly true anymore as we now have
functional parameters for everything. -L specifies where vgabios.bin is, -bios where... 
the main bios is, and -hda where the disk image is. small.ffs is NetBSD 1.2

Check "WinQemuTest.exe -help" for BIOS options, as by default it skips boot menu 
per default configuration from upstream codebase. 

# Progress Updates

2/13/24 - Build system now fixed to output directories being correct for debug builds. 
To run debugger, copy the two library files from WinQEMU\Binary Artifacts\Debug to the 
WinQEMU\Debug folder (fmod.dll and SDL.dll). You must launch WinQemuTest as the debug
target as WinQemu is built as a DLL.

3/7/2024 - as a change introduced in qemu around August 10, 2009 using typeof() in macros
this feature wasn't supported/standardized in C until C23. Therefore, until we switch to
/std:clatest in the build system (which is impossible to do currently in the VS GUI)
we are using \_\_typeof\_\_ instead, which allows us to use this new implementation 
regardless of current set C language STD. Unfortunately, this means we must use at minimum 
Visual Studio 2022 17.9 or higher, introduced in February 2024 when typeof() and 
\_\_typeof\_\_ support were added.

7/18/24 - -m 1024 causes null pointer failures in some functions, for now we'll just
use -m 512 while working on bringing in some finishes to currently incomplete changes.
Changes to Visual Studio (to remove customizations to this project) 
will necessitate upgrading to VS 2022 17.11. With 17.11 we can also now use
/cstd:clatest so we no longer have to use \_\_typeof\_\_() instead of just typeof()
leaving more code unmodified

# pthreads and vcpkg installation notes 

vcpkg is required to be installed with visual studio 2022. It is a selected component during
Visual Studio installation, however using this version can cause problems.

To allivieate any potential issues, we will use the recent version directly from Microsoft's
public github. 

While location of vcpkg doesn't matter, for our example we will use 'c:\dev\vcpkg'

Within a command prompt, run 'git clone https://github.com/Microsoft/vcpkg.git ' inside 'C:\dev'
directory to get the current vcpkg setup cloned and ready in 'c:\dev\vcpkg'

From inside C:\dev\vcpkg run 'bootstrap-vcpkg.bat'

Next, execute 'vcpkg integrate install' to commence the visual studio integration. 

Finally, run 'vcpkg install pthreads:x86-windows' to install the required pthreads library, 
with the integration it will automatically be found and linked to the project appropriately. 
This will now automatically, after following those steps, build pthreadVC3.dll inside the
target folder as well, which is now required to run. This installs the 32-bit x86 version,
while vcpkg install pthreads alone installs the x64 version by default instead. Currently,
the 64-bit build is non-functional, so we require the 32-bit version. 

Note: Powershell 7 is brought in/used by vcpkg but you should probably install it system 
wide. Otherwise you will get a pwsh.exe is not recognized as an internal or external command 
when attempting the build (but it will still work for now). 

Note: On one system, I had some issue with the include directories being automatically 
recognized in visual studio from the vcpkg directories, which should happen automatically.
Changing the toolchain from VS 2022 to something else and then back again resolved this issue.

# Building instructions

Only Debug/Win32 is currently "fixed up" and working as of this time. 

Visual Studio 2008 / Visual C++ 2008 Runtime DEBUG version required for currently included fmod library
build. (Satisified by having VS2008 installed, or otherwise acquiring MSVCR90D.dll ) - this is next
on roadmap to fix. Setting build to RELEASE will enable it to build/run without VS2022 debugging 
capabilities, but will allow you to use the regular VC++ 2008 Runtime currently. Fix will probably
be removal of fmod dependency. 

For a working debug environment, add to D:\Images\ (currently hard coded) vgabios-cirrus.bin, 
small.ffs, and bios.bin for a minimal x86 emulated system during debugging. This path can 
be modified in project WinQemu\qemu\inc\config-host.h

Add '-net none -cpu coreduo -m 512 -M pc -vga std -sdl -hda D:\Images\small.ffs -bios D:\Images\bios.bin -L D:\Images'  
to the command arguments part of the WinQemuTest project to reproduce the 'test' environment
or the paths of your choosing now that we can specify arbitrary paths. 

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

QEMU BIOS - build: 09/22/09
VGABIOS - "current-cvs 17 Dec 2008"

Working startup commandline:

winqemutest.exe -net none -cpu coreduo -m 512 -M pc -vga std -sdl -hda D:\Images\small.ffs -bios D:\Images\bios.bin -L D:\Images

Adding alpha target bits on the side for OpenVMS, Tru64, and AXP Windows NT emulation over time. 