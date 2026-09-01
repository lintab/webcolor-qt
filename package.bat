@echo off
chcp 65001 >nul
echo ============================================
echo   恺祁颜色系统 - 打包安装程序
echo ============================================
echo.

:: 检查 Inno Setup
set "ISCC="
if exist "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" (
    set "ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
) else if exist "C:\Program Files\Inno Setup 6\ISCC.exe" (
    set "ISCC=C:\Program Files\Inno Setup 6\ISCC.exe"
) else (
    echo [错误] 未找到 Inno Setup 6
    echo 请从 https://jrsoftware.org/isinfo.php 下载安装
    pause
    exit /b 1
)

:: 检查构建产物
if not exist "build-win\WebColorQt.exe" (
    echo [错误] 未找到构建产物，请先运行 build-win.bat
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
echo   输出文件: installer-output\恺祁颜色系统_Setup_1.0.0.exe
echo ============================================
pause
