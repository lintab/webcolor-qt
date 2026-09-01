# Windows 构建指南

## 环境准备

### 1. 安装 Qt 6
- 下载 [Qt Online Installer](https://www.qt.io/download-qt-installer)
- 安装时选择：
  - Qt 6.x → MSVC 2022 64-bit（或 MinGW 64-bit）
  - Qt 6.x → Additional Libraries → Qt Network（通常默认已选）
- 记住安装路径，默认如 `C:\Qt\6.8.0\msvc2022_64`

### 2. 安装 CMake
- 下载 [CMake](https://cmake.org/download/)
- 安装时勾选 "Add CMake to system PATH"

### 3. 安装编译器（二选一）

**方案 A：MSVC（推荐）**
- 下载 [Visual Studio Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)
- 安装时勾选 "使用 C++ 的桌面开发"
- 安装后，在开始菜单找到 "x64 Native Tools Command Prompt" 打开

**方案 B：MinGW**
- Qt 安装时如果选了 MinGW，则自带编译器，无需额外安装

### 4. 安装 Inno Setup（制作安装程序）
- 下载 [Inno Setup 6](https://jrsoftware.org/download.php/is.exe)
- 默认安装即可

## 构建步骤

### 方式一：命令行构建

1. **打开命令行**
   - MSVC：打开 "x64 Native Tools Command Prompt for VS"
   - MinGW：打开普通 CMD 即可

2. **进入项目目录**
   ```
   cd C:\path\to\webcolor-qt
   ```

3. **运行构建脚本**
   ```
   build-win.bat
   ```

4. **打包安装程序**
   ```
   package.bat
   ```

5. **完成**
   - 安装程序在 `installer-output\恺祁颜色系统_Setup_1.0.0.exe`

### 方式二：手动构建

```bat
:: 设置 Qt 路径（根据实际安装路径修改）
set QT_DIR=C:\Qt\6.8.0\msvc2022_64
set PATH=%QT_DIR%\bin;%PATH%

:: 构建
mkdir build-win
cd build-win
cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_DIR%"
nmake

:: 打包依赖
windeployqt WebColorQt.exe

:: 生成安装程序（需要 Inno Setup）
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" ..\setup.iss
```

## 文件说明

| 文件 | 说明 |
|------|------|
| `build-win.bat` | 一键构建脚本，自动检测 Qt 路径 |
| `package.bat` | 一键打包安装程序脚本 |
| `setup.iss` | Inno Setup 安装程序配置 |

## 常见问题

**Q: build-win.bat 提示找不到 Qt？**
A: 打开脚本，修改 `QT_DIR` 的自动检测部分，手动设置你的 Qt 安装路径。

**Q: 编译报 "nmake not found"？**
A: 需要使用 MSVC 的命令行环境，请从开始菜单打开 "x64 Native Tools Command Prompt"。

**Q: windeployqt 报错？**
A: 确保 Qt 的 bin 目录在 PATH 中，且 `WebColorQt.exe` 存在于当前目录。
