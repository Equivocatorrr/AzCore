add_executable(persistence
	src/main.cpp
)

target_include_directories(persistence PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_link_libraries(persistence PRIVATE AzCore)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/persistence")

set_target_properties(persistence PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
