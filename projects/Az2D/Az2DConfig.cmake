include(CMakeFindDependencyMacro)
find_dependency(AzCore REQUIRED)
find_dependency(OpenAL REQUIRED)

include(${CMAKE_CURRENT_LIST_DIR}/Az2D.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Az2DDataCopy.cmake)
