; Audiokings Vocals - Windows installer (Inno Setup 6)
; Build:  ISCC.exe /DAppVersion=3.0.0 installer\AudiokingsVocals.iss
; The VST3 bundle must already be built in ..\build\AudiokingsVocals_artefacts\Release\VST3

#ifndef AppVersion
  #define AppVersion "3.4.0"
#endif
#define AppName "Audiokings Vocals"
#define VstSrc "..\build\AudiokingsVocals_artefacts\Release\VST3\Audiokings Vocals.vst3"

[Setup]
AppId={{B3F1C2D4-8E2A-4C57-9F1B-6A2D7E5C1A90}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=Audiokings
DefaultDirName={autopf}\Audiokings Vocals
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
OutputDir=..\dist
OutputBaseFilename=AudiokingsVocals-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#AppName} (VST3)
CloseApplications=no
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Files]
; the plug-in goes to the standard VST3 folder (FL Studio and Ableton Live scan it)
Source: "{#VstSrc}\*"; DestDir: "{commoncf64}\VST3\Audiokings Vocals.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[InstallDelete]
; clean any previous copy (installer-based or manual) before installing the new one
Type: filesandordirs; Name: "{commoncf64}\VST3\Audiokings Vocals.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\Audiokings*.vst3"
Type: filesandordirs; Name: "{commoncf32}\VST3\Audiokings Vocals.vst3"

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Audiokings Vocals.vst3"

[Code]
const
  UninstKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{B3F1C2D4-8E2A-4C57-9F1B-6A2D7E5C1A90}_is1';

function GetOldUninstaller(): String;
var
  S: String;
begin
  S := '';
  if not RegQueryStringValue(HKLM64, UninstKey, 'UninstallString', S) then
    if not RegQueryStringValue(HKLM32, UninstKey, 'UninstallString', S) then
      RegQueryStringValue(HKCU, UninstKey, 'UninstallString', S);
  Result := RemoveQuotes(S);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  U: String;
  RC: Integer;
begin
  if CurStep = ssInstall then
  begin
    // an older installer-based version is removed silently before the new one is copied
    U := GetOldUninstaller();
    if (U <> '') and FileExists(U) then
      Exec(U, '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-', '', SW_HIDE, ewWaitUntilTerminated, RC);
  end;
end;

function InitializeSetup(): Boolean;
begin
  Result := True;
  if (not WizardSilent) and (MsgBox('Please close FL Studio / Ableton Live (or any host using Audiokings Vocals) before continuing.' + #13#10 +
            'Previous versions will be removed automatically.' + #13#10#13#10 + 'Continue?',
            mbInformation, MB_YESNO) = IDNO) then
    Result := False;
end;
