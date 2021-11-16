cmake_minimum_required(VERSION 3.18)

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
set(CMAKE_CXX_STANDARD 11)          # verify that the compiler supports C++11, as some targets could reversely fallback to c++14
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -mavx2")
# Additional flags for vectorized/multithread profiling and debugging
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -O3  -shared-libgcc")

find_package(OpenMP)
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
find_package(OpenCV 4   REQUIRED core imgproc
                        
                        PATHS /usr/local /local/software/opencv/4.5.3
                        NO_DEFAULT_PATH)

# If the package has been found, several variables will be set, you can find the full list with descriptions
# in the OpenCVConfig.cmake file.
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

# Looking for GDAL;: https://cmake.org/cmake/help/v3.0/module/FindGDAL.html
    # export GDAL_DIR=/local/software/gdal/3.0.1/
    # On Iridis5- hinting to /local/software path using GDAL_DIR ENV
find_package(GDAL MODULE REQUIRED) # Newest releases of GDAL provide VERSION information
message(STATUS "GDAL library status:")
message(STATUS "    version: ${GDAL_VERSION}") # FindGDAL.cmake does not provide VERSION info for cmake < 3.13
message(STATUS "    libraries: ${GDAL_LIBRARY}")
message(STATUS "    include path: ${GDAL_INCLUDE_DIRS}")

#find_package(PCL REQUIRED COMPONENTS common io) #not required yet

# Boost and its components
find_package( Boost REQUIRED )

if ( NOT Boost_FOUND )
  message(STATUS "This project requires the Boost library, and will not be compiled.")
  return()  
endif()

find_package(CGAL REQUIRED)
# find_package(CGAL REQUIRED OPTIONAL_COMPONENTS Qt5)

# if(CGAL_Qt5_FOUND)
#   #required to use basic_viewer
#   add_definitions(-DCGAL_USE_BASIC_VIEWER -DQT_NO_KEYWORDS)
# endif()

if ( NOT CGAL_FOUND )
  message(STATUS "This project requires the CGAL library, and will not be compiled.")
  return()  
endif()

message(STATUS "CGAL library status:")
message(STATUS "    libraries: ${CGAL_LIBRARY}")
message(STATUS "    include path: ${CGAL_INCLUDE_DIR}")

set(TBB_VERSION "2021.2") # to deal with broken TBBFind.cmake from CGAL
find_package( TBB)

# include helper file
include( ${CGAL_USE_FILE} )

# Add submodule as external/geotiff subdirectory. But we do not want to trigger its own CMake chain, so we just set the path
#add_subdirectory(external/geotiff)
set (GEOTIFF_MODULE external/geotiff)

# Greedy include of all headers
file(GLOB PROJECT_HEADERS include/*.h include/*.hpp)


set (YAML_DIR $ENV{HOME}/local)

include_directories(BEFORE ../include
                    include
                    ${OpenCV_INCLUDE_DIRS}
                    ${GDAL_INCLUDE_DIRS}
                    ${GEOTIFF_MODULE}/include)
 #                   ${PCL_INCLUDE_DIRS})
          
include_directories(AFTER ${YAML_DIR}/include)

if (TARGET_IRIDIS)
    message (WARNING "Iridis specific build")
    add_definitions (-DIRIDIS_BUILD)
    add_definitions (-DTARGET_IRIDIS)
    # This invocation flag was added to enable Iridis5 specific patch for broken OpenCV installation. Libs are missing
endif()


# Retrieve git commit information, forward it as compiler flags so they can be used during building time
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

# find_path(yaml-cpp INCLUDE_DIRS yaml-cpp/yaml.h )
# message (STATUS Yaml-cpp: ${yaml-cpp})

find_library (YAML_LIB yaml-cpp PATHS ${YAML_DIR}/lib64)

message (STATUS "Using yaml-cpp libraries from:" ${YAML_LIB})
# message (WARNING "    libraries: ${OpenCV_LIBS}")

# message (STATUS "Searching for yaml-cpp in " ${YAML_DIR})
# target_include_directories (mad_test PUBLIC ${YAML_DIR}/include)


target_compile_options(mad_test     PUBLIC -std=c++11 -mavx)
target_compile_options(tiff2png     PUBLIC -std=c++11 -mavx)
target_compile_options(img.resample PUBLIC -std=c++11 -mavx)
target_link_libraries(tiff2png      ${OpenCV_LIBS} ${GDAL_LIBRARY} ${YAML_LIB})
target_link_libraries(tiff2rugosity ${OpenCV_LIBS} ${GDAL_LIBRARY} ${YAML_LIB})
target_link_libraries(img.resample  ${OpenCV_LIBS} ${GDAL_LIBRARY} ${YAML_LIB})
target_link_libraries(mad_test      ${OpenCV_LIBS} ${GDAL_LIBRARY} ${YAML_LIB})

install (TARGETS mad_test       DESTINATION $ENV{HOME}/bin)
install (TARGETS tiff2png       DESTINATION $ENV{HOME}/bin)
install (TARGETS tiff2rugosity  DESTINATION $ENV{HOME}/bin)
install (TARGETS img.resample   DESTINATION $ENV{HOME}/bin)
configure_file(scripts/tiff.convert2png.sh  $ENV{HOME}/bin/scripts)
configure_file(scripts/collect_maps.sh      $ENV{HOME}/bin/scripts)
