# Microsoft Developer Studio Project File - Name="VGMPlay" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=VGMPlay - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "VGMPlay.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "VGMPlay.mak" CFG="VGMPlay - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "VGMPlay - Win32 Release" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE "VGMPlay - Win32 Debug" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Ox /Ot /Og /Oi /Ob2 /I "zlib" /D "NDEBUG" /D "WIN32_LEAN_AND_MEAN" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "ENABLE_ALL_CORES" /D "CONSOLE_MODE" /D "ADDITIONAL_FORMATS" /D "SET_CONSOLE_TITLE" /FD /c
# SUBTRACT CPP /Oa /Ow /YX /Yc /Yu
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 msvcrt.lib kernel32.lib user32.lib advapi32.lib winmm.lib zdll.lib /nologo /subsystem:console /machine:I386 /nodefaultlib /libpath:"zlib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=..\vgm2txt\HiddenMsg.exe Release\VGMPlay.exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "zlib" /D "_DEBUG" /D "WIN32_LEAN_AND_MEAN" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "ENABLE_ALL_CORES" /D "CONSOLE_MODE" /D "ADDITIONAL_FORMATS" /D "SET_CONSOLE_TITLE" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib kernel32.lib user32.lib advapi32.lib winmm.lib zdll.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib /libpath:"zlib"
# SUBTRACT LINK32 /profile /map

!ENDIF 

# Begin Target

# Name "VGMPlay - Win32 Release"
# Name "VGMPlay - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\pt_ioctl.c
# End Source File
# Begin Source File

SOURCE=.\Stream.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\VGMPlay.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\VGMPlay_AddFmts.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\VGMPlayUI.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\PortTalk_IOCTL.h
# End Source File
# Begin Source File

SOURCE=.\Stream.h
# End Source File
# Begin Source File

SOURCE=".\XMasFiles\SWJ-SQRC01_1C.h"
# End Source File
# Begin Source File

SOURCE=.\VGMFile.h
# End Source File
# Begin Source File

SOURCE=.\VGMPlay.h
# End Source File
# Begin Source File

SOURCE=.\VGMPlay_Intf.h
# End Source File
# Begin Source File

SOURCE=.\XMasFiles\XMasBonus.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "SoundCore"

# PROP Default_Filter "c;h;cpp"
# Begin Group "FM OPL Chips"

# PROP Default_Filter "c;h"
# Begin Source File

SOURCE=.\chips\2413intf.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\2413intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\2413tone.h
# End Source File
# Begin Source File

SOURCE=.\chips\262intf.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\262intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\281btone.h
# End Source File
# Begin Source File

SOURCE=.\chips\3526intf.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /W1 /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

# ADD CPP /W1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\3526intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\3812intf.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /W1 /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

# ADD CPP /W1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\3812intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\8950intf.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /W1 /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

# ADD CPP /W1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\8950intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\adlibemu.h
# End Source File
# Begin Source File

SOURCE=.\chips\adlibemu_opl2.c
# End Source File
# Begin Source File

SOURCE=.\chips\adlibemu_opl3.c
# End Source File
# Begin Source File

SOURCE=.\chips\emu2413.c
# End Source File
# Begin Source File

SOURCE=.\chips\emu2413.h
# End Source File
# Begin Source File

SOURCE=.\chips\emutypes.h
# End Source File
# Begin Source File

SOURCE=.\chips\fmopl.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\chips\fmopl.h
# End Source File
# Begin Source File

SOURCE=.\chips\opl.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\chips\opl.h
# End Source File
# Begin Source File

SOURCE=.\chips\vrc7tone.h
# End Source File
# Begin Source File

SOURCE=.\chips\ym2413.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\chips\ym2413.h
# End Source File
# Begin Source File

SOURCE=.\chips\ymf262.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\chips\ymf262.h
# End Source File
# Begin Source File

SOURCE=.\chips\ymf278b.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\chips\ymf278b.h
# End Source File
# End Group
# Begin Group "FM OPN Chips"

