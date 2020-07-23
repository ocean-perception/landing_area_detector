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
 * @brief Prototype definition of base namespace <lad>
 * 
 */
namespace lad
{ 
    class Pipeline
    {
    private:
        std::map <std::string, std::shared_ptr<Layer>> mapLayers;
        int currentAvailableID;

    public:
        Pipeline() //!< Default contructor
        {
            apInputGeotiff = nullptr;
            inputFileTIFF = "";
            verbosity = NO_VERBOSE;
            currentAvailableID = 0;
        }

        ~Pipeline()
        {
            mapLayers.clear();
            inputFileTIFF = "";
        }

        Geotiff *apInputGeotiff; //< Pointer to geoTIFF container
        std::string inputFileTIFF; //< Input TIFF filename containing base bathymetry. It can be used as base name for output products files
        int verbosity; //!< Verbosity levels (from 0 to 2)

        // Methods **************

        int readTIFF(std::string inputFile); // Read a given geoTIFF file an loads into current container

        std::string getLayerName(int id);              // Returns name of Layer with given ID number
        int setLayerName(int id, std::string newName); // Overwrite Layers name using is ID

        int getLayerID(std::string name);            // Return the ID layer identified by its name
        int setLayerID(std::string name, int newID); // Modify the ID of layer identified by its name.

        std::shared_ptr<Layer> getLayer(int id);           // Return shared_ptr to a Layer identified by its ID
        std::shared_ptr<Layer> getLayer(std::string name); // Return shared_ptr to a Layer identified by its name

        int createLayer(std::string name, int type);   // Create a new layer "name" of given type and insert it into the pipeline stack.
        int InsertLayer(std::shared_ptr<Layer> layer); // Insert previously created layer into the pipeline stack

        int removeLayer(std::string name); // Remove layer by its name
        int removeLayer(int ID);           // Remove layer by its ID

        int exportLayer(std::string name, std::string outfile = "", int format = FMT_CSV, int coords = NO_COORDINATE); // Export a given layer in the stack identified by its name, to

        int getTotalLayers(int type = LAYER_ANYTYPE); // Return the total number of layer ot a given type in the stack

        int isValidName(std::string name); // Verify if "name" is a valid layer name for the current pipeline stack
        int isValidID(int ID);             // Verify is ID is a valid layer ID for the current pipeline stack

        int isValid(int);         // Returs true if the provided ID is valid. It does not check whether it is available in the current stack
        int isValid(std::string); // Returs true if the provided NAME is valid. It does not check whether it is available in the current stack

        int isAvailable(int);         // Return true if the provided ID is not taken in the current stack. It's validity is assumed but not verified
        int isAvailable(std::string); // Return true if the provided NAME is not taken in the current stack. It's validity is assumed but not verified

        int getValidID(); // Return a valid ID available for the current stack

        int showInfo(int level = 0);              // Show summary information of current pipeline object
        int showLayers(int type = LAYER_ANYTYPE); // Call showInformation() method for each layer

        int uploadData(int id, void *data);           // uploads data into a layer identified by its id
        int uploadData(std::string name, void *data); // uploads data into a layer identified by its name

        int createKernelTemplate (std::string name, double width, double length, double sx, double sy); // Create a Kernel layer using a rectangular shape as template
        int processGeotiff(std::string dataName, std::string maskName, int showImage = false); // Process Geotiff object and generate correspondig data and mask raster layers
        int extractContours(std::string rasterName, std::string contourName, int showImage = false);
        int computeExclusionMap(std::string raster, std::string kernel, std::string dst);
    };
} // namespace lad
#endif // _LAD_CORE_HPP_