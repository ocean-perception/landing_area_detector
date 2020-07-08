cmake_minimum_required(VERSION 2.8)  # For legacy system compatibility.

set(CMAKE_COLOR_MAKEFILE ON)  # For fancy colouring scheme
if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options (project_options INTERFACE -fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        target_compile_options (project_options INTERFACE -fcolor-diagnostics)
    endif ()
endif ()

# Define project name
project(lad_project)

# Looking for GDAL;: https://cmake.org/cmake/help/v3.0/module/FindGDAL.html
find_package(GDAL 2.2 REQUIRED) # Newest releases of GDAL provide VERSION information
message(STATUS "GDAL library status:")
#message(STATUS "    version: ${GDAL_version}") # FindGDAL.cmake does not provide VERSION info for cmake < 3.13
message(STATUS "    libraries: ${GDAL_LIBRARY}")
message(STATUS "    include path: ${GDAL_INCLUDE_DIR}")

# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# Being honest, we could do it with 3.4.X but for future compatibility we should push to the latest LTS release (4.2)
find_package(OpenCV 4.2 REQUIRED
                        NO_MODULE
                        PATHS /usr/local
                        NO_DEFAULT_PATH)

# If the package has been found, several variables will
# be set, you can find the full list with descriptions
# in the OpenCVConfig.cmake file.
# Print some message showing some of them
# At this level, it should be useful to check minimum required version or exact version
# So far, there are still some issues with OpenCV installations included in ROS
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

# Cross your fingers
#find_package(CUDA) #not required yet

#find_package(CGAL REQUIRED COMPONENTS Qt5 Core)
find_package( CGAL QUIET COMPONENTS  )
if ( NOT CGAL_FOUND )
  message(STATUS "This project requires the CGAL library, and will not be compiled.")
  return()  
endif()
message(STATUS "CGAL library status:")
#message(STATUS "    version: ${GDAL_version}") # FindGDAL.cmake does not provide VERSION info for cmake < 3.13
message(STATUS "    libraries: ${CGAL_LIBRARY}")
message(STATUS "    include path: ${CGAL_INCLUDE_DIR}")

# include helper file
include( ${CGAL_USE_FILE} )

# Boost and its components
find_package( Boost REQUIRED )

if ( NOT Boost_FOUND )
  message(STATUS "This project requires the Boost library, and will not be compiled.")
  return()  
endif()

# Add submodule as external/geotiff subdirectory. But we do not want to trigger its own CMake chain, so we just set the path
#add_subdirectory(external/geotiff)
set (GEOTIFF_MODULE external/geotiff)

# Greedy include of all headers
file(GLOB PROJECT_HEADERS include/*.h include/*.hpp)
include_directories(BEFORE ../include
                    include
                    ${OpenCV_INCLUDE_DIRS}
                    ${GDAL_INCLUDE_DIR}
                    ${GEOTIFF_MODULE}/include)

# Retrieve git commit information, forward it to compilation time
exec_program(
    "git"
    ${CMAKE_CURRENT_SOURCE_DIR}
    ARGS "describe --abbrev=4 --dirty --always --tags"
    OUTPUT_VARIABLE GIT_INFO )
add_definitions( -DGIT_COMMIT="${GIT_INFO}" ) # Forward current git info as a single string

############################ GDAL_TEST ####################
add_executable (lad_gdal  src/gdal_test.cpp
                          ${GEOTIFF_MODULE}/src/geotiff.cpp
                          ${PROJECT_HEADERS})

target_compile_options(lad_gdal PUBLIC -std=c++11)
target_link_libraries(lad_gdal ${OpenCV_LIBS} ${GDAL_LIBRARY})

############################ LAD_TEST ####################
add_executable(cgal_test  src/cgal_test.cpp
                          ${PROJECT_HEADERS})
add_to_cached_list( CGAL_EXECUTABLE_TARGETS cgal_test )

target_compile_options(cgal_test PUBLIC -std=c++11)
target_link_libraries(cgal_test   ${CGAL_LIBRARIES} ${CGAL_3RD_PARTY_LIBRARIES} )

############################ LAD_TEST ####################
add_executable(lad_test   src/lad_test.cpp
                          src/lad_analysis.cpp
                          ${GEOTIFF_MODULE}/src/geotiff.cpp
                          src/lad_core.cpp
                          src/lad_layer.cpp
                          src/lad_processing.cpp
                          ${PROJECT_HEADERS})

target_compile_options(lad_test PUBLIC -std=c++11)
target_link_libraries(lad_test ${OpenCV_LIBS} ${GDAL_LIBRARY})

# Define install target for main testing module
install (TARGETS lad_test DESTINATION $ENV{HOME}/bin)