add_executable(window_test
	src/main.cpp
)

target_include_directories(window_test PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_link_libraries(window_test PRIVATE AzCore_IO)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/window_test")

set_target_properties(window_test PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
