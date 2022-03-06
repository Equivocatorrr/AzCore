cmake_minimum_required(VERSION 3.13)

set(CMAKE_DEBUG_POSTFIX _debug)

project(AzCore)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Wall -mavx2 -DAZCORE_IO_FOR_VULKAN)

if (CMAKE_SYSTEM MATCHES Windows)
	add_compile_options(-D_GLIBCXX_USE_NANOSLEEP)
	add_link_options(-static-libgcc -static-libstdc++ -static -Llibwinpthread-1.dll)
	if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
		message( WARNING "AzCore was designed for x86_64 and is likely to break on ${CMAKE_SYSTEM_PROCESSOR}!")
	endif()
elseif (CMAKE_SYSTEM MATCHES Linux)
	if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
		message( WARNING "AzCore was designed for x86_64 and is likely to break on ${CMAKE_SYSTEM_PROCESSOR}!")
	endif()
else()
	message(FATAL_ERROR "AzCore was made to be built on Linux and Windows. Tried to build for ${CMAKE_SYSTEM}!" )
endif()

set(VULKAN_SDK CACHE STRING "")

if (EXISTS "${VULKAN_SDK}")
	message(STATUS "Using '${VULKAN_SDK}' as the path to the Vulkan SDK")
	add_library(Vulkan INTERFACE)
	if (CMAKE_SYSTEM MATCHES Windows)
		target_link_libraries(Vulkan INTERFACE "${VULKAN_SDK}/Lib/vulkan-1.lib")
	elseif (CMAKE_SYSTEM MATCHES Linux)
		target_link_libraries(Vulkan INTERFACE "${VULKAN_SDK}/x86_64/lib/libvulkan.so")
	endif()
	add_library(Vulkan::Vulkan ALIAS Vulkan)
else()
	find_package(Vulkan)
	if (NOT Vulkan_FOUND)
		message(FATAL_ERROR "No Vulkan SDK was found. Try specifying one by setting VULKAN_SDK to the path of your SDK.")
	endif()
endif()

function(VULKAN_SHADERS target SHADER_SOURCE_DIR SHADER_OUT_DIR)
	file(GLOB_RECURSE SHADER_SOURCES
	  "${SHADER_SOURCE_DIR}/*.frag"
	  "${SHADER_SOURCE_DIR}/*.vert"
	)

	if (CMAKE_HOST_SYSTEM MATCHES Windows)
	  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
	elseif (CMAKE_HOST_SYSTEM MATCHES Linux)
	  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/bin/glslangValidator")
	endif()

	foreach(SHADER ${SHADER_SOURCES})
	  get_filename_component(FILENAME ${SHADER} NAME)
	  set(SPIRV "${SHADER_OUT_DIR}/${FILENAME}.spv")
	  add_custom_command(
		  OUTPUT "${SPIRV}"
		  COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUT_DIR}/"
		  COMMAND "${GLSL_VALIDATOR}" -V ${SHADER} -o ${SPIRV}
		  DEPENDS ${SHADER}
	  )
	  list(APPEND SHADER_BINARIES ${SPIRV})
	endforeach(SHADER)

	add_custom_target(${target}_shaders DEPENDS ${SHADER_BINARIES})
	add_dependencies(${target} ${target}_shaders)
endfunction()

add_subdirectory(external/Vulkan-Headers EXCLUDE_FROM_ALL)
add_subdirectory(external/openal-soft EXCLUDE_FROM_ALL)

# Actual library
add_subdirectory(base)

# Projects meta-list
add_subdirectory(projects)

install(EXPORT AzCore DESTINATION "${CMAKE_INSTALL_LIBDIR}/AzCore")
