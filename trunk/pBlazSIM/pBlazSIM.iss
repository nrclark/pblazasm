; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{4D3FAEB5-7D6A-4A25-B89D-EC130B735020}
AppName=pBlazSIM
AppVersion=1.7
;AppVerName=pBlazSIM 1.7
AppPublisher=Mediatronix BV
AppPublisherURL=http://www.mediatronix.com
AppSupportURL=http://www.mediatronix.com
AppUpdatesURL=http://www.mediatronix.com
DefaultDirName={pf}\Mediatronix\pBlazSIM
DefaultGroupName=pBlazSIM  
LicenseFile=C:\Users\henk\Projects\pblazasm\trunk\gpl-3.0.txt
OutputBaseFilename=pBlazSIM_Qt501_Install
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"          

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Files]
Source: "C:\Users\henk\Projects\pblazasm\trunk\build-pBlazSIM-Qt_5_0_1_mingw47_32-Release\release\pBlazSIM.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\icudt49.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\icuin49.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\icuuc49.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\libEGL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\libGLESv2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\libgcc_s_sjlj-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\D3DCompiler_43.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\Qt5Script.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\Qt5ScriptTools.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Qt\Qt5.0.1\5.0.1\mingw47_32\bin\Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\henk\Projects\Projects\PBSupport\pblazbit.lst"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\henk\Projects\Projects\PBSupport\pblazbit.scr"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\pBlazSIM"; Filename: "{app}\pBlazSIM.exe"
Name: "{group}\{cm:UninstallProgram,pBlazSIM}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\pBlazSIM"; Filename: "{app}\pBlazSIM.exe"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\pBlazSIM"; Filename: "{app}\pBlazSIM.exe"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\pBlazSIM.exe"; Description: "{cm:LaunchProgram,pBlazSIM}"; Flags: nowait postinstall skipifsilent
