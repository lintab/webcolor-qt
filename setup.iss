; ============================================
;   KQ Cloud - Inno Setup 安装程序配置
;   需要先安装 Inno Setup: https://jrsoftware.org/isinfo.php
; ============================================

#define MyAppName "KQ Cloud"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "恺祁化工"
#define MyAppExeName "KQCloud.exe"

; 打包用的构建产物目录：VS 生成器构建（build-win-vs.bat）的输出。
; 自带 NMake 脚本在中文源码路径下链接必失败，所以不用 build-win。
; 换构建目录时只改这一行即可，注意要和 package.bat 里的 BUILDDIR 保持一致。
#define BuildDir "..\..\kqcloud-build-vs\Release"

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
OutputBaseFilename=KQCloud_Setup_{#MyAppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
SetupIconFile=resources\setup.ico
WizardImageFile=resources\wizardimage.bmp
WizardSmallImageFile=resources\wizardsmallimage.bmp
UninstallDisplayIcon={app}\{#MyAppExeName}
; 语言监测：按本机 Windows 界面语言预选向导语言（Inno 默认就是 uilanguage，写出来是为了明确行为）。
; ShowLanguageDialog 保持 yes：检测只是预选，用户仍可在「Select Language」里改成别的语言，
; 选中的语言同时会写进程序的 language 设置（见 [Registry]）。
; 注意 UsePreviousLanguage 默认 yes：同一台机器上卸载前装过 KQ Cloud，重装时向导会先沿用
; 上次那次的语言而不是重新检测本机语言，反复测试检测效果时要先卸干净。
LanguageDetectionMethod=uilanguage
ShowLanguageDialog=yes

[Languages]
; 安装向导的语言就是程序首启的语言（见下面 [Registry] 写入的 language 值）。
; 顺序有讲究：Inno 先用界面语言精确匹配各 .isl 的 LanguageID，不中再退而匹配主语言，
; 两次都没命中时直接取本节的**第一项**。所以英文必须排第一 —— 安装包只覆盖六语，
; 德语/西语/葡语等机子上就是英文向导 + 英文程序，不会硬套一个看不懂的语言。
; 中文 Windows 的界面语言是 $0804，能精确命中 ChineseSimplified.isl，仍然默认走中文。
; （繁体中文 $0404 不会命中简体中文，Inno 对简繁有专门的排除规则，会落到英文。）
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "chinesesimplified"; MessagesFile: "resources\ChineseSimplified.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "arabic"; MessagesFile: "compiler:Languages\Arabic.isl"
; Inno Setup 官方没有越南语语言包，Vietnamese.isl 暂时沿用英文消息、只改语言名，
; 这样安装时能选到越南语，程序界面语言也会是越南语
Name: "vietnamese"; MessagesFile: "resources\Vietnamese.isl"

[Code]
{ 按当前向导语言取一段文案，六个参数依次是 zh/en/ru/fr/ar/vi }
function KqText(const zh, en, ru, fr, ar, vi: String): String;
begin
  case ActiveLanguage of
    'chinesesimplified': Result := zh;
    'russian':           Result := ru;
    'french':            Result := fr;
    'arabic':            Result := ar;
    'vietnamese':        Result := vi;
  else
    Result := en;
  end;
end;

function GetDriverGroupDesc(Param: String): String;
begin
  Result := KqText('设备驱动：', 'Device drivers:', 'Драйверы устройств:',
                   'Pilotes de périphériques :', 'تعريفات الجهاز:',
                   'Trình điều khiển thiết bị:');
end;

function GetDriverTaskDesc(Param: String): String;
begin
  Result := KqText('安装 X-Rite SDX4 设备驱动（使用测量功能需要）',
                   'Install the X-Rite SDX4 device driver (needed for measurement)',
                   'Установить драйвер устройства X-Rite SDX4 (нужен для измерений)',
                   'Installer le pilote de périphérique X-Rite SDX4 (nécessaire pour mesurer)',
                   'تثبيت تعريف جهاز X-Rite SDX4 (مطلوب لوظيفة القياس)',
                   'Cài đặt trình điều khiển thiết bị X-Rite SDX4 (cần để đo màu)');
end;

function GetDriverStatusMsg(Param: String): String;
begin
  Result := KqText('正在安装 X-Rite SDX4 设备驱动...',
                   'Installing the X-Rite SDX4 device driver...',
                   'Установка драйвера X-Rite SDX4...',
                   'Installation du pilote X-Rite SDX4...',
                   'جاري تثبيت تعريف X-Rite SDX4...',
                   'Đang cài trình điều khiển X-Rite SDX4...');
end;

function GetUninstallIconName(Param: String): String;
begin
  Result := KqText('卸载 {#MyAppName}', 'Uninstall {#MyAppName}',
                   'Удалить {#MyAppName}', 'Désinstaller {#MyAppName}',
                   'إلغاء تثبيت {#MyAppName}', 'Gỡ cài đặt {#MyAppName}');
end;

{ 写进 QSettings 的语言代码，程序启动时直接读这个值 }
function GetAppLanguage(Param: String): String;
begin
  case ActiveLanguage of
    'chinesesimplified': Result := 'zh';
    'russian':           Result := 'ru';
    'french':            Result := 'fr';
    'arabic':            Result := 'ar';
    'vietnamese':        Result := 'vi';
  else
    Result := 'en';
  end;
end;

{ 安装结束时清掉本机残留的登录凭据，装完打开就是空白登录页。三个键名来自源码：
  webcolor_remember —— LoginWidget 的「记住密码」，账号+密码明文 JSON
  webcolor_token / webcolor_user —— Auth 里的会话 token 与用户信息
  旧版「恺祁颜色系统」写的是同一个 QSettings 路径，所以升级安装也会把旧残留一起清掉。
  软件内「自动更新」会用 /KEEPLOGIN 启动安装包（见 UpdateWidget::launchInstaller）跳过这里，
  免得每次更新都把用户踢下线；手动双击安装包不带这个参数，正常清除。
  语言、保存路径、测量次数、搜索历史这些设置一概不动。 }
procedure CurStepChanged(CurStep: TSetupStep);
var
  SettingsKey: String;
begin
  if CurStep <> ssPostInstall then Exit;
  if Pos('/KEEPLOGIN', Uppercase(GetCmdTail)) > 0 then Exit;
  SettingsKey := 'Software\KaiqiChemical\恺祁颜色系统';
  RegDeleteValue(HKCU, SettingsKey, 'webcolor_remember');
  RegDeleteValue(HKCU, SettingsKey, 'webcolor_token');
  RegDeleteValue(HKCU, SettingsKey, 'webcolor_user');
end;

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
; SDX4（XDS4）分光光度计的设备驱动，连接仪器测量时必需，默认勾选
Name: "sdx4driver"; Description: "{code:GetDriverTaskDesc}"; GroupDescription: "{code:GetDriverGroupDesc}"

[Registry]
; 子键就是 QSettings 的路径（organizationName=KaiqiChemical + applicationName=恺祁颜色系统），
; 程序里读的是 language 这个值。createvalueifdoesntexist 保证升级安装不会覆盖
; 用户自己在设置里换过的语言。
Root: HKCU; Subkey: "Software\KaiqiChemical\恺祁颜色系统"; ValueType: string; ValueName: "language"; ValueData: "{code:GetAppLanguage}"; Flags: createvalueifdoesntexist

[InstallDelete]
; AppId 沿用旧版，老用户是原地升级；程序改名后 exe 也从 WebColorQt.exe 换成 KQCloud.exe，
; 不清理的话开始菜单里会同时留着旧程序和旧快捷方式
Type: files; Name: "{app}\WebColorQt.exe"
Type: files; Name: "{group}\恺祁颜色系统.lnk"
Type: files; Name: "{group}\卸载 恺祁颜色系统.lnk"
Type: files; Name: "{autodesktop}\恺祁颜色系统.lnk"

[Files]
; 复制构建产物（含 SDX4 驱动运行文件 MAC.dll / libusb-1.0.dll / spectro.ini）
Source: "{#BuildDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; 设备驱动安装包解压到临时目录，装完即删，不留在程序目录
Source: "drivers\X-Rite_MAC_Driver_v3.0.0.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; Tasks: sdx4driver

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{code:GetUninstallIconName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
; 设备驱动安装程序会自行弹出管理员授权提示，请按提示完成安装
Filename: "{tmp}\X-Rite_MAC_Driver_v3.0.0.exe"; Tasks: sdx4driver; StatusMsg: "{code:GetDriverStatusMsg}"; Flags: waituntilidle skipifdoesntexist
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
