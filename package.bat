@echo off
chcp 65001 >nul
echo ============================================
echo   KQ Cloud - 打包安装程序
echo ============================================
echo.

:: 检查 Inno Setup（含用户目录下的免管理员安装位置）
set "ISCC="
if exist "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" (
    set "ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
) else if exist "C:\Program Files\Inno Setup 6\ISCC.exe" (
    set "ISCC=C:\Program Files\Inno Setup 6\ISCC.exe"
) else if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" (
    set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
) else (
    echo [错误] 未找到 Inno Setup 6
    echo 请从 https://jrsoftware.org/isinfo.php 下载安装
    pause
    exit /b 1
)

:: 检查构建产物（与 setup.iss 中的 BuildDir 保持一致：VS 构建输出目录）
set "BUILDDIR=..\..\kqcloud-build-vs\Release"
if not exist "%BUILDDIR%\KQCloud.exe" (
    echo [错误] 未找到构建产物 %BUILDDIR%\KQCloud.exe
    echo 请先运行 build-win-vs.bat 完成构建
    pause
    exit /b 1
)
if not exist "%BUILDDIR%\MAC.dll" (
    echo [错误] 构建产物缺少 SDX4 驱动文件 MAC.dll
    echo 请确认 resources\sdx4 下的驱动文件已随构建部署
    pause
    exit /b 1
)
:: Git LFS 没装时 clone 下来的驱动是 130 字节的指针文本，装进包里等于没带驱动
for %%F in ("%BUILDDIR%\MAC.dll") do if %%~zF LSS 10000 (
    echo [错误] MAC.dll 只有 %%~zF 字节，是 Git LFS 指针文件
    echo 请先安装 git-lfs 并执行 git lfs pull 再重新构建
    pause
    exit /b 1
)
if not exist "drivers\X-Rite_MAC_Driver_v3.0.0.exe" (
    echo [错误] 缺少设备驱动安装包 drivers\X-Rite_MAC_Driver_v3.0.0.exe
    pause
    exit /b 1
)

echo [信息] 正在生成安装程序...
echo.

"%ISCC%" setup.iss

if %errorlevel% neq 0 (
    echo [错误] 安装程序生成失败
    pause
    exit /b 1
)

echo.
echo ============================================
echo   安装程序生成完成！
echo   输出文件: installer-output\KQCloud_Setup_1.0.0.exe
echo ============================================
pause
