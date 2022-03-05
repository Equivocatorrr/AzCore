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
)

target_include_directories(AzCore PUBLIC "${PROJECT_SOURCE_DIR}/ThirdParty/Vulkan-Headers/include")

# gets included in src/entities.cpp
set_source_files_properties(src/entity_basics.cpp PROPERTIES HEADER_FILE_ONLY TRUE)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/tower_defense")

file(GLOB_RECURSE SHADER_SOURCES
	"src/shaders/*.frag"
	"src/shaders/*.vert"
)

if (CMAKE_HOST_SYSTEM MATCHES Windows)
	set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
elseif (CMAKE_HOST_SYSTEM MATCHES Linux)
	set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/bin/glslangValidator")
endif()

foreach(SHADER ${SHADER_SOURCES})
	get_filename_component(FILENAME ${SHADER} NAME)
	set(SPIRV "${OUTPUT_DIR}/data/shaders/${FILENAME}.spv")
	add_custom_command(
		OUTPUT "${SPIRV}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}/data/shaders/"
		COMMAND "${GLSL_VALIDATOR}" -V ${SHADER} -o ${SPIRV}
		DEPENDS ${SHADER}
	)
	list(APPEND SHADER_BINARIES ${SPIRV})
endforeach(SHADER)

add_custom_target(tower_defense_shaders DEPENDS ${SHADER_BINARIES})
add_dependencies(tower_defense tower_defense_shaders)

target_link_libraries(tower_defense PRIVATE AzCore_Vulkan)

if (CMAKE_SYSTEM MATCHES Windows)
	target_link_libraries(tower_defense PRIVATE lib/OpenAl32.dll)
elseif (CMAKE_SYSTEM MATCHES Linux)
	target_link_libraries(tower_defense PRIVATE openal)
endif()

set_target_properties(tower_defense PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin")
