/**
 * @file lad_config.hpp
 * @author Jose Cappelletto (j.cappelletto@soton.ac.uk)
 * @brief Core module of Landing Area Detection (lad) algorithm
 * @version 0.1
 * @date 2020-08-24
 * 
 * @copyright Copyright (c) 2020
 * 
 */
// pragma once is not "standard"
#ifndef _LAD_CONFIG_HPP_
#define _LAD_CONFIG_HPP_

#include "headers.h"
#include "lad_enum.hpp"
#include "lad_core.hpp"

#include <yaml-cpp/yaml.h>

namespace lad
{ // landing area detection algorithm namespace
    // structure that holds most of the relevant pipeline parameters
    typedef struct parameterStruct_{
        double robotHeight;     // robot height [m]
        double robotWidth;      // robot width [m] 
        double robotLength;     // robot length [m]
        double robotDiagonal;   // locally computed robot diagonal dimension, used for lowPassFilter of the terrain
        bool   fixRotation;     // flag indication if we are going to use a single LAUV heading [true]. If [false], pipeline operates in range mode
        double rotation;        // heading value [deg]
        double rotationMin;     // min heading value [deg] when in range mode
        double rotationMax;     // max heading value [deg] when in range mode
        double rotationStep;    // heading ange steps [deg] when in range mode
        double heightThreshold; // critical height [m] to separate Low Protrusions from High Protrusions 
        double slopeThreshold;  // critical slope [deg]
        double groundThreshold; // min. height [m] to consider a protrusion
        double protrusionSize;  // min. planar size [m] to consider a protrusion
        float  alphaShapeRadius;// radius [m] of alphaShape contour detection
        bool   maskBorder;      // indicate to mask the border of the map
        bool   useNoDataMask;   // indicate if we use rasterMask cv::Mat to mask the exported/visualized images (PNG) 
        double defaultNoData;   // redefine value of NODATA field when exporting geoTIFF files
        int    verbosity;       // define verbosity level [0,3]
        bool   exportIntermediate; // indicate to export intermediate layers of the pipeline, useful for detailed analysis. Default: true
        bool   exportRotated;   // indicate to export every rotation independent intermediate and final layer. Warning: can take up a lot of disk space. Default: false
    }parameterStruct;

    void printParams(lad::parameterStruct *p);

    YAML::Node readConfiguration(std::string file, lad::parameterStruct *);

    lad::parameterStruct getDefaultParams();

}

#endif 