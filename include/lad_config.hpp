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
    typedef struct parameterStruct_{
        double robotHeight;
        double robotWidth;
        double robotLength;
        bool   fixRotation;
        double rotation;
        double rotationMin;
        double rotationMax;
        double rotationStep;
        double heightThreshold;
        double slopeThreshold;
        double groundThreshold;
        double protrusionSize;
        float  alphaShapeRadius;
        bool   maskBorder;
        bool   useNoDataMask;
        double defaultNoData;
        int    verbosity;
    }parameterStruct;

    void printParams(lad::parameterStruct *p);

    YAML::Node readConfiguration(std::string file, lad::parameterStruct *);

}

#endif 