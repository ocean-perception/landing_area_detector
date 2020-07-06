/**
 * @file lad_core.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief  Landing Area Detection algorithm core header
 * @version 0.1
 * @date 2020-07-03
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "lad_core.hpp"

namespace lad{

/**
 * @brief Returns (if present) the name of the vector layer that matches provided ID
 * 
 * @param id Layer ID number to be searched
 * @return std::string 
 */
std::string ladPipeline::getLayerName (int id){
    if (Layers.size() <=0) return "EMPTY_VECTOR";
    if (id < 0) return "INVALID_ID";
    // Check each raster in the array, compare its ID against search index
    for (auto layer:Layers){
        if (layer->getID() == id) return layer->layerName;
    }
    return "NO_LAYER";
}

/**
 * @brief Overwrite the name of a layer (if present) with a unique string. 
 * @details If provided string 
 * @param id 
 * @return std::string 
 */
int ladPipeline::setLayerName (int id, std::string newName){
    if (Layers.size()<=0) return LAYER_NONE; // Layers vector is empty
    if (id < 0) return LAYER_INVALID_ID;    // Provided ID is invalid
    // Check each raster in the array, compare its ID against search index
    // WARNING: TODO: Check if newName is already taken
    for (auto layer:Layers){
        if (layer->getID() == id){
             layer->layerName = newName;
             return LAYER_OK;
        }
    }
    return LAYER_NOT_FOUND; // Layer ID not found
}

/**
 * @brief Return the number of layers of a given type within the current stack.
 * 
 * @param type Type of layer to be searched for. If no value is provided it will return the total number of layers in the stack
 * @return int Number of layers of 'type'
 */
int ladPipeline::getTotalLayers (int type){
    if (type == LAYER_ANYTYPE) return Layers.size();
    return -1;
} 

/**
 * @brief Determine if a given layer ID is valid. It checks both availability and correctness
 * 
 * @param checkID ID to be evaluated
 * @return int Evaluation status. If valid returns LAYER_OK, else return corresponding error code
 */
int ladPipeline::isValidID(int checkID){
    if (checkID < 0) return LAYER_INVALID_ID; //!< First, we check is a positive ID value

    if (Layers.empty()) //!< If Layers vector is empty, then chckID is definitelty available
        return LAYER_OK;

    for (auto layer:Layers){
        if (layer->getID()==checkID) //!< The checkID is already taken, return correspoding error code
            return LAYER_DUPLICATED_ID;
    }
    // If we reach this point, then the checkID is available. Return ok    
    return LAYER_OK;
}



}