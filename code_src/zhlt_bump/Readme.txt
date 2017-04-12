
==========
Background
==========

    ZHLT is developed on two main systems: A Dual Processor (p3-700) 
Windows 2000 machine, and a Dual Processor (p3-500) Linux machine running Debian.
The released versions for windows are compiled with the Intel C/C++ 4.5 compiler
and GNU make.  The visual studio projects, while included, aren't used and have
a tendency to not always have all the newborn files in a release added to their
projects correctly :)


===============
Windows Systems
===============
Requirements:
 (x)   Microsoft Visual C++ 6

Recommended:
 (x)   Intel C/C++ Compiler 4.0 or 4.5
 (x)   GNU make, on Windows, Cygwin can provide a working GNU make, available
    at http://sourceware.cygnus.com/cygwin/
 (x)   CommonC++ 1.4.2+ libraries (for compiling netvis) Available at 
    http://sourceforge.net/project/?group_id=1523

    The project can either be built with GNU make (including on Windows 
platforms) or directly with Microsoft Visual Studio 6 (just open zhlt.dsw)
When using GNU make, choosing between MSVC or ICL requires editing make.ctl
and changing the line COMPILER:=intel with COMPILER:=msvc


==================
Unix/Posix Systems
==================
Requirements:
 (x)   g++
 (x)   GNU make
 (x)   pthread compatibility

Recommended:
 (x)   CommonC++ 1.4.2+ libraries (for compiling netvis) Available at 
    http://sourceforge.net/project/?group_id=1523

ATTENTION: Currently the posix tree of CommonC++ will compile and link with ZHLT 2.5.x
but the socket code mysteriously seems to not be working.  This should be addressed in
an upcoming point release in either ZHLT or CommonC++ when the problem is found and fixed.

	The unix compile relies on several things, which probably needs friendlier
cross-platform handling.  The big requirements are requiring <asm/atomic.h> and
<pthread.h>, GNU make, and gcc.
	configure MUST be run.  The 'makefile' that exists while looking innocent enough
has a few things missing and is setup for GNU make on windows to work out of the box.
configure has to run to populate some variables from makefile.in and generate a 
custom makefile for the machine in question.
    The only known issue with compiling Netvis & CommonC++ was that PTHREAD_HAVE_RWLOCK
was defined through CommonC++'s and in their config.h and my Linux/Debian system did not like
that define enabled, so I had to edit config.h and remove the #define PTHREAD_HAVE_RWLOCK before
building CommonC++ and installing it.


******************************
*** Linux: Debian & Redhat ***
******************************
    Should work with no changes.  Don't know about the other flavors.

***************
*** FreeBSD ***
***************
    I dont have a FreeBSD system so this information comes 2nd hand from a user:
Linux emulation needed (linux_base_6.1.tar.gz or newer?)
Requires fbsd4.1 stable to work
Had to edit make.os and replace -lpthread with -pthread
    


=========
Misc Info
=========
****************
*** make.ctl *** 
****************
The file make.ctl controls options in the make process
MODE   : sets the default mode of compilation.  valid choices are
         release, debug, super_debug (runtime heap checking), and 
         release_w_symbols (meaningless on unix)
MAPS   : On Win32 systems, controls whether or not .map files are
         generated on link.


*************************
*** WAD files on unix ***
*************************
    Create a mirror of your Half-life directory somewhere, but only 
include .wad files.  Make sure the filenames match up as it will become 
case sensitive.  Next, create an environment variable called WADROOT and
set it to this directory (with no trailing slash).

Example:
WADROOT=/home/mcp/wads

mcp@oven:~$ ls -alR wads
wads:
total 28
drwsrwsr-x    7 mcp      mcpadmin     4096 Aug  2 21:23 .
drwxr-sr-x   14 mcp      mcp          4096 Oct  8 00:13 ..
drwsrwsr-x    2 mcp      mcpadmin     4096 Jul 19 14:54 gearbox
drwsrwsr-x    2 mcp      mcpadmin     4096 Jul 19 14:38 valve

wads/gearbox:
total 10364
drwsrwsr-x    2 mcp      mcpadmin     4096 Jul 19 14:54 .
drwsrwsr-x    7 mcp      mcpadmin     4096 Aug  2 21:23 ..
-rw-rw-r--    1 mcp      mcpadmin  1047952 Jul 13 00:56 decals.wad
-rw-rw-r--    1 mcp      mcpadmin  4085336 Jul 17 18:18 op4ctf.wad
-rw-rw-r--    1 mcp      mcpadmin  5308820 Jul 13 00:56 opfor.wad

wads/valve:
total 45272
drwsrwsr-x    2 mcp      mcpadmin     4096 Jul 19 14:38 .
drwsrwsr-x    7 mcp      mcpadmin     4096 Aug  2 21:23 ..
-rw-rw-r--    1 mcp      mcpadmin   924840 Jul 13 00:57 decals.wad
-rw-rw-r--    1 mcp      mcpadmin 37914096 Jul 13 00:57 halflife.wad
-rw-rw-r--    1 mcp      mcpadmin   475820 Jul 13 00:57 liquids.wad
-rw-rw-r--    1 mcp      mcpadmin  6514528 Jul 13 00:57 xeno.wad
-rw-rw-r--    1 mcp      mcpadmin     2380 Jul 13 00:57 zhlt.wad


