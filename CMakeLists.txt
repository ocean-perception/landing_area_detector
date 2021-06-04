cmake_minimum_required(VERSION 3.2)

if(NOT DEFINED CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebug RelWithDebInfo MinSizeRel."
      FORCE)
endif()

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread -mavx2")
# Additional flags for vectorized/multithread profiling and debugging
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -O3  -shared-libgcc")

# WARNING: CGAL / QT5 setup required this preproc
# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTORCC ON)
# set(CMAKE_AUTOUIC ON)

find_package(OpenMP)
if(OPENMP_FOUND)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# Looking for GDAL;: https://cmake.org/cmake/help/v3.0/module/FindGDAL.html
find_package(GDAL 2.2 REQUIRED) # Newest releases of GDAL provide VERSION information
message(STATUS "GDAL library status:")
#message(STATUS "    version: ${GDAL_version}") # FindGDAL.cmake does not provide VERSION info for cmake < 3.13
message(STATUS "    libraries: ${GDAL_LIBRARY}")
message(STATUS "    include path: ${GDAL_INCLUDE_DIR}")

# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# Being honest, we could do it with 3.4.X but for future compatibility we should push to the latest LTS release (4.2)
find_package(OpenCV 4.4 REQUIRED
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
# find_package(CUDA)

# if(CUDA_FOUND)
#     INCLUDE(FindCUDA)
#   # If the package has been found, several variables will
#   # be set, you can find the full list with descriptions
#   # in the CUDAConfig.cmake file.
#   # Print some message showing some of them
#   message(STATUS "CUDA library status:")
#   message(STATUS "    toolkit root dir: ${CUDA_TOOLKIT_ROOT_DIR}")
#   message(STATUS "    version: ${CUDA_VERSION}")
#   message(STATUS "    libraries: ${CUDA_LIBRARIES}")
#   message(STATUS "    include path: ${CUDA_INCLUDE_DIRS}")
# endif(CUDA_FOUND)

#find_package(CGAL REQUIRED COMPONENTS Qt5 Core)
#CGAL_Qt5 is needed for the drawing.
find_package(CGAL REQUIRED OPTIONAL_COMPONENTS Qt5)

if(CGAL_Qt5_FOUND)
  #required to use basic_viewer
  add_definitions(-DCGAL_USE_BASIC_VIEWER -DQT_NO_KEYWORDS)
endif()

if ( NOT CGAL_FOUND )
  message(STATUS "This project requires the CGAL library, and will not be compiled.")
  return()  
endif()


set(TBB_VERSION "2021.2") # to deal with broken TBBFind.cmake from CGAL
find_package( TBB)

message(STATUS "CGAL library status:")
#message(STATUS "    version: ${GDAL_version}") # FindGDAL.cmake does not provide VERSION info for cmake < 3.13
message(STATUS "    libraries: ${CGAL_LIBRARY}")
message(STATUS "    include path: ${CGAL_INCLUDE_DIR}")



# include helper file
include( ${CGAL_USE_FILE} )

#find_package(PCL REQUIRED COMPONENTS common io)

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
 #                   ${PCL_INCLUDE_DIRS})

# Retrieve git commit information, forward it to compilation time
exec_program(
    "git"
    ${CMAKE_CURRENT_SOURCE_DIR}
    ARGS "describe --abbrev=4 --dirty --always --tags"
    OUTPUT_VARIABLE GIT_INFO )
add_definitions( -DGIT_COMMIT="${GIT_INFO}" ) # Forward current git info as a single string
add_definitions( -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" ) # Forward current git info as a single string
add_definitions(-DCGAL_USE_BASIC_VIEWER)

############################ MAD_TEST ####################
add_executable(mad_test   src/mad_test.cpp
                          src/lad_analysis.cpp
                          ${GEOTIFF_MODULE}/src/geotiff.cpp
                          src/lad_core.cpp
                          src/lad_layer.cpp
                          src/lad_processing.cpp
                          src/lad_thread.cpp
                          src/lad_config.cpp
                          ${PROJECT_HEADERS})

############################ TIFF2PNG ####################
add_executable(tiff2png   src/tiff2png.cpp
                          src/lad_analysis.cpp
                          ${GEOTIFF_MODULE}/src/geotiff.cpp
                          src/lad_core.cpp
                          src/lad_layer.cpp
                          src/lad_processing.cpp
                          src/lad_thread.cpp
                          src/lad_config.cpp
                          ${PROJECT_HEADERS})

############################ TIFFRUGOSITY#################
add_executable(tiff2rugosity   src/tiff2rugosity.cpp
                          src/lad_analysis.cpp
                          ${GEOTIFF_MODULE}/src/geotiff.cpp
                          src/lad_core.cpp
                          src/lad_layer.cpp
                          src/lad_processing.cpp
                          src/lad_thread.cpp
                          src/lad_config.cpp
                          ${PROJECT_HEADERS})

############################ IMG_RESAMPLE ####################
add_executable(img.resample   src/img.resample.cpp
                          src/lad_analysis.cpp
                          ${GEOTIFF_MODULE}/src/geotiff.cpp
                          src/lad_core.cpp
                          src/lad_layer.cpp
                          src/lad_processing.cpp
                          src/lad_thread.cpp
                          # src/helper.cpp      # DO NOT ADD VIA add_executable
                          ${PROJECT_HEADERS})
                          #find_path(yaml-cpp_INCLUDE_DIRS yaml-cpp/yaml.h )
############################ IMG_RESAMPLE ####################
add_executable(cgal.test   src/cgal-test.cpp
                          ${PROJECT_HEADERS})
                          #find_path(yaml-cpp_INCLUDE_DIRS yaml-cpp/yaml.h )
#message (${yaml-cpp_INCLUDE_DIRS})

target_compile_options(mad_test     PUBLIC -std=c++14 -mavx)
target_compile_options(tiff2png     PUBLIC -std=c++14 -mavx)
target_compile_options(img.resample PUBLIC -std=c++14 -mavx)
target_link_libraries(tiff2png      ${OpenCV_LIBS} ${GDAL_LIBRARY} yaml-cpp)
target_link_libraries(tiff2rugosity ${OpenCV_LIBS} ${GDAL_LIBRARY} yaml-cpp)
target_link_libraries(img.resample  ${OpenCV_LIBS} ${GDAL_LIBRARY} yaml-cpp)
target_link_libraries(mad_test      ${OpenCV_LIBS} ${GDAL_LIBRARY} yaml-cpp)

if(CGAL_Qt5_FOUND)
  #link it with the required CGAL libraries
  target_link_libraries(cgal.test PUBLIC CGAL::CGAL_Qt5)
endif()


#find_package(TBB)
# include(CGAL_TBB_support)
#include(CGAL_target_use_TBB)

 # Declare the executable target built from your sources
#  if(USE_CUDA AND CUDA_FOUND)
#   # Add definition to CUDA SUPPORT
#   add_definitions(-D USE_GPU)
#   message(STATUS "Configuring for GPU version.")
#   # Link your application with OpenCV libraries
#   target_link_libraries(mad_test ${OpenCV_LIBS} ${CUDA_LIBRARIES} ${GDAL_LIBRARY} yaml-cpp)
# else()
#   message(STATUS "Configuring for non-GPU version.")
#   message(STATUS "	Expect a slower speed...")
#   # Link your application with OpenCV libraries
#   target_link_libraries(mad_test ${OpenCV_LIBS} ${GDAL_LIBRARY} yaml-cpp)
# endif()


# if(TBB_FOUND)
#   target_link_libraries(mad_test PUBLIC ${OpenCV_LIBS} ${GDAL_LIBRARY} yaml-cpp)
#   message(
#     STATUS "NOTICE: Intel TBB was found...")
# else()
#   target_link_libraries(mad_test ${OpenCV_LIBS} ${GDAL_LIBRARY} yaml-cpp)
#   message(
#     STATUS "NOTICE: Intel TBB was not found. Sequential code will be used.")
# endif()

install (TARGETS cgal.test      DESTINATION $ENV{HOME}/bin)
install (TARGETS mad_test       DESTINATION $ENV{HOME}/bin)
install (TARGETS tiff2png       DESTINATION $ENV{HOME}/bin)
install (TARGETS tiff2rugosity  DESTINATION $ENV{HOME}/bin)
install (TARGETS img.resample   DESTINATION $ENV{HOME}/bin)
configure_file(scripts/tiff.convert2png.sh  $ENV{HOME}/bin/scripts)
configure_file(scripts/collect_maps.sh      $ENV{HOME}/bin/scripts)