# PROP Default_Filter "c;h"
# Begin Source File

SOURCE=.\chips\2203intf.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\2203intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\2608intf.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\2608intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\2610intf.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\2610intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\2612intf.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\2612intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\fm.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\chips\fm.h
# End Source File
# Begin Source File

SOURCE=.\chips\fm2612.c
# End Source File
# Begin Source File

SOURCE=.\chips\ym2612.c
# End Source File
# Begin Source File

SOURCE=.\chips\ym2612.h
# End Source File
# End Group
# Begin Group "FM OPx Chips"

# PROP Default_Filter "c;h"
# Begin Source File

SOURCE=.\chips\2151intf.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\2151intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\scsp.c
# End Source File
# Begin Source File

SOURCE=.\chips\scsp.h
# End Source File
# Begin Source File

SOURCE=.\chips\scspdsp.c
# End Source File
# Begin Source File

SOURCE=.\chips\scspdsp.h
# End Source File
# Begin Source File

SOURCE=.\chips\scsplfo.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\chips\ym2151.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\chips\ym2151.h
# End Source File
# Begin Source File

SOURCE=.\chips\ymf271.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\chips\ymf271.h
# End Source File
# End Group
# Begin Group "PCM Chips"

# PROP Default_Filter "c;h"
# Begin Source File

SOURCE=.\chips\c140.c
# End Source File
# Begin Source File

SOURCE=.\chips\c140.h
# End Source File
# Begin Source File

SOURCE=.\chips\c352.c
# End Source File
# Begin Source File

SOURCE=.\chips\c352.h
# End Source File
# Begin Source File

SOURCE=.\chips\es5503.c
# End Source File
# Begin Source File

SOURCE=.\chips\es5503.h
# End Source File
# Begin Source File

SOURCE=.\chips\es5506.c
# End Source File
# Begin Source File

SOURCE=.\chips\es5506.h
# End Source File
# Begin Source File

SOURCE=.\chips\iremga20.c
# End Source File
# Begin Source File

SOURCE=.\chips\iremga20.h
# End Source File
# Begin Source File

SOURCE=.\chips\k053260.c
# End Source File
# Begin Source File

SOURCE=.\chips\k053260.h
# End Source File
# Begin Source File

SOURCE=.\chips\k054539.c
# End Source File
# Begin Source File

SOURCE=.\chips\k054539.h
# End Source File
# Begin Source File

SOURCE=.\chips\multipcm.c
# End Source File
# Begin Source File

SOURCE=.\chips\multipcm.h
# End Source File
# Begin Source File

SOURCE=.\chips\okim6258.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\okim6258.h
# End Source File
# Begin Source File

SOURCE=.\chips\okim6295.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\okim6295.h
# End Source File
# Begin Source File

SOURCE=.\chips\pwm.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\pwm.h
# End Source File
# Begin Source File

SOURCE=.\chips\qsound.c
# End Source File
# Begin Source File

SOURCE=.\chips\qsound.h
# End Source File
# Begin Source File

SOURCE=.\chips\rf5c68.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\rf5c68.h
# End Source File
# Begin Source File

SOURCE=.\chips\scd_pcm.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\scd_pcm.h
# End Source File
# Begin Source File

SOURCE=.\chips\segapcm.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\segapcm.h
# End Source File
# Begin Source File

SOURCE=.\chips\upd7759.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\upd7759.h
# End Source File
# Begin Source File

SOURCE=.\chips\x1_010.c
# End Source File
# Begin Source File

SOURCE=.\chips\x1_010.h
# End Source File
# Begin Source File

SOURCE=.\chips\ymdeltat.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\ymdeltat.h
# End Source File
# Begin Source File

SOURCE=.\chips\ymz280b.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /W1 /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

# ADD CPP /W1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\ymz280b.h
# End Source File
# End Group
# Begin Group "OPL Mapper"

