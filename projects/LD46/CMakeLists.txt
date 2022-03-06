add_executable(LD46
	src/main.cpp
	src/assets.cpp
	src/assets.hpp
	src/entities.cpp
	src/entities.hpp
	src/entity_basics.cpp
	src/entity_basics.hpp
	src/globals.cpp
	src/globals.hpp
	src/gui.cpp
	src/gui.hpp
	src/objects.cpp
	src/objects.hpp
	src/rendering.cpp
	src/rendering.hpp
	src/sound.cpp
	src/sound.hpp
	src/resources.h
	src/resources.rc
	"${PROJECT_SOURCE_DIR}/base/src/manifest.xml"
)

target_include_directories(LD46 PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_include_directories(LD46 PRIVATE "${PROJECT_SOURCE_DIR}/external/Vulkan-Headers/include")
target_include_directories(LD46 PRIVATE "${PROJECT_SOURCE_DIR}/external/openal-soft/include")
target_include_directories(LD46 PRIVATE "${PROJECT_SOURCE_DIR}/external/stb")

# gets included in src/entities.cpp
set_source_files_properties(src/entity_basics.cpp PROPERTIES HEADER_FILE_ONLY TRUE)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/LD46")

vulkan_shaders(LD46 "src/shaders" "${OUTPUT_DIR}/data/shaders")

target_link_libraries(LD46 PRIVATE AzCore_Vulkan)

if (CMAKE_SYSTEM MATCHES Windows)
	add_library(OpenAL32 SHARED IMPORTED)
	set_target_properties(OpenAL32 PROPERTIES
		IMPORTED_LOCATION "${OUTPUT_DIR}/lib/OpenAL32.dll"
		IMPORTED_IMPLIB "${OUTPUT_DIR}/lib/OpenAL32.lib"
	)
	target_link_libraries(LD46 PRIVATE OpenAL32)
elseif (CMAKE_SYSTEM MATCHES Linux)
	target_link_libraries(LD46 PRIVATE openal)
endif()

set_target_properties(LD46 PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin")
