# Microsoft Developer Studio Project File - Name="hlbsp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=hlbsp - Win32 Super_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hlbsp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hlbsp.mak" CFG="hlbsp - Win32 Super_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hlbsp - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "hlbsp - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "hlbsp - Win32 Release w Symbols" (based on "Win32 (x86) Console Application")
!MESSAGE "hlbsp - Win32 Super_Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hlbsp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\common" /I "..\template" /D "NDEBUG" /D "DOUBLEVEC_T" /D "WIN32" /D "_CONSOLE" /D "SYSTEM_WIN32" /D "STDC_HEADERS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 binmode.obj /nologo /stack:0x400000,0x100000 /subsystem:console /machine:I386
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "hlbsp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "..\common" /I "..\template" /D "DOUBLEVEC_T" /D "_DEBUG" /D "STDC_HEADERS" /D "WIN32" /D "_CONSOLE" /D "SYSTEM_WIN32" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 binmode.obj /nologo /stack:0x1000000,0x100000 /subsystem:console /profile /map /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "hlbsp - Win32 Release w Symbols"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_w_Symbols"
# PROP BASE Intermediate_Dir "Release_w_Symbols"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_w_Symbols"
# PROP Intermediate_Dir "Release_w_Symbols"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W4 /GX /O2 /I "..\common" /D "NDEBUG" /D "DOUBLEVEC_T" /D "WIN32" /D "_CONSOLE" /D "SYSTEM_WIN32" /FR /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "..\common" /I "..\template" /D "NDEBUG" /D "DOUBLEVEC_T" /D "WIN32" /D "_CONSOLE" /D "SYSTEM_WIN32" /D "STDC_HEADERS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 binmode.obj /nologo /stack:0x400000,0x100000 /subsystem:console /machine:I386
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 binmode.obj /nologo /stack:0x400000,0x100000 /subsystem:console /pdb:none /map /debug /machine:I386

!ELSEIF  "$(CFG)" == "hlbsp - Win32 Super_Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Super_Debug"
# PROP BASE Intermediate_Dir "Super_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Super_Debug"
# PROP Intermediate_Dir "Super_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W4 /Gm /GX /Zi /Od /I "..\common" /D "DOUBLEVEC_T" /D "_DEBUG" /D "STDC_HEADERS" /D "WIN32" /D "_CONSOLE" /D "SYSTEM_WIN32" /FR /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\common" /I "..\template" /D "DOUBLEVEC_T" /D "_DEBUG" /D "STDC_HEADERS" /D "WIN32" /D "_CONSOLE" /D "SYSTEM_WIN32" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 binmode.obj /nologo /stack:0x1000000,0x100000 /subsystem:console /profile /map /debug /machine:I386
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 binmode.obj /nologo /stack:0x1000000,0x100000 /subsystem:console /profile /map /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "hlbsp - Win32 Release"
# Name "hlbsp - Win32 Debug"
# Name "hlbsp - Win32 Release w Symbols"
# Name "hlbsp - Win32 Super_Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\blockmem.cpp
# End Source File
# Begin Source File

SOURCE=..\common\bspfile.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cmdlib.cpp
# End Source File
# Begin Source File

SOURCE=..\common\filelib.cpp
# End Source File
# Begin Source File

SOURCE=..\common\log.cpp
# End Source File
# Begin Source File

SOURCE=..\common\mathlib.cpp
# End Source File
# Begin Source File

SOURCE=..\common\messages.cpp
# End Source File
# Begin Source File

SOURCE=..\common\scriplib.cpp
# End Source File
# Begin Source File

SOURCE=..\common\threads.cpp
# End Source File
# Begin Source File

SOURCE=.\writebsp.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\merge.cpp
# End Source File
# Begin Source File

SOURCE=.\outside.cpp
# End Source File
# Begin Source File

SOURCE=.\portals.cpp
# End Source File
# Begin Source File

SOURCE=.\qbsp.cpp
# End Source File
# Begin Source File

SOURCE=.\solidbsp.cpp
# End Source File
# Begin Source File

SOURCE=.\surfaces.cpp
# End Source File
# Begin Source File

SOURCE=.\tjunc.cpp
# End Source File
# Begin Source File

SOURCE=..\common\winding.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\common\blockmem.h
# End Source File
# Begin Source File

SOURCE=.\bsp5.h
# End Source File
# Begin Source File

SOURCE=..\common\bspfile.h
# End Source File
# Begin Source File

SOURCE=..\common\cmdlib.h
# End Source File
# Begin Source File

SOURCE=..\common\filelib.h
# End Source File
# Begin Source File

SOURCE=..\common\hlassert.h
# End Source File
# Begin Source File

SOURCE=..\common\log.h
# End Source File
# Begin Source File

SOURCE=..\common\mathlib.h
# End Source File
# Begin Source File

SOURCE=..\common\mathtypes.h
# End Source File
# Begin Source File

SOURCE=..\common\messages.h
# End Source File
# Begin Source File

SOURCE=..\common\scriplib.h
# End Source File
# Begin Source File

SOURCE=..\common\threads.h
# End Source File
# Begin Source File

SOURCE=..\common\win32fix.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
