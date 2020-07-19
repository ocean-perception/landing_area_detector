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

#include "headers.h"
#include "geotiff.hpp"
#include "lad_layer.hpp"
#include "lad_enum.hpp"

#include <regex>

using namespace std; // STL
using namespace cv;  // OpenCV

/**
 * @brief Protoype definition of base namespace lad
 * 
 */
namespace lad
{ //!< landing area detection algorithm namespace

    class Pipeline
    {
    private:
        // int pipelineStep;
        // int bValidInput;

    public:
        Pipeline()
        {
            apInputGeotiff = nullptr;
            inputFileTIFF = "";
            verbosity = NO_VERBOSE;
            LUT_ID.resize(DEFAULT_STACK_SIZE);
            std::fill(LUT_ID.begin(), LUT_ID.end(), ID_AVAILABLE);
        }

        ~Pipeline()
        {
            // delete apInputGeotiff; //!< Removed because Geotiff object must be destroyed using separate method
            Layers.clear();
            LUT_ID.clear();
            inputFileTIFF = "";
        }

        Geotiff *apInputGeotiff; //!< Pointer to geoTIFF container

        std::string inputFileTIFF; //!< Input TIFF filename containing base bathymetry. Base name for output products files

        vector<std::shared_ptr<Layer>> Layers; //!< Collection of layers. Using smart shared pointers for tree-like pipeline structures
        vector<int> LUT_ID; //!< Look-up table of Layer ID's. Intended to speed-up ID validation and retrieval
        int verbosity;
        // Methods **************

        int readTIFF(std::string inputFile); //!< Read a given geoTIFF file an loads into current container

        std::string getLayerName(int id);              //!< Returns name of Layer with given ID number
        int setLayerName(int id, std::string newName); //!< Overwrite Layers name using is ID

        int getLayerID(std::string name);            //!< Return the ID layer identified by its name
        int setLayerID(std::string name, int newID); //!< Modify the ID of layer identified by its name.

        std::shared_ptr<Layer> getLayer(int id);           //!< Return shared_ptr to a Layer identified by its ID
        std::shared_ptr<Layer> getLayer(std::string name); //!< Return shared_ptr to a Layer identified by its name

        int createLayer(std::string name, int type);   //!< Create a new layer "name" of given type and insert it into the pipeline stack.
        int InsertLayer(std::shared_ptr<Layer> layer); //!< Insert previously created layer into the pipeline stack

        int removeLayer(std::string name); //!< Remove layer by its name
        int removeLayer(int ID);           //!< Remove layer by its ID

        int exportLayer(std::string name, std::string outfile = "", int format = FMT_CSV, int coords = NO_COORDINATE); //!< Export a given layer in the stack identified by its name, to

        int getTotalLayers(int type = LAYER_ANYTYPE); //!< Return the total number of layer ot a given type in the stack

        int isValidName(std::string name); //!< Verify if "name" is a valid layer name for the current pipeline stack
        int isValidID(int ID);             //!< Verify is ID is a valid layer ID for the current pipeline stack

        int isValid(int);         //!< Returs true if the provided ID is valid. It does not check whether it is available in the current stack
        int isValid(std::string); //!< Returs true if the provided NAME is valid. It does not check whether it is available in the current stack

        int isAvailable(int);         //!< Return true if the provided ID is not taken in the current stack. It's validity is assumed but not verified
        int isAvailable(std::string); //!< Return true if the provided NAME is not taken in the current stack. It's validity is assumed but not verified

        int getValidID(); //!< Return a valid ID available for the current stack

        int showInfo(int level = 0);              //!< Show summary information of current pipeline object
        int showLayers(int type = LAYER_ANYTYPE); //!< Call showInformation() method for each layer

        int uploadData(int id, void *data);           //!< uploads data into a layer identified by its id
        int uploadData(std::string name, void *data); //!< uploads data into a layer identified by its name

        int processGeotiff(std::string dataName, std::string maskName, int showImage = false); //!< Process Geotiff object and generate correspondig data and mask raster layers
        int extractContours(std::string rasterName, std::string contourName, int showImage = false);
    };
} // namespace lad
#endif // _LAD_CORE_HPP_