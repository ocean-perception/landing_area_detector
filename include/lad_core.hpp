/**
 * @file lad_core.hpp
 * @author Jose Cappelletto (j.cappelletto@soton.ac.uk)
 * @brief Core module of Landing Area Detection (lad) algorithm
 * @version 0.1
 * @date 2020-07-03
 * 
 * @copyright Copyright (c) 2020
 * 
 */

// pragma once is not "standard"
#ifndef _LAD_CORE_HPP_ 
#define _LAD_CORE_HPP_

#include "geotiff.hpp"
#include "headers.h"
#include "lad_layer.hpp"
#include "lad_enum.hpp"

using namespace std;    // STL
using namespace cv;     // OpenCV

/**
 * @brief Protoype definition of base namespace lad
 * 
 */
namespace lad{      //!< landing area detection algorithm namespace


    class ladPipeline{
        private:
            int pipelineStep;
            int bValidInput;

        public:
            ladPipeline(){
                apInputGeotiff = NULL;
                inputFileTIFF = "";
            }

            Geotiff *apInputGeotiff;    //!< landing area detection algorithm namespace

            std::string inputFileTIFF;  //!< Input TIFF filename containing base bathymetry. Base name for output products files

            vector <std::shared_ptr <Layer>> Layers; //!< Collection of layers. Using smart shared pointers for tree-like pipeline structures 

            int ReadTIFF (std::string inputFile);   //!< Read a given geoTIFF file an loads into current container

            std::string GetLayerName (int id); //!< Returns name of Layer with given ID number

            int GetVectorID (std::string name); //!< Return first vector that matches 'name' as layer name

            int SetLayerName (int id, std::string newName); //!< Overwrite Layers name using is ID

    };

}

#endif // _LAD_CORE_HPP_ 