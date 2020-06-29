# cmake needs this line
cmake_minimum_required(VERSION 2.8)

# Define project name
project(lad_project)

# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# via the command line or GUI

#find_package(OpenCV 4.1 REQUIRED
#NO_MODULE
#PATHS /usr/local
#NO_DEFAULT_PATH)

# If the package has been found, several variables will
# be set, you can find the full list with descriptions
# in the OpenCVConfig.cmake file.
# Print some message showing some of them
# At this level, it should be useful to check minimum required version or exact version
# So far, there are still some issues with OpenCV installations included in ROS
#message(STATUS "OpenCV library status:")
#message(STATUS "    version: ${OpenCV_VERSION}")
#message(STATUS "    libraries: ${OpenCV_LIBS}")
#message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

# Cross your fingers
#find_package(CUDA)

#if(CUDA_FOUND)
#    INCLUDE(FindCUDA)
  # If the package has been found, several variables will
  # be set, you can find the full list with descriptions
  # in the CUDAConfig.cmake file.
  # Print some message showing some of them
#  message(STATUS "CUDA library status:")
#  message(STATUS "    toolkit root dir: ${CUDA_TOOLKIT_ROOT_DIR}")
#  message(STATUS "    version: ${CUDA_VERSION}")
#  message(STATUS "    libraries: ${CUDA_LIBRARIES}")
#  message(STATUS "    include path: ${CUDA_INCLUDE_DIRS}")
#endif(CUDA_FOUND)

# a bit greedy approach to include all the files
file(GLOB project-include-files
    "../common/*.h"
    "../common/*.hxx"
    "../common/*.cpp"
    "include/*.h"
    "include/*.hpp"
    "src/*.cpp"
)

# Retrieve git commit information that is forwarded at compilation time
exec_program(
    "git"
    ${CMAKE_CURRENT_SOURCE_DIR}
    ARGS "describe --abbrev=4 --dirty --always --tags"
    OUTPUT_VARIABLE GIT_INFO )
# fwd git commit info via GIT_COMMIT variable
add_definitions( -DGIT_COMMIT="${GIT_INFO}" )


 # Declare the executable target built from your sources
if(CUDA_FOUND)
  # Add definition to CUDA SUPPORT
  add_definitions(-D USE_GPU)
  message(STATUS "Configuring for GPU version.")
  # Link your application with OpenCV libraries
 target_link_libraries(mp_cpfind ${OpenCV_LIBS} ${CUDA_LIBRARIES})
else()
  message(STATUS "Configuring for non-GPU version.")
  message(STATUS "	Expect a slower speed...")
  # Link your application with OpenCV libraries
 # WARNING remove comment from next line
 #target_link_libraries(mp_cpfind ${OpenCV_LIBS})
endif(CUDA_FOUND)

add_executable(lad_test ${project-include-files})
target_compile_options(lad_test PUBLIC -std=c++11)