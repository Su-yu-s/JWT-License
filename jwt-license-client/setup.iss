; ============================================================
;  iK 安装包脚本 —— Inno Setup 7
;  编译: ISCC.exe setup.iss
;  源码目录: dist\（由 build 整理出的运行文件）
; ============================================================

[Setup]
AppId={{7D197688-D5D0-4FFE-81C5-363F763884E1}
AppName=iK
AppVersion=1.0.0
AppVerName=iK 1.0.0
AppPublisher=iK
DefaultDirName={autopf}\iK
DefaultGroupName=iK
DisableProgramGroupPage=yes
OutputDir=install
OutputBaseFilename=iK-Setup-1.0.0
SetupIconFile=resources\app.ico
UninstallDisplayIcon={app}\iK.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加任务:"

[Files]
Source: "dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\iK"; Filename: "{app}\iK.exe"
Name: "{autodesktop}\iK"; Filename: "{app}\iK.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\iK.exe"; Description: "启动 iK"; Flags: nowait postinstall skipifsilent
