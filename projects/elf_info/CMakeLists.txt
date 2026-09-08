add_executable(elf_info
	src/main.cpp
)

target_include_directories(elf_info PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_link_libraries(elf_info PRIVATE AzCore)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/elf_info")

set_target_properties(elf_info PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
