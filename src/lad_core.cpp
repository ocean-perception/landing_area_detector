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

/**
 * @brief Determine if a given layer ID is valid. It checks both availability and correctness
 * 
 * @param checkID ID to be evaluated
 * @return int Evaluation status. If valid returns LAYER_OK, else return corresponding error code
 */
int ladPipeline::isValidName(std::string checkName){
    if (checkName.empty()) return LAYER_INVALID_NAME; //!< First, we check is a positive ID value

    // Now we must test that the name follows the target convention: alphanumeric with {-_} as special characters
    // Use regex to determine if any foreing character is present
    // --> /[a-zA-Z0-9_\-]/g
    // std::regex rgx("/[a-zA-Z0-9_-]/g"); //!< Regex list: will retrieve invalid characters
//    std::regex rgx("/^[a-zA-Z0-9]/g",std::regex_constants::egrep); //!< Regex list: will retrieve invalid characters

    if (Layers.empty()) //!< If Layers vector is empty, then given name is definitelty available
        return LAYER_OK;

    // TODO complete string based name match against all the other names
    for (auto layer:Layers){
        if (checkName.compare(layer->layerName)) //!< The checkID is already taken, return correspoding error code
            return LAYER_DUPLICATED_NAME;
    }
    // If we reach this point, then the checkID is available. Return ok    
    return LAYER_OK;
}

int ladPipeline::CreateLayer (std::string name, int type){
    // Let's check name
    if (isValidName(name) != LAYER_OK){
        cout << "[ladPipeline] Invalid name: " << name << endl;
        return LAYER_INVALID_NAME;
    }
    // Type can be any of enumerated types, or any user defined
    if (type == LAYER_VECTOR){
//        cout << "[ladPipeline] Creating VECTOR layer" << endl;
        std::shared_ptr <lad::VectorLayer> newLayer = std::make_shared<lad::VectorLayer>(name, 0);
        Layers.push_back(newLayer);
    }
    // Type can be any of enumerated types, or any user defined
    if (type == LAYER_RASTER){
        cout << "[ladPipeline] Creating RASTER layer" << endl;
    }
    // Type can be any of enumerated types, or any user defined
    if (type == LAYER_KERNEL){
        cout << "[ladPipeline] Creating KERNEL layer" << endl;
    }
}

/**
 * @brief Show t(if any) the summary information for each layer
 * 
 * @return int LAYER_NONE if Layers vector is empty, LAYER_OK otherwise
 */
int ladPipeline::showLayers(){
    if (!Layers.size()){
        cout << "No layer to show" << endl;
        return lad::LAYER_NONE;
    }
    for (auto it:Layers){
        // it->showI
        it->showInformation();
    }
        return lad::LAYER_OK;
} 


}