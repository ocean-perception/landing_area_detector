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

#include <regex>

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

            std::string getLayerName (int id); //!< Returns name of Layer with given ID number

            int getLayerID (std::string name); //!< Return layer with given name

            int setLayerName (int id, std::string newName); //!< Overwrite Layers name using is ID

            int CreateLayer (std::string name, int type); //!< Create a new layer "name" of given type and insert it into the pipeline stack.

            int InsertLayer (std::shared_ptr <Layer> layer);  //!< Insert previously created layer into the pipeline stack

            int RemoveLayer (std::string name); //!< Remove layer by its name
            int RemoveLayer (int ID);           //!< Remove layer by its ID

            int getTotalLayers (int type = LAYER_ANYTYPE); //!< Return the total number of layer ot a given type in the stack

            int isValidName(std::string name);  //!< Verify if "name" is a valid layer name for the current pipeline stack
            int isValidID(int ID); //!< Verify is ID is a valid layer ID for the current pipeline stack

            int getValidID();   //!< Return a valid ID available for the current stack

            int showLayers(); //!< Call showInformation() method for each layer
    };

}

#endif // _LAD_CORE_HPP_ 