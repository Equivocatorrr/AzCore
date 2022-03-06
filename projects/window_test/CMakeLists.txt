add_executable(window_test
	src/main.cpp
)

target_link_libraries(window_test PRIVATE AzCore_IO)

set_target_properties(window_test PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/projects/window_test/bin")
