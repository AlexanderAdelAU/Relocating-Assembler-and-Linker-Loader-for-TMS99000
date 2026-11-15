@echo off
setlocal EnableExtensions

REM ================================================================
REM CLIB99 strict split build for Small-C/Plus
REM
REM   src\       = all source/build input files
REM   toolchain\ = compiler/assembler/library executables
REM   build\     = disposable working directory (recreated every run)
REM   dist\      = the finished libraries only
REM
REM Nothing is compiled in the project root.
REM
REM THREE LIBRARIES ARE BUILT
REM   clib99.LIB    printf and the number conversions
REM   clib99f.LIB   the same with printf2 and floating point
REM   strlib99.LIB  the string and character routines
REM
REM WHAT CHANGED FROM THE OLD BUILD
REM   - Every source file is now named after the FUNCTION it defines.
REM     They were not: atoi.c defined utoi() and utoi.c defined atoi().
REM   - itod itoo itou itox utoi atoi xtoi poll live HERE now and have
REM     been REMOVED from CommonLibrary.  They were in both, and had
REM     diverged - CLIB99's itou returned an EMPTY string for any value
REM     above 32767, which is the whole point of an unsigned convertor.
REM     printf needs itod/itou/itox/utoi, which is why the duplicates
REM     grew in the first place; keeping them here makes clib99 self
REM     contained and leaves exactly one copy of each in the world.
REM   - The old build compiled "pole" from a file called POLL.C, so that
REM     step failed every run and a stale pole.R99 went into the library.
REM   - strrchr was put into strlib99 but never compiled.
REM   - strcpy and strlen went into clib99 as well as strlib99, though
REM     printf needs neither.  They are in strlib99 only now.
REM   - cfloatm48.A99 is an EMPTY module - NAM, EVEN, END and no code.
REM     It is assembled and included so the library keeps its shape, but
REM     it does nothing.  Worth deciding whether it should exist.
REM ================================================================

set "ROOT=%~dp0"
set "SRC=%ROOT%src"
set "TOOLS=%ROOT%toolchain"
set "BUILD=%ROOT%build"
set "DIST=%ROOT%dist"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"

if not exist "%SRC%\printf.c" goto :missing_source
if not exist "%TOOLS%\smallcp.exe" goto :missing_tools
if not exist "%TOOLS%\r99.exe" goto :missing_tools
if not exist "%TOOLS%\lib99.exe" goto :missing_tools

REM Start with a completely clean work area so stale generated files
REM can never leak into a new library.
if exist "%BUILD%" rmdir /s /q "%BUILD%"
mkdir "%BUILD%"
if errorlevel 1 goto :fail_root

if not exist "%DIST%" mkdir "%DIST%"
del /f /q "%DIST%\clib99.LIB" "%DIST%\clib99.NDX" 2>nul
del /f /q "%DIST%\clib99f.LIB" "%DIST%\clib99f.NDX" 2>nul
del /f /q "%DIST%\strlib99.LIB" "%DIST%\strlib99.NDX" 2>nul

echo === Stage source into BUILD ===
copy /y "%SRC%\*.*" "%BUILD%\" >nul
if errorlevel 1 goto :fail_root

echo === Stage toolchain into BUILD ===
copy /y "%TOOLS%\*.exe" "%BUILD%\" >nul
if errorlevel 1 goto :fail_root

pushd "%BUILD%"
if errorlevel 1 goto :fail_root

echo.
echo === Compile - number conversion ===
call :compile itod
if errorlevel 1 goto :fail_build
call :compile itoo
if errorlevel 1 goto :fail_build
call :compile itou
if errorlevel 1 goto :fail_build
call :compile itox
if errorlevel 1 goto :fail_build
call :compile utoi
if errorlevel 1 goto :fail_build
call :compile atoi
if errorlevel 1 goto :fail_build
call :compile xtoi
if errorlevel 1 goto :fail_build

echo.
echo === Compile - formatted output ===
call :compile printf
if errorlevel 1 goto :fail_build
call :compile printf2
if errorlevel 1 goto :fail_build

echo.
echo === Compile - strings and characters ===
call :compile strcat
if errorlevel 1 goto :fail_build
call :compile strchr
if errorlevel 1 goto :fail_build
call :compile strcmp
if errorlevel 1 goto :fail_build
call :compile strcpy
if errorlevel 1 goto :fail_build
call :compile strlen
if errorlevel 1 goto :fail_build
call :compile strncat
if errorlevel 1 goto :fail_build
call :compile strncmp
if errorlevel 1 goto :fail_build
call :compile strncpy
if errorlevel 1 goto :fail_build
call :compile strrchr
if errorlevel 1 goto :fail_build
call :compile toupper
if errorlevel 1 goto :fail_build
call :compile iscons
if errorlevel 1 goto :fail_build
call :compile poll
if errorlevel 1 goto :fail_build

echo.
echo === Compile - floating point ===
call :compile float
if errorlevel 1 goto :fail_build

