add_executable(font_test
	src/main.cpp
)

target_include_directories(font_test PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_include_directories(font_test PRIVATE "${PROJECT_SOURCE_DIR}/external/stb")

target_link_libraries(font_test PRIVATE AzCore)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/font_test")

set_target_properties(font_test PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
