#define SourceFileDir32 "..\build-out\Release"
#define SourceFileDir64 "..\build-out\Release-x64"
#define NfoProgId "iNFEKT.NFOFile.1"

#include "C:\Program Files (x86)\Inno Download Plugin\idp.iss"

[Setup]
AppId={{B1AC8E6A-6C47-4B6D-A853-B4BF5C83421C}
AppName=iNFekt NFO Viewer
AppVerName=iNFekt 0.9.7
AppVersion=0.9.7
AppPublisher=syndicode
AppPublisherURL=http://infekt.ws/
DefaultDirName={pf}\iNFekt
DefaultGroupName=iNFekt NFO Viewer
AllowNoIcons=yes
OutputDir=..\build-out
OutputBaseFilename=infekt-setup
Compression=lzma
SolidCompression=yes
WizardSmallImageFile=setup-bmp-small.bmp
WizardImageFile=setup-bmp-left.bmp
WizardImageStretch=yes
ChangesAssociations=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64
MinVersion=0,5.1sp1
AppMutex=iNFektNfoViewerOneInstanceMutex
UninstallDisplayIcon={app}\infekt-win64.exe
UninstallDisplayName=iNFekt NFO Viewer

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: checkedonce
Name: "nfoassoc"; Description: "Make iNFekt the default viewer for .nfo files"; GroupDescription: "Shell integration"; Check: not WizardSilent
Name: "dizassoc"; Description: "Make iNFekt the default viewer for .diz files"; GroupDescription: "Shell integration"; Flags: unchecked
Name: "ansiassoc"; Description: "Make iNFekt the default viewer for .ans and .asc files"; GroupDescription: "Shell integration"; Flags: unchecked
Name: "shellpreview"; Description: "Install Explorer preview pane and thumbnail integration for associated files"; GroupDescription: "Shell integration"; MinVersion: 0,6.0
Name: "cppredist2015"; Description: "Download and install Microsoft C++ runtime if necessary"; GroupDescription: "Advanced"; Flags: checkedonce

[Files]
Source: "{#SourceFileDir32}\infekt-win32.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\infekt-win64.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#SourceFileDir32}\infekt-cmd.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\infekt-cmd.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#SourceFileDir32}\cairo.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\cairo.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#SourceFileDir32}\libpng16.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\libpng16.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#SourceFileDir32}\zlib.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\zlib.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#SourceFileDir32}\infekt-gpu.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode; MinVersion: 0,6.0
Source: "{#SourceFileDir64}\infekt-gpu.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode; MinVersion: 0,6.0
Source: "{#SourceFileDir32}\infekt-nfo-shell.dll"; DestDir: "{app}"; Flags: ignoreversion regserver; Tasks: shellpreview; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\infekt-nfo-shell.dll"; DestDir: "{app}"; Flags: ignoreversion regserver; Tasks: shellpreview; Check: Is64BitInstallMode
Source: "{#SourceFileDir32}\infekt-win32-updater.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceFileDir32}\plugins\rescene.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\plugins\rescene.dll"; DestDir: "{app}\plugins"; Flags: ignoreversion; Check: Is64BitInstallMode

[InstallDelete]
Type: files; Name: "{app}\cudart.dll"
; cudart.dll was used up to v0.6.0
Type: files; Name: "{app}\libpng14.dll"
Type: files; Name: "{app}\zlib1.dll"
Type: files; Name: "{app}\cudart32_31_9.dll"
Type: files; Name: "{app}\cudart64_31_9.dll"
; these 4 were used up to v0.7.9
Type: files; Name: "{app}\cuda-blur.dll"
Type: files; Name: "{app}\cudart32_32_16.dll"
Type: files; Name: "{app}\cudart64_32_16.dll"
Type: files; Name: "{app}\libpng15.dll"
; these 4 were used up to 0.8.5
Type: files; Name: "{app}\pcre.dll"
; these were used up to 0.9.5

Type: files; Name: "{app}\MSVCP100.dll"
Type: files; Name: "{app}\MSVCR100.dll"
Type: files; Name: "{app}\VCOMP100.dll"
Type: files; Name: "{app}\MSVCP110.dll"
Type: files; Name: "{app}\MSVCR110.dll"
Type: files; Name: "{app}\VCOMP110.dll"
Type: files; Name: "{app}\VCAMP110.dll"
Type: files; Name: "{app}\concrt140.dll"
Type: files; Name: "{app}\msvcp140.dll"
Type: files; Name: "{app}\vcruntime140.dll"
Type: files; Name: "{app}\vcamp140.dll"
Type: files; Name: "{app}\vcomp140.dll"
; these should not be in the target directory, clean up in case of previous portable version at the same location etc.

[UnInstallDelete]
Type: files; Name: "{app}\infekt-nfo-shell.dll"
; in case someone copied this file into the program folder manually (when it has NOT been installed using the "shellpreview" task)
Type: files; Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\iNFekt NFO Viewer.lnk"

[Icons]
Name: "{group}\iNFekt NFO Viewer"; Filename: "{app}\infekt-win32.exe"; Check: not Is64BitInstallMode
Name: "{group}\iNFekt NFO Viewer"; Filename: "{app}\infekt-win64.exe"; Check: Is64BitInstallMode
Name: "{group}\{cm:ProgramOnTheWeb,iNFekt NFO Viewer}"; Filename: "http://infekt.ws/"
Name: "{group}\{cm:UninstallProgram,iNFekt NFO Viewer}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\iNFekt NFO Viewer"; Filename: "{app}\infekt-win32.exe"; Tasks: desktopicon; Check: not Is64BitInstallMode
Name: "{commondesktop}\iNFekt NFO Viewer"; Filename: "{app}\infekt-win64.exe"; Tasks: desktopicon; Check: Is64BitInstallMode

[Run]
Filename: "{app}\infekt-win32.exe"; Description: "{cm:LaunchProgram,iNFekt NFO Viewer}"; Flags: nowait postinstall skipifsilent; Check: not Is64BitInstallMode
Filename: "{app}\infekt-win64.exe"; Description: "{cm:LaunchProgram,iNFekt NFO Viewer}"; Flags: nowait postinstall skipifsilent; Check: Is64BitInstallMode

[UninstallRun]
Filename: "{sys}\regsvr32.exe"; Parameters: "/s /u ""{app}\infekt-nfo-shell.dll"""; Flags: skipifdoesntexist runhidden
; in case someone manually registered the shell extension, make sure to clean up.

[Registry]
; Set up ProgId for file associations:
Root: HKLM; Subkey: "SOFTWARE\Classes\{#NfoProgId}"; ValueType: string; ValueName: ""; ValueData: "NFO File"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\{#NfoProgId}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\infekt-win32.exe"",0"; Check: not Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Classes\{#NfoProgId}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\infekt-win64.exe"",0"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Classes\{#NfoProgId}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\infekt-win32.exe"" ""%1"""; Check: not Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Classes\{#NfoProgId}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\infekt-win64.exe"" ""%1"""; Check: Is64BitInstallMode
; Claim machine-level file associations:
Root: HKLM; Subkey: "SOFTWARE\Classes\.nfo"; ValueType: string; ValueName: ""; ValueData: "{#NfoProgId}"; Flags: uninsclearvalue; Tasks: "nfoassoc"
Root: HKLM; Subkey: "SOFTWARE\Classes\.diz"; ValueType: string; ValueName: ""; ValueData: "{#NfoProgId}"; Flags: uninsclearvalue; Tasks: "dizassoc"
Root: HKLM; Subkey: "SOFTWARE\Classes\.ans"; ValueType: string; ValueName: ""; ValueData: "{#NfoProgId}"; Flags: uninsclearvalue; Tasks: "ansiassoc"
Root: HKLM; Subkey: "SOFTWARE\Classes\.asc"; ValueType: string; ValueName: ""; ValueData: "{#NfoProgId}"; Flags: uninsclearvalue; Tasks: "ansiassoc"
; Register the application with Default Programs
Root: HKLM; Subkey: "SOFTWARE\cxxjoe\iNFEKT"; ValueType: string; ValueName: ""; ValueData: ""; Flags: uninsdeletekey; MinVersion: 0,6.0
Root: HKLM; Subkey: "SOFTWARE\cxxjoe\iNFEKT\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "iNFekt NFO Viewer"; MinVersion: 0,6.0
Root: HKLM; Subkey: "SOFTWARE\cxxjoe\iNFEKT\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "iNFekt NFO Viewer is the next generation NFO Viewer Application!"; MinVersion: 0,6.0
Root: HKLM; Subkey: "SOFTWARE\cxxjoe\iNFEKT\Capabilities\FileAssociations"; ValueType: string; ValueName: ".nfo"; ValueData: "{#NfoProgId}"; MinVersion: 0,6.0
Root: HKLM; Subkey: "SOFTWARE\cxxjoe\iNFEKT\Capabilities\FileAssociations"; ValueType: string; ValueName: ".diz"; ValueData: "{#NfoProgId}"; MinVersion: 0,6.0
Root: HKLM; Subkey: "SOFTWARE\cxxjoe\iNFEKT\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ans"; ValueData: "{#NfoProgId}"; MinVersion: 0,6.0
Root: HKLM; Subkey: "SOFTWARE\cxxjoe\iNFEKT\Capabilities\FileAssociations"; ValueType: string; ValueName: ".asc"; ValueData: "{#NfoProgId}"; MinVersion: 0,6.0
Root: HKLM; Subkey: "SOFTWARE\RegisteredApplications"; ValueType: string; ValueName: "iNFekt NFO Viewer"; ValueData: "SOFTWARE\cxxjoe\iNFEKT\Capabilities"; Flags: uninsdeletevalue; MinVersion: 0,6.0
; Clear user-overridden stuff on 5.x:
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.nfo"; ValueType: none; ValueName: "Progid"; Flags: deletevalue; Tasks: "nfoassoc"
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.diz"; ValueType: none; ValueName: "Progid"; Flags: deletevalue; Tasks: "dizassoc"
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ans"; ValueType: none; ValueName: "Progid"; Flags: deletevalue; Tasks: "ansiassoc"
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.asc"; ValueType: none; ValueName: "Progid"; Flags: deletevalue; Tasks: "ansiassoc"
; Clear user-overridden stuff on Vista/7:
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.nfo\UserChoice"; ValueType: none; Flags: deletekey; Tasks: "nfoassoc"; MinVersion: 0,6.0
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.diz\UserChoice"; ValueType: none; Flags: deletekey; Tasks: "dizassoc"; MinVersion: 0,6.0
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ans\UserChoice"; ValueType: none; Flags: deletekey; Tasks: "ansiassoc"; MinVersion: 0,6.0
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.asc\UserChoice"; ValueType: none; Flags: deletekey; Tasks: "ansiassoc"; MinVersion: 0,6.0

; Uninstall-only stuff:

; Program settings:
Root: HKCU; Subkey: "Software\cxxjoe\iNFEKT"; Flags: dontcreatekey uninsdeletekey
; Association created by Windows:
Root: HKCU; Subkey: "Software\Classes\Applications\infekt-win32.exe"; Flags: dontcreatekey uninsdeletekey; Check: not Is64BitInstallMode
Root: HKCU; Subkey: "Software\Classes\Applications\infekt-win64.exe"; Flags: dontcreatekey uninsdeletekey; Check: Is64BitInstallMode

[Code]
type
 TCopyDataStruct = record
 dwData: DWORD;
 cbData: DWORD;
 lpData: AnsiString;
 end;

function FindWindow(lpClassName, lpWindowName: AnsiString): HWND;
  external 'FindWindowA@User32.dll stdcall';

function SendMessageCopyData(a: HWND; m: Integer; x: Integer; var data: TCopyDataStruct): Integer;
	external 'SendMessageA@User32.dll stdcall'; 

const
	WM_COPYDATA = $4A;

procedure SendStatusMessageToUpdater(msg: String);
var
	updWindow: HWND;
	cds: TCopyDataStruct;
begin
	updWindow := FindWindow('#32770', 'iNFekt Program Updater');
	if updWindow <> 0 then
	begin
		cds.dwData := 666;
		cds.cbData := Length(msg) + 1;
		cds.lpData := msg;

		SendMessageCopyData(updWindow, WM_COPYDATA, 0, cds);
	end;
end;


function MsiQueryProductState(ProductCode: String): Integer; external 'MsiQueryProductStateW@msi.dll stdcall';

var
	cppRuntimeInstalled: Boolean;
	allowCppRuntimeInstall: Boolean;

const
	INSTALLSTATE_DEFAULT = 5;

	MSVC_X64_URL = '://download.microsoft.com/download/6/A/A/6AA4EDFF-645B-48C5-81CC-ED5963AEAD48/vc_redist.x64.exe';
	MSVC_X86_URL = '://download.microsoft.com/download/6/A/A/6AA4EDFF-645B-48C5-81CC-ED5963AEAD48/vc_redist.x86.exe';


function InstallCppRuntime(): Boolean;
begin
		Result := allowCppRuntimeInstall and not cppRuntimeInstalled;
end;


procedure InitializeWizard();
var
	WinVersion: TWindowsVersion;
	DownloadProtocol: String;
	U3Installed: Boolean;
begin
	idpSetOption('AllowContinue', '1');

	GetWindowsVersionEx(WinVersion);

	if (WinVersion.Major = 5) then
		DownloadProtocol := 'http'
	else
		DownloadProtocol := 'https';

	// GUIDs from: https://svn.wsusoffline.net/svn/wsusoffline/trunk/client/cmd/DetermineSystemProperties.vbs

	if Is64BitInstallMode() then
	begin
		idpAddFile(DownloadProtocol + MSVC_X64_URL, expandconstant('{tmp}\vcredist_x64.exe'));
		if DownloadProtocol = 'https' then
		begin
			idpAddMirror('https://syndicode.org/infekt/mirror/vcredist_x64_2015u3.exe', expandconstant('{tmp}\vcredist_x64.exe'));
		end;

  // Microsoft Visual C++ 2015 x64 Additional Runtime - 14.0.24215
		U3Installed := (MsiQueryProductState('{EF1EC6A9-17DE-3DA9-B040-686A1E8A8B04}') = INSTALLSTATE_DEFAULT)
		// Microsoft Visual C++ 2015 x64 Minimum Runtime - 14.0.24215
			          and (MsiQueryProductState('{50A2BC33-C9CD-3BF1-A8FF-53C10A0B183C}') = INSTALLSTATE_DEFAULT);
	end
	else
	begin
		idpAddFile(DownloadProtocol + MSVC_X86_URL, expandconstant('{tmp}\vcredist_x86.exe'));
		if DownloadProtocol = 'https' then
		begin
			idpAddMirror('https://syndicode.org/infekt/mirror/vcredist_x86_2015u3.exe', expandconstant('{tmp}\vcredist_x86.exe'));
		end;

  // Microsoft Visual C++ 2015 x86 Additional Runtime - 14.0.24215
  U3Installed := (MsiQueryProductState('{69BCE4AC-9572-3271-A2FB-9423BDA36A43}') = INSTALLSTATE_DEFAULT)
		// Microsoft Visual C++ 2015 x86 Minimum Runtime - 14.0.24215
			          and (MsiQueryProductState('{BBF2AC74-720C-3CB3-8291-5E34039232FA}') = INSTALLSTATE_DEFAULT);
	end;

	allowCppRuntimeInstall := true; // default
	cppRuntimeInstalled := U3Installed;

	if not cppRuntimeInstalled then
	begin
  // note: cannot consider checkbox here, it's not even been created.
		idpDownloadAfter(wpReady);
	end;
end;


function NextButtonClick(CurPageID: Integer): Boolean;
var
  Index: Integer;
begin
	Result := True;
	if CurPageID = wpSelectTasks then
	begin
		Index := WizardForm.TasksList.Items.IndexOf('Download and install Microsoft C++ runtime if necessary');
		if Index <> -1 then
		begin
					allowCppRuntimeInstall := WizardForm.TasksList.Checked[Index];
		end;
	end;
end;


procedure CurStepChanged(CurStep: TSetupStep);
var
	exepath: String;
	exitcode: Integer;
begin
	if (CurStep = ssInstall) and InstallCppRuntime() then
	begin
		// Install C++ Runtime. This MUST NOT be done in [Run] because
		// regsvr32'ing the shell extension requires the runtimes to be installed
		// already whereas [Run] would only install them afterwards.

		if Is64BitInstallMode() then
			exepath := ExpandConstant('{tmp}\vcredist_x64.exe')
		else
			exepath := ExpandConstant('{tmp}\vcredist_x86.exe');

		if FileExists(exepath) then
		begin
			if Is64BitInstallMode() then
				WizardForm.StatusLabel.Caption := 'Installing Microsoft Visual C++ 2015 (x64) runtime...'
			else
				WizardForm.StatusLabel.Caption := 'Installing Microsoft Visual C++ 2015 (x86) runtime...';

			SendStatusMessageToUpdater(WizardForm.StatusLabel.Caption);

			Exec(exepath, '/install /quiet /norestart', '', SW_HIDE, ewWaitUntilTerminated, exitcode);
		end;
	end;
end;


procedure CurPageChanged(CurPageID: Integer);
var
	msg: String;
begin
	if (CurPageID = wpReady) and InstallCppRuntime() then
		msg := 'Downloading Microsoft Visual C++ 2015 runtime... this may take a minute.'
	else if WizardForm.StatusLabel.Caption <> '' then
		msg := WizardForm.StatusLabel.Caption
	else if WizardForm.PageNameLabel.Caption <> '' then
		msg := WizardForm.PageNameLabel.Caption + ': ' + WizardForm.PageDescriptionLabel.Caption
	else
		msg := 'Installing update...';

	SendStatusMessageToUpdater(msg);

	// skip download if checkbox hasn't been selected:
	if (CurPageID = wpReady) and not InstallCppRuntime() then
		idpClearFiles();
end;
