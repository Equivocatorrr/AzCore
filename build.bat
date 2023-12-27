@echo off

setlocal enabledelayedexpansion

set /a BuildDebug=0
set /a BuildRelease=0
set /a has_args=0
set /a run_arg=0
set /a run=0
set /a run_debug=0
set run_target=
set /a clean=0
set /a install=0
set trace=
set verbose=
set user_vulkan_sdk=
set /a vulkan_sdk_arg=0

set /a argCount=0
for %%a in (%*) do (
	set "argValues[!argCount!]=%%~a"
	set /a argCount+=1
)

set /a argIndex=0
:loopStart
set arg=!argValues[%argIndex%]!
if %run_arg% == 1 (
	set run_target=%arg%
	set /a run_arg=0
	goto Done
)
if %vulkan_sdk_arg% == 1 (
	set user_vulkan_sdk=-DUSER_VULKAN_SDK=%arg%
	set /a vulkan_sdk_arg=0
	goto Done
)
set arg=Arg%arg%
goto %arg%
goto Arg
:ArgAll
	set /a BuildDebug=1
	set /a BuildRelease=1
	goto Done
:ArgRelease
	set /a BuildRelease=1
	goto Done
:ArgDebug
	set /a BuildDebug=1
	goto Done
:Argrun
	set /a run_arg=1
	set /a run=1
	set /a run_debug=0
	goto Done
:Argrun_debug
	set /a run_arg=1
	set /a run=0
	set /a run_debug=1
	goto Done
:ArgUSER_VULKAN_SDK
	set /a vulkan_sdk_arg=1
	goto Done
:Argclean
	set /a clean=1
	goto Done
:Argtrace
	set trace=--trace
	goto Done
:Argverbose
	set verbose=--verbose
	goto Done
:Arginstall
	set /a install=1
	goto Done
:Arg
:Arg%arg%
	echo "Usage: build.bat [clean]? [verbose]? [trace]? [install]? (USER_VULKAN_SDK path_to_sdk)? [All|Debug|Release]? ([run|run_debug] project_name)?"
	goto EndOfScript
:Done
set /a argIndex+=1
if %argIndex% neq %argCount% goto loopStart


if %clean% == 1 (
	for /f %%i in ('dir /a:d /b "projects\*"') do rd /s /q projects\%%i\bin
	rd /s /q build
	@REM rd /s /q projects/*/bin projects/*/*.log build
)

if %BuildDebug% == 1 (
	echo "Building Win32 Debug"
	md build
	cd build
	cmake %trace% %user_vulkan_sdk% ..
	if ERRORLEVEL 1 (
		echo "CMake configure failed! Aborting..."
		goto EndOfScript
	)
	cmake --build . %verbose% -j %number_of_processors% --config Debug
	if ERRORLEVEL 1 (
		echo "CMake build failed! Aborting..."
		goto EndOfScript
	)
	if %install% == 1 (
		cmake --install . %verbose% --config Debug
	)
	cd ..
)

if %BuildRelease% == 1 (
	echo "Building Win32 Release"
	md build
	cd build
	cmake %trace% %user_vulkan_sdk% ..
	if ERRORLEVEL 1 (
		echo "CMake configure failed! Aborting..."
		goto EndOfScript
	)
	cmake --build . %verbose% -j %number_of_processors% --config Release
	if ERRORLEVEL 1 (
		echo "CMake build failed! Aborting..."
		goto EndOfScript
	)
	if %install% == 1 (
		cmake --install . %verbose% --config Release
	)
	cd ..
)

if %run% == 1 (
	cd projects\%run_target%
	%~dp0projects\%run_target%\bin\Release\%run_target%.exe
	cd ..\..
)

if %run_debug% == 1 (
	cd projects\%run_target%
	%~dp0projects\%run_target%\bin\Debug\%run_target%_debug.exe
	cd ..\..
)
if %errorlevel% NEQ 0 (
	echo "Failed with code " %errorlevel%
	goto EndOfScript
)

echo "All builds complete!"

:EndOfScript
