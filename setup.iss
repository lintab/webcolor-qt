; ============================================
;   恺祁颜色系统 - Inno Setup 安装程序配置
;   需要先安装 Inno Setup: https://jrsoftware.org/isinfo.php
; ============================================

#define MyAppName "恺祁颜色系统"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "恺祁化工"
#define MyAppExeName "WebColorQt.exe"

; 打包用的构建产物目录：VS 生成器构建（build-win-vs.bat）的输出。
; 自带 NMake 脚本在中文源码路径下链接必失败，所以不用 build-win。
; 换构建目录时只改这一行即可。
#define BuildDir "..\..\webcolor-qt-build-vs\Release"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=
OutputDir=installer-output
OutputBaseFilename=恺祁颜色系统_Setup_{#MyAppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
SetupIconFile=resources\setup.ico
WizardImageFile=resources\wizardimage.bmp
WizardSmallImageFile=resources\wizardsmallimage.bmp
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "chinesesimplified"; MessagesFile: "resources\ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
; SDX4（XDS4）分光光度计的设备驱动，连接仪器测量时必需，默认勾选
Name: "sdx4driver"; Description: "安装 X-Rite SDX4 设备驱动（使用测量功能需要）"; GroupDescription: "设备驱动："

[Files]
; 复制构建产物（含 SDX4 驱动运行文件 MAC.dll / libusb-1.0.dll / spectro.ini）
Source: "{#BuildDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; 设备驱动安装包解压到临时目录，装完即删，不留在程序目录
Source: "drivers\X-Rite_MAC_Driver_v3.0.0.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; Tasks: sdx4driver

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\卸载 {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
; 设备驱动安装程序会自行弹出管理员授权提示，请按提示完成安装
Filename: "{tmp}\X-Rite_MAC_Driver_v3.0.0.exe"; Tasks: sdx4driver; StatusMsg: "正在安装 X-Rite SDX4 设备驱动..."; Flags: waituntil skipifdoesntexist
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
