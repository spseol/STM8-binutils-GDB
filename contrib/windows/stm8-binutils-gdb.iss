; Inno Setup script of the Windows package of STM8 binutils-gdb.
; Called by contrib/build-windows.sh:
;   ISCC.exe /DAppVersion=15.0.50 /DSourceDir=<stage dir> /DOutputDir=<out dir> stm8-binutils-gdb.iss
;
; Installs into %ProgramFiles%\STM8 binutils-gdb (all users, administrator) or into
; %LocalAppData%\Programs\STM8 binutils-gdb (current user only, no administrator), and
; adds its bin directory to PATH (of the machine or of the user) so that stm8-gdb,
; stm8-as, stm8-ld, ... are found in every new terminal and in VS Code.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef SourceDir
  #error "pass /DSourceDir=<stage dir with bin, lib, share>"
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif

[Setup]
AppId={{5B6E0C52-8A0B-4C5E-9E5F-3D0D9C1E6A41}
AppName=STM8 binutils-gdb
AppVersion={#AppVersion}
AppPublisher=SPSE a VOS Liberec
AppPublisherURL=https://gitlab.com/spseol/mit-no/stm8-binutils-gdb
AppSupportURL=https://gitlab.com/spseol/mit-no/stm8-binutils-gdb/-/issues
DefaultDirName={autopf}\STM8 binutils-gdb
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputDir}
OutputBaseFilename=stm8-binutils-gdb-windows-x64-setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ChangesEnvironment=yes
LicenseFile={#SourceDir}\share\licenses\COPYING3
UninstallDisplayName=STM8 binutils-gdb {#AppVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Tasks]
Name: "addpath"; Description: "Add the bin directory to PATH (needed for stm8-gdb in a terminal and in VS Code)"; Flags: checkedonce

[Code]
const
  UserEnvKey = 'Environment';
  MachineEnvKey = 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';

function EnvRoot(): Integer;
begin
  if IsAdminInstallMode then Result := HKEY_LOCAL_MACHINE
  else Result := HKEY_CURRENT_USER;
end;

function EnvKey(): String;
begin
  if IsAdminInstallMode then Result := MachineEnvKey
  else Result := UserEnvKey;
end;

function PathHasDir(const Path, Dir: String): Boolean;
begin
  Result := Pos(';' + Lowercase(Dir) + ';', ';' + Lowercase(Path) + ';') > 0;
end;

procedure AddToPath();
var
  Path, Dir: String;
begin
  Dir := ExpandConstant('{app}\bin');
  if not RegQueryStringValue(EnvRoot(), EnvKey(), 'Path', Path) then Path := '';
  if PathHasDir(Path, Dir) then exit;
  if (Path <> '') and (Path[Length(Path)] <> ';') then Path := Path + ';';
  RegWriteExpandStringValue(EnvRoot(), EnvKey(), 'Path', Path + Dir);
end;

procedure RemoveFromPath();
var
  Path, Dir: String;
  P: Integer;
begin
  Dir := ExpandConstant('{app}\bin');
  if not RegQueryStringValue(EnvRoot(), EnvKey(), 'Path', Path) then exit;
  P := Pos(';' + Lowercase(Dir) + ';', ';' + Lowercase(Path) + ';');
  if P = 0 then exit;
  Delete(Path, P, Length(Dir) + 1);   { the directory and one semicolon }
  if (Path <> '') and (Path[Length(Path)] = ';') then Delete(Path, Length(Path), 1);
  RegWriteExpandStringValue(EnvRoot(), EnvKey(), 'Path', Path);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('addpath') then AddToPath();
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then RemoveFromPath();
end;
