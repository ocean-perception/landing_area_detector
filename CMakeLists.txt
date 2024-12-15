cmake_minimum_required(VERSION 3.23)

if(NOT DEFINED CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebug RelWithDebInfo MinSizeRel."
      FORCE)
endif()

set(CMAKE_COLOR_MAKEFILE ON)
if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options (project_options INTERFACE -fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        target_compile_options (project_options INTERFACE -fcolor-diagnostics)
    endif ()
endif ()

if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(ColourBold  "${Esc}[1m")
  set(Red         "${Esc}[31m")
  set(Green       "${Esc}[32m")
  set(Yellow      "${Esc}[33m")
  set(Blue        "${Esc}[34m")
  set(Magenta     "${Esc}[35m")
  set(Cyan        "${Esc}[36m")
  set(White       "${Esc}[37m")
  set(BoldRed     "${Esc}[1;31m")
  set(BoldGreen   "${Esc}[1;32m")
  set(BoldYellow  "${Esc}[1;33m")
  set(BoldBlue    "${Esc}[1;34m")
  set(BoldMagenta "${Esc}[1;35m")
  set(BoldCyan    "${Esc}[1;36m")
  set(BoldWhite   "${Esc}[1;37m")
endif()

project(lad_project)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message ("${Cyan} Configuring system")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -g -pthread -mavx2 -shared-libgcc")

find_package(OpenMP)
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

if (USE_CUDA)
  message ("${BoldBlue} Asked for CUDA, searching... ${ColourReset}")
  find_package(CUDA)
  if(CUDA_FOUND)
    add_definitions( -DUSE_CUDA=1 )
    include(FindCUDA)
    message("${BoldGreen}" "CUDA library status: ${ColourReset}")
    message(STATUS "    toolkit root dir: ${CUDA_TOOLKIT_ROOT_DIR}")
    message(STATUS "    version: ${CUDA_VERSION}")
    message(STATUS "    libraries: ${CUDA_LIBRARIES}")
    message(STATUS "    include path: ${CUDA_INCLUDE_DIRS}")
  endif(CUDA_FOUND)
endif()

find_package(OpenCV 4.6 REQUIRED)
message("${BoldGreen}" "OpenCV library status:" "${ColourReset}")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

find_package(GDAL REQUIRED)
message("${BoldGreen}" "GDAL library status:" "${ColourReset}")
message(STATUS "    version: ${GDAL_VERSION}")
message(STATUS "    libraries: ${GDAL_LIBRARY}")
message(STATUS "    include path: ${GDAL_INCLUDE_DIRS}")

find_package(Boost 1.74 REQUIRED)
if ( NOT Boost_FOUND )
  message(WARNING "Boost library not found! Stopping...")
  return()  
else()
  message("${BoldGreen}" "Boost library status:" "${ColourReset}")
  message(STATUS "    version: ${Boost_VERSION}")
  message(STATUS "    libraries: ${Boost_LIBRARIES}")
  message(STATUS "    include path: ${Boost_INCLUDE_DIRS}")
endif()

find_package(CGAL REQUIRED)
if ( NOT CGAL_FOUND )
  message(WARNING "Required CGAL library not found! Stopping...")
  return()  
endif()

message("${BoldGreen}" "CGAL library status:" "${ColourReset}")
message(STATUS "    libraries: ${CGAL_LIBRARY}")
message(STATUS "    include path: ${CGAL_INCLUDE_DIR}")
include( ${CGAL_USE_FILE} )

set (GEOTIFF_MODULE external/geotiff)

file(GLOB PROJECT_HEADERS include/*.h include/*.hpp)

include_directories(BEFORE ../include
                    include
                    ${OpenCV_INCLUDE_DIRS}
                    ${GDAL_INCLUDE_DIRS}
                    ${GEOTIFF_MODULE}/include)
          
include_directories(AFTER ${YAML_DIR}/include)

add_definitions(-DCGAL_USE_BASIC_VIEWER)

execute_process(
    COMMAND git describe --tags
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE PROJECT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_definitions(-DPROJECT_VERSION="${PROJECT_VERSION}")

add_executable(land       src/land.cpp
                          src/lad_analysis.cpp
                          ${GEOTIFF_MODULE}/src/geotiff.cpp
                          src/lad_core.cpp
                          src/lad_layer.cpp
                          src/lad_processing.cpp
                          src/lad_thread.cpp
                          src/lad_config.cpp
                          ${PROJECT_HEADERS})

add_executable(tiff2rugosity   src/tiff2rugosity.cpp
                          src/lad_analysis.cpp
                          ${GEOTIFF_MODULE}/src/geotiff.cpp
                          src/lad_core.cpp
                          src/lad_layer.cpp
                          src/lad_processing.cpp
                          src/lad_thread.cpp
                          src/lad_config.cpp
                          ${PROJECT_HEADERS})

add_executable(img.resample   src/img.resample.cpp
                          src/lad_analysis.cpp
                          ${GEOTIFF_MODULE}/src/geotiff.cpp
                          src/lad_core.cpp
                          src/lad_layer.cpp
                          src/lad_processing.cpp
                          src/lad_thread.cpp
                          ${PROJECT_HEADERS})

find_library (YAML_LIB yaml-cpp PATHS ${YAML_DIR}/lib64)
message (STATUS "Using yaml-cpp libraries from:" ${YAML_LIB})

target_compile_options(land         PUBLIC -std=c++17 -mavx2)
target_compile_options(img.resample PUBLIC -std=c++17 -mavx2)
target_link_libraries(tiff2rugosity ${OpenCV_LIBS} ${GDAL_LIBRARY} ${YAML_LIB})
target_link_libraries(img.resample  ${OpenCV_LIBS} ${GDAL_LIBRARY} ${YAML_LIB})
target_link_libraries(land      ${OpenCV_LIBS} ${GDAL_LIBRARY} ${YAML_LIB})

install (TARGETS land       DESTINATION $ENV{HOME}/bin)
install (TARGETS tiff2rugosity  DESTINATION $ENV{HOME}/bin)
install (TARGETS img.resample   DESTINATION $ENV{HOME}/bin)
