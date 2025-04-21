# check target platform
if (CMAKE_SYSTEM MATCHES Windows)
	set(TARGET_PLATFORM_NAME Win32)
elseif (CMAKE_SYSTEM MATCHES Linux)
	set(TARGET_PLATFORM_NAME Linux)
endif()

add_library(AzCore STATIC
	# dwarf
		src/AzCore/dwarf/definitions.cpp
		src/AzCore/dwarf/definitions.hpp
		src/AzCore/dwarf/dwarf.cpp
		src/AzCore/dwarf/dwarf.hpp
	# elf
		src/AzCore/elf/definitions.cpp
		src/AzCore/elf/definitions.hpp
		src/AzCore/elf/elf.cpp
		src/AzCore/elf/elf.hpp
	# Font
		src/AzCore/Font/Font.cpp
		src/AzCore/Font/Font.hpp
		src/AzCore/Font/CFF.cpp
		src/AzCore/Font/CFF.hpp
		src/AzCore/Font/font_cff_std_data.c
		src/AzCore/Font/FontTables.cpp
		src/AzCore/Font/FontTables.hpp
	# IO
		src/AzCore/IO/io.cpp
		src/AzCore/IO/io.hpp
		src/AzCore/IO/${TARGET_PLATFORM_NAME}/RawInput.cpp
		src/AzCore/IO/${TARGET_PLATFORM_NAME}/Window.cpp
		src/AzCore/IO/${TARGET_PLATFORM_NAME}/WindowData.hpp
		src/AzCore/IO/ButtonState.cpp
		src/AzCore/IO/ButtonState.hpp
		src/AzCore/IO/FileManager.cpp
		src/AzCore/IO/FileManager.hpp
		src/AzCore/IO/Gamepad.cpp
		src/AzCore/IO/Gamepad.hpp
		src/AzCore/IO/Input.cpp
		src/AzCore/IO/Input.hpp
		src/AzCore/IO/KeyCodes.cpp
		src/AzCore/IO/KeyCodes.hpp
		src/AzCore/IO/Log.cpp
		src/AzCore/IO/Log.hpp
		src/AzCore/IO/RawInput.cpp
		src/AzCore/IO/RawInput.hpp
		src/AzCore/IO/vt_strings.hpp
		src/AzCore/IO/Window.hpp
	# Math
		src/AzCore/Math/Math.hpp
		src/AzCore/Math/Angle.cpp
		src/AzCore/Math/Angle.hpp
		src/AzCore/Math/Basic.cpp
		src/AzCore/Math/Basic.hpp
		src/AzCore/Math/BigInt.cpp
		src/AzCore/Math/BigInt.hpp
		src/AzCore/Math/Color.cpp
		src/AzCore/Math/Color.hpp
		src/AzCore/Math/complex_t.hpp
		src/AzCore/Math/Equations.hpp
		src/AzCore/Math/FastApprox.hpp
		src/AzCore/Math/mat2_t.hpp
		src/AzCore/Math/mat3_t.hpp
		src/AzCore/Math/mat4_t.hpp
		src/AzCore/Math/mat4_t.cpp
		src/AzCore/Math/mat5_t.hpp
		src/AzCore/Math/Matrix.hpp
		src/AzCore/Math/Noise.cpp
		src/AzCore/Math/Noise.hpp
		src/AzCore/Math/quat_t.hpp
		src/AzCore/Math/RandomNumberGenerator.cpp
		src/AzCore/Math/RandomNumberGenerator.hpp
		src/AzCore/Math/vec2_t.hpp
		src/AzCore/Math/vec3_t.hpp
		src/AzCore/Math/vec4_t.hpp
		src/AzCore/Math/vec5_t.hpp
	# Memory
		src/AzCore/Memory/Memory.hpp
		src/AzCore/Memory/TemplateForwardDeclares.hpp
		src/AzCore/Memory/Any.hpp
		src/AzCore/Memory/Array.hpp
		src/AzCore/Memory/ArrayList.hpp
		src/AzCore/Memory/ArrayWithBucket.hpp
		src/AzCore/Memory/BinaryMap.hpp
		src/AzCore/Memory/BinarySet.hpp
		src/AzCore/Memory/StaticArray.hpp
		src/AzCore/Memory/HashMap.hpp
		src/AzCore/Memory/HashSet.hpp
		src/AzCore/Memory/List.hpp
		src/AzCore/Memory/None.hpp
		src/AzCore/Memory/Optional.hpp
		src/AzCore/Memory/Ptr.hpp
		src/AzCore/Memory/Range.hpp
		src/AzCore/Memory/String.cpp
		src/AzCore/Memory/String.hpp
		src/AzCore/Memory/StringArena.cpp
		src/AzCore/Memory/StringArena.hpp
		src/AzCore/Memory/StringCommon.hpp
		src/AzCore/Memory/UniquePtr.hpp
	# Net
		src/AzCore/Net/${TARGET_PLATFORM_NAME}/Sockets.cpp
		src/AzCore/Net/Sockets.cpp
		src/AzCore/Net/Sockets.hpp
	# SIMD
		src/AzCore/SIMD/Simd.hpp
		src/AzCore/SIMD/SimdAVX.hpp
		src/AzCore/SIMD/SimdSSE2.hpp
	# Utility
		src/AzCore/Utility/ByteHash.hpp
		src/AzCore/Utility/cli.cpp
		src/AzCore/Utility/cli.hpp
		src/AzCore/Utility/Endian.cpp
		src/AzCore/Utility/Endian.hpp
		src/AzCore/Utility/Environment.cpp
		src/AzCore/Utility/Environment.hpp
		src/AzCore/Utility/IndexHash.hpp
		src/AzCore/Utility/Memory.cpp
		src/AzCore/Utility/Memory.hpp
		src/AzCore/Utility/Profiling.cpp
		src/AzCore/Utility/Profiling.hpp
		src/AzCore/Utility/RAIIHacks.hpp
		src/AzCore/Utility/Sort.hpp
		src/AzCore/Utility/Template.hpp
		src/AzCore/Utility/Time.cpp
		src/AzCore/Utility/Time.hpp
		src/AzCore/Utility/TypeHash.hpp
		src/AzCore/Utility/TypeName.hpp
		src/AzCore/Utility/WindowsHeaderPredefines.h
		src/AzCore/Utility/Windows.h
		src/AzCore/Utility/WindowsHeaderCleanup.h
	src/AzCore/BasicTypes.hpp
	src/AzCore/Assert.hpp
	src/AzCore/Assert.cpp
	src/AzCore/gpu.cpp
	src/AzCore/gpu.hpp
	src/AzCore/gui.cpp
	src/AzCore/gui.hpp
	src/AzCore/Image.cpp
	src/AzCore/Image.hpp
	src/AzCore/SoftwareRendering.cpp
	src/AzCore/SoftwareRendering.hpp
	src/AzCore/Thread.hpp
	src/AzCore/Thread.cpp
	src/AzCore/Thread${TARGET_PLATFORM_NAME}.cpp
	src/AzCore/vk.cpp
	src/AzCore/vk.hpp

	src/resources.h
	src/resources.rc
	src/manifest.xml.manifest
)

