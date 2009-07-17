# Microsoft Developer Studio Project File - Name="aldumb" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=aldumb - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "aldumb.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aldumb.mak" CFG="aldumb - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aldumb - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "aldumb - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "aldumb - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX- /O2 /I "../../include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DUMB_DECLARE_DEPRECATED" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\aldmb.lib"

!ELSEIF  "$(CFG)" == "aldumb - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX- /ZI /Od /I "../../include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DUMB_DECLARE_DEPRECATED" /D DEBUGMODE=1 /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\aldmd.lib"

!ENDIF 

# Begin Target

# Name "aldumb - Win32 Release"
# Name "aldumb - Win32 Debug"
# Begin Group "src"

# PROP Default_Filter ""
# Begin Group "allegro"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\allegro\alplay.c
# End Source File
# Begin Source File

SOURCE=..\..\src\allegro\datduh.c
# End Source File
# Begin Source File

SOURCE=..\..\src\allegro\datit.c
# End Source File
# Begin Source File

SOURCE=..\..\src\allegro\datitq.c
# End Source File
# Begin Source File

SOURCE=..\..\src\allegro\datmod.c
# End Source File
# Begin Source File

SOURCE=..\..\src\allegro\datmodq.c
# End Source File
# Begin Source File

SOURCE=..\..\src\allegro\dats3m.c
# End Source File
# Begin Source File

SOURCE=..\..\src\allegro\dats3mq.c
# End Source File
# Begin Source File

SOURCE=..\..\src\allegro\datunld.c
# End Source File
# Begin Source File

SOURCE=..\..\src\allegro\datxm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\allegro\datxmq.c
# End Source File
# Begin Source File

SOURCE=..\..\src\allegro\packfile.c
# End Source File
# End Group
# End Group
# Begin Group "include"

# PROP Default_Filter ""
# Begin Group "internal"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\internal\aldumb.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\include\aldumb.h
# End Source File
# End Group
# End Target
# End Project
