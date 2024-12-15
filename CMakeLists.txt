cmake_minimum_required(VERSION 3.23)

# ---------------------------------------
# Project setup
# ---------------------------------------
project(lad_project
    VERSION 4.0.1 # Fallback version if git describe fails
    LANGUAGES CXX
    DESCRIPTION "Generalized Landing Area Detection for field robotics application"
)

# Set C++ standard and require it
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

add_compile_definitions(CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")

# ---------------------------------------
# Options
# ---------------------------------------
option(USE_CUDA "Enable CUDA support" OFF)
option(FORCE_COLORED_OUTPUT "Force colored output" OFF)

# ---------------------------------------
# Version and Git commit retrieval
# ---------------------------------------
find_package(Git QUIET)
if(Git_FOUND)
    # Try to get the project version from git tags following "vX.Y.Z"
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --match "v[0-9]*" --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(GIT_TAG_VERSION)
        # Remove leading 'v' if present
        # TODO: Not replacing the project version with the latest tag
        # string(REGEX REPLACE "^v" "" PROJECT_VERSION "${GIT_TAG_VERSION}")
        # Get short commit hash
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    else()
        # If no version tag found, fallback to the initial PROJECT_VERSION from project()
        set(GIT_COMMIT_HASH "unknown")
    endif()
else()
    message(WARNING "Git not found. Using fallback version ${PROJECT_VERSION}")
    set(GIT_COMMIT_HASH "unknown")
endif()

# Update the project version and define preprocessor macros
add_compile_definitions(PROJECT_VERSION="${PROJECT_VERSION}")
add_compile_definitions(PROJECT_GIT_COMMIT="${GIT_COMMIT_HASH}")

# Configure version header (not implemented yet)
# configure_file(
#     ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
#     ${CMAKE_CURRENT_BINARY_DIR}/version.h
# )

# ---------------------------------------
# Colors (non-Windows)
# ---------------------------------------
if(NOT WIN32)
    string(ASCII 27 Esc)
    set(ColourReset "${Esc}[m")
    set(ColourBold  "${Esc}[1m")
    set(BoldGreen   "${Esc}[1;32m")
    set(BoldBlue    "${Esc}[1;34m")
    set(ColourReset "${Esc}[m")
endif()

# ---------------------------------------
# Compiler and Linker Flags
# Centralized definition
# ---------------------------------------
set(COMMON_CXX_FLAGS "-fopenmp -pthread -mavx2 -O3 -g")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Additional GCC-specific flags if needed
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Additional Clang-specific flags if needed
endif()

if(FORCE_COLORED_OUTPUT)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -fdiagnostics-color=always")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -fcolor-diagnostics")
    endif()
endif()

# Apply flags globally
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")

# ---------------------------------------
# Dependencies
# ---------------------------------------
find_package(OpenMP REQUIRED)
find_package(OpenCV 4.6 REQUIRED)
find_package(GDAL REQUIRED)
find_package(Boost 1.74 REQUIRED)
find_package(CGAL REQUIRED)

if(USE_CUDA)
    find_package(CUDA)
    if(CUDA_FOUND)
        add_definitions(-DUSE_CUDA=1)
        message("${BoldBlue}CUDA found:${ColourReset}")
        message(STATUS "    toolkit root dir: ${CUDA_TOOLKIT_ROOT_DIR}")
        message(STATUS "    version: ${CUDA_VERSION}")
        message(STATUS "    libraries: ${CUDA_LIBRARIES}")
        message(STATUS "    include path: ${CUDA_INCLUDE_DIRS}")
    endif()
endif()

# External geotiff module path
set(GEOTIFF_MODULE external/geotiff)

message("${BoldGreen}OpenCV library status:${ColourReset}")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

message("${BoldGreen}GDAL library status:${ColourReset}")
message(STATUS "    version: ${GDAL_VERSION}")
message(STATUS "    libraries: ${GDAL_LIBRARY}")
message(STATUS "    include path: ${GDAL_INCLUDE_DIRS}")

message("${BoldGreen}Boost library status:${ColourReset}")
message(STATUS "    version: ${Boost_VERSION}")
message(STATUS "    libraries: ${Boost_LIBRARIES}")
message(STATUS "    include path: ${Boost_INCLUDE_DIRS}")

message("${BoldGreen}CGAL library status:${ColourReset}")
message(STATUS "    include path: ${CGAL_INCLUDE_DIR}")

include(${CGAL_USE_FILE})

# ---------------------------------------
# Include directories
# ---------------------------------------
# Use target_include_directories later for targets
set(YAML_DIR "/path/to/yaml") # Adjust as needed
find_library(YAML_LIB yaml-cpp PATHS ${YAML_DIR}/lib64)
message(STATUS "Using yaml-cpp libraries from: ${YAML_LIB}")

# ---------------------------------------
# Source files
# ---------------------------------------
file(GLOB PROJECT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp)

set(SOURCES_COMMON
    src/lad_analysis.cpp
    ${GEOTIFF_MODULE}/src/geotiff.cpp
    src/lad_core.cpp
    src/lad_layer.cpp
    src/lad_processing.cpp
    src/lad_thread.cpp
    src/lad_config.cpp
    ${PROJECT_HEADERS}
)

# ---------------------------------------
# Executables
# ---------------------------------------
add_executable(land src/land.cpp ${SOURCES_COMMON})
add_executable(tiff2rugosity src/tiff2rugosity.cpp ${SOURCES_COMMON})
add_executable(img.resample src/img.resample.cpp ${SOURCES_COMMON})

# ---------------------------------------
# Target properties and linking
# ---------------------------------------
# Set common properties via a function or directly
foreach(_tgt land tiff2rugosity img.resample)
    target_include_directories(${_tgt} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
        ${GDAL_INCLUDE_DIRS}
        ${GEOTIFF_MODULE}/include
        ${YAML_DIR}/include
    )
    target_link_libraries(${_tgt} PRIVATE ${OpenCV_LIBS} ${GDAL_LIBRARY} ${YAML_LIB} ${Boost_LIBRARIES})
    target_compile_features(${_tgt} PUBLIC cxx_std_17)
endforeach()

# tiff2rugosity and img.resample do not differ in dependencies from land in this example,
# but if they do, adjust as needed.

# ---------------------------------------
# Installation
# ---------------------------------------
install(TARGETS land tiff2rugosity img.resample DESTINATION $ENV{HOME}/bin)

message("${BoldGreen}Configuration complete.${ColourReset}")
message(STATUS "Project version: ${PROJECT_VERSION}")
message(STATUS "Commit hash: ${GIT_COMMIT_HASH}")
