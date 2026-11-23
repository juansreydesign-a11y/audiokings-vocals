; Audiokings Vocals - Inno Setup 6 installer
; Modern dark theme, credits, clean upgrade

#ifndef AppVersion
  #define AppVersion "3.6.0"
#endif
#define AppName    "Audiokings Vocals"
#define Publisher  "JRey | Paranoyz Music - ReVerse - Faraday Audio"
#define VstSrc     "..\build\AudiokingsVocals_artefacts\Release\VST3\Audiokings Vocals.vst3"

[Setup]
AppId={{B3F1C2D4-8E2A-4C57-9F1B-6A2D7E5C1A90}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#Publisher}
AppPublisherURL=https://github.com/juansreydesign-a11y/audiokings-vocals
AppCopyright=Copyright (C) 2026 JRey / Paranoyz Music / ReVerse / Faraday Audio
DefaultDirName={autopf}\Audiokings Vocals
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
OutputDir=..\dist
OutputBaseFilename=AudiokingsVocals-Setup-{#AppVersion}
SetupIconFile=icon.ico
WizardImageFile=wizard_banner.bmp
WizardSmallImageFile=wizard_small.bmp
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
BackColor=$000000
BackColor2=$0A0414
BackColorDirection=toptoleftcorner
WindowVisible=yes
WindowShowCaption=yes
UninstallDisplayName={#AppName}
CloseApplications=no
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Messages]
english.BeveledLabel=Audiokings Vocals {#AppVersion}
spanish.BeveledLabel=Audiokings Vocals {#AppVersion}

[CustomMessages]
english.WelcomeLabel2=This installer will place [name/ver] on your computer.%n%nDeveloped by JRey%nIn collaboration with Paranoyz Music, ReVerse and Faraday Audio.%n%nTested and shaped by:%nAudiokings, Clonex, Essyboy P, Fash Oxigeno,%nNueb9, Oral MCMP and K1.%n%nAny previous version will be removed automatically.
spanish.WelcomeLabel2=Este instalador colocara [name/ver] en tu equipo.%n%nDesarrollado por JRey%nEn colaboracion con Paranoyz Music, ReVerse y Faraday Audio.%n%nTesters:%nAudiokings, Clonex, Essyboy P, Fash Oxigeno,%nNueb9, Oral MCMP y K1.%n%nCualquier version anterior se eliminara automaticamente.
english.ReadyLabel2b=The plug-in will be installed to the system-wide VST3 folder.%nFL Studio, Ableton Live, Cubase and Reaper will find it automatically after a plug-in rescan.
spanish.ReadyLabel2b=El plugin se instalara en la carpeta VST3 del sistema.%nFL Studio, Ableton Live, Cubase y Reaper lo detectaran tras un reescaneo de plugins.

[Files]
Source: "{#VstSrc}\*"; DestDir: "{commoncf64}\VST3\Audiokings Vocals.vst3"; \
        Flags: ignoreversion recursesubdirs createallsubdirs

[InstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Audiokings Vocals.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\Audiokings*.vst3"
Type: filesandordirs; Name: "{commoncf32}\VST3\Audiokings Vocals.vst3"

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Audiokings Vocals.vst3"

[Code]
const
  UninstKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{B3F1C2D4-8E2A-4C57-9F1B-6A2D7E5C1A90}_is1';

function GetOldUninstaller(): String;
var S: String;
begin
  S := '';
  if not RegQueryStringValue(HKLM64, UninstKey, 'UninstallString', S) then
    if not RegQueryStringValue(HKLM32, UninstKey, 'UninstallString', S) then
      RegQueryStringValue(HKCU, UninstKey, 'UninstallString', S);
  Result := RemoveQuotes(S);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var U: String; RC: Integer;
begin
  if CurStep = ssInstall then
  begin
    U := GetOldUninstaller();
    if (U <> '') and FileExists(U) then
      Exec(U, '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-', '', SW_HIDE, ewWaitUntilTerminated, RC);
  end;
end;

procedure InitializeWizard;
begin
  WizardForm.Color := $000000;
  WizardForm.MainPanel.Color := $000000;
  if WizardForm.InnerPage <> nil then
    WizardForm.InnerPage.Color := $0A0414;

  WizardForm.WelcomeLabel1.Font.Color  := $FF8C38;    // accent orange
  WizardForm.WelcomeLabel1.Font.Size   := 18;
  WizardForm.WelcomeLabel1.Font.Style  := [fsBold];

  WizardForm.WelcomeLabel2.Font.Color  := $C8BEFF;    // soft violet
  WizardForm.WelcomeLabel2.Font.Size   := 9;

  WizardForm.FinishedLabel.Font.Color  := $38E1FF;    // cyan
  WizardForm.FinishedLabel.Font.Size   := 11;
  WizardForm.FinishedLabel.Font.Style  := [fsBold];

  WizardForm.NextButton.Font.Color     := $000000;
  WizardForm.BackButton.Font.Color     := $C8BEFF;
  WizardForm.CancelButton.Font.Color   := $C8BEFF;
end;

function InitializeSetup(): Boolean;
begin
  Result := True;
  if not WizardSilent then
    if MsgBox(
      'Please close FL Studio / Ableton Live / Cubase / Reaper before continuing.' + #13#10 +
      'Previous versions of Audiokings Vocals will be removed automatically.' + #13#10#13#10 +
      'Continue?',
      mbInformation, MB_YESNO) = IDNO then
      Result := False;
end;
