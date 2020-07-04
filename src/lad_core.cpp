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
 * @brief Returns (if present) the name of the raster layer that matches provided ID
 * 
 * @param id Layer ID number to be searched
 * @return std::string 
 */
std::string ladPipeline::GetRasterLayerName (int id){
    if (RasterLayers.size() <=0){
        return "EMPTY_ARRAY";
    }
    // Check each raster in the array, compare its ID against search index
    for (auto layer:RasterLayers){
        if (layer.LayerID() == id) return layer.layerName;
    }
    return "NO_LAYER";
}

/**
 * @brief Returns (if present) the name of the kernel layer that matches provided ID
 * 
 * @param id Layer ID number to be searched
 * @return std::string 
 */
std::string ladPipeline::GetKernelLayerName (int id){
    if (KernelLayers.size() <=0){
        return "NO_LAYER";
    }
    // Check each raster in the array, compare its ID against search index
    for (auto layer:KernelLayers){
        if (layer.LayerID() == id) return layer.layerName;
    }
    return "NO_LAYER";
}

/**
 * @brief Returns (if present) the name of the vector layer that matches provided ID
 * 
 * @param id Layer ID number to be searched
 * @return std::string 
 */
std::string ladPipeline::GetVectorLayerName (int id){
    if (VectorLayers.size() <=0){
        return "NO_LAYER";
    }
    // Check each raster in the array, compare its ID against search index
    for (auto layer:VectorLayers){
        if (layer.LayerID() == id) return layer.layerName;
    }
    return "NO_LAYER";
}

/*
int ladPipeline::GetRasterLayerID (std::string name); //!< Return first raster that matches 'name' as layer name
int ladPipeline::GetKernelLayerID (std::string name); //!< Return first kernel that matches 'name' as layer name
int ladPipeline::GetVectorLayerID (std::string name); //!< Return first vector that matches 'name' as layer name
*/
}