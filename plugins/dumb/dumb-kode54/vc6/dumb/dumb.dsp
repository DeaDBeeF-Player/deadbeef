# Microsoft Developer Studio Project File - Name="dumb" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=dumb - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dumb.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dumb.mak" CFG="dumb - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dumb - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "dumb - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dumb - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /I "../../include" /W3 /O2 /Ob2 /D "NDEBUG" /D "WIN32" /D "_LIB" /D "DUMB_DECLARE_DEPRECATED" /D "_MBCS" /GF /Gy /Fp".\Release/dumb.pch" /Fo".\Release/" /Fd".\Release/" /c /GX 
# ADD CPP /nologo /MD /I "../../include" /W3 /O2 /Ob2 /D "NDEBUG" /D "WIN32" /D "_LIB" /D "DUMB_DECLARE_DEPRECATED" /D "_MBCS" /GF /Gy /Fp".\Release/dumb.pch" /Fo".\Release/" /Fd".\Release/" /c /GX 
# ADD BASE MTL /nologo /win32 
# ADD MTL /nologo /win32 
# ADD BASE RSC /l 1033 /d "NDEBUG" 
# ADD RSC /l 1033 /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:".\Release\dumb.lib" 
# ADD LIB32 /nologo /out:".\Release\dumb.lib" 

!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /I "../../include" /ZI /W3 /Od /D "_DEBUG" /D "WIN32" /D "_LIB" /D "DUMB_DECLARE_DEPRECATED" /D "DEBUGMODE=1" /D "_MBCS" /Fp".\Debug/dumb.pch" /Fo".\Debug/" /Fd".\Debug/" /GZ /c /GX 
# ADD CPP /nologo /MDd /I "../../include" /ZI /W3 /Od /D "_DEBUG" /D "WIN32" /D "_LIB" /D "DUMB_DECLARE_DEPRECATED" /D "DEBUGMODE=1" /D "_MBCS" /Fp".\Debug/dumb.pch" /Fo".\Debug/" /Fd".\Debug/" /GZ /c /GX 
# ADD BASE MTL /nologo /win32 
# ADD MTL /nologo /win32 
# ADD BASE RSC /l 1033 /d "_DEBUG" 
# ADD RSC /l 1033 /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"Debug\dumbd.lib" 
# ADD LIB32 /nologo /out:"Debug\dumbd.lib" 

!ENDIF

# Begin Target

# Name "dumb - Win32 Release"
# Name "dumb - Win32 Debug"
# Begin Group "src"

# PROP Default_Filter ""
# Begin Group "core"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\core\atexit.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\core\duhlen.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\core\duhtag.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\core\dumbfile.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\core\loadduh.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\core\makeduh.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\core\rawsig.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\core\readduh.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\core\register.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\core\rendduh.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\core\rendsig.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\core\unload.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# End Group
# Begin Group "helpers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\helpers\barray.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\helpers\clickrem.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\helpers\memfile.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\helpers\resample.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\helpers\sampbuf.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\helpers\silence.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\helpers\stdfile.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# End Group
# Begin Group "it"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\it\itload.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\itmisc.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\itorder.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\itread.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\itrender.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\itunload.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\load669.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\loadmod.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\loadmtm.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\loadoldpsm.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\loadpsm.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\loads3m.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\loadxm.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\ptmeffect.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\read669.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\readmod.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\readmtm.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\readoldpsm.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\readpsm.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\readptm.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\reads3m.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\readxm.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\src\it\xmeffect.c

!IF  "$(CFG)" == "dumb - Win32 Release"

# ADD CPP /nologo /O2 /GX 
!ELSEIF  "$(CFG)" == "dumb - Win32 Debug"

# ADD CPP /nologo /Od /GZ /GX 
!ENDIF

# End Source File
# End Group
# End Group
# Begin Group "include"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\dumb.h
# End Source File
# Begin Group "internal"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\internal\dumb.h
# End Source File
# Begin Source File

SOURCE=..\..\include\internal\it.h
# End Source File
# End Group
# End Group
# End Target
# End Project

