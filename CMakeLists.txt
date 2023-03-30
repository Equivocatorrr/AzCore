cmake_minimum_required(VERSION 3.25)

set(CMAKE_DEBUG_POSTFIX _debug)

project(AzCore)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT ${CMAKE_INSTALL_PREFIX})
	set(CMAKE_INSTALL_PREFIX $ENV{CMAKE_INSTALL_PREFIX})
endif()

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
	add_compile_options(-Wall -mavx2 -DAZCORE_IO_FOR_VULKAN -g)
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
	if (CMAKE_HOST_SYSTEM MATCHES Windows)
		set(GLSL_VALIDATOR "${VULKAN_SDK}/Bin/glslangValidator.exe")
	elseif (CMAKE_HOST_SYSTEM MATCHES Linux)
		set(GLSL_VALIDATOR "${VULKAN_SDK}/x86_64/bin/glslangValidator")
	endif()
else()
	find_package(Vulkan)
	if (NOT Vulkan_FOUND)
		message(FATAL_ERROR "No Vulkan SDK was found. Try specifying one by setting VULKAN_SDK to the path of your SDK.")
	endif()
	if (NOT EXISTS Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
		find_program(GLSL_VALIDATOR
			NAMES glslangValidator glslangValidator.exe
			HINTS "$ENV{VULKAN_SDK}/x86_64/bin" "$ENV{VULKAN_SDK}/Bin"
		)
		if (GLSL_VALIDATOR STREQUAL "GLSL_VALIDATOR-NOTFOUND")
			message(FATAL_ERROR "Couldn't find glslangValidator, needed for compiling spir-v")
		endif()
	else()
		set(GLSL_VALIDATOR Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
	endif()
	message( STATUS "GLSL_VALIDATOR: ${GLSL_VALIDATOR}" )
endif()

function(VULKAN_SHADERS target SHADER_SOURCE_DIR SHADER_OUT_DIR)
	file(GLOB_RECURSE SHADER_SOURCES
	  "${SHADER_SOURCE_DIR}/*.frag"
	  "${SHADER_SOURCE_DIR}/*.vert"
	)

	foreach(SHADER ${SHADER_SOURCES})
	  get_filename_component(FILENAME ${SHADER} NAME)
	  set(SPIRV "${SHADER_OUT_DIR}/${FILENAME}.spv")
	  add_custom_command(
		  OUTPUT "${SPIRV}"
		#   COMMAND echo "Running custom command: \"${GLSL_VALIDATOR} -V ${SHADER} -o ${SPIRV}\""
		  COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUT_DIR}/"
		  COMMAND "${GLSL_VALIDATOR}" -V ${SHADER} -o ${SPIRV}
		  DEPENDS ${SHADER}
	  )
	  list(APPEND SHADER_BINARIES ${SPIRV})
	endforeach(SHADER)

	set_source_files_properties(${SHADER_BINARIES} PROPERTIES GENERATED TRUE)
	add_custom_target("${target}_Shaders" DEPENDS ${SHADER_BINARIES})
	add_dependencies(${target} "${target}_Shaders")
	# target_sources(${target} PRIVATE ${SHADER_BINARIES})
endfunction()

function(AZ2D_DATA_COPY target OUTPUT_DIR)
	file(GLOB_RECURSE AZ2D_DATA_FILES
		RELATIVE "${PROJECT_SOURCE_DIR}/projects/Az2D/data"
		CONFIGURE_DEPENDS
		"${PROJECT_SOURCE_DIR}/projects/Az2D/data/*"
	)
	# message( STATUS "AZ2D_DATA_FILES: ${AZ2D_DATA_FILES}" )
	foreach(AZ2D_DATA_FILE ${AZ2D_DATA_FILES})
		# message( STATUS "AZ2D_DATA_FILE: ${AZ2D_DATA_FILE}" )
		add_custom_command(OUTPUT "${OUTPUT_DIR}/data/Az2D/${AZ2D_DATA_FILE}"
			COMMAND echo "Copying Az2D data/${AZ2D_DATA_FILE} to ${target}"
			COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/projects/Az2D/data/${AZ2D_DATA_FILE}" "${OUTPUT_DIR}/data/Az2D/${AZ2D_DATA_FILE}"
			DEPENDS "${PROJECT_SOURCE_DIR}/projects/Az2D/data/${AZ2D_DATA_FILE}"
		)
		target_sources(${target} PRIVATE "${OUTPUT_DIR}/data/Az2D/${AZ2D_DATA_FILE}")
	endforeach(AZ2D_DATA_FILE)
endfunction()

add_subdirectory(external/Vulkan-Headers)
find_package(OpenAL)
if(NOT TARGET OpenAL::OpenAL)
	add_subdirectory(external/openal-soft)
endif()

# Actual library
add_subdirectory(base)

# Projects meta-list
add_subdirectory(projects)
