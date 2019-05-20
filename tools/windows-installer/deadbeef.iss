#define MyAppName "DeaDBeeF for Windows"
#define MyAppNameShort "DeaDBeeF"
; Version based on date
#define MyAppVersion GetDateTimeString('yyyy-mm-dd', '', '');
; Version defined manually
;#define MyAppVersion "1.8.0"
; Version saved in deadbeef source ('PORTABLE' file)
;#define VerFile FileOpen("../../PORTABLE")
;#define MyAppVersion FileRead(VerFile)
;#expr FileClose(VerFile)
;#undef VerFile

;#define DEBUG

#define MyAppPublisher "kuba_160"
#define MyAppURL "https://deadbeef-for-windows.github.io/"
#define MyAppExeName "deadbeef.exe"

#ifdef DEBUG
#define BuildPath "..\..\bin\debug"
#define MyAppVersion MyAppVersion + "-DEBUG"
#else
#define BuildPath "..\..\bin\release"
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
DefaultDirName={pf}\DeaDBeeF
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
DisableDirPage=no
OutputBaseFilename=setup
Compression=lzma
SolidCompression=yes
WizardSmallImageFile=deadbeef.bmp
UninstallDisplayIcon={app}\{#MyAppExeName}
ChangesAssociations = yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: association; Description: "Associate supported audio files"; GroupDescription: File extensions:

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
Root: HKCR; Subkey: ".mp3"; ValueType: string; ValueName: ""; ValueData: "{#MyAppNameShort}"; Flags: uninsdeletevalue  ; Tasks: association 
Root: HKCR; Subkey: ".flac"; ValueType: string; ValueName: ""; ValueData: "{#MyAppNameShort}"; Flags: uninsdeletevalue ; Tasks: association 
Root: HKCR; Subkey: ".aac"; ValueType: string; ValueName: ""; ValueData: "{#MyAppNameShort}"; Flags: uninsdeletevalue  ; Tasks: association 
Root: HKCR; Subkey: "{#MyAppNameShort}"; ValueType: string; ValueName: ""; ValueData: "DeaDBeeF for Windows"; Flags: uninsdeletekey; Tasks: association 
Root: HKCR; Subkey: "{#MyAppNameShort}\DefaultIcon";  ValueData: "{app}\{#MyAppExeName},0";  ValueType: string;  ValueName: "" ; Tasks: association 
Root: HKCR; Subkey: "{#MyAppNameShort}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1""" ; Tasks: association 