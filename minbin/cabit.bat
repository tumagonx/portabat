@echo off

setlocal enableextensions
set Test=tst%RANDOM%.tmp
set TmpDir=%TEMP%\$%RANDOM%$
set List=%TmpDir%\cab_%RANDOM%_.lst
set DirDump=%TmpDir%\cab_%RANDOM%_.dir
set CT=CompressionType=LZX
set CM=CompressionMemory=21
set CV=CurrentVersion
set DSize=2147483648
set NumTest=1
REM FSize=8388608
set FSize=16388608
if "%~1"=="" goto help
if "%~1"=="\" echo The whole drive?! & pause
md "%TmpDir%"
:switch
if "%~1"=="-r" (set Down=+1) & shift & goto switch
if "%~1"=="-o" (set Out=%~2) & shift & shift & goto switch
if "%~1"=="-d" (set Ddf=1) & shift & goto switch
if "%~1"=="-s" (set SMode=1) & shift & goto switch
if "%~1"=="-z" (set CT=CompressionType=MSZIP) & (set FSize=1048576) & shift & goto switch
if "%~1"=="-v" (set Size=%~2) & shift & shift
set /a NumTest=%Size%+1
if %NumTest% GEQ 2 (
    set /a DSize=%Size%*1048576
    set NumTest=1
    set Size=
    goto switch
)
for /f "tokens=1,2 delims=:" %%I in ("n%~1") do if %%I==n set Flt=%%J
if defined Flt shift & set Flt=%Flt:,=;%

REM Single Mode
REM trailing \ treated as escape char
if not defined SMode goto archive
set Opt=/D %CT% /D %CM%
:scab
if exist "%~1\*" goto snext
for %%F in ("%~1") do (
	if exist %%~dpnxF echo %%~nxF|findstr ".*\.cab\> .*\..*_\>">nul
	if errorlevel 1 (echo>>"%List%" "%%~dpnxF") else (echo>>"%DirDump%" "%%~dpnxF")
)
:snext
shift
if not "%~1"=="" goto scab
if not exist "%DirDump%" goto scomp
echo.Extracting:
for /f "usebackq delims=" %%F in ("%DirDump%") do (
	set Cab=%%~dpnxF
	set Dest=%%~dpF
	findstr /n /b "^MSCF" "%%~dpnxF">nul & if not errorlevel 1 (
		echo Extracting %%~nxF...
		if /i "%%~xF"==".cab" (
			::if %WINVER% GEQ 6.0 (
				echo %%~nxF & call :sextr
			::) else echo %%~nxF: Insufficient version of expand.exe
		) else (
			expand -r %%F>nul
			if errorlevel 1 (echo %%~nxF:failed) else echo %%~nxF
		)
	) else echo>>"%List%" %%F
)
:scomp
if not exist "%List%" goto end
echo.Compressing:
for /f "usebackq delims=" %%F in ("%List%") do (
	echo Compressing %%~nxF...
	makecab %Opt% /l "%%~dpF\" %%F>nul
	if errorlevel 1 (echo %%~nxF:failed) else echo %%~nxF
)
goto end
:sextr
set /p Confirm=Extract to current folder (and overwrite)? [Any]=Yes [S]pecify
if /i "%Confirm%"=="s" call :ask
if defined Stop goto snext
expand "%Cab%" -f:* "%Dest%\">nul
if errorlevel 1 (echo %Cab%:failed)
goto :EOF

