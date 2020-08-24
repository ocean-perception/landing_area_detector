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
#include "lad_processing.hpp"
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
            inputFileTIFF = "";
            verbosity = NO_VERBOSE;
            currentAvailableID = 0;
            useNodataMask = false;
        }

        ~Pipeline()
        {
            mapLayers.clear();
            inputFileTIFF = "";
        }

        std::string geoProjection;
        double      geoTransform[6]; //!< 6 DOF geotransformation matrix
        std::string inputFileTIFF; //< Input TIFF filename containing base bathymetry. It can be used as base name for output products files
        int         verbosity; //!< Verbosity levels (from 0 to 2)
        int         useNodataMask;

        // Methods **************

        int setTemplate (std::string); // Use raster layer as template for geoTIFF structure
        int readTIFF(std::string inputFile, std::string rasterLayer, std::string maskLayer); // Read a given geoTIFF file an loads into current container

        std::string getLayerName(int id);              // Returns name of Layer with given ID number
        int setLayerName(int id, std::string newName); // Overwrite Layers name using is ID

        int getLayerID(std::string name);            // Return the ID layer identified by its name
        int setLayerID(std::string name, int newID); // Modify the ID of layer identified by its name.

        std::shared_ptr<Layer> getLayer(int id);           // Return shared_ptr to a Layer identified by its ID
        std::shared_ptr<Layer> getLayer(std::string name); // Return shared_ptr to a Layer identified by its name

        int createLayer(std::string name, int type);   // Create a new layer "name" of given type and insert it into the pipeline stack.
        // int insertLayer(std::shared_ptr<Layer> layer); // Insert externally created layer into the pipeline stack

        int removeLayer(std::string name); // Remove layer by its name
        int removeLayer(int ID);           // Remove layer by its ID

        int exportLayer(std::string name, std::string outfile = "", int format = FMT_CSV, int coords = NO_COORDINATE); // Export a given layer in the stack identified by its name, to

        int getTotalLayers(int type = LAYER_ANYTYPE); // Return the total number of layer ot a given type in the stack

        int isValidName(std::string name); // Verify if "name" is a valid layer name for the current pipeline stack
        int isValidID(int ID);             // Verify is ID is a valid layer ID for the current pipeline stack
        int isValid(int);         // Returs true if the provided ID is valid. It does not check whether it is available in the current stack
        int isValid(std::string); // Returs true if the provided NAME is valid. It does not check whether it is available in the current stack

        int getValidID(); // Return a valid ID available for the current stack

        int isAvailable(int);         // Return true if the provided ID is not taken in the current stack. It's validity is assumed but not verified
        int isAvailable(std::string); // Return true if the provided NAME is not taken in the current stack. It's validity is assumed but not verified

        int showInfo(int level = 0);              // Show summary information of current pipeline object
        int showLayers(int type = LAYER_ANYTYPE); // Call showInformation() method for each layer
        int showImage(std::string layer, int colormap = cv::COLORMAP_HOT); // show, if available, the raster content of the layer as an image
        int saveImage(std::string layer, std::string filename, int colormap = cv::COLORMAP_HOT); // save as image file the raster content of the layer

        int uploadData(int id, void *data);           // uploads data into a layer identified by its id
        int uploadData(std::string name, void *data); // uploads data into a layer identified by its name

        int copyMask (std::string src, std::string dst); // propagates rasterMask from src to dst
        int maskLayer(std::string src, std::string mask, std::string dst, int useRotated = true); // apply mask to raster layer src and store it in dst layer
        int rotateLayer(std::string src, double angle); // rotate a kernel layer a given angle
        
        int createKernelTemplate (std::string name, double width, double length, int morphtype = cv::MORPH_RECT); // Create a Kernel layer using a rectangular shape as template. Uses implicit values for pixel scale
        int createKernelTemplate (std::string name, double width, double length, double sx, double sy, int morphtype = cv::MORPH_RECT); // Create a Kernel layer using a rectangular shape as template
        int processGeotiff(std::string dataName, std::string maskName, int showImage = false); // Process Geotiff object and generate correspondig data and mask raster layers
        int extractContours(std::string rasterName, std::string contourName, int showImage = false);
        int computeExclusionMap(std::string src, std::string kernel, std::string dst);
        int computeMeanSlopeMap(std::string src, std::string kernel, std::string mask, std::string dst);
        int lowpassFilter      (std::string src, std::string kernel, std::string mask, std::string dst); // apply lowpass filter to input raster Layer and stores the resulting raster in dst Layer
        int applyWindowFilter  (std::string src, std::string kernel, std::string mask, std::string dst, int filtertype);
        int computeHeight      (std::string src, std::string filt, std::string dst);

        int lowpassFilter(std::string src, std::string dst, cv::Size filterSize, int filterType = 0); // apply lowpass filter to input raster Layer and stores the resulting raster in dst Layer
        int computeHeight(std::string src, std::string dst, cv::Size filterSize, int filterType = 0);

        int computeFinalMap(std::string src1, std::string src2, std::string src3, std::string dst);

        int compareLayer(std::string src, std::string dst, double threshold, int cmpop); // apply scalar threshold to src raster and store resulting raster in dst layer
        int generatePlaneMap (std::string src, KPlane plane, std::string templ);
    };


    class tictac{
        private:
        protected:
        public:
            int64 start_time;
            int64 stop_time;
            int64 last_lap;
            std::string message;

            /**
             * @brief Construct a new tictac object
             * 
             */
            tictac(){
                start_time = 0;
                stop_time = 0;
                last_lap = 0;
            }
            
            /**
             * @brief Destroy the tictac object
             * 
             */
            ~tictac(){

            }

            void  start();
            void  stop();
            void  clear();
            void  show();
            int64 elapsed();
            void lap(std::string);
    };

} // namespace lad
#endif // _LAD_CORE_HPP_