set_source_files_properties(
	src/AzCore/Thread${TARGET_PLATFORM_NAME}.cpp
	PROPERTIES HEADER_FILE_ONLY TRUE
)

if (CMAKE_SYSTEM MATCHES Linux)
	target_sources(AzCore PRIVATE
		src/AzCore/IO/Linux/XCB.cpp
		src/AzCore/IO/Linux/Wayland.cpp
		src/AzCore/IO/Linux/WaylandProtocols/xdg-shell.c
		src/AzCore/IO/Linux/WaylandProtocols/xdg-shell.h
		src/AzCore/IO/Linux/WaylandProtocols/pointer-constraints-unstable-v1.c
		src/AzCore/IO/Linux/WaylandProtocols/pointer-constraints-unstable-v1.h
		src/AzCore/IO/Linux/WaylandProtocols/relative-pointer-manager-unstable-v1.c
		src/AzCore/IO/Linux/WaylandProtocols/relative-pointer-manager-unstable-v1.h
	)
	set_source_files_properties(
		src/AzCore/IO/Linux/Wayland.cpp
		src/AzCore/IO/Linux/XCB.cpp
		PROPERTIES HEADER_FILE_ONLY TRUE
	)
endif()

target_include_directories(AzCore PRIVATE "${PROJECT_SOURCE_DIR}/external/Vulkan-Headers/include")
target_include_directories(AzCore PRIVATE "${PROJECT_SOURCE_DIR}/external/stb")
target_include_directories(AzCore PUBLIC
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/base/src>
	$<INSTALL_INTERFACE:include>
)

