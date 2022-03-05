#!/bin/sh

BuildReleaseL=0
BuildDebugL=0
BuildReleaseW=0
BuildDebugW=0

run_arg=0
run=0
run_debug=0
run_target=""

for arg in "$@"; do
	if [ $run_arg -ne 0 ]; then
		run_target="$run_arg"
		run_arg=0
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
		else
			echo "Usage: build.sh [All|Debug|Release|Linux|Win32|DebugL|ReleaseL|DebugW|ReleaseW]? ([run|run_debug] project_name)?"
			exit 1
		fi
	fi
done


NumThreads=`grep processor /proc/cpuinfo | wc -l`

abort_if_failed()
{
	if [ $? -ne 0 ]; then
		echo "$1 Aborting..."
		exit $?
	fi
}

if [ $BuildDebugL -eq 1 ]
then
	echo "Building Linux Debug"
	mkdir -p buildDebugL
	# rm -rf buildDebugL/*
	cd buildDebugL
	cmake -DCMAKE_BUILD_TYPE=Debug ../
	abort_if_failed "CMake configure failed!"
	cmake --build . -j $NumThreads
	abort_if_failed "CMake build failed!"
	cd ..
fi

if [ $BuildReleaseL -eq 1 ]
then
	echo "Building Linux Release"
	mkdir -p buildReleaseL
	# rm -rf buildReleaseL/*
	cd buildReleaseL
	cmake -DCMAKE_BUILD_TYPE=Release ../
	abort_if_failed "CMake configure failed!"
	cmake --build . -j $NumThreads
	abort_if_failed "CMake build failed!"
	cd ..
fi

if [ $BuildDebugW -eq 1 ]
then
	echo "Building Windows Debug"
	mkdir -p buildDebugW
	rm -rf buildDebugW/*
	cd buildDebugW
	cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-x86_64.cmake ../
	abort_if_failed "CMake configure failed!"
	cmake --build . -j $NumThreads
	abort_if_failed "CMake build failed!"
	cd ..
fi

if [ $BuildReleaseW -eq 1 ]
then
	echo "Building Windows Release"
	mkdir -p buildReleaseW
	# rm -rf buildReleaseW/*
	cd buildReleaseW
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-x86_64.cmake ../
	abort_if_failed "CMake configure failed!"
	cmake --build . -j $NumThreads
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
