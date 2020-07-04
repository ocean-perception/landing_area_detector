/**
 * @file headers.h
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief Single collection of global libraries required in major modules
 * @version 0.1
 * @date 2020-07-03
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef _HEADERS_H_

#define _HEADERS_H_
///Basic C and C++ libraries
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cmath>
#include <stdexcept>
#include <vector>

/// OpenCV libraries. May need review for the final release
#include <opencv2/core.hpp>
#include "opencv2/core/ocl.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <opencv2/video.hpp>
#include <opencv2/features2d.hpp>
#include "opencv2/calib3d.hpp"
#include <opencv2/xfeatures2d.hpp>

/// CUDA specific libraries
#if USE_GPU
    #include <opencv2/cudafilters.hpp>
    #include "opencv2/cudafeatures2d.hpp"
    #include "opencv2/xfeatures2d/cuda.hpp"
#endif

const std::string green("\033[1;32m");
const std::string yellow("\033[1;33m");
const std::string cyan("\033[1;36m");
const std::string red("\033[1;31m");
const std::string reset("\033[0m");

#define DEFAULT_OUTPUT_FILE "output.tif"

#endif