add_executable(polyprox
	src/main.cpp
)

target_include_directories(polyprox PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_link_libraries(polyprox PRIVATE AzCore_IO)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/polyprox")

set_target_properties(polyprox PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
