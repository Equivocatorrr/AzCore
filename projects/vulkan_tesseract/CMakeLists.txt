add_executable(vulkan_tesseract
	src/main.cpp
	src/resources.h
	src/resources.rc
	"${PROJECT_SOURCE_DIR}/base/src/manifest.xml"
)

target_include_directories(vulkan_tesseract PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_include_directories(vulkan_tesseract PUBLIC "${PROJECT_SOURCE_DIR}/ThirdParty/Vulkan-Headers/include")

# gets included in src/entities.cpp
set_source_files_properties(src/entity_basics.cpp PROPERTIES HEADER_FILE_ONLY TRUE)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/vulkan_tesseract")

vulkan_shaders(vulkan_tesseract "src/shaders" "${OUTPUT_DIR}/data/shaders")

target_link_libraries(vulkan_tesseract PRIVATE AzCore_Vulkan)

set_target_properties(vulkan_tesseract PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin")
