add_library(Az2D STATIC
	src/Az2D/assets.cpp
	src/Az2D/assets.hpp
	src/Az2D/entity_basics.cpp
	src/Az2D/entity_basics.hpp
	src/Az2D/settings.cpp
	src/Az2D/settings.hpp
	src/Az2D/gui_basics.cpp
	src/Az2D/gui_basics.hpp
	src/Az2D/game_systems.cpp
	src/Az2D/game_systems.hpp
	src/Az2D/rendering.cpp
	src/Az2D/rendering.hpp
	src/Az2D/sound.cpp
	src/Az2D/sound.hpp
	src/Az2D/sprite.cpp
	src/Az2D/sprite.hpp
)

target_include_directories(Az2D PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_include_directories(Az2D PRIVATE "${PROJECT_SOURCE_DIR}/external/Vulkan-Headers/include")
target_include_directories(Az2D PRIVATE "${PROJECT_SOURCE_DIR}/external/openal-soft/include")
target_include_directories(Az2D PRIVATE "${PROJECT_SOURCE_DIR}/external/stb")

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/Az2D")

vulkan_shaders(Az2D "src/Az2D/shaders" "${OUTPUT_DIR}/data/shaders")

function(AZ2D_COPY_SHADERS output_target)
	add_custom_command(TARGET Az2D_Shaders POST_BUILD
		COMMAND echo "Copying Az2D data/shaders to ${output_target}"
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${OUTPUT_DIR}/data/shaders" "${PROJECT_SOURCE_DIR}/projects/${output_target}/data/Az2D/shaders"
	)
endfunction()

AZ2D_COPY_SHADERS(Az2D_Example)
AZ2D_COPY_SHADERS(tower_defense)

target_link_libraries(Az2D PRIVATE AzCore_Vulkan)

if (CMAKE_SYSTEM MATCHES Windows)
	add_library(OpenAL32 SHARED IMPORTED)
	set_target_properties(OpenAL32 PROPERTIES
		IMPORTED_LOCATION "${OUTPUT_DIR}/lib/OpenAL32.dll"
		IMPORTED_IMPLIB "${OUTPUT_DIR}/lib/OpenAL32.lib"
	)
	target_link_libraries(Az2D PRIVATE OpenAL32)
elseif (CMAKE_SYSTEM MATCHES Linux)
	target_link_libraries(Az2D PRIVATE openal)
endif()

set_target_properties(Az2D PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/lib"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)