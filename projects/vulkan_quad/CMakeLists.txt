add_executable(vulkan_quad
	src/main.cpp
	src/resources.h
	src/resources.rc
	"${PROJECT_SOURCE_DIR}/base/src/manifest.xml"
)

target_include_directories(vulkan_quad PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_include_directories(vulkan_quad PUBLIC "${PROJECT_SOURCE_DIR}/ThirdParty/Vulkan-Headers/include")
target_include_directories(vulkan_quad PUBLIC "${PROJECT_SOURCE_DIR}/ThirdParty/stb")

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/vulkan_quad")

vulkan_shaders(vulkan_quad "src/shaders" "${OUTPUT_DIR}/data/shaders")

target_link_libraries(vulkan_quad PRIVATE AzCore_Vulkan)

set_target_properties(vulkan_quad PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin")