set_target_properties(AzCore PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

# these are all included in another source file and get compiled that way
set_source_files_properties(
	src/AzCore/Font/CFF.cpp
	src/AzCore/Font/font_cff_std_data.c
	src/AzCore/Font/FontTables.cpp
	src/AzCore/IO/ButtonState.cpp
	src/AzCore/IO/Gamepad.cpp
	src/AzCore/IO/Input.cpp
	src/AzCore/IO/RawInput.cpp
	src/AzCore/IO/${TARGET_PLATFORM_NAME}/RawInput.cpp
	src/AzCore/IO/${TARGET_PLATFORM_NAME}/Window.cpp
	src/AzCore/Net/${TARGET_PLATFORM_NAME}/Sockets.cpp
	PROPERTIES HEADER_FILE_ONLY TRUE
)

set_property(TARGET AzCore PROPERTY RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/base/lib")

# Link Net libraries
add_library(AzCore_Net INTERFACE)
target_link_libraries(AzCore_Net INTERFACE AzCore)

# Link Window and RawInput libraries
add_library(AzCore_IO INTERFACE)
target_link_libraries(AzCore_IO INTERFACE AzCore)

# Link Vulkan libraries
add_library(AzCore_Vulkan INTERFACE)
target_link_libraries(AzCore_Vulkan INTERFACE AzCore_IO Vulkan::Vulkan)

# Link Software Renderer libraries
add_library(AzCore_SoftwareRenderer INTERFACE)
target_link_libraries(AzCore_SoftwareRenderer INTERFACE AzCore_IO)

if (CMAKE_SYSTEM MATCHES Windows)
	target_link_libraries(AzCore INTERFACE winmm psapi dbghelp)
	target_link_libraries(AzCore_Net INTERFACE ws2_32)
	target_link_libraries(AzCore_IO INTERFACE gdi32 dinput8 dxguid)
elseif (CMAKE_SYSTEM MATCHES Linux)
	target_link_libraries(AzCore INTERFACE pthread)
	target_link_libraries(AzCore_IO INTERFACE xcb xcb-randr xkbcommon xkbcommon-x11 xcb-xkb wayland-client wayland-cursor)
	target_link_libraries(AzCore_SoftwareRenderer INTERFACE xcb-shm xcb-image)
endif()

install(TARGETS AzCore AzCore_Net AzCore_IO AzCore_Vulkan AzCore_SoftwareRenderer
	EXPORT AzCore
	CONFIGURATIONS Debug Release RelWithDebInfo
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/AzCore"
	INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/AzCore"
)
install(EXPORT AzCore
	FILE AzCore.cmake
	NAMESPACE AzCore::
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/AzCore"
)
if (NOT Vulkan_FOUND)
	install(TARGETS Vulkan
		EXPORT AzCore
		CONFIGURATIONS Debug Release
	)
endif()
install(FILES AzCoreConfig.cmake DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/AzCore")
install(DIRECTORY "src/AzCore"
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	FILES_MATCHING
	PATTERN "*.hpp"
	PATTERN "*.h"
)

file(GLOB_RECURSE SOURCE_LIST
	"${CMAKE_CURRENT_SOURCE_DIR}*.[hc](pp)?"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES SOURCE_LIST)