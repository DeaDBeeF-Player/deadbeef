#define MyAppName "DeaDBeeF"
#define MyAppNameShort "DeaDBeeF"
; Version based on date
; #define MyAppVersion GetDateTimeString('yyyy-mm-dd', '', '');
; Version defined manually
;#define MyAppVersion "1.8.0"
; Version saved in deadbeef source ('PORTABLE' file)
#define VerFile FileOpen("../../build_data/VERSION")
#define MyAppVersion FileRead(VerFile)
#expr FileClose(VerFile)
;#undef VerFile

;#define DEBUG

#define MyAppPublisher "DeaDBeeF"
#define MyAppURL "https://deadbeef.sourceforge.net"
#define MyAppExeName "deadbeef.exe"

#ifdef DEBUG
#define BuildPath "..\..\bin\debug"
#define OutputSuffix "windows-x86_64_DEBUG"
#else
#define BuildPath "..\..\bin\release"
#define OutputSuffix "windows-x86_64"
#endif
#define IconPath "deadbeef.bmp"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{4E5B405A-0184-4256-B305-5B57F71B6093}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName}
ArchitecturesInstallIn64BitMode=x64
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf64}\DeaDBeeF
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
DisableDirPage=no
OutputBaseFilename=deadbeef-{#MyAppVersion}-{#OutputSuffix}
Compression=lzma
SolidCompression=yes
WizardSmallImageFile=deadbeef.bmp
UninstallDisplayIcon={app}\{#MyAppExeName}
ChangesAssociations = yes
PrivilegesRequiredOverridesAllowed=commandline dialog

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: association; Description: "Associate with supported audio files"; GroupDescription: File extensions:

[Files]
Source: "{#BuildPath}\*"; DestDir: "{app}"; Excludes: "config"; Flags: recursesubdirs ignoreversion 
Source: "{#BuildPath}\config\*"; DestDir: "{userappdata}\deadbeef\"; Flags: recursesubdirs onlyifdoesntexist uninsneveruninstall
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Dirs]
Name: "{userappdata}\deadbeef\plugins"; Flags: uninsneveruninstall

[Icons]
Name: "{group}\{#MyAppNameShort}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppNameShort}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Registry]
Root: HKCR; Subkey: ".cue"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".aac"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".mp4"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".m4a"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".m4b"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".ape"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".flac"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue    ; Tasks: association
Root: HKCR; Subkey: ".oga"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".mp1"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".mp2"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".mp3"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".mpga"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue    ; Tasks: association
Root: HKCR; Subkey: ".mpc"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".mpp"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".mp+"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".ogg"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".opus"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue    ; Tasks: association
Root: HKCR; Subkey: ".ogv"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".psf"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".psf2"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue    ; Tasks: association
Root: HKCR; Subkey: ".spu"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".ssf"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".qsf"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".dsf"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".minipsf"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue ; Tasks: association
Root: HKCR; Subkey: ".minipsf2"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue; Tasks: association
Root: HKCR; Subkey: ".minissf"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue ; Tasks: association
Root: HKCR; Subkey: ".miniqsf"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue ; Tasks: association
Root: HKCR; Subkey: ".minidsf"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue ; Tasks: association
Root: HKCR; Subkey: ".wav"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".aif"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".aiff"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue    ; Tasks: association
Root: HKCR; Subkey: ".snd"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".au"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue      ; Tasks: association
Root: HKCR; Subkey: ".paf"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".svx"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".nist"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue    ; Tasks: association
Root: HKCR; Subkey: ".voc"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".ircam"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue   ; Tasks: association
Root: HKCR; Subkey: ".w64"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".mat4"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue    ; Tasks: association
Root: HKCR; Subkey: ".mat5"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue    ; Tasks: association
Root: HKCR; Subkey: ".pvf"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".xi"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue      ; Tasks: association
Root: HKCR; Subkey: ".htk"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".sds"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".avr"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".wavex"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue   ; Tasks: association
Root: HKCR; Subkey: ".sd2"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".caf"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".wve"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".ogg"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".ogx"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".oga"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue     ; Tasks: association
Root: HKCR; Subkey: ".wv"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue      ; Tasks: association
Root: HKCR; Subkey: "{#MyAppName}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletekey  ; Tasks: association
Root: HKCR; Subkey: "{#MyAppName}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\deadbeef.exe,0"     ; Tasks: association
Root: HKCR; Subkey: "{#MyAppName}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1""" ; Tasks: association
