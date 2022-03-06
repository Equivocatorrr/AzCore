# check target platform
if (CMAKE_SYSTEM MATCHES Windows)
	set(TARGET_PLATFORM_NAME Win32)
elseif (CMAKE_SYSTEM MATCHES Linux)
	set(TARGET_PLATFORM_NAME Linux)
endif()

add_library(AzCore STATIC
	src/resources.h
	src/resources.rc
	src/manifest.xml
	src/AzCore/basictypes.hpp
	src/AzCore/common.hpp
	src/AzCore/Environment.cpp
	src/AzCore/Environment.hpp
	src/AzCore/font.cpp
	src/AzCore/font.hpp
		src/AzCore/Font/CFF.cpp
		src/AzCore/Font/CFF.hpp
		src/AzCore/Font/font_cff_std_data.c
		src/AzCore/Font/FontTables.cpp
		src/AzCore/Font/FontTables.hpp
	src/AzCore/Image.cpp
	src/AzCore/Image.hpp
	src/AzCore/io.cpp
	src/AzCore/io.hpp
		src/AzCore/IO/${TARGET_PLATFORM_NAME}/RawInput.cpp
		src/AzCore/IO/${TARGET_PLATFORM_NAME}/Window.cpp
		src/AzCore/IO/${TARGET_PLATFORM_NAME}/WindowData.hpp
		src/AzCore/IO/ButtonState.cpp
		src/AzCore/IO/ButtonState.hpp
		src/AzCore/IO/Gamepad.cpp
		src/AzCore/IO/Gamepad.hpp
		src/AzCore/IO/Input.cpp
		src/AzCore/IO/Input.hpp
		src/AzCore/IO/Log.cpp
		src/AzCore/IO/Log.hpp
		src/AzCore/IO/LogStream.cpp
		src/AzCore/IO/LogStream.hpp
		src/AzCore/IO/RawInput.cpp
		src/AzCore/IO/RawInput.hpp
		src/AzCore/IO/Window.hpp
	src/AzCore/keycodes.cpp
	src/AzCore/keycodes.hpp
	src/AzCore/math.cpp
	src/AzCore/math.hpp
		src/AzCore/Math/Angle.cpp
		src/AzCore/Math/Angle.hpp
		src/AzCore/Math/basic.cpp
		src/AzCore/Math/basic.hpp
		src/AzCore/Math/complex_t.hpp
		src/AzCore/Math/Equations.hpp
		src/AzCore/Math/mat2_t.hpp
		src/AzCore/Math/mat3_t.hpp
		src/AzCore/Math/mat4_t.hpp
		src/AzCore/Math/mat5_t.hpp
		src/AzCore/Math/quat_t.hpp
		src/AzCore/Math/RandomNumberGenerator.cpp
		src/AzCore/Math/RandomNumberGenerator.hpp
		src/AzCore/Math/vec2_t.hpp
		src/AzCore/Math/vec3_t.hpp
		src/AzCore/Math/vec4_t.hpp
		src/AzCore/Math/vec5_t.hpp
	src/AzCore/memory.cpp
	src/AzCore/memory.hpp
		src/AzCore/Memory/Array.hpp
		src/AzCore/Memory/ArrayList.hpp
		src/AzCore/Memory/ArrayWithBucket.hpp
		src/AzCore/Memory/BigInt.cpp
		src/AzCore/Memory/BigInt.hpp
		src/AzCore/Memory/BinaryMap.hpp
		src/AzCore/Memory/BinarySet.hpp
		src/AzCore/Memory/BucketArray.hpp
		src/AzCore/Memory/Endian.cpp
		src/AzCore/Memory/Endian.hpp
		src/AzCore/Memory/HashMap.hpp
		src/AzCore/Memory/HashSet.hpp
		src/AzCore/Memory/IndexHash.hpp
		src/AzCore/Memory/List.hpp
		src/AzCore/Memory/Ptr.hpp
		src/AzCore/Memory/Range.hpp
		src/AzCore/Memory/String.cpp
		src/AzCore/Memory/String.hpp
		src/AzCore/Memory/StringCommon.hpp
		src/AzCore/Memory/UniquePtr.hpp
	src/AzCore/Simd.hpp
		src/AzCore/SIMD/SimdAVX.hpp
		src/AzCore/SIMD/SimdSSE2.hpp
	# Net
		src/AzCore/Net/${TARGET_PLATFORM_NAME}/Sockets.cpp
		src/AzCore/Net/Sockets.cpp
		src/AzCore/Net/Sockets.hpp
	src/AzCore/SoftwareRendering.cpp
	src/AzCore/SoftwareRendering.hpp
	src/AzCore/Thread.hpp
	src/AzCore/Thread${TARGET_PLATFORM_NAME}.hpp
	src/AzCore/Time.hpp
	src/AzCore/vk.cpp
	src/AzCore/vk.hpp
)

# file(GLOB AZCORE_HEADER_FILES
	# "src/AzCore/*.hpp"
	# "src/AzCore/*/*.hpp"
# )

# message(STATUS "List of headers we want to install: ${AZCORE_HEADER_FILES}")

# set_target_properties(AzCore PROPERTIES PUBLIC_HEADER "${AZCORE_HEADER_FILES}")

install(TARGETS AzCore
	EXPORT AzCore
	CONFIGURATIONS Release
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" # is destination required?
)
install(FILES AzCore-config.cmake DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY "src/AzCore"
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	FILES_MATCHING PATTERN "*.hpp"
)

target_include_directories(AzCore PRIVATE "${PROJECT_SOURCE_DIR}/ThirdParty/Vulkan-Headers/include")
target_include_directories(AzCore PRIVATE "${PROJECT_SOURCE_DIR}/ThirdParty/stb")

set_target_properties(AzCore PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

# these are all included in another source file and get compiled that way
set_source_files_properties(src/AzCore/Font/CFF.cpp PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(src/AzCore/Font/font_cff_std_data.c PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(src/AzCore/Font/FontTables.cpp PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(src/AzCore/IO/ButtonState.cpp PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(src/AzCore/IO/Gamepad.cpp PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(src/AzCore/IO/Input.cpp PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(src/AzCore/IO/RawInput.cpp PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(src/AzCore/IO/${TARGET_PLATFORM_NAME}/RawInput.cpp PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(src/AzCore/Net/${TARGET_PLATFORM_NAME}/Sockets.cpp PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(src/AzCore/Math/Angle.cpp PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(src/AzCore/Math/RandomNumberGenerator.cpp PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(src/AzCore/Memory/String.cpp PROPERTIES HEADER_FILE_ONLY TRUE)

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
	target_link_libraries(AzCore_Net INTERFACE ws2_32)
	target_link_libraries(AzCore_IO INTERFACE gdi32 dinput8 dxguid)
elseif (CMAKE_SYSTEM MATCHES Linux)
	target_link_libraries(AzCore INTERFACE pthread)
	target_link_libraries(AzCore_IO INTERFACE xcb xkbcommon xkbcommon-x11 xcb-xkb)
	target_link_libraries(AzCore_SoftwareRenderer INTERFACE xcb-shm xcb-image)
endif()
