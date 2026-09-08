include(CMakeFindDependencyMacro)
find_dependency(AzCore REQUIRED)
find_dependency(OpenAL REQUIRED)

include(${CMAKE_CURRENT_LIST_DIR}/Az3D.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Az3DDataCopy.cmake)
