@echo off
setlocal enabledelayedexpansion

REM �汾�����ű� (Windows�汾)
REM �����Զ����ɰ汾��Ϣ�͹����̼�

set PROJECT_NAME=pet-feeder
set BUILD_DIR=build
set VERSION_FILE=main\version_info.h

echo �汾�����ű�����...

REM ��ȡGit��Ϣ
for /f "tokens=*" %%i in ('git rev-parse --short HEAD 2^>nul') do set GIT_HASH=%%i
if "%GIT_HASH%"=="" set GIT_HASH=unknown

for /f "tokens=*" %%i in ('git rev-parse --abbrev-ref HEAD 2^>nul') do set GIT_BRANCH=%%i
if "%GIT_BRANCH%"=="" set GIT_BRANCH=unknown

for /f "tokens=*" %%i in ('git describe --tags --exact-match 2^>nul') do set GIT_TAG=%%i
if "%GIT_TAG%"=="" set GIT_TAG=

REM ��ȡ����ʱ��
for /f "tokens=1-3 delims=/ " %%a in ('date /t') do set BUILD_DATE=%%c-%%a-%%b
for /f "tokens=1-2 delims=: " %%a in ('time /t') do set BUILD_TIME=%%a:%%b
set BUILD_TIMESTAMP=%time:~0,2%%time:~3,2%%time:~6,2%

REM �汾�Ž���
if not "%GIT_TAG%"=="" (
    REM ��Git��ǩ�����汾��
    for /f "tokens=1-3 delims=." %%a in ("%GIT_TAG:v=%") do (
        set VERSION_MAJOR=%%a
        set VERSION_MINOR=%%b
        set VERSION_PATCH=%%c
    )
    set VERSION_SUFFIX=stable
) else (
    REM ��version.h�ļ���ȡ�汾��
    if exist "main\version.h" (
        for /f "tokens=2" %%a in ('findstr "FIRMWARE_VERSION_MAJOR" main\version.h') do set VERSION_MAJOR=%%a
        for /f "tokens=2" %%a in ('findstr "FIRMWARE_VERSION_MINOR" main\version.h') do set VERSION_MINOR=%%a
        for /f "tokens=2" %%a in ('findstr "FIRMWARE_VERSION_PATCH" main\version.h') do set VERSION_PATCH=%%a
        for /f "tokens=2 delims=^"" %%a in ('findstr "FIRMWARE_VERSION_SUFFIX" main\version.h') do set VERSION_SUFFIX=%%a
    ) else (
        echo ����: �Ҳ���version.h�ļ�
        exit /b 1
    )
)

REM �����ţ�ʹ��ʱ�����
set BUILD_NUMBER=%BUILD_TIMESTAMP%

echo ���ɰ汾��Ϣ...
echo �汾��: v%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%
echo ������: %BUILD_NUMBER%
echo ��׺: %VERSION_SUFFIX%
echo Git��ϣ: %GIT_HASH%
echo Git��֧: %GIT_BRANCH%
if not "%GIT_TAG%"=="" echo Git��ǩ: %GIT_TAG%
echo ����ʱ��: %BUILD_DATE% %BUILD_TIME%

REM ���ɰ汾��Ϣͷ�ļ�
(
echo // �Զ����ɵİ汾��Ϣ�ļ�
echo // ����ʱ��: %BUILD_DATE% %BUILD_TIME%
echo // �����ֶ��޸Ĵ��ļ�
echo.
echo #pragma once
echo.
echo // �汾�Ŷ���
echo #define FIRMWARE_VERSION_MAJOR    %VERSION_MAJOR%
echo #define FIRMWARE_VERSION_MINOR    %VERSION_MINOR%
echo #define FIRMWARE_VERSION_PATCH    %VERSION_PATCH%
echo #define FIRMWARE_VERSION_BUILD    %BUILD_NUMBER%
echo #define FIRMWARE_VERSION_SUFFIX   "%VERSION_SUFFIX%"
echo.
echo // ������Ϣ
echo #define BUILD_DATE                "%BUILD_DATE%"
echo #define BUILD_TIME                "%BUILD_TIME%"
echo #define BUILD_TIMESTAMP           %BUILD_TIMESTAMP%
echo #define GIT_HASH                  "%GIT_HASH%"
echo #define GIT_BRANCH                "%GIT_BRANCH%"
echo #define GIT_TAG                   "%GIT_TAG%"
echo.
echo // �汾�ַ���
echo #define VERSION_STRING            "v%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%-%BUILD_NUMBER%-%VERSION_SUFFIX%"
) > %VERSION_FILE%

echo �汾��Ϣ������: %VERSION_FILE%

REM ��鹹������
set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=release

if "%BUILD_TYPE%"=="debug" (
    echo ��������: Debug
    set BUILD_FLAGS=
) else if "%BUILD_TYPE%"=="release" (
    echo ��������: Release
    set BUILD_FLAGS=
) else (
    echo δ֪�Ĺ�������: %BUILD_TYPE%
    echo �÷�: %0 [debug^|release]
    exit /b 1
)

REM ������Ŀ¼
if "%2"=="clean" (
    echo ������Ŀ¼...
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
)

REM ������Ŀ
echo ��ʼ������Ŀ...
idf.py build %BUILD_FLAGS%

if %ERRORLEVEL% equ 0 (
    echo �����ɹ�!
    
    REM ���ɹ̼���Ϣ
    set FIRMWARE_FILE=%BUILD_DIR%\%PROJECT_NAME%.bin
    if exist "%FIRMWARE_FILE%" (
        for %%A in ("%FIRMWARE_FILE%") do set FIRMWARE_SIZE=%%~zA
        set /a FIRMWARE_SIZE_KB=%FIRMWARE_SIZE%/1024
        echo �̼���Ϣ:
        echo   �ļ�: %FIRMWARE_FILE%
        echo   ��С: %FIRMWARE_SIZE_KB% KB
        echo   �汾: v%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%-%BUILD_NUMBER%-%VERSION_SUFFIX%
    )
    
    REM ���ɰ汾����
    set VERSION_REPORT=%BUILD_DIR%\version_report.txt
    (
        echo �汾��������
        echo =============
        echo.
        echo ��Ŀ����: %PROJECT_NAME%
        echo ����ʱ��: %BUILD_DATE% %BUILD_TIME%
        echo ��������: %BUILD_TYPE%
        echo.
        echo �汾��Ϣ:
        echo   ���汾��: %VERSION_MAJOR%
        echo   �ΰ汾��: %VERSION_MINOR%
        echo   �����汾��: %VERSION_PATCH%
        echo   ������: %BUILD_NUMBER%
        echo   �汾��׺: %VERSION_SUFFIX%
        echo   �����汾��: v%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH%-%BUILD_NUMBER%-%VERSION_SUFFIX%
        echo.
        echo Git��Ϣ:
        echo   �ύ��ϣ: %GIT_HASH%
        echo   ��֧: %GIT_BRANCH%
        echo   ��ǩ: %GIT_TAG%
        echo.
        echo ������Ϣ:
        echo   ����ʱ���: %BUILD_TIMESTAMP%
        echo   �̼��ļ�: %FIRMWARE_FILE%
        echo   �̼���С: %FIRMWARE_SIZE_KB% KB
        echo.
        echo ��������:
        echo   ����ϵͳ: Windows
        echo   �ܹ�: x64
    ) > "%VERSION_REPORT%"
    
    echo �汾����������: %VERSION_REPORT%
    
) else (
    echo ����ʧ��!
    exit /b 1
)

echo �������! 