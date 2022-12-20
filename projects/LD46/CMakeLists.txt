add_executable(LD46
	src/main.cpp
	src/entities.cpp
	src/entities.hpp
	src/gui.cpp
	src/gui.hpp
	src/resources.h
	src/resources.rc
	"${PROJECT_SOURCE_DIR}/base/src/manifest.xml.manifest"
)

target_include_directories(LD46 PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_include_directories(LD46 PUBLIC "${PROJECT_SOURCE_DIR}/projects/Az2D/src/")
target_include_directories(LD46 PRIVATE "${PROJECT_SOURCE_DIR}/external/Vulkan-Headers/include")
target_include_directories(LD46 PRIVATE "${PROJECT_SOURCE_DIR}/external/openal-soft/include")
target_include_directories(LD46 PRIVATE "${PROJECT_SOURCE_DIR}/external/stb")

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/LD46")

target_link_libraries(LD46 PRIVATE Az2D)

AZ2D_DATA_COPY(LD46 ${OUTPUT_DIR})
# vulkan_shaders(LD46 "src/shaders" "${OUTPUT_DIR}/data/shaders")

set_target_properties(LD46 PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
