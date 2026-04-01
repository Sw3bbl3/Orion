#define AppName "Orion"
#define AppPublisher "WayV"
#define AppExeName "Orion.exe"

#ifndef AppBuildDir
  #error "AppBuildDir is not defined. Pass /DAppBuildDir=<absolute path to staged app files>."
#endif

#ifndef InstallerOutputDir
  #define InstallerOutputDir AddBackslash(SourcePath) + "..\dist\installer"
#endif

#ifndef WizardAssetsDir
  #define WizardAssetsDir AddBackslash(SourcePath) + "art"
#endif

#ifndef SetupIconPath
  #define SetupIconPath AddBackslash(SourcePath) + "..\assets\orion\Orion_logo.ico"
#endif

#define AppVersion GetVersionNumbersString(AddBackslash(AppBuildDir) + AppExeName)

[Setup]
AppId={{7B6DB5DA-17F4-47A7-A6E6-AE00BC521A4E}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayIcon={app}\{#AppExeName}
OutputDir={#InstallerOutputDir}
OutputBaseFilename=Orion-Setup-{#AppVersion}
SetupIconFile={#SetupIconPath}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
WizardImageFile={#WizardAssetsDir}\wizard.bmp
WizardSmallImageFile={#WizardAssetsDir}\wizard-small.bmp
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
DisableProgramGroupPage=yes
AllowNoIcons=yes
UsePreviousAppDir=yes
SetupLogging=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Files]
Source: "{#AppBuildDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent

[Code]
procedure InitializeWizard;
begin
  WizardForm.WelcomeLabel1.Font.Style := [fsBold];
  WizardForm.WelcomeLabel1.Font.Size := 16;
  WizardForm.WelcomeLabel2.Font.Size := 10;
  WizardForm.FinishedHeadingLabel.Font.Style := [fsBold];
  WizardForm.FinishedHeadingLabel.Font.Size := 14;
end;
