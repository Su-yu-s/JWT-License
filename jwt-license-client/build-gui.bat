@echo off
chcp 65001 >nul 2>&1
setlocal enabledelayedexpansion
REM ============================================================
REM  iK Client — GUI Build & Deploy Script
REM  编译器: MinGW (Qt6.11.1 自带)
REM  构建系统: CMake + MinGW Makefiles
REM ============================================================

REM ---------- 路径配置 ----------
set QT_PATH=D:/Qt/qt/6.11.1/mingw_64
set MINGW_BIN=D:/Qt/qt/Tools/mingw1310_64/bin
set MAKE=%MINGW_BIN%/mingw32-make.exe
set WINDEPLOYQT=%QT_PATH%/bin/windeployqt.exe

set PROJECT_DIR=%~dp0
set BUILD_DIR=%PROJECT_DIR%build
set EXE_NAME=iK.exe

REM ---------- 环境校验 ----------
if not exist "%QT_PATH%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo [ERROR] Qt6 未找到: %QT_PATH%
    echo 请确认 Qt 6.5+ ^(MinGW 64-bit^) 已安装，或修改脚本中的 QT_PATH。
    pause
    exit /b 1
)

if not exist "%WINDEPLOYQT%" (
    echo [ERROR] windeployqt 未找到: %WINDEPLOYQT%
    pause
    exit /b 1
)

if not exist "%MINGW_BIN%\g++.exe" (
    echo [ERROR] MinGW 编译器未找到: %MINGW_BIN%\g++.exe
    pause
    exit /b 1
)

echo ============================================================
echo  iK Client — GUI Build
echo ============================================================
echo.
echo   Qt 路径:      %QT_PATH%
echo   MinGW 路径:   %MINGW_BIN%
echo   输出目录:      %BUILD_DIR%
echo.

REM ---------- 清理 & 创建构建目录 ----------
if exist "%BUILD_DIR%" (
    echo [1/4] 清理旧的构建目录...
    rmdir /s /q "%BUILD_DIR%"
)
mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM ---------- 编译 .exe 图标资源 ----------
echo [1.5/4] 编译图标资源...
set "PATH=%MINGW_BIN%;%PATH%"
windres --use-temp-file -J rc -O coff -I "%PROJECT_DIR%resources" -i "%PROJECT_DIR%resources\app.rc" -o "%BUILD_DIR%\app_icon.res"
if %errorlevel% neq 0 (
    echo   [警告] 图标编译失败，将使用默认图标
)

REM ---------- CMake 配置 ----------
echo [2/4] CMake 配置...
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%QT_PATH%" -DCMAKE_CXX_COMPILER="%MINGW_BIN%/g++.exe" -DCMAKE_MAKE_PROGRAM="%MAKE%" -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake 配置失败！
    cd /d "%PROJECT_DIR%"
    pause
    exit /b 1
)

REM ---------- 编译 ----------
echo [3/4] 编译中...
%MAKE% -j%NUMBER_OF_PROCESSORS%

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] 编译失败！
    cd /d "%PROJECT_DIR%"
    pause
    exit /b 1
)

REM ---------- 验证 exe 存在 ----------
if not exist "%BUILD_DIR%\%EXE_NAME%" (
    echo.
    echo [ERROR] 未生成 %EXE_NAME%，编译可能不完整。
    cd /d "%PROJECT_DIR%"
    pause
    exit /b 1
)

REM ---------- 部署 DLL ----------
echo [4/4] 部署 Qt 运行时 ^& 依赖...

REM 使用 windeployqt 自动扫描并复制所有 Qt DLL + 插件
echo   运行 windeployqt...
"%WINDEPLOYQT%" --release --no-translations --no-compiler-runtime "%BUILD_DIR%\%EXE_NAME%"

if %errorlevel% neq 0 (
    echo   [警告] windeployqt 返回非零，继续手动补全...
)

REM windeployqt 的 MinGW 版本可能漏掉 MinGW 运行时 DLL，手动补全
echo   [OK] windeployqt 完成，补全 MinGW 运行时...
for %%d in (
    libgcc_s_seh-1.dll
    libstdc++-6.dll
    libwinpthread-1.dll
) do (
    if exist "%MINGW_BIN%\%%d" (
        copy /y "%MINGW_BIN%\%%d" "%BUILD_DIR%\" >nul
    )
)

REM ---------- 输出汇总 ----------
echo.
echo ============================================================
echo   构建成功！
echo ============================================================
echo.
echo   可执行文件:   %BUILD_DIR%\%EXE_NAME%
echo.
echo   已部署的组件:
echo     - Qt 核心 DLL ^(Qt6Core, Qt6Gui, Qt6Widgets, Qt6Network^)
echo     - MinGW 运行时 DLL
echo     - platforms\qwindows.dll     ^(窗口平台插件^)
echo     - tls\*.dll                  ^(HTTPS/TLS 支持^)
echo     - imageformats\*.dll         ^(图片格式支持^)
echo     - styles\*.dll               ^(界面风格插件^)
echo.
echo   运行方式:
echo     %BUILD_DIR%\%EXE_NAME%
echo     或在文件管理器中双击 %EXE_NAME%
echo.
echo   分发给其他机器时，确保整个 build 目录一并拷贝。
echo ============================================================

cd /d "%PROJECT_DIR%"
pause
endlocal
