add_executable(Az2D_Example
	src/main.cpp
	src/entities.cpp
	src/entities.hpp
	src/gui.cpp
	src/gui.hpp
	src/resources.h
	src/resources.rc
	"${PROJECT_SOURCE_DIR}/base/src/manifest.xml.manifest"
)

target_include_directories(Az2D_Example PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_include_directories(Az2D_Example PUBLIC "${PROJECT_SOURCE_DIR}/projects/Az2D/src/")
target_include_directories(Az2D_Example PRIVATE "${PROJECT_SOURCE_DIR}/external/Vulkan-Headers/include")
target_include_directories(Az2D_Example PRIVATE "${PROJECT_SOURCE_DIR}/external/openal-soft/include")
target_include_directories(Az2D_Example PRIVATE "${PROJECT_SOURCE_DIR}/external/stb")

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/Az2D_Example")

# we may want to be smarter about this by using a custom target and adding the files as a dependency
file(GLOB AZ2D_DATA CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/projects/Az2D/data/*")
file(COPY ${AZ2D_DATA} DESTINATION "${OUTPUT_DIR}/data/Az2D")
vulkan_shaders(Az2D_Example "src/shaders" "${OUTPUT_DIR}/data/shaders")

target_link_libraries(Az2D_Example PRIVATE Az2D)

if (CMAKE_SYSTEM MATCHES Windows)
	add_library(OpenAL32 SHARED IMPORTED)
	set_target_properties(OpenAL32 PROPERTIES
		IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/projects/Az2D/lib/OpenAL32.dll"
		IMPORTED_IMPLIB "${PROJECT_SOURCE_DIR}/projects/Az2D/lib/OpenAL32.lib"
	)
	target_link_libraries(Az2D_Example PRIVATE OpenAL32)
elseif (CMAKE_SYSTEM MATCHES Linux)
	target_link_libraries(Az2D_Example PRIVATE openal)
endif()

set_target_properties(Az2D_Example PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
