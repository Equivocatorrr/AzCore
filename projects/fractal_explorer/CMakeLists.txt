add_executable(fractal_explorer
	src/main.cpp
	src/resources.h
	src/resources.rc
)

target_include_directories(fractal_explorer PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")

target_link_libraries(fractal_explorer PRIVATE AzCore_SoftwareRenderer)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/fractal_explorer")

set_target_properties(fractal_explorer PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
