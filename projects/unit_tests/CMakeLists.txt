add_executable(unit_tests
	src/main.cpp
	src/UnitTests.cpp
	src/UnitTests.hpp
	src/Utilities.cpp
	src/Utilities.hpp
	# actual tests
	src/tests/BinarySet.cpp
	src/tests/BranchPredictionVsReturnAddressPrediction.cpp
	src/tests/BinaryMap.cpp
	src/tests/BasicMath.cpp
	src/tests/CustomMaths.cpp
	src/tests/Angle.cpp
	src/tests/Quaternion.cpp
	src/tests/String.cpp
	src/tests/StringArena.cpp
	src/tests/Shuffle.cpp
	src/tests/RayTriangleIntersect.cpp
	src/tests/RandomNumberGenerator.cpp
	src/tests/Noise.cpp
	src/tests/Equations.cpp
	src/tests/Matrix.cpp
)

target_include_directories(unit_tests PUBLIC "${PROJECT_SOURCE_DIR}/base/src/")
target_link_libraries(unit_tests PRIVATE AzCore)

set(OUTPUT_DIR "${PROJECT_SOURCE_DIR}/projects/unit_tests")

set_target_properties(unit_tests PROPERTIES
	DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin"
	VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}"
)