REM Archive Mode
:archive
set Input=0
set Ins=0
set BaseDir=%CD%
set Tail=%BaseDir:~-1,1%
if not "%Tail%"=="\" set BaseDir=%BaseDir%\
set SD=.Set SourceDir
set DD=.Set DestinationDir
set Template=%TmpDir%\cab_%RANDOM%_.ddf
set Inf=cab_%RANDOM%_
set Rpt=cab_%RANDOM%_
if "%~1"=="" goto :EOF
:multiple
for /f "delims=/" %%I in ("none%~1") do if %%I==none goto help
set SubPath=
REM SendTo or CLI?
if defined Dest if "%BaseDir%"=="%USERPROFILE%\" if not "%Dest%"=="%~dp1" (
	set AskDest=1
)
if "%BaseDir%"=="%USERPROFILE%\" (set Dest=%~dp1) else set Dest=%BaseDir%
if exist "%~1\*" goto folders
:files
if defined Down goto next
if not exist "%~1" goto next
for /f "delims=*?" %%H in ("%~1") do if not "%%H"=="%~1" (
	set SubPath=%~1.
	call :trim
) else set BN=%~n1
for /f "usebackq delims=" %%F in (`dir "%~1" /a-d /b /oen`) do (
	echo>>"%List%" "%~dp1%%~F" "%%~F"
)
set /a Input+=1
goto next
:folders
set Source=%~dpnx1\
set BN=%~nx1
if defined Down (
	set Root=%Source%
	set DName=
) else (
	set Root=%~dp1
	set DName=%BN%
)
if not defined Flt set Flt=*
cd /d %Source%
if not "%CD%\"=="%Source%" echo "%Source%" not accessible & goto next
set /a Lvl=0%Down%
:level
if not "%CD%"=="%~d1\" (set /a Lvl+=1) & cd .. & goto level
cd %Source%
dir>"%DirDump%" %Flt% /s /a-d /b /oen
for %%I in ("%DirDump%") do if %%~zI==0 goto next
for /F "usebackq tokens=%Lvl%* delims=\" %%I in ("%DirDump%") do (
	echo>>"%List%" "%Root%%%J" "%%J"
)
(set /a Input+=1) & goto next
:next
cd /d %BaseDir%
set /a Ins+=1
shift
if not "%~1"=="" goto multiple
if %Input%==0 echo File not found & goto end
set Colon=%Dest:~-2,1%
if not "%Colon%"==":" set Aname=%Dest:~0,-1%
if %Input% GTR 1 (
	for %%I in ("%Aname%") do if "%%~nxI"=="" (
		set BN=archive
	) else set BN=%%~nxI
)
if defined Out for %%I in ("%Out%") do (
	set Dest=%%~dpI
	set BN=%%~nI
)
if defined AskDest (
	set BN=archive
	call :ask
) else call :test
if defined Stop goto end
if not defined Ddf (
	set DiskLoc=%Dest%
	if exist "%Dest%%BN%.cab" (
		echo %Dest%%BN%.cab already exist!
		echo Please specify name or Enter to overwrite..
		call :askname
	)
) else (
	set DiskLoc=.
	set Inf=%BN%
	set Rpt=%BN%
)
REM Finalize
echo>"%Template%" .Set CabinetNameTemplate=%BN%*.cab
echo>>"%Template%" .Set DiskDirectoryTemplate=%DiskLoc%
echo>>"%Template%" .Set CabinetName1=%BN%.cab
echo>>"%Template%" .Set MaxErrors=1
echo>>"%Template%" .Set %CT%
echo>>"%Template%" .Set %CM%
echo>>"%Template%" .Set MaxDiskSize=%DSize%
echo>>"%Template%" .Set RptFileName=%Rpt%.rpt
echo>>"%Template%" .Set InfFileName=%Inf%.inf
echo>>"%Template%" .Set FolderSizeThreshold=%FSize%
echo.>>"%Template%"
copy /y "%Template%"+"%List%" "%Template%">nul
if defined Ddf (
	echo Saving %BN%.ddf & move /y "%Template%" "%Dest%%BN%.ddf">nul
	goto end
)
::cd /d %TmpDir%
echo CabIt: %Input% of %Ins% input(s)
echo Compressing "%BN%.cab"...
makecab /f "%Template%"
if errorlevel 1 pause & goto end
goto end

:trim
set Tail=%SubPath:~-1,1%
if not "%Tail%"=="" for %%Z in ("%CD%") do (set BN=%%~nxZ) & goto :EOF
if not "%Tail%"=="\" (set SubPath=%SubPath:~0,-1%) & goto trim
set SubPath=%SubPath:~0,-1%
for %%Z in ("%SubPath%") do set BN=%%~nxZ
goto :EOF
:test
echo Checking if "%Dest%" writable...
for %%F in ("%Dest%") do if "%%~dF"=="" goto ask
set Tail=%Dest:~-1,1%
if not "%Tail%"=="\" set Dest=%Dest%\
if exist "%Dest%" (
	copy nul "%Dest%%Test%">nul & if not errorlevel 1 (del "%Dest%%Test%")
	goto :EOF
	) else md "%Dest%" & if not errorlevel 1 goto :EOF
)
:ask
if defined AskDest echo Can't determine how to save output. & set AskDest=
set /p Where=Enter new destination [H]ome [S]pecify [C]ancel?
if /i "%Where%"=="s" set /p Dest=[New Destination] : & goto test
if /i "%Where%"=="h" (set Dest=%USERPROFILE%\) & goto test
if /i "%Where%"=="c" (set Stop=1) & goto :EOF
goto ask
:askname
set /p UName=[New Filename] : 
if not defined UName goto :EOF
set UName=%UName:"=%
for /f "delims=/\:*<>?" %%F in ("%UName%ok") do (
	if /i not "%UName%ok"=="%%F" goto askname
	copy nul "%TmpDir%\%UName%">nul & if errorlevel 1 goto askname
	for %%G in ("%UName%") do (set XT=%%~xG) & (set BN=%%~nG)
	del /q %TmpDir%\%UName%
)
if /i not "%XT%"==".cab" set BN=%BN%%XT%
goto :EOF

:help
echo CabIt v0.2.1 An Interface to Makecab and Expand
echo.2010 by tumagonx. Licensed under GPL 2.1 or later
echo.
echo CLI Usage: cabit [switch] [:filter,filter] foo1 foo2 ... ...
echo Foo can be a folder or file, filter is filename pattern. 
echo Note: empty folder is not supported by cabinet format
echo.
echo Ex.: cabit -o "E:\archive\mydoc.cab" :april?.doc,*.xls D:\Documents
echo Compress matched files in Documents and its subfolders and saved as mydoc.cab
echo.
echo Makecab mode (-s) use *.??_ as ouput instead of .cab. In this mode files that
echo already compressed will be extracted including *.cab file if supported.
echo.
echo Switches:
echo -d Generate DDF only (can be used later with "makecab /f *.ddf")
echo -o Specify output filename. Default is autoguess (N/A with -s) 
echo -v [size] Specify volume split size in integer MB. Default is max:2048
echo -z Hurry Up Mode, appropriate for very large set of files
echo -r [folder] compress all files inside folder. Only folder input(s) accepted
echo -s Single file output mode (filter N/A)
goto :EOF

:end
echo CabIt - Finished
rd /s /q "%TmpDir%">nul 2>&1
del /q "%Inf%.inf">nul 2>&1
del /q "%Rpt%.rpt">nul 2>&1