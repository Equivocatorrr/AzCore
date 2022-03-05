cmake_minimum_required(VERSION 3.10)

set(CMAKE_DEBUG_POSTFIX _debug)

project(AzCore)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Wall -mavx2 -DAZCORE_IO_FOR_VULKAN)


if (CMAKE_SYSTEM MATCHES Windows)
	if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
		message( WARNING "AzCore was designed for x86_64 and is likely to break on ${CMAKE_SYSTEM_PROCESSOR}!")
	endif()
	add_compile_options(-D_GLIBCXX_USE_NANOSLEEP -static-libgcc -static-libstdc++)
elseif (CMAKE_SYSTEM MATCHES Linux)
	if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
		message( WARNING "AzCore was designed for x86_64 and is likely to break on ${CMAKE_SYSTEM_PROCESSOR}!")
	endif()
else()
	message( FATAL_ERROR "AzCore was made to be built on Linux and Windows. Tried to build for ${CMAKE_SYSTEM}!" )
	# can we error out?
endif()

# find_package(Vulkan REQUIRED FATAL_ERROR)

add_subdirectory(ThirdParty/Vulkan-Headers EXCLUDE_FROM_ALL)

# Actual library
add_subdirectory(base)

# Projects meta-list
add_subdirectory(projects)
