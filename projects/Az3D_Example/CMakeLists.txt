add_executable(Az3D_Example
	src/main.cpp
	src/resources.h
	src/resources.rc
	"${PROJECT_SOURCE_DIR}/base/src/manifest.xml.manifest"
)

target_include_directories(Az3D_Example PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_include_directories(Az3D_Example PUBLIC "${PROJECT_SOURCE_DIR}/projects/Az3D/src/")
target_include_directories(Az3D_Example PRIVATE "${PROJECT_SOURCE_DIR}/external/Vulkan-Headers/include")
target_include_directories(Az3D_Example PRIVATE "${PROJECT_SOURCE_DIR}/external/openal-soft/include")
target_include_directories(Az3D_Example PRIVATE "${PROJECT_SOURCE_DIR}/external/stb")

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/Az3D_Example")

target_link_libraries(Az3D_Example PRIVATE Az3D)

AZ3D_DATA_COPY(Az3D_Example ${OUTPUT_DIR})

set_target_properties(Az3D_Example PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
