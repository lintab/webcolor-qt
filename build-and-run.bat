@echo off
chcp 65001 >nul

:: 初始化 MSVC 环境
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" amd64
if %errorlevel% neq 0 (
    echo [错误] MSVC 环境初始化失败
    pause
    exit /b 1
)
echo [信息] MSVC 环境已加载

:: 设置 Qt 路径
set "QT_DIR=D:\新系统\Qt\6.8.2\msvc2022_64"
set "PATH=%QT_DIR%\bin;%PATH%"
echo [信息] Qt 路径: %QT_DIR%

:: 进入项目目录
cd /d "d:\新系统\webcolor-qt"

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
windeployqt KQCloud.exe --no-translations --no-system-d3d-compiler --no-opengl-sw

echo.
echo ============================================
echo   构建完成！
echo   输出目录: build-win\
echo ============================================

:: 直接启动程序
echo [信息] 启动 KQCloud...
start "" "KQCloud.exe"
