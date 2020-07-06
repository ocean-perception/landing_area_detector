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
std::string ladPipeline::GetLayerName (int id){
    if (Layers.size() <=0) return "EMPTY_VECTOR";
    if (id < 0) return "INVALID_ID";
    // Check each raster in the array, compare its ID against search index
    for (auto layer:Layers){
        if (layer->GetID() == id) return layer->layerName;
    }
    return "NO_LAYER";
}

/**
 * @brief Overwrite the name of a layer (if present) with a unique string. 
 * @details If provided string 
 * @param id 
 * @return std::string 
 */
int ladPipeline::SetLayerName (int id, std::string newName){
    if (Layers.size()<=0) return LAYER_NONE; // Layers vector is empty
    if (id < 0) return LAYER_INVALID_ID;    // Provided ID is invalid
    // Check each raster in the array, compare its ID against search index
    // WARNING: TODO: Check if newName is already taken
    for (auto layer:Layers){
        if (layer->GetID() == id){
             layer->layerName = newName;
             return LAYER_OK;
        }
    }
    return LAYER_NOT_FOUND; // Layer ID not found
}


}