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

target_link_libraries(Az2D_Example PRIVATE Az2D)

# we may want to be smarter about this by using a custom target and adding the files as a dependency
file(GLOB AZ2D_DATA CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/projects/Az2D/data/*")
file(COPY ${AZ2D_DATA} DESTINATION "${OUTPUT_DIR}/data/Az2D")
vulkan_shaders(Az2D_Example "src/shaders" "${OUTPUT_DIR}/data/shaders")

set_target_properties(Az2D_Example PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
