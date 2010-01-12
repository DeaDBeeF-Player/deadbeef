# Microsoft Developer Studio Project File - Name="libsidplay" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libsidplay - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libsidplay.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libsidplay.mak" CFG="libsidplay - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libsidplay - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libsidplay - Win32 Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libsidplay - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../include" /I "../../include/sidplay" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DLL_EXPORT" /D "HAVE_MSWINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../../../binaries/Release/libsidplay2.dll"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "libsidplay - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "../../include" /I "../../include/sidplay" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DLL_EXPORT" /D "HAVE_MSWINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"../../../binaries/Debug/libsidplay2.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "libsidplay - Win32 Release"
# Name "libsidplay - Win32 Debug"
# Begin Group "Components"

# PROP Default_Filter ""
# Begin Group "MOS6510"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\mos6510\conf6510.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\mos6510.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\mos6510.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\cycle_based\mos6510c.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\cycle_based\mos6510c.i
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\opcodes.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\cycle_based\sid6510c.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\cycle_based\sid6510c.i
# End Source File
# End Group
# Begin Group "MOS6526"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\mos6526\mos6526.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6526\mos6526.h
# End Source File
# End Group
# Begin Group "MOS656X"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\mos656x\mos656x.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\mos656x\mos656x.h
# End Source File
# End Group
# Begin Group "SID6526"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\sid6526\sid6526.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sid6526\sid6526.h
# End Source File
# End Group
# Begin Group "SidTune"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\sidplay\Buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\IconInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\InfoFile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\MUS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\PP20.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\PP20.h
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\PP20_Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\PSID.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\SidTune.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\SidTune.h
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\SidTuneCfg.h
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\SidTuneTools.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\SidTuneTools.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\SmartPtr.h
# End Source File
# End Group
# Begin Group "xSID"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\xsid\xsid.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\xsid\xsid.h
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=..\..\src\c64\c64cia.h
# End Source File
# Begin Source File

SOURCE=..\..\src\c64\c64vic.h
# End Source File
# Begin Source File

SOURCE=..\..\src\c64\c64xsid.h
# End Source File
# Begin Source File

SOURCE=..\..\include\config.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=..\..\src\nullsid.h
# End Source File
# Begin Source File

SOURCE=..\..\src\player.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sidconfig.h
# End Source File
# End Group
# Begin Group "Header Files (Public)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\sidplay\c64env.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\component.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\event.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sid2types.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sidbuilder.h
# End Source File
# Begin Source File

SOURCE=.\sidconfig.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sidendian.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidenv.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sidplay.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sidplay2.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sidtypes.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "cpp;cc;cxx;c;i"
# Begin Source File

SOURCE=..\..\src\config.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\event.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\mixer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\player.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\psiddrv.a65
# End Source File
# Begin Source File

SOURCE=..\..\src\psiddrv.bin
# End Source File
# Begin Source File

SOURCE=..\..\src\psiddrv.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\reloc65.c
# End Source File
# Begin Source File

SOURCE=..\..\src\sidplay2.cpp
# End Source File
# End Group
# End Target
# End Project
