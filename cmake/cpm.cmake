# Include CPM for managing dependencies, and set it up to cache them in the deps folder.
set(CPM_DOWNLOAD_VERSION 0.36.0)    
set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_LIST_DIR}/../deps" CACHE PATH "Dependencies path.")
set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
file(DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
        ${CPM_DOWNLOAD_LOCATION}
)
endif()
include(${CPM_DOWNLOAD_LOCATION})
