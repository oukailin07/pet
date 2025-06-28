@echo off
setlocal enabledelayedexpansion

REM 版本构建脚本 (Windows版本)
REM 用于自动生成版本信息和构建固件

set PROJECT_NAME=pet-feeder
set BUILD_DIR=build
set VERSION_FILE=main\version_info.h

echo 版本构建脚本启动...

REM 获取Git信息
for /f "tokens=*" %%i in ('git rev-parse --short HEAD 2^>nul') do set GIT_HASH=%%i
if "%GIT_HASH%"=="" set GIT_HASH=unknown

for /f "tokens=*" %%i in ('git rev-parse --abbrev-ref HEAD 2^>nul') do set GIT_BRANCH=%%i
if "%GIT_BRANCH%"=="" set GIT_BRANCH=unknown

for /f "tokens=*" %%i in ('git describe --tags --exact-match 2^>nul') do set GIT_TAG=%%i
if "%GIT_TAG%"=="" set GIT_TAG=

REM 获取构建时间
for /f "tokens=1-3 delims=/ " %%a in ('date /t') do set BUILD_DATE=%%c-%%a-%%b
for /f "tokens=1-2 delims=: " %%a in ('time /t') do set BUILD_TIME=%%a:%%b
set BUILD_TIMESTAMP=%time:~0,2%%time:~3,2%%time:~6,2%

REM 版本号解析
if not "%GIT_TAG%"=="" (
    REM 从Git标签解析版本号
    for /f "tokens=1-3 delims=." %%a in ("%GIT_TAG:v=%") do (
        set VERSION_MAJOR=%%a
        set VERSION_MINOR=%%b
        set VERSION_PATCH=%%c
    )
    set VERSION_SUFFIX=stable
) else (
    REM 从version.h文件读取版本号
    if exist "main\version.h" (
        for /f "tokens=2" %%a in ('findstr "FIRMWARE_VERSION_MAJOR" main\version.h') do set VERSION_MAJOR=%%a
        for /f "tokens=2" %%a in ('findstr "FIRMWARE_VERSION_MINOR" main\version.h') do set VERSION_MINOR=%%a
        for /f "tokens=2" %%a in ('findstr "FIRMWARE_VERSION_PATCH" main\version.h') do set VERSION_PATCH=%%a
        for /f "tokens=2 delims=^"" %%a in ('findstr "FIRMWARE_VERSION_SUFFIX" main\version.h') do set VERSION_SUFFIX=%%a
    ) else (
        echo 错误: 找不到version.h文件
        exit /b 1
    )
)

REM 构建号（使用时间戳）
set BUILD_NUMBER=%BUILD_TIMESTAMP%

echo 生成版本信息...
echo 版本号: v%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%
echo 构建号: %BUILD_NUMBER%
echo 后缀: %VERSION_SUFFIX%
echo Git哈希: %GIT_HASH%
echo Git分支: %GIT_BRANCH%
if not "%GIT_TAG%"=="" echo Git标签: %GIT_TAG%
echo 构建时间: %BUILD_DATE% %BUILD_TIME%

REM 生成版本信息头文件
(
echo // 自动生成的版本信息文件
echo // 生成时间: %BUILD_DATE% %BUILD_TIME%
echo // 请勿手动修改此文件
echo.
echo #pragma once
echo.
echo // 版本号定义
echo #define FIRMWARE_VERSION_MAJOR    %VERSION_MAJOR%
echo #define FIRMWARE_VERSION_MINOR    %VERSION_MINOR%
echo #define FIRMWARE_VERSION_PATCH    %VERSION_PATCH%
echo #define FIRMWARE_VERSION_BUILD    %BUILD_NUMBER%
echo #define FIRMWARE_VERSION_SUFFIX   "%VERSION_SUFFIX%"
echo.
echo // 构建信息
echo #define BUILD_DATE                "%BUILD_DATE%"
echo #define BUILD_TIME                "%BUILD_TIME%"
echo #define BUILD_TIMESTAMP           %BUILD_TIMESTAMP%
echo #define GIT_HASH                  "%GIT_HASH%"
echo #define GIT_BRANCH                "%GIT_BRANCH%"
echo #define GIT_TAG                   "%GIT_TAG%"
echo.
echo // 版本字符串
echo #define VERSION_STRING            "v%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%-%BUILD_NUMBER%-%VERSION_SUFFIX%"
) > %VERSION_FILE%

echo 版本信息已生成: %VERSION_FILE%

REM 检查构建类型
set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=release

if "%BUILD_TYPE%"=="debug" (
    echo 构建类型: Debug
    set BUILD_FLAGS=
) else if "%BUILD_TYPE%"=="release" (
    echo 构建类型: Release
    set BUILD_FLAGS=
) else (
    echo 未知的构建类型: %BUILD_TYPE%
    echo 用法: %0 [debug^|release]
    exit /b 1
)

REM 清理构建目录
if "%2"=="clean" (
    echo 清理构建目录...
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
)

REM 构建项目
echo 开始构建项目...
idf.py build %BUILD_FLAGS%

if %ERRORLEVEL% equ 0 (
    echo 构建成功!
    
    REM 生成固件信息
    set FIRMWARE_FILE=%BUILD_DIR%\%PROJECT_NAME%.bin
    if exist "%FIRMWARE_FILE%" (
        for %%A in ("%FIRMWARE_FILE%") do set FIRMWARE_SIZE=%%~zA
        set /a FIRMWARE_SIZE_KB=%FIRMWARE_SIZE%/1024
        echo 固件信息:
        echo   文件: %FIRMWARE_FILE%
        echo   大小: %FIRMWARE_SIZE_KB% KB
        echo   版本: v%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%-%BUILD_NUMBER%-%VERSION_SUFFIX%
    )
    
    REM 生成版本报告
    set VERSION_REPORT=%BUILD_DIR%\version_report.txt
    (
        echo 版本构建报告
        echo =============
        echo.
        echo 项目名称: %PROJECT_NAME%
        echo 构建时间: %BUILD_DATE% %BUILD_TIME%
        echo 构建类型: %BUILD_TYPE%
        echo.
        echo 版本信息:
        echo   主版本号: %VERSION_MAJOR%
        echo   次版本号: %VERSION_MINOR%
        echo   补丁版本号: %VERSION_PATCH%
        echo   构建号: %BUILD_NUMBER%
        echo   版本后缀: %VERSION_SUFFIX%
        echo   完整版本号: v%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%-%BUILD_NUMBER%-%VERSION_SUFFIX%
        echo.
        echo Git信息:
        echo   提交哈希: %GIT_HASH%
        echo   分支: %GIT_BRANCH%
        echo   标签: %GIT_TAG%
        echo.
        echo 构建信息:
        echo   构建时间戳: %BUILD_TIMESTAMP%
        echo   固件文件: %FIRMWARE_FILE%
        echo   固件大小: %FIRMWARE_SIZE_KB% KB
        echo.
        echo 构建环境:
        echo   操作系统: Windows
        echo   架构: x64
    ) > "%VERSION_REPORT%"
    
    echo 版本报告已生成: %VERSION_REPORT%
    
) else (
    echo 构建失败!
    exit /b 1
)

echo 构建完成! 