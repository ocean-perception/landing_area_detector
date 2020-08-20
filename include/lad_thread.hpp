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

// pragma once is not "standard", so we use ifndef guards
#ifndef _LAD_THREAD_HPP_
#define _LAD_THREAD_HPP_

#include "headers.h"
#include "lad_core.hpp"
#include "lad_enum.hpp"

using namespace std; // STL
using namespace cv;  // OpenCV

namespace lad{

    /**
     * @brief Computes the sequence of maps related to lane (A): Detailed slope (low&high), with exclusion maps
     * 
     * @param ap Pointer to Pipeline object containing a valid stack for processing
     * @param param Pointer to structure containing all the parameters (interested in slope for this lane)
     * @return int error code, if any
     */
    int processLaneA(lad::Pipeline *ap, parameterStruct *param);

    /**
     * @brief Computes the sequence of maps related to lane (B): Low pass terrain map, Terrain height map
     * 
     * @param ap Pointer to Pipeline object containing a valid stack for processing
     * @param param Pointer to structure containing all the parameters (interested in slope for this lane)
     * @return int error code, if any
     */
    int processLaneB(lad::Pipeline *ap, parameterStruct *param);

    /**
     * @brief Computes the sequence of maps related to lane (C): Mean slope, with exclusion maps
     * 
     * @param ap Pointer to Pipeline object containing a valid stack for processing
     * @param param Pointer to structure containing all the parameters (interested in slope for this lane)
     * @return int error code, if any
     */
    int processLaneC(lad::Pipeline *ap, parameterStruct *param);

    /**
     * @brief Computes the maps correspondnig to lane (D): Hi and Lo Protrusion maps
     * 
     * @param ap Pointer to Pipeline object containing a valid stack for processing
     * @param param Pointer to structure containing all the parameters (interested in slope for this lane)
     * @return int error code, if any
     */
    int processLaneD(lad::Pipeline *ap, parameterStruct *param);

}


#endif //_LAD_THREAD_HPP_