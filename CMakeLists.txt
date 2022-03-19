cmake_minimum_required(VERSION 3.13)

set(CMAKE_DEBUG_POSTFIX _debug)

project(AzCore)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (MSVC)
	add_compile_options(/W3 /arch:AVX2 -DAZCORE_IO_FOR_VULKAN)
	# Disable common warnings that we don't care about.
	# C4267: 'var' : conversion from 'size_t' to 'type', possible loss of data
	add_compile_options(/wd4267)
	# C4244: 'argument' : conversion from 'type1' to 'type2', possible loss of data
	add_compile_options(/wd4244)
	# C4996: Your code uses a function, class member, variable, or typedef that's marked deprecated.
	# This one sounds bad, but c'mon... deprecating stdio.h functions is ridiculous...
	add_compile_options(/wd4996)
	# Disable generating a manifest in Visual Studio since we provide our own
	add_link_options(/MANIFEST:NO)
else()
	add_compile_options(-Wall -mavx2 -DAZCORE_IO_FOR_VULKAN)
	if (CMAKE_SYSTEM MATCHES Windows)
		add_compile_options(-D_GLIBCXX_USE_NANOSLEEP)
		add_link_options(-static-libgcc -static-libstdc++ -static -Llibwinpthread-1.dll)
	endif()
endif()

if (CMAKE_SYSTEM MATCHES Windows)
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

	if (NOT EXISTS "${VULKAN_SDK}")
		if (NOT EXISTS Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
			message(FATAL_ERROR "Compiling shaders requires either the Vulkan SDK or Vulkan::glslc, neither of which are present.")
		else()
			set(GLSL_VALIDATOR Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
		endif()
	else()
		if (CMAKE_HOST_SYSTEM MATCHES Windows)
			set(GLSL_VALIDATOR "${VULKAN_SDK}/Bin/glslangValidator.exe")
		elseif (CMAKE_HOST_SYSTEM MATCHES Linux)
			set(GLSL_VALIDATOR "${VULKAN_SDK}/bin/glslangValidator")
		endif()
	endif()

	foreach(SHADER ${SHADER_SOURCES})
	  get_filename_component(FILENAME ${SHADER} NAME)
	  set(SPIRV "${SHADER_OUT_DIR}/${FILENAME}.spv")
	  add_custom_command(
		  OUTPUT "${SPIRV}"
		  COMMAND echo "Running custom command: \"${GLSL_VALIDATOR} -V ${SHADER} -o ${SPIRV}\""
		  COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUT_DIR}/"
		  COMMAND "${GLSL_VALIDATOR}" -V ${SHADER} -o ${SPIRV}
		  DEPENDS ${SHADER}
	  )
	  list(APPEND SHADER_BINARIES ${SPIRV})
	endforeach(SHADER)

	set_source_files_properties(${SHADER_BINARIES} PROPERTIES GENERATED TRUE)
	# add_custom_target(${target}_shaders DEPENDS ${SHADER_BINARIES})
	# add_dependencies(${target} ${target}_shaders)
	target_sources(${target} PRIVATE ${SHADER_BINARIES})
endfunction()

add_subdirectory(external/Vulkan-Headers EXCLUDE_FROM_ALL)
add_subdirectory(external/openal-soft EXCLUDE_FROM_ALL)

# Actual library
add_subdirectory(base)

# Projects meta-list
add_subdirectory(projects)

install(EXPORT AzCore DESTINATION "${CMAKE_INSTALL_LIBDIR}/AzCore")
