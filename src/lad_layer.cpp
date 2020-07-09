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

namespace lad{

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
    cout << "> Raster data container size: " << yellow << rasterData.size() << reset << endl;
}

/**
 * @brief Extended method that prints general and vector specific information
 * 
 */
void VectorLayer::showInformation(){
    cout << "Name: [" << green << layerName << reset << "]\t ID: [" << getID() << "]\tType: [VECTOR]\tStatus: [" << green << getLayerStatus() << reset << "]" << endl;
    cout << "> Vector Data container size: " << yellow << vectorData.size() << reset << endl;
}

/**
 * @brief Extended method that prints general and kernel specific information
 * 
 */
void KernelLayer::showInformation(){
    cout << "Name: [" << green << layerName << reset << "]\t ID: [" << getID() << "]\tType: [KERNEL]\tStatus: [" << green << getLayerStatus() << reset << "]" << endl;
    cout << "> Kernel data container size: " << yellow << rasterData.size() << reset << "\tKernel rotation: " << yellow << dRotation << reset << endl;
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

}
#endif //_LAD_LAYER_CPP_