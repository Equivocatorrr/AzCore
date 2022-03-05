add_executable(font_test
	src/main.cpp
)

target_link_libraries(font_test PRIVATE AzCore)

set_target_properties(font_test PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

set_property(TARGET font_test PROPERTY RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/projects/font_test/bin")
