cmake_minimum_required(VERSION 2.8)  # we could bump to >3.4
set(CMAKE_COLOR_MAKEFILE ON)
#set(CMAKE_VERBOSE_MAKEFILE ON)

# Define project name
project(lad_project)

# Looking for GDAL;: https://cmake.org/cmake/help/v3.0/module/FindGDAL.html
find_package(GDAL 2.2 REQUIRED)
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

# Greedy include of all headers
file(GLOB PROJECT_HEADERS include/*.h)
include_directories(include
                    ${OpenCV_INCLUDE_DIRS}
                    ${GDAL_INCLUDE_DIRS})

# Retrieve git commit information, forward it to compilation time
exec_program(
    "git"
    ${CMAKE_CURRENT_SOURCE_DIR}
    ARGS "describe --abbrev=4 --dirty --always --tags"
    OUTPUT_VARIABLE GIT_INFO )
# fwd git commit info via GIT_COMMIT variable
add_definitions( -DGIT_COMMIT="${GIT_INFO}" )

############################ GDAL_TEST ####################
add_executable (lad_gdal  src/gdal_test.cpp
                          #src/geotiff.cpp
                          ${PROJECT_HEADERS})

target_compile_options(lad_gdal PUBLIC -std=c++11)
target_link_libraries(lad_gdal ${GDAL_LIBRARIES})

############################ LAD_TEST ####################
add_executable(lad_test   src/lad_test.cpp
                          #src/geotiff.cpp
                          ${PROJECT_HEADERS})

target_compile_options(lad_test PUBLIC -std=c++11)
target_link_libraries(lad_test ${OpenCV_LIBS})
