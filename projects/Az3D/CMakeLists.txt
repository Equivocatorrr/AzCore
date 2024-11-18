add_library(Az3D STATIC
	src/Az3D/animation.cpp
	src/Az3D/animation.hpp
	src/Az3D/assets.cpp
	src/Az3D/assets.hpp
	src/Az3D/Az3DObj.cpp
	src/Az3D/Az3DObj.hpp
	# src/Az3D/entity_basics.cpp
	# src/Az3D/entity_basics.hpp
	src/Az3D/settings.cpp
	src/Az3D/settings.hpp
	src/Az3D/game_systems.cpp
	src/Az3D/game_systems.hpp
	src/Az3D/rendering.cpp
	src/Az3D/rendering.hpp
	src/Az3D/sound.cpp
	src/Az3D/sound.hpp
)

target_include_directories(Az3D PRIVATE "${PROJECT_SOURCE_DIR}/external/Vulkan-Headers/include")
target_include_directories(Az3D PRIVATE "${PROJECT_SOURCE_DIR}/external/openal-soft/include")
target_include_directories(Az3D PRIVATE "${PROJECT_SOURCE_DIR}/external/stb")
target_include_directories(Az3D PUBLIC
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/projects/Az3D/src>
	$<INSTALL_INTERFACE:include>
)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/Az3D")

vulkan_shaders(Az3D "src/Az3D/shaders" "${OUTPUT_DIR}/data/shaders")

function(AZ3D_COPY_SHADERS output_target)
	add_custom_command(TARGET Az3D_Shaders POST_BUILD
		COMMAND echo "Copying Az3D data/shaders to ${output_target}"
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${OUTPUT_DIR}/data/shaders" "${PROJECT_SOURCE_DIR}/projects/${output_target}/data/Az3D/shaders"
	)
endfunction()


target_link_libraries(Az3D PUBLIC AzCore_Vulkan)

target_link_libraries(Az3D PUBLIC OpenAL::OpenAL)

set_target_properties(Az3D PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/lib"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
install(TARGETS Az3D
	EXPORT Az3D
	CONFIGURATIONS Debug Release
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/Az3D"
	INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Az3D"
)
install(EXPORT Az3D
	FILE Az3D.cmake
	NAMESPACE Az3D::
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Az3D"
)
install(FILES Az3DConfig.cmake Az3DDataCopy.cmake DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Az3D")
install(DIRECTORY "src/Az3D"
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	FILES_MATCHING PATTERN "*.hpp"
)
install(DIRECTORY "data"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Az3D"
)