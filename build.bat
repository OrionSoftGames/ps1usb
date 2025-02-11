@echo off

set clean_failed=0

if "%1" == "-h" goto usage
if "%1" == "help" goto usage
if not "%1" == "" if not "%1" == "clean" goto usage

set "build_dir=%~dp0"
:: Remove trailing slash if present because make doesn't like such a path with quotes...
if "%build_dir:~-1%" == "\" set "build_dir=%build_dir:~0,-1%"
set target=%1

make -C "%build_dir%" %target%
if not %errorlevel% equ 0 (
	if not "%target%" == "clean" goto build_error
	set clean_failed=1
)

make -C "%build_dir%\cartrom" %target%
if not %errorlevel% equ 0 (
	if not "%target%" == "clean" goto build_error
	set clean_failed=1
)

make -C "%build_dir%\ar_flash" %target%
if not %errorlevel% equ 0 (
	if not "%target%" == "clean" goto build_error
	set clean_failed=1
)

if "%clean_failed%" == "1" goto clean_error

goto exit

:build_error
echo.
echo Error: Build process failed!
echo.
exit /b 1

:clean_error
echo.
echo Error: Some cleaning failed!
echo.
exit /b 2

:usage
echo Usage: %~nx0 [clean]
echo.

:exit
