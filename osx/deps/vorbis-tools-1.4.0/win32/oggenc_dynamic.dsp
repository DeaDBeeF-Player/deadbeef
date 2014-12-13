# Microsoft Developer Studio Project File - Name="oggenc_dynamic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=oggenc_dynamic - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "oggenc_dynamic.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "oggenc_dynamic.mak" CFG="oggenc_dynamic - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "oggenc_dynamic - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "oggenc_dynamic - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "oggenc_dynamic - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "oggenc_dynamic___Win32_Release"
# PROP BASE Intermediate_Dir "oggenc_dynamic___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release\oggenc\dynamic"
# PROP Intermediate_Dir "Release\oggenc\dynamic"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "..\..\ogg\include" /I "..\..\vorbis\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ogg.lib vorbis.lib vorbisenc.lib /nologo /subsystem:console /machine:I386 /out:"Release\oggenc\dynamic/oggenc.exe" /libpath:"..\..\ogg\win32\Dynamic_Release" /libpath:"..\..\vorbis\win32\Vorbis_Dynamic_Release" /libpath:"..\..\vorbis\win32\VorbisEnc_Dynamic_Release"

!ELSEIF  "$(CFG)" == "oggenc_dynamic - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "oggenc_dynamic___Win32_Debug"
# PROP BASE Intermediate_Dir "oggenc_dynamic___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug\oggenc\dynamic"
# PROP Intermediate_Dir "Debug\oggenc\dynamic"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\include" /I "..\..\ogg\include" /I "..\..\vorbis\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ogg_d.lib vorbis_d.lib vorbisenc_d.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug\oggenc\dynamic/oggenc.exe" /pdbtype:sept /libpath:"..\..\ogg\win32\Dynamic_Debug" /libpath:"..\..\vorbis\win32\Vorbis_Dynamic_Debug" /libpath:"..\..\vorbis\win32\VorbisEnc_Dynamic_Debug"

!ENDIF 

# Begin Target

# Name "oggenc_dynamic - Win32 Release"
# Name "oggenc_dynamic - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\oggenc\audio.c
# End Source File
# Begin Source File

SOURCE=..\oggenc\encode.c
# End Source File
# Begin Source File

SOURCE=..\share\getopt.c
# End Source File
# Begin Source File

SOURCE=..\share\getopt1.c
# End Source File
# Begin Source File

SOURCE=..\oggenc\oggenc.c
# End Source File
# Begin Source File

SOURCE=..\oggenc\platform.c
# End Source File
# Begin Source File

SOURCE=..\oggenc\resample.c
# End Source File
# Begin Source File

SOURCE=..\share\utf8.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\oggenc\audio.h
# End Source File
# Begin Source File

SOURCE=..\oggenc\encode.h
# End Source File
# Begin Source File

SOURCE=..\include\getopt.h
# End Source File
# Begin Source File

SOURCE=..\oggenc\platform.h
# End Source File
# Begin Source File

SOURCE=..\include\utf8.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
