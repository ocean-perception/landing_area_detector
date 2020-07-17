/**
 * @file lad_core.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief  Landing Area Detection algorithm core header
 * @version 0.1
 * @date 2020-07-05
 * 
 * @copyright Copyright (c) 2020
 * 
 */

// pragma once is not "standard"
#ifndef _LAD_LAYER_CPP_ 
#define _LAD_LAYER_CPP_

#include "lad_core.hpp"
#include "lad_layer.hpp"
#include "lad_processing.hpp"
#include <iostream>
#include "ogrsf_frmts.h"

namespace lad{

/**
 * @brief Copy the content of a Layer into the current one. Derived classes must perform container specific deep-copy
 * 
 * @param layer Input layer to be copied
 * @return int Error code, if any.
 */
int Layer::copy(Layer layer){
    layerFilePath = layer.layerFilePath;
    layerFileName = layer.layerFileName;
    layerName = layer.layerName;
    layerID = layer.layerID;
    layerStatus = layer.layerStatus;
    layerType = layer.layerType;
    return NO_ERROR;
}   

/**
 * @brief Copy the content of a Layer into the current one. Derived classes must perform container specific deep-copy
 * @details Overloaded version for pointer to Layer 
 * @param layer Pointer to layer to be copied
 * @return int Error code, if any.
 */
int Layer::copy(*Layer layer){
    layerFilePath = layer->layerFilePath;
    layerFileName = layer->layerFileName;
    layerName = layer->layerName;
    layerID = layer->layerID;
    layerStatus = layer->layerStatus;
    layerType = layer->layerType;
    // return NO_ERROR;
    return NO_ERROR;
}

/**
 * @brief Derived classes are expected to release containers and perform class-specific clean-out
 * 
 * @return int It should return NO_ERROR unless any exception is triggered
 */
int Layer::clear(){
    layerID = LAYER_INVALID_ID;
    layerType = LAYER_INVALID;
    layerName = "";
    layerFileName = "";
    return NO_ERROR;
}
/**
 * @brief Return the ID of the given layer
 * 
 * @return int ID value
 */
int Layer::getID(){
    return layerID;
} 
/**
 * @brief Update the ID value of the layer.
 * 
 * @param newID New ID value of the value. It must be a valid ID
 * @return int Error code, if any. If the provided ID is valid it will return LAYER_OK
 */
int Layer::setID(int newID){
    if (newID < 0) return LAYER_INVALID_ID; // If invalid, return error code
    layerID = newID; // if newID is valid update layerID.
    return LAYER_OK; // return succesful code
}
/**
 * @brief get the Layer Status object
 * 
 * @return int Layer status value, from enumerated list
 */
int Layer::getLayerStatus(){
    return layerStatus;
}

/**
 * @brief Update the Layer status
 * 
 * @param newStatus New value of layer status. It should be any of possible values in the enumerated list. No validation is enforced
 * @return int return a copy of the new status value
 */
int Layer::setLayerStatus(int newStatus){
    layerStatus = newStatus;
    return layerStatus;
}

/**
 * @brief get the Layer Type object
 * 
 * @return int Return the layer type, from enumerated list (RASTER, KERNEL, VECTOR, UNDEFINED)
 */
int Layer::getLayerType(){
    return layerType;
} 

/**
 * @brief Update the Layer type
 * 
 * @param newType New value of layer type. It is user's responsability to correctly recast (if necessary) the data container accordingly
 * @return int return a copy of the new layer type
 */
int Layer::setLayerType(int newType){
    layerType = newType;
}
/**
 * @brief Prints summary information of the layer
 */
void Layer::showInformation(){
    cout << "Name: [" << green << layerName << reset << "]\t ID: [" << layerID << "]\tType: [" << layerType << "]\tStatus: [" << layerStatus << "]" << endl;
}

/**
 * @brief Extended method that prints general and raster specific information
 * 
 */
void RasterLayer::showInformation(){
    cout << "Name: [" << green << layerName << reset << "]\t ID: [" << getID() << "]\tType: [RASTER]\tStatus: [" << green << getLayerStatus() << reset << "]" << endl;
    cout << "\t> Raster data container size: " << yellow << rasterData.size() << reset << endl;
}

/**
 * @brief Extended method that prints general and vector specific information
 * 
 */
void VectorLayer::showInformation(){
    cout << "Name: [" << green << layerName << reset << "]\t ID: [" << getID() << "]\tType: [VECTOR]\tStatus: [" << green << getLayerStatus() << reset << "]\tCoordinates: [";
    if (coordinateSpace == PIXEL_COORDINATE){
        cout << yellow << "PIXEL]" << reset << endl; 
    }
    else{
                cout << yellow << "WORLD]" << reset << endl; 
    }
    cout << "\t> Vector Data container size: " << yellow << vectorData.size() << reset << endl;
}

/**
 * @brief Convert the vectorData content to the new coordinate space
 * 
 * @param newSpace New coordinate space. It can be either PIXEl_COORDINATE or WORLD_COORDINATE
 * @param apTransform 6x1 double matrix containing the transformation coefficients 
 * @return int Success code
 */
int VectorLayer::convertSpace(int newSpace, double *apTransform){
    if (newSpace = coordinateSpace){
        cout << yellow << "[convertSpace] Source and target coordinate space are the same when trying to convert [" << layerName << "]. No operation was performed" << reset << endl;
        // it is the same space! nothing to change
        return 0;
    }
    if (apTransform==NULL){
        cout << red << "[convertSpace] Wrong 6D transforation matrix when trying to convert [" << layerName << "]. No operation was performed" << endl;
        return -1;
    }
    // ok we have a valid 6D transformation (somewhat...)
    int retval;
    vector<cv::Point2d> newData;
    retval = convertDataSpace(&vectorData, &newData, coordinateSpace, newSpace, apTransform);
    vectorData = newData;
    coordinateSpace = newSpace; //!< Update to the new coordinate space
    return retval;
}

/**
 * @brief Export vectorData to layerFileName file using fileFmt format
 * 
 * @param fileFmt Output file format. It must be a valid value from enum ExportFormat
 */
int VectorLayer::writeLayer(std::string exportName, int fileFmt, std::string strWKTSpatialRef, int outputCoordinate, double *apGeoTransform){
    //*************************************************************
    if (fileFmt == FMT_TIFF){
        cout << red << "[writeLayer] Error, vector layer cannot be exported as TIFF. Please convert it to raster first" << reset << endl;
        return ERROR_WRONG_ARGUMENT;
    }
    //*************************************************************
    if (fileFmt == FMT_SHP){
        std::vector<cv::Point2d> transformedData;

        //we need to check if the data points need a space transformation
        if (coordinateSpace != outputCoordinate){
            cout << "Converting coordinate space of [" << exportName << "] to [";
            if (outputCoordinate == PIXEL_COORDINATE){
                cout << yellow << "PIXEL" << reset << "]" << endl;
            }
            else{
                cout << yellow << "WORLD" << reset << "]" << endl;
            }
            convertDataSpace (&vectorData, &transformedData, coordinateSpace, outputCoordinate, apGeoTransform);
        }
        //vectorData
        int i = exportShapefile(exportName, layerName, transformedData, strWKTSpatialRef);
        if (i!=NO_ERROR){
            cout << "\tSome error ocurred while exporting [" << yellow << layerName << reset << "] to [" << yellow << exportName << "]" << reset << endl; 
            return ERROR_GDAL_FAILOPEN;
        }
        return NO_ERROR;;
    }
    //*************************************************************
    if (fileFmt == FMT_CSV){
        // check if default filename has been already defined, if not what?

        std::vector<cv::Point2d> transformedData;

        //we need to check if the data points need a space transformation
        if (coordinateSpace != outputCoordinate){
            cout << "Converting coordinate space of [" << exportName << "] to [";
            if (outputCoordinate == PIXEL_COORDINATE){
                cout << yellow << "PIXEL" << reset << "]" << endl;
            }
            else{
                cout << yellow << "WORLD" << reset << "]" << endl;
            }
            convertDataSpace (&vectorData, &transformedData, coordinateSpace, outputCoordinate, apGeoTransform);
        }

        if (exportName.empty()){
            if (layerFileName.empty()){
                cout << "[writeLayer] " << yellow << "Layer filename not defined, will try to use layer name as export file" << reset << endl;
                if( layerName.empty()){
                    cout << "[writeLayer] " << red << "ERROR: Layer name not defined. Won't export layer" << reset << endl;
                    return ERROR_MISSING_ARGUMENT;
                }
                exportName = layerName;
            }
        }
        cout << reset << "[writeLayer] Exporting " << yellow << layerName << reset << " as CSV file: " << yellow << exportName << reset << endl; 
        cout << "\tVector layer size: " << vectorData.size() << endl;

        ofstream outfile(exportName, ios::out);
        if (!outfile.good()){
            cout << "[writeLayer] " << red << "Error creating output file: " << exportName << reset << endl;
            return ERROR_WRONG_ARGUMENT;
        }
        std::string separator = ", ";
        outfile << "X" << separator << "Y" << endl;
        // Now we can export the content of the vector data (X,Y)
        for (auto element:transformedData){
            outfile << element.x << separator << element.y << endl;
        }
        cout << "\tVector layer exported to: " << exportName << endl;
        outfile.close();
        return EXPORT_OK;
    }
    //*************************************************************
    else{
        cout << yellow << "[writeLayer] Unknown format: " << fileFmt << reset << endl;
        return ERROR_WRONG_ARGUMENT;
    }
}

/**
 * @brief Extended method that prints general and kernel specific information
 * 
 */
void KernelLayer::showInformation(){
    cout << "Name: [" << green << layerName << reset << "]\t ID: [" << getID() << "]\tType: [KERNEL]\tStatus: [" << green << getLayerStatus() << reset << "]" << endl;
    cout << "\t> Kernel data container size: " << yellow << rasterData.size() << reset << "\tKernel rotation: " << yellow << dRotation << reset << endl;
}

/**
 * @brief Loads a vector of Point2d points in the layer container. It replaces the existing data.
 * 
 * @param inputData New data to be stored in the container
 * @return int size of the new stored data vector
 */
int VectorLayer::loadData(vector <Point2d> *inputData){
    // We can not avoid a deep copy of the input vector. We could iterate through each element and assign it, or just use = operator
    vectorData = *inputData;
    setLayerStatus(LAYER_OK);
    return vectorData.size();
}

/**
 * @brief Import and copy the content from an input cvMat to the internal storage rasterData
 * 
 * @param input A valid cvMat matrix (yet, no validation is performed)
 * @return int 
 */
int RasterLayer::loadData(cv::Mat *input){
    input->copyTo(rasterData);   // deep copy of the Mat content and header to avoid original owner to accidentally overwrite the data
    setLayerStatus(LAYER_OK);
}

/**
 * @brief Empty definition of virtual prototype. In theory, there is no need to do anything with the data when using the basic container
 * 
 * @param data pointer to data that should be stored in the internal container
 * @return int return generic error code
 */
/*int Layer::loadData(void *data){
    // cout << cyan << "Layer::loadData(void *data) called" << reset << endl;
    return LAYER_UNDEFINED;
}*/

/**
 * @brief Export Point2d vector as single layer ESRI Shapefile
 * 
 * @param filename Name of the output SHP file
 * @param layerName Layer name inside of the SHP file
 * @param data Vector containing the Point2d data (x,y)
 * @return int Error/sucess code.
 */
int exportShapefile (string filename, string layerName, vector<Point2d> data, std::string strWKTSpatialRef){

    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;
    GDALAllRegister();

    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName );
    if( poDriver == NULL )
    {
        cout << red << "Error w/GDAL: " << pszDriverName << " not available" << endl;
        return ERROR_GDAL_FAILOPEN;
    }

