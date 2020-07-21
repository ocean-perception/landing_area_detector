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
#ifndef _LAD_LAYER_HPP_
#define _LAD_LAYER_HPP_

#include "headers.h"
#include "lad_enum.hpp"

using namespace std; // STL
using namespace cv;  // OpenCV

/**
 * @brief Extend <lad> namespace with basic Layer class, and extended Raster, Kernel and Vector layer classes 
 * 
 */
namespace lad
{ //!< landing area detection algorithm namespace

    class Layer
    {
    private:
        int layerID;     //!< Layer identifier that can be used as UID in multilayer implementations
        int layerStatus; //!< Status identifier: takes a value from enumerated list of layer status.
        int layerType;   //!< Layer type identifier: takes a value from enumerated list of layer types

    protected:
    public:
        std::string layerName;     //!<  Layer name (mandatory)
        std::string fileName; //!< Name of associated output file (optional)
        std::string filePath; //!< Name of associated output filepath (optional)

        /**
             * @brief Construct a new Layer object
             * 
             * @param newName Optional layer name. If none is specified at construction time, DEFAULT_LAYER_NAME will be assigned  
             */
        Layer(std::string newName = lad::DEFAULT_LAYER_NAME, int id = lad::LAYER_INVALID_ID, int type = lad::LAYER_UNDEFINED)
        {
            // \todo Create empty constructors were no value is defined
            layerName = newName;
            layerID = id;
            layerType = type;
            layerStatus = lad::LAYER_INVALID;
        }
        /**
             * @brief Virtual destructor of the Layer object. 
             * @details  Depending on the type of container in the inherited instances, a type specific cleanup may be required
             */
        virtual ~Layer()
        {
        }

        int getID();                    //!< Return the layer ID
        int setID(int newID);           //!< set the new layer ID. It must be a valid ID
        int getStatus();                //!< Return a copy of the layer status
        int setStatus(int newStatus);   //!< Modify the layer status
        int getType();                  //!< Return a copy of the layer type
        int setType(int newType);       //!< Modify the layer type
        virtual void showInformation(); //!< Dumps relevant information of the layer

        virtual int clear();       //!< Clear all the layer specific information, and stored data if any.
        virtual int copy(Layer);   //!< Copy the content of a Layer into the current one. Derived classes must perform container specific deep-copy
        virtual int copy(Layer *); //!< Overloaded version for pointer to Layer type

        // Check if they need to be virtual
        /*
        int clone();
        int copy();
        int create();
        //*/
    };

    class RasterLayer : public Layer
    {
    public:
        // this should interface with OpenCV Mat and 2D matrix (vector style)
        // \todo check if size/type must/can be updated at construction time
        cv::Mat rasterData; //OpenCV matrix that will hold the data

        RasterLayer(std::string name, int id) : Layer(name, id)
        {
            setType(LAYER_RASTER); // This can be done passing LAYER_VECTOR as 3rd argument of the constructor
            // rasterData =
        }

        int writeLayer(std::string outputFilename, int fileFormat, Geotiff *geotiff, int outputCoordinate, double *apMatrix); //!< Overloaded method of exporting vectorData to user defined file
        void showInformation();
        int loadData(cv::Mat *);
    };

    class VectorLayer : public Layer
    {
    public:
        // this should interface with both GDAL and CGAL containers
        vector<cv::Point2d> vectorData; //!<Vector of 2D points defining vectorized layer (e.g. bounding polygon)
        int coordinateSpace;            //!< Defines if the data stored in vectorData is world coordinates or pixel coordinates

        VectorLayer(std::string name, int id) : Layer(name, id)
        {
            coordinateSpace = PIXEL_COORDINATE;
            setType(LAYER_VECTOR); // This can be done passing LAYER_VECTOR as 3rd argument of the constructor
        }

        void showInformation();

        int loadData(std::vector<cv::Point2d> *); //!< Import data into vectorData container
        // int writeLayer(int fileFormat = FMT_CSV); //!< export vectorData to the fileName as fileFormat (default CSV)
        int writeLayer(std::string outputFilename = NULL, int fileFormat = FMT_CSV, 
                       std::string strWKTSpatialRef = "", int outputCoordinate = WORLD_COORDINATE, double *apMatrix = nullptr); //!< Overloaded method of exporting vectorData to user defined file
        int convertSpace(int newSpace, double *apTransformMatrix);                                                                                                                        //!< Convert vectorData content to new coordinate space
    };

    class KernelLayer : public virtual RasterLayer
    {
    private:
            double dRotation; //!< Rotation angle of the given kernel (in radians)

    public:
        cv::Mat rotatedData; //OpenCV matrix that will hold the data

        void showInformation();
        void setRotation(double);
        double getRotation();
        

        KernelLayer(std::string name, int id, double rot = 0) : RasterLayer(name, id)
        {
            dRotation = rot;
            setType(LAYER_KERNEL);
        }
    };

    int exportShapefile(std::string filename, std::string layerName, std::vector<Point2d> data, std::string strWKTSpatialRef);

} // namespace lad

#endif // _LAD_LAYER_HPP_