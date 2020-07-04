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
#include "lad_enum.hpp"

using namespace std;    // STL
using namespace cv;     // OpenCV

/**
 * @brief Protoype definition of base namespace lad
 * 
 */
namespace lad{      //!< landing area detection algorithm namespace

    class Layer{
        private:

        protected:
            bool bValid;    // flag indicating that layer holds non-NULL valid data
            int layerID;    // Layer identifier
            int layerType;  // Layer type identifier (from enum)

        public:
            std::string layerName;  // Layer name
            std::string layerFileName;  // Suggested output file name

            /**
             * @brief Construct a new Layer object
             * 
             * @param newName Optional layer name. "noname" if none is specified at construction time
             */
            Layer(std::string newName = "noname"){
                bValid = false;
                layerName = newName;
                layerID = lad::LAYER_INVALID_ID; 
            }
            /**
             * @brief Virtual destructor of the Layer object. 
             * @details  Depending on the type of container in the inherited instances, a type specific cleanup may be required
             */
            virtual ~Layer(){
                bValid = false;
            }

            /**
             * @brief Gets the layer ID value. If newID param is provided, then it is used to update layer ID value
             * 
             * @param newID User provided new layer ID value. It must be positive, otherwise it will be ignored.
             * @return int  Up-to-date layer ID value
             */
            int LayerID(int newID=-1)
            {
                if (newID >= 0) layerID = newID; // if valid newID value is provided, update layerID. Ignore if negative
                return layerID;
            }
    };

    class classRasterLayer: public Layer{
        public:
            // this should interface with OpenCV Mat and 2D matrix (vector style)
            cv::Mat rasterData; //OpenCV matrix that will hold the data
    };

    class classVectorLayer: public Layer{
        public:
            // this should interface with both GDAL and CGAL containers
            vector <cv::Point2d> vectorData; //Vector of 2D points defining vectorized layer (e.g. bounding polygon)
            /**
             * @brief Destroy the class Vector Layer object
             * 
             */
            ~classVectorLayer(){
                // Just in case vector wasn't properly deallocated (it should once we are out of its scope!)
            }
    };

    class classKernelLayer: public virtual classRasterLayer{
        public:
            double dRotation; //!< Rotation angle of the given kernel (in radians)
    };

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
            vector <classRasterLayer> RasterLayers;  //!< Vector of OpenCV raster images with maps
            vector <classRasterLayer> KernelLayers;  //!< Vector of OpenCV raster images containing kernels
            vector <classVectorLayer> VectorLayers;  // TODO: define proper structure to store vector shapefiles
            std::string inputFileTIFF;  //!< Input TIFF filename containing base bathymetry. Base name for output products files

            int ReadTIFF (std::string inputFile);   //!< Read a given geoTIFF file an loads into current container
            std::string GetRasterLayerName (int id); //!< Returns name of RasterLayer with given ID number
            std::string GetKernelLayerName (int id); //!< Returns name of KernelLayer with given ID number
            std::string GetVectorLayerName (int id); //!< Returns name of VectorLayer with given ID number

            int GetRasterLayerID (std::string name); //!< Return first raster that matches 'name' as layer name
            int GetKernelLayerID (std::string name); //!< Return first kernel that matches 'name' as layer name
            int GetVectorLayerID (std::string name); //!< Return first vector that matches 'name' as layer name

    };

}

#endif // _LAD_CORE_HPP_ 