echo.
echo === Assemble ===
call :assemble itod
if errorlevel 1 goto :fail_build
call :assemble itoo
if errorlevel 1 goto :fail_build
call :assemble itou
if errorlevel 1 goto :fail_build
call :assemble itox
if errorlevel 1 goto :fail_build
call :assemble utoi
if errorlevel 1 goto :fail_build
call :assemble atoi
if errorlevel 1 goto :fail_build
call :assemble xtoi
if errorlevel 1 goto :fail_build
call :assemble printf
if errorlevel 1 goto :fail_build
call :assemble printf2
if errorlevel 1 goto :fail_build
call :assemble strcat
if errorlevel 1 goto :fail_build
call :assemble strchr
if errorlevel 1 goto :fail_build
call :assemble strcmp
if errorlevel 1 goto :fail_build
call :assemble strcpy
if errorlevel 1 goto :fail_build
call :assemble strlen
if errorlevel 1 goto :fail_build
call :assemble strncat
if errorlevel 1 goto :fail_build
call :assemble strncmp
if errorlevel 1 goto :fail_build
call :assemble strncpy
if errorlevel 1 goto :fail_build
call :assemble strrchr
if errorlevel 1 goto :fail_build
call :assemble toupper
if errorlevel 1 goto :fail_build
call :assemble iscons
if errorlevel 1 goto :fail_build
call :assemble poll
if errorlevel 1 goto :fail_build
call :assemble float
if errorlevel 1 goto :fail_build
call :assemble cfloatm48
if errorlevel 1 goto :fail_build

echo.
echo === Create clib99.LIB ===
lib99 -U clib99 printf itod itoo itou itox utoi atoi xtoi iscons poll > lib99.log
set "RC=%ERRORLEVEL%"
type lib99.log
if not "%RC%"=="0" goto :fail_build
if not exist "clib99.LIB" goto :fail_build
if not exist "clib99.NDX" goto :fail_build

echo.
echo === Create clib99f.LIB - the same, with float ===
lib99 -U clib99f printf2 itod itoo itou itox utoi atoi xtoi iscons poll float cfloatm48 > lib99f.log
set "RC=%ERRORLEVEL%"
type lib99f.log
if not "%RC%"=="0" goto :fail_build
if not exist "clib99f.LIB" goto :fail_build
if not exist "clib99f.NDX" goto :fail_build

echo.
echo === Create strlib99.LIB ===
lib99 -U strlib99 strcat strchr strcmp strcpy strlen strncat strncmp strncpy strrchr toupper > libstr.log
set "RC=%ERRORLEVEL%"
type libstr.log
if not "%RC%"=="0" goto :fail_build
if not exist "strlib99.LIB" goto :fail_build
if not exist "strlib99.NDX" goto :fail_build

echo.
echo === Publish to DIST ===
copy /y "clib99.LIB" "%DIST%\clib99.LIB" >nul
if errorlevel 1 goto :fail_build
copy /y "clib99.NDX" "%DIST%\clib99.NDX" >nul
if errorlevel 1 goto :fail_build
copy /y "clib99f.LIB" "%DIST%\clib99f.LIB" >nul
if errorlevel 1 goto :fail_build
copy /y "clib99f.NDX" "%DIST%\clib99f.NDX" >nul
if errorlevel 1 goto :fail_build
copy /y "strlib99.LIB" "%DIST%\strlib99.LIB" >nul
if errorlevel 1 goto :fail_build
copy /y "strlib99.NDX" "%DIST%\strlib99.NDX" >nul
if errorlevel 1 goto :fail_build

popd

echo.
echo ================================================================
echo CLIB99 build completed successfully.
echo Output: "%DIST%\clib99.LIB"  "%DIST%\clib99f.LIB"  "%DIST%\strlib99.LIB"
echo Work  : "%BUILD%"
echo ================================================================
exit /b 0

:compile
REM Modules whose source holds BOTH a C and an assembly version are built
REM from the assembly.  The C stays in the file as the definition - drop a
REM name from this list to fall back to it.
set "ASMVER="
if /i "%1"=="itox" set "ASMVER=-DASMVER"
if /i "%1"=="itou" set "ASMVER=-DASMVER"
if /i "%1"=="xtoi" set "ASMVER=-DASMVER"
if /i "%1"=="utoi" set "ASMVER=-DASMVER"
smallcp -C -M %ASMVER% %1 > %1.compile.log
set "RC=%ERRORLEVEL%"
type %1.compile.log
if not "%RC%"=="0" exit /b 1
"%FINDSTR%" /R /C:"Errors: *0" %1.compile.log >nul
if errorlevel 1 exit /b 1
exit /b 0

:assemble
R99 %1 SCHCLC > %1.assemble.log
set "RC=%ERRORLEVEL%"
type %1.assemble.log
if not "%RC%"=="0" exit /b 1
"%FINDSTR%" /C:"No error(s)." %1.assemble.log >nul
if errorlevel 1 exit /b 1
exit /b 0

:missing_source
echo.
echo *** BUILD FAILED: required source files are missing from "%SRC%" ***
exit /b 1

:missing_tools
echo.
echo *** BUILD FAILED: required toolchain files are missing from "%TOOLS%" ***
exit /b 1

:fail_build
echo.
echo *** CLIB99 BUILD FAILED - no DIST libraries will be published ***
del /f /q "%DIST%\clib99.LIB" "%DIST%\clib99.NDX" 2>nul
del /f /q "%DIST%\clib99f.LIB" "%DIST%\clib99f.NDX" 2>nul
del /f /q "%DIST%\strlib99.LIB" "%DIST%\strlib99.NDX" 2>nul
popd
exit /b 1

:fail_root
echo.
echo *** CLIB99 BUILD FAILED during staging ***
exit /b 1
