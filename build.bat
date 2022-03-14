@echo off

setlocal enabledelayedexpansion

set /a BuildDebugL=0
set /a BuildReleaseL=0
set /a BuildDebugW=0
set /a BuildReleaseW=0
set /a has_args=0
set /a run_arg=0
set /a run=0
set /a run_debug=0
set run_target=""
set /a clean=0
set /a install=0
set trace=""
set verbose=""
set vulkan_sdk_win32=""
set vulkan_sdk_linux=""
set /a vulkan_sdk_win32_arg=0
set /a vulkan_sdk_linux_arg=0

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
if %vulkan_sdk_linux_arg% == 1 (
	set vulkan_sdk_linux="-DVULKAN_SDK"%arg%
	set /a vulkan_sdk_linux_arg=0
	goto Done
)
if %vulkan_sdk_win32_arg% == 1 (
	set vulkan_sdk_win32="-DVULKAN_SDK"%arg%
	set /a vulkan_sdk_win32_arg=0
	goto Done
)
set arg=Arg%arg%
goto %arg%
goto Arg
:ArgAll
	set /a BuildDebugL=1
	set /a BuildReleaseL=1
	set /a BuildDebugW=1
	set /a BuildReleaseW=1
	goto Done
:ArgRelease
	set /a BuildReleaseL=1
	set /a BuildReleaseW=1
	goto Done
:ArgDebug
	set /a BuildDebugL=1
	set /a BuildDebugW=1
	goto Done
:ArgLinux
	set /a BuildDebugL=1
	set /a BuildReleaseL=1
	goto Done
:ArgWin32
	set /a BuildDebugW=1
	set /a BuildReleaseW=1
	goto Done
:ArgDebugL
	set /a BuildDebugL=1
	goto Done
:ArgReleaseL
	set /a BuildReleaseL=1
	goto Done
:ArgDebugW
	set /a BuildDebugW=1
	goto Done
:ArgReleaseW
	set /a BuildReleaseW=1
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
:ArgLINUX_VULKAN_SDK
	set /a vulkan_sdk_linux_arg=1
	goto Done
:ArgWIN32_VULKAN_SDK
	set /a vulkan_sdk_win32_arg=1
	goto Done
:Argclean
	set /a clean=1
	goto Done
:Argtrace
	set trace="--trace"
	goto Done
:Argverbose
	set verbose="--verbose"
	goto Done
:Arginstall
	set /a install=1
	goto Done
:Arg
:Arg%arg%
	echo "Usage: build.sh [clean]? [verbose]? [trace]? [install]? (WIN32_VULKAN_SDK path_to_sdk)? (LINUX_VULKAN_SDK path_to_sdk)? [All|Debug|Release|Linux|Win32|DebugL|ReleaseL|DebugW|ReleaseW]? ([run|run_debug] project_name)?"
	goto EndOfScript
:Done
set /a argIndex+=1
if %argIndex% neq %argCount% goto loopStart


if %clean% == 1 (
	rm -rf projects/*/bin projects/*/*.log buildDebugL buildReleaseL buildDebugW buildReleaseW
)

if %BuildDebugL% == 1 (
	echo "Building Linux Debug"
	md buildDebugL
	cd buildDebugL
	cmake %trace~% %vulkan_sdk_linux~% -DCMAKE_BUILD_TYPE=Debug ..
	if ERRORLEVEL 1 (
		echo "CMake configure failed! Aborting..."
		goto EndOfScript
	)
	cmake --build . %verbose~% -j %number_of_processors%
	if ERRORLEVEL 1 (
		echo "CMake build failed! Aborting..."
		goto EndOfScript
	)
	cd ..
)

if %BuildReleaseL% == 1 (
	echo "Building Linux Release"
	md buildReleaseL
	cd buildReleaseL
	cmake %trace~% %vulkan_sdk_linux~% -DCMAKE_BUILD_TYPE=Release ..
	if ERRORLEVEL 1 (
		echo "CMake configure failed! Aborting..."
		goto EndOfScript
	)
	cmake --build . %verbose~% -j %number_of_processors%
	if ERRORLEVEL 1 (
		echo "CMake build failed! Aborting..."
		goto EndOfScript
	)
	cd ..
)

if %BuildDebugW% == 1 (
	echo "Building Win32 Debug"
	md buildDebugW
	cd buildDebugW
	cmake %trace~% %vulkan_sdk_win32~% -DCMAKE_BUILD_TYPE=Debug ..
	if ERRORLEVEL 1 (
		echo "CMake configure failed! Aborting..."
		goto EndOfScript
	)
	cmake --build . %verbose~% -j %number_of_processors%
	if ERRORLEVEL 1 (
		echo "CMake build failed! Aborting..."
		goto EndOfScript
	)
	cd ..
)

if %BuildReleaseW% == 1 (
	echo "Building Win32 Release"
	md buildReleaseW
	cd buildReleaseW
	cmake %trace~% %vulkan_sdk_win32~% -DCMAKE_BUILD_TYPE=Release ..
	if ERRORLEVEL 1 (
		echo "CMake configure failed! Aborting..."
		goto EndOfScript
	)
	cmake --build . %verbose~% -j %number_of_processors%
	if ERRORLEVEL 1 (
		echo "CMake build failed! Aborting..."
		goto EndOfScript
	)
	cd ..
)

echo "All builds complete!"

:EndOfScript