# PROP Default_Filter "c;h"
# Begin Source File

SOURCE=.\chips\ay8910_opl.c
# End Source File
# Begin Source File

SOURCE=.\chips\sn76496_opl.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\ym2413_opl.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /W1 /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\ym2413hd.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /W1 /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\ym2413hd.h
# End Source File
# End Group
# Begin Group "PSG Chips"

# PROP Default_Filter "c;h"
# Begin Source File

SOURCE=.\chips\ay8910.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\ay8910.h
# End Source File
# Begin Source File

SOURCE=.\chips\ay_intf.c
# End Source File
# Begin Source File

SOURCE=.\chips\ay_intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\c6280.c
# End Source File
# Begin Source File

SOURCE=.\chips\c6280.h
# End Source File
# Begin Source File

SOURCE=.\chips\c6280intf.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\c6280intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\emu2149.c
# End Source File
# Begin Source File

SOURCE=.\chips\emu2149.h
# End Source File
# Begin Source File

SOURCE=.\chips\gb.c
# End Source File
# Begin Source File

SOURCE=.\chips\gb.h
# End Source File
# Begin Source File

SOURCE=.\chips\k051649.c
# End Source File
# Begin Source File

SOURCE=.\chips\k051649.h
# End Source File
# Begin Source File

SOURCE=.\chips\Ootake_PSG.c
# End Source File
# Begin Source File

SOURCE=.\chips\Ootake_PSG.h
# End Source File
# Begin Source File

SOURCE=.\chips\pokey.c
# End Source File
# Begin Source File

SOURCE=.\chips\pokey.h
# End Source File
# Begin Source File

SOURCE=.\chips\saa1099.c
# End Source File
# Begin Source File

SOURCE=.\chips\saa1099.h
# End Source File
# Begin Source File

SOURCE=.\chips\sn76489.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\sn76489.h
# End Source File
# Begin Source File

SOURCE=.\chips\sn76496.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\sn76496.h
# End Source File
# Begin Source File

SOURCE=.\chips\sn764intf.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\sn764intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\vsu.c
# End Source File
# Begin Source File

SOURCE=.\chips\vsu.h
# End Source File
# Begin Source File

SOURCE=.\chips\ws_audio.c
# End Source File
# Begin Source File

SOURCE=.\chips\ws_audio.h
# End Source File
# Begin Source File

SOURCE=.\chips\ws_initialIo.h
# End Source File
# End Group
# Begin Group "NES Chips"

# PROP Default_Filter "c;h"
# Begin Source File

SOURCE=.\chips\nes_apu.c
# End Source File
# Begin Source File

SOURCE=.\chips\nes_apu.h
# End Source File
# Begin Source File

SOURCE=.\chips\nes_defs.h
# End Source File
# Begin Source File

SOURCE=.\chips\nes_intf.c
# End Source File
# Begin Source File

SOURCE=.\chips\nes_intf.h
# End Source File
# Begin Source File

SOURCE=.\chips\np_nes_apu.c
# End Source File
# Begin Source File

SOURCE=.\chips\np_nes_apu.h
# End Source File
# Begin Source File

SOURCE=.\chips\np_nes_dmc.c
# End Source File
# Begin Source File

SOURCE=.\chips\np_nes_dmc.h
# End Source File
# Begin Source File

SOURCE=.\chips\np_nes_fds.c
# End Source File
# Begin Source File

SOURCE=.\chips\np_nes_fds.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\chips\ChipIncl.h
# End Source File
# Begin Source File

SOURCE=.\ChipMapper.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ChipMapper.h
# End Source File
# Begin Source File

SOURCE=.\chips\dac_control.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\dac_control.h
# End Source File
# Begin Source File

SOURCE=.\chips\mamedef.h
# End Source File
# Begin Source File

SOURCE=.\chips\panning.c

!IF  "$(CFG)" == "VGMPlay - Win32 Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "VGMPlay - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chips\panning.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
