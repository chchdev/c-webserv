; Inno Setup script for c-webserv

#define MyAppName "c-webserv"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "c-webserv"
#define MyAppExeName "webserv-gui.exe"

[Setup]
AppId={{9A4C091D-1FEE-4BF4-9A89-D7D445E7F33D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\c-webserv
DefaultGroupName=c-webserv
DisableProgramGroupPage=yes
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\bin\installer
OutputBaseFilename=c-webserv-setup
SetupIconFile=..\assets\app.ico
UninstallDisplayIcon={app}\bin\webserv-gui.exe
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"; Flags: unchecked

[Files]
Source: "..\bin\webserv.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\bin\webserv-gui.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\www\*"; DestDir: "{app}\www"; Flags: ignoreversion recursesubdirs createallsubdirs

[Dirs]
Name: "{app}\www"

[Icons]
Name: "{autoprograms}\c-webserv\c-webserv"; Filename: "{app}\bin\webserv-gui.exe"; WorkingDir: "{app}\bin"
Name: "{autodesktop}\c-webserv"; Filename: "{app}\bin\webserv-gui.exe"; WorkingDir: "{app}\bin"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\webserv-gui.exe"; Description: "Launch c-webserv"; Flags: nowait postinstall skipifsilent
