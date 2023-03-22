add_executable(noise_test
	src/main.cpp
	src/resources.h
	src/resources.rc
)

target_include_directories(noise_test PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")

target_link_libraries(noise_test PRIVATE AzCore_SoftwareRenderer)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/noise_test")

set_target_properties(noise_test PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
