/**
 * @file lad_core.hpp
 * @author Jose Cappelletto (j.cappelletto@soton.ac.uk)
 * @brief Core module of Landing Area Detection (lad) algorithm
 * @version 0.1
 * @date 2020-07-05
 * 
 * @copyright Copyright (c) 2020
 * 
 */

// pragma once is not "standard"
#ifndef _LAD_PROCESSING_HPP_
#define _LAD_PROCESSING_HPP_

#include "headers.h"
#include "lad_enum.hpp"

using namespace std; // STL
using namespace cv;  // OpenCV

/**
 * @brief Extend <lad> namespace with major processing tools for every type of layer 
 * 
 */
namespace lad
{ // landing area detection algorithm namespace

    int processGeotiff(std::string dataName, std::string maskName, int showImage = false); // Process Geotiff object and generate correspondig data and mask raster layers
    int extractContours(std::string rasterName, std::string contourName, int showImage = false);

    // Potential functions:

    int convertDataSpace(vector<cv::Point2d> *inputData, vector<cv::Point2d> *outputData, int inputSpace, int outputSpace, double *apTransform = nullptr);

    // CGAL implementations associated to convexHull / alphaShape // Terrain slope
    double computeMeanSlope (std::vector<cv::Point2d> inputPoints, cv::Vec3d normalVector){

    // Own implementation for scale independant landability analysis

    // Own implementation of rotation invariant landability detector

    // Pixel to World coordinates transformation
    // World to Pixel coordinates transformation
    // Already internally implemented with convertSpace + geoTransform matrix

} // namespace lad

#endif // _LAD_PROCESSING_HPP_