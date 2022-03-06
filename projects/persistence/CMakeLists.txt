add_executable(persistence
	src/main.cpp
)

target_include_directories(persistence PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_link_libraries(persistence PRIVATE AzCore)

set_target_properties(persistence PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/projects/persistence/bin")
