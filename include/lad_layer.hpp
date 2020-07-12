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

// pragma once is not "standard"
#ifndef _LAD_LAYER_HPP_ 
#define _LAD_LAYER_HPP_

#include "headers.h"
#include "lad_enum.hpp"

using namespace std;    // STL
using namespace cv;     // OpenCV

/**
 * @brief Extend <lad> namespace with basic Layer class, and extended Raster, Kernel and Vector layer classes 
 * 
 */
namespace lad{      //!< landing area detection algorithm namespace

    class Layer{
        private:
            int layerID;    // Layer identifier
            int layerStatus; //!< Status identifier: takes a value from enumerated list of possible status. bValid valid flag deprecated
            int layerType;  //!< Layer type identifier, takes a possibe value from enumerated list of layer types

        protected:

        public:
            std::string layerName;  //!<  Layer name
            std::string layerFileName;  //!< Name of associated output file
            std::string layerFilePath;  //!< Name of associated output filepath (optional)

            /**
             * @brief Construct a new Layer object
             * 
             * @param newName Optional layer name. "noname" if none is specified at construction time
             */
            Layer(std::string newName = "noname", int id = lad::LAYER_INVALID_ID, int type = lad::LAYER_UNDEFINED){
                layerName = newName;
                layerID = id; 
                layerType = type;
                layerStatus = lad::LAYER_INVALID;
            }
            /**
             * @brief Virtual destructor of the Layer object. 
             * @details  Depending on the type of container in the inherited instances, a type specific cleanup may be required
             */
            virtual ~Layer(){
            }

            int getID();    //!< Return the layer ID 
            int setID(int newID); //!< set the new layer ID. It must be a valid ID
            int getLayerStatus(); //!< Return a copy of the layer status
            int setLayerStatus(int newStatus); //!< Modify the layer status
            int getLayerType(); //!< Return a copy of the layer type
            int setLayerType(int newType); //!< Modify the layer type
            virtual void showInformation(); //!<< Dumps relevant information of the layer
            //virtual int loadData(void *); //!< Virtual declaration of function to upload data into the appropiate container inside of the layer
            //virtual int writeLayer(std::string newname);    //!< Virtual declaration of export method for base layer class. To be expanded on each derived class
    };

    class RasterLayer: public Layer{
        public:
            // this should interface with OpenCV Mat and 2D matrix (vector style)
            cv::Mat rasterData; //OpenCV matrix that will hold the data

            RasterLayer(std::string name, int id):Layer(name, id){
                setLayerType(LAYER_RASTER); // This can be done passing LAYER_VECTOR as 3rd argument of the constructor
                // rasterData = 
            }

            void showInformation();
            int loadData(cv::Mat *);
    };

    class VectorLayer: public Layer{
        public:
            // this should interface with both GDAL and CGAL containers
            vector <cv::Point2d> vectorData; //Vector of 2D points defining vectorized layer (e.g. bounding polygon)

            VectorLayer(std::string name, int id):Layer(name, id){
                setLayerType(LAYER_VECTOR); // This can be done passing LAYER_VECTOR as 3rd argument of the constructor
            }

            void showInformation();

            int loadData(std::vector <cv::Point2d> *); //!< Import data into vectorData container
            // int writeLayer(int fileFormat = FMT_CSV); //!< export vectorData to the layerFileName as fileFormat (default CSV)
            int writeLayer(std::string outputFilename = NULL, int fileFormat = FMT_CSV); //!< Overloaded method of exporting vectorData to user defined file 
    };

    class KernelLayer: public virtual RasterLayer{
        public:
            double dRotation; //!< Rotation angle of the given kernel (in radians)
            void showInformation();

            KernelLayer(std::string name, int id):RasterLayer(name, id){
                dRotation = 0;
                setLayerType(LAYER_KERNEL);
            }

    };

}

#endif // _LAD_LAYER_HPP_ 