    GDALDataset *poDS;
    poDS = poDriver->Create( filename.c_str(), 0, 0, 0, GDT_Unknown, NULL );
    if( poDS == NULL )
    {
        cout << red << "Error creating output file: [" << filename << "]" << reset << endl;
        return ERROR_GDAL_FAILOPEN;
    }

    OGRLayer *poLayer;
    OGRSpatialReference spatialRef(strWKTSpatialRef.c_str());
    poLayer = poDS->CreateLayer( layerName.c_str(), &spatialRef, wkbPoint, NULL );
    if( poLayer == NULL )
    {
        cout << yellow << "Error creating layer: [" << layerName << "]" << reset << endl;
        return ERROR_GDAL_FAILOPEN;
    }

//    OGRFieldDefn oField( "Name", OFTString );
//    oField.SetWidth(32);

    // if( poLayer->CreateField( &oField ) != OGRERR_NONE )
    // {
    //     printf( "Creating Name field failed.\n" );
    //     exit( 1 );
    // }

    double x, y;
    char szName[33]= "szName";

    for (auto it:data){
        OGRFeature *poFeature;
        poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
//        poFeature->SetField( "Name", szName );

        OGRPoint pt;
        pt.setX( it.x );
        pt.setY( it.y );

        poFeature->SetGeometry( &pt );

        if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
        {
            cout << yellow << "Error GDAL: Failed to create feature in shapefile." << endl;
            return ERROR_GDAL_FAILOPEN;
        }

        OGRFeature::DestroyFeature( poFeature );
    }

    GDALClose( poDS );
    return NO_ERROR;
}

}
#endif //_LAD_LAYER_CPP_