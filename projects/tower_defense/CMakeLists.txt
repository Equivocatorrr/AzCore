add_executable(tower_defense
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
	"${PROJECT_SOURCE_DIR}/base/src/manifest.xml.manifest"
)

target_include_directories(tower_defense PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_include_directories(tower_defense PUBLIC "${PROJECT_SOURCE_DIR}/external/Vulkan-Headers/include")
target_include_directories(tower_defense PUBLIC "${PROJECT_SOURCE_DIR}/external/openal-soft/include")
target_include_directories(tower_defense PUBLIC "${PROJECT_SOURCE_DIR}/external/stb")

# gets included in src/entities.cpp
set_source_files_properties(src/entity_basics.cpp PROPERTIES HEADER_FILE_ONLY TRUE)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/tower_defense")

vulkan_shaders(tower_defense "src/shaders" "${OUTPUT_DIR}/data/shaders")

target_link_libraries(tower_defense PRIVATE AzCore_Vulkan)

if (CMAKE_SYSTEM MATCHES Windows)
	add_library(OpenAL32 SHARED IMPORTED)
	set_target_properties(OpenAL32 PROPERTIES
		IMPORTED_LOCATION "${OUTPUT_DIR}/lib/OpenAL32.dll"
		IMPORTED_IMPLIB "${OUTPUT_DIR}/lib/OpenAL32.lib"
	)
	target_link_libraries(tower_defense PRIVATE OpenAL32)
elseif (CMAKE_SYSTEM MATCHES Linux)
	target_link_libraries(tower_defense PRIVATE openal)
endif()

set_target_properties(tower_defense PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
