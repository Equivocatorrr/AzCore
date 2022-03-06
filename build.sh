#!/bin/sh

BuildReleaseL=0
BuildDebugL=0
BuildReleaseW=0
BuildDebugW=0

has_args=0
run_arg=0
run=0
run_debug=0
run_target=""
clean=0
install=0
trace=""
verbose=""
vulkan_sdk_win32=""
vulkan_sdk_linux=""
vulkan_sdk_win32_arg=0
vulkan_sdk_linux_arg=0

if [ $WIN32_VULKAN_SDK != "" ]; then
	vulkan_sdk_win32="-DVULKAN_SDK=$WIN32_VULKAN_SDK"
fi
if [ $LINUX_VULKAN_SDK != "" ]; then
	vulkan_sdk_linux="-DVULKAN_SDK=$LINUX_VULKAN_SDK"
fi

usage()
{
	echo "Usage: build.sh [clean]? [verbose]? [trace]? (WIN32_VULKAN_SDK path_to_sdk)? (LINUX_VULKAN_SDK path_to_sdk)? [All|Debug|Release|Linux|Win32|DebugL|ReleaseL|DebugW|ReleaseW]? ([run|run_debug] project_name)?"
	exit 1
}

for arg in "$@"; do
	has_args=1
	if [ $run_arg -ne 0 ]; then
		run_target="$arg"
		run_arg=0
	elif [ $vulkan_sdk_linux_arg -ne 0 ]; then
		vulkan_sdk_linux="-DVULKAN_SDK=$arg"
		vulkan_sdk_linux_arg=0
	elif [ $vulkan_sdk_win32_arg -ne 0 ]; then
		vulkan_sdk_win32="-DVULKAN_SDK=$arg"
		vulkan_sdk_win32_arg=0
	else
		if [ "$arg" = "All" ]
		then
			BuildReleaseL=1
			BuildDebugL=1
			BuildReleaseW=1
			BuildDebugW=1
		elif [ "$arg" = "Release" ]
		then
			BuildReleaseL=1
			BuildReleaseW=1
		elif [ "$arg" = "Debug" ]
		then
			BuildDebugL=1
			BuildDebugW=1
		elif [ "$arg" = "Linux" ]
		then
			BuildReleaseL=1
			BuildDebugL=1
		elif [ "$arg" = "Win32" ]
		then
			BuildReleaseW=1
			BuildDebugW=1
		elif [ "$arg" = "ReleaseL" ]
		then
			BuildReleaseL=1
		elif [ "$arg" = "DebugL" ]
		then
			BuildDebugL=1
		elif [ "$arg" = "ReleaseW" ]
		then
			BuildReleaseW=1
		elif [ "$arg" = "DebugW" ]
		then
			BuildDebugW=1
		elif [ "$arg" = "run" ]
		then
			run_arg=1
			run=1
			run_debug=0
		elif [ "$arg" = "run_debug" ]
		then
			run_arg=1
			run=0
			run_debug=1
		elif [ "$arg" = "LINUX_VULKAN_SDK" ]
		then
			vulkan_sdk_linux_arg=1
		elif [ "$arg" = "WIN32_VULKAN_SDK" ]
		then
			vulkan_sdk_win32_arg=1
		elif [ "$arg" = "clean" ]
		then
			clean=1
		elif [ "$arg" = "trace" ]
		then
			trace="--trace"
		elif [ "$arg" = "verbose" ]
		then
			verbose="--verbose"
		elif [ "$arg" = "install" ]
		then
			install=1
		else
			usage
		fi
	fi
done

if [ $has_args -eq 0 ]; then
	usage
fi

NumThreads=`grep processor /proc/cpuinfo | wc -l`

abort_if_failed()
{
	if [ $? -ne 0 ]; then
		echo "$1 Aborting..."
		exit $?
	fi
}

if [ $clean -ne 0 ]; then
	rm -rf projects/*/bin projects/*/*.log
fi

if [ $BuildDebugL -eq 1 ]
then
	echo "Building Linux Debug"
	mkdir -p buildDebugL
	if [ $clean -ne 0 ]; then
		rm -rf buildDebugL/*
	fi
	cd buildDebugL
	cmake $trace $vulkan_sdk_linux -DCMAKE_BUILD_TYPE=Debug ../
	abort_if_failed "CMake configure failed!"
	cmake --build . $verbose -j $NumThreads
	abort_if_failed "CMake build failed!"
	cd ..
fi

if [ $BuildReleaseL -eq 1 ]
then
	echo "Building Linux Release"
	mkdir -p buildReleaseL
	if [ $clean -ne 0 ]; then
		rm -rf buildReleaseL/*
	fi
	cd buildReleaseL
	cmake $trace $vulkan_sdk_linux -DCMAKE_BUILD_TYPE=Release ../
	abort_if_failed "CMake configure failed!"
	cmake --build . $verbose -j $NumThreads
	abort_if_failed "CMake build failed!"
	if [ $install -ne 0 ]; then
		echo "To install, you need root access:"
		sudo cmake --install . $verbose
	fi
	cd ..
fi

if [ $BuildDebugW -eq 1 ]
then
	echo "Building Windows Debug"
	mkdir -p buildDebugW
	if [ $clean -ne 0 ]; then
		rm -rf buildDebugW/*
	fi
	cd buildDebugW
	cmake $trace $vulkan_sdk_win32 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-x86_64.cmake ../
	abort_if_failed "CMake configure failed!"
	cmake --build . $verbose -j $NumThreads
	abort_if_failed "CMake build failed!"
	cd ..
fi

if [ $BuildReleaseW -eq 1 ]
then
	echo "Building Windows Release"
	mkdir -p buildReleaseW
	if [ $clean -ne 0 ]; then
		rm -rf buildReleaseW/*
	fi
	cd buildReleaseW
	cmake $trace $vulkan_sdk_win32 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-x86_64.cmake ../
	abort_if_failed "CMake configure failed!"
	cmake --build . $verbose -j $NumThreads
	abort_if_failed "CMake build failed!"
	cd ..
fi

echo "All builds complete!"

if [ $run -ne 0 ]; then
	cd "projects/$run_target"
	bin/$run_target
elif [ $run_debug -ne 0 ]; then
	cd "projects/$run_target"
	bin/${run_target}_debug
fi
