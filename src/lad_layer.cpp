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
int Layer::setLayerstatus(int newStatus){
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

}
#endif //_LAD_LAYER_CPP_