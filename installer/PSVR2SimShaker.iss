#ifndef AppVersion
  #define AppVersion "0.2.2-alpha"
#endif
#ifndef AppNumericVersion
  #define AppNumericVersion "0.2.2"
#endif
#ifndef AppDisplayVersion
  #define AppDisplayVersion "0.2.2 Alpha"
#endif
#ifndef PackageDir
  #define PackageDir "..\dist\package"
#endif
[Setup]
AppId={{1051BB21-1FCD-4EDC-B2B6-813B730DDB73}
AppName=PSVR2SimShaker
AppVersion={#AppVersion}
AppVerName=PSVR2SimShaker {#AppDisplayVersion}
VersionInfoVersion={#AppNumericVersion}
VersionInfoProductVersion={#AppNumericVersion}
VersionInfoProductTextVersion={#AppVersion}
AppPublisher=Adam Chesters
AppPublisherURL=https://github.com/AdamChesters/PSVR2SimShaker
AppSupportURL=https://github.com/AdamChesters/PSVR2SimShaker/issues
DefaultDirName={localappdata}\Programs\PSVR2SimShaker
DefaultGroupName=PSVR2SimShaker
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
DisableProgramGroupPage=yes
WizardStyle=modern
LicenseFile=..\LICENSE
OutputDir=..\dist
OutputBaseFilename=PSVR2SimShaker-{#AppVersion}-Setup
Compression=lzma2
SolidCompression=yes
UninstallDisplayIcon={app}\PSVR2SimShaker.exe
SetupIconFile=..\assets\PSVR2SimShaker.ico
CloseApplications=yes
RestartApplications=no
SetupLogging=yes

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked

[Files]
Source: "{#PackageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueName: "PSVR2SimShaker"; Flags: deletevalue

[Icons]
Name: "{group}\PSVR2SimShaker"; Filename: "{app}\PSVR2SimShaker.exe"
Name: "{autodesktop}\PSVR2SimShaker"; Filename: "{app}\PSVR2SimShaker.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\PSVR2SimShaker.exe"; Description: "Open PSVR2SimShaker"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{app}\PSVR2SimShaker.exe"; Parameters: "--uninstall-integration"; Flags: runhidden waituntilterminated; RunOnceId: "RemoveDcsHooks"

; Personal settings are deliberately retained for reinstalls.
