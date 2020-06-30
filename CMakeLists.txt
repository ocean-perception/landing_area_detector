# cmake needs this line
cmake_minimum_required(VERSION 2.8)

# Define project name
project(lad_project)

# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# via the command line or GUI

# Looking for GDAL
find_package(GDAL)
message(STATUS "GDAL library status:")
#message(STATUS "    version: ${GDAL_VERSION}")
message(STATUS "    libraries: ${GDAL_LIBRARIES}")
#message(STATUS "    include path: ${GDAL_includedir}")

# Being honest, we could do it with 3.4.X but for future compatibility we should push to the latest LTS release
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
#find_package(CUDA)

file(GLOB PROJECT_HEADERS include/*.h)

include_directories(include
                    ${OpenCV_INCLUDE_DIRS}
                    ${GDAL_INCLUDE_DIRS})

# Retrieve git commit information that is forwarded at compilation time
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

