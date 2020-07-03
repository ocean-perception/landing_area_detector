/********************************************************************/
/* Project: Landing Area Detection algorithm						*/
/* Module: 	lad_core												*/
/* File: 	lad_core.hpp                                            */
/* Created:		03/07/2020                                          */
/* Description
*/

/********************************************************************/
/* Created by:                                                      */
/* Jose Cappelletto - j.cappelletto@soton.ac.uk		                */
/********************************************************************/
// pragma once is not "standard"
#ifndef _LAD_CORE_HPP_ 
#define _LAD_CORE_HPP_

#include "geotiff.hpp"
#include "headers.h"

using namespace std;    // STL
using namespace cv;     // OpenCV

/**
 * @brief Protoype definition of base namespace lad
 * 
 */
namespace lad{      //!< landing area detection algorithm namespace

    class LAD{
        private:
            int pipelineStep;
            int bValidInput;

        public:
            Geotiff *apInputGeotiff;    //!< landing area detection algorithm namespace
            vector <cv::Mat> RasterLayers;  //!< Vector of OpenCV raster images containing intermediate and final products
            // vector <vector> VectorLayers;  // TODO: define proper structure to store vector shapefiles
            vector <std::string> outputLayerNames;   //!< Vector containing name of each layer
            std::string inputFileTIFF;  //!< Input TIFF filename containing base bathymetry. Base name for output products files
    };
}

#endif // _LAD_CORE_HPP_ 