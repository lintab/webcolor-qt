@echo off
setlocal
rem ASCII-only on purpose: cmd.exe mis-parses UTF-8 .bat files.
rem Uses the Visual Studio generator with an ASCII build dir, because CMake 4.4.0's
rem NMake generator fails on non-ASCII paths (cmake -E vs_link_exe "Unknown system error -1").
set "QT_DIR=%~dp0..\Qt\6.8.2\msvc2022_64"
set "PATH=%QT_DIR%\bin;%PATH%"
set "SRC=%~dp0."
set "BUILD=D:\webcolor-qt-build-vs"
if not exist "%BUILD%" mkdir "%BUILD%"
echo === configure ===
cmake -S "%SRC%" -B "%BUILD%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%QT_DIR%"
echo CONFIGURE_EXIT=%errorlevel%
echo === build ===
cmake --build "%BUILD%" --config Release -- /m
echo BUILD_EXIT=%errorlevel%
if not exist "%BUILD%\Release\WebColorQt.exe" (echo EXE_MISSING & pause & exit /b 1)
echo === windeployqt ===
windeployqt "%BUILD%\Release\WebColorQt.exe" --no-translations --no-system-d3d-compiler --no-opengl-sw
echo DEPLOY_EXIT=%errorlevel%
echo DONE: %BUILD%\Release\WebColorQt.exe
pause
