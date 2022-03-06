add_executable(unit_tests
	src/main.cpp
)

target_include_directories(unit_tests PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_link_libraries(unit_tests PRIVATE AzCore)

set_target_properties(unit_tests PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/projects/unit_tests/bin")
