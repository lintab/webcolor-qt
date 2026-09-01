@echo off
chcp 65001 >nul
echo ============================================
echo   恺祁颜色系统 - Windows 构建脚本
echo ============================================
echo.

:: 检查 Qt 和 CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 未找到 cmake，请先安装 CMake 并添加到 PATH
    pause
    exit /b 1
)

:: 自动检测 Qt 安装路径
set "QT_DIR="
if exist "C:\Qt\6.8.0\msvc2022_64" (
    set "QT_DIR=C:\Qt\6.8.0\msvc2022_64"
) else if exist "C:\Qt\6.7.0\msvc2022_64" (
    set "QT_DIR=C:\Qt\6.7.0\msvc2022_64"
) else if exist "C:\Qt\6.8.0\mingw_64" (
    set "QT_DIR=C:\Qt\6.8.0\mingw_64"
) else if exist "C:\Qt\6.7.0\mingw_64" (
    set "QT_DIR=C:\Qt\6.7.0\mingw_64"
) else (
    echo [错误] 未自动检测到 Qt 安装路径，请手动设置 QT_DIR
    echo 请修改本脚本中的 QT_DIR 变量
    pause
    exit /b 1
)

echo [信息] 检测到 Qt 路径: %QT_DIR%

:: 将 Qt 的 bin 目录加入 PATH
set "PATH=%QT_DIR%\bin;%PATH%"

:: 创建构建目录
if not exist build-win mkdir build-win
cd build-win

:: CMake 配置
echo.
echo [1/3] CMake 配置中...
cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_DIR%"
if %errorlevel% neq 0 (
    echo [错误] CMake 配置失败
    pause
    exit /b 1
)

:: 编译
echo.
echo [2/3] 编译中...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [错误] 编译失败
    pause
    exit /b 1
)

:: 打包 Qt 依赖
echo.
echo [3/3] 打包 Qt 依赖...
windeployqt WebColorQt.exe --no-translations --no-system-d3d-compiler --no-opengl-sw

echo.
echo ============================================
echo   构建完成！
echo   输出目录: build-win\
echo ============================================
echo.
echo 接下来可以运行 package.bat 制作安装程序
pause
