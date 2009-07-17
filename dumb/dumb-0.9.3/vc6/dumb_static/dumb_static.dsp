# Microsoft Developer Studio Project File - Name="dumb_static" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=dumb_static - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dumb_static.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dumb_static.mak" CFG="dumb_static - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dumb_static - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "dumb_static - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dumb_static - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /O2 /I "../../include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DUMB_DECLARE_DEPRECATED" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "dumb_static - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /I "../../include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DUMB_DECLARE_DEPRECATED" /D DEBUGMODE=1 /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\dumbd_static.lib"

!ENDIF 

# Begin Target

# Name "dumb_static - Win32 Release"
# Name "dumb_static - Win32 Debug"
# Begin Group "src"

# PROP Default_Filter ""
# Begin Group "core"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\core\atexit.c
# End Source File
# Begin Source File

SOURCE=..\..\src\core\duhlen.c
# End Source File
# Begin Source File

SOURCE=..\..\src\core\duhtag.c
# End Source File
# Begin Source File

SOURCE=..\..\src\core\dumbfile.c
# End Source File
# Begin Source File

SOURCE=..\..\src\core\loadduh.c
# End Source File
# Begin Source File

SOURCE=..\..\src\core\makeduh.c
# End Source File
# Begin Source File

SOURCE=..\..\src\core\rawsig.c
# End Source File
# Begin Source File

SOURCE=..\..\src\core\readduh.c
# End Source File
# Begin Source File

SOURCE=..\..\src\core\register.c
# End Source File
# Begin Source File

SOURCE=..\..\src\core\rendduh.c
# End Source File
# Begin Source File

SOURCE=..\..\src\core\rendsig.c
# End Source File
# Begin Source File

SOURCE=..\..\src\core\unload.c
# End Source File
# End Group
# Begin Group "helpers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\helpers\clickrem.c
# End Source File
# Begin Source File

SOURCE=..\..\src\helpers\memfile.c
# End Source File
# Begin Source File

SOURCE=..\..\src\helpers\resample.c
# End Source File
# Begin Source File

SOURCE=..\..\src\helpers\sampbuf.c
# End Source File
# Begin Source File

SOURCE=..\..\src\helpers\silence.c
# End Source File
# Begin Source File

SOURCE=..\..\src\helpers\stdfile.c
# End Source File
# End Group
# Begin Group "it"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\it\itload.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\itload2.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\itmisc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\itorder.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\itread.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\itread2.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\itrender.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\itunload.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\loadmod.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\loadmod2.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\loads3m.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\loads3m2.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\loadxm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\loadxm2.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\readmod.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\readmod2.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\reads3m.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\reads3m2.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\readxm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\readxm2.c
# End Source File
# Begin Source File

SOURCE=..\..\src\it\xmeffect.c
# End Source File
# End Group
# End Group
# Begin Group "include"

# PROP Default_Filter ""
# Begin Group "internal"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\internal\dumb.h
# End Source File
# Begin Source File

SOURCE=..\..\include\internal\it.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\include\dumb.h
# End Source File
# End Group
# End Target
# End Project
