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
{ // landing area detection algorithm namespace

    class Layer
    {
    private:
        int layerID;     // Layer identifier that can be used as UID in multilayer implementations
        int layerStatus; // Status identifier: takes a value from enumerated list of layer status.
        int layerType;   // Layer type identifier: takes a value from enumerated list of layer types

    protected:
    public:
        double noDataValue;
        std::string layerName;     //  Layer name (mandatory)
        std::string fileName; // Name of associated output file (optional)
        std::string filePath; // Name of associated output filepath (optional)

        /**
         * @brief Base empty constructor. Other case-specific constructors are available
         * 
         */
        Layer(){
            layerName = "";
            layerID = 0;
            layerType = LAYER_ANYTYPE;
            layerStatus = LAYER_INVALID;
            noDataValue = 0.0;
        }

        /**
         * @brief Copy constructor from reference to a source Layer
         * 
         * @param src 
         */
        Layer(Layer *src){
            layerName = src->layerName;
            layerID = src->layerID;
            layerType = src->layerType;
            layerStatus = src->layerStatus;
            layerName = src->layerName;
            fileName = src->fileName;
            filePath = src->filePath;
            noDataValue = src->getNoDataValue();
        }

        /**
             * @brief Construct a new Layer object
             * 
             * @param newName Optional layer name. If none is specified at construction time, DEFAULT_LAYER_NAME will be assigned  
             */
        Layer(std::string newName = DEFAULT_LAYER_NAME, int id = LAYER_INVALID_ID, int type = LAYER_UNDEFINED)
        {
            // \todo Create empty constructors were no value is defined
            layerName = newName;
            layerID = id;
            layerType = type;
            layerStatus = LAYER_INVALID;
        }
        /**
             * @brief Virtual destructor of the Layer object. 
             * @details  Depending on the type of container in the inherited instances, a type specific cleanup may be required
             */
        virtual ~Layer()
        {
        }

        // operator = (shared_ptr<Layer>);

        int getID();                    // Return the layer ID
        int setID(int newID);           // set the new layer ID. It must be a valid ID
        int getStatus();                // Return a copy of the layer status
        int setStatus(int newStatus);   // Modify the layer status
        int getType();                  // Return a copy of the layer type
        int setType(int newType);       // Modify the layer type
        double  getNoDataValue(){ return noDataValue;}
        void    setNoDataValue(double newval){noDataValue = newval;}
        virtual void showInformation(); // Dumps relevant information of the layer

        virtual int clear();       // Clear all the layer specific information, and stored data if any.
        virtual int copy(Layer);   // Copy the content of a Layer into the current one. Derived classes must perform container specific deep-copy
        virtual int copy(Layer *); // Overloaded version for pointer to Layer type

        // Check if they need to be virtual
        /*
        int clone();
        int create();
        //*/
    };

// TODO: Create copyContructor for Layer derived classes
    class RasterLayer : public Layer
    {
    private:
        double rasterStats[4];
        // double dfNoData;

    public:
        // this should interface with OpenCV Mat and 2D matrix (vector style)
        // \todo check if size/type must/can be updated at construction time
        cv::Mat rasterData; //OpenCV matrix that will hold the data
        cv::Mat rasterMask; //OpenCV matrix with valida data mask (0=invalid, 255=valid)
        double  transformMatrix[6];
        int     layerDimensions[3];
        std::string layerProjection;

        RasterLayer(std::string name, int id) : Layer(name, id)
        {
            setType(LAYER_RASTER); 
        }

        int loadData(cv::Mat *);
        int readTIFF(std::string name); // read and load raster data from a geoTIFF file
        int writeLayer(std::string outputFilename, int fileFormat, int outputCoordinate); // Overloaded method of exporting vectorData to user defined file
        void showInformation();
        double getDiagonalSize();

        void copyGeoProperties(shared_ptr<RasterLayer> src); //!< Copy geoTIFF specific properties from a source layer
        void updateStats(); //!< Recomputes stats of valid raster data
        void updateMask();          //!< Update valid data mask by comparing rasterData with implicit no-data value 
        void updateMask(double nd); //!< Update valid data mask by comparing rasterData with user-provided no-data value
        double getMin()     {return rasterStats[LAYER_MIN];}    // we assume the values are up-to-date      
        double getMax()     {return rasterStats[LAYER_MAX];}    //\todo force update after modiciations
        double getMean()    {return rasterStats[LAYER_MEAN];}    // easy to enforce when loading raster data from file
        double getStdev()   {return rasterStats[LAYER_STDEV];}    // easy to enforce when loading raster data from file
        double *getStats()  {return rasterStats;}       // return a copy of the 4 element vector
        void getStats(double *);
    };

    class VectorLayer : public Layer
    {
    public:
        // this should interface with both GDAL and CGAL containers
        vector<cv::Point2d> vectorData; //Vector of 2D points defining vectorized layer (e.g. bounding polygon)
        int coordinateSpace;            // Defines if the data stored in vectorData is world coordinates or pixel coordinates

        VectorLayer(std::string name, int id) : Layer(name, id)
        {
            coordinateSpace = PIXEL_COORDINATE;
            setType(LAYER_VECTOR); // This can be done passing LAYER_VECTOR as 3rd argument of the constructor
        }

        void showInformation();

        int loadData(std::vector<cv::Point2d> *); // Import data into vectorData container
        // int writeLayer(int fileFormat = FMT_CSV); // export vectorData to the fileName as fileFormat (default CSV)
        int writeLayer(std::string outputFilename = NULL, int fileFormat = FMT_CSV, 
                       std::string strWKTSpatialRef = "", int outputCoordinate = WORLD_COORDINATE, double *apMatrix = nullptr); // Overloaded method of exporting vectorData to user defined file
        int convertSpace(int newSpace, double *apTransformMatrix);                                                                                                                        // Convert vectorData content to new coordinate space
    };

    class KernelLayer : public virtual RasterLayer
    {
    private:
            double dRotation; // Rotation angle of the given kernel (in radians)

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