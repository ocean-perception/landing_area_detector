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
 * @brief Returns (if present) the ID of the vector layer that matches provided name
 * 
 * @param name Layer name to be searched
 * @return int ID of the layer, if found any. Returns LAYER_NOTFOUN 
 */
int ladPipeline::getLayerID (std::string name){
    if (Layers.size() <=0) return LAYER_INVALID_ID;

    if (!name.size())   return LAYER_INVALID_ID;

    // Check each raster in the array, compare its ID against search index
    for (auto layer:Layers){
        if (layer->layerName == name) return layer->getID();
    }
    return LAYER_NOT_FOUND;
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
 * @brief Determine if a given layer name is valid. It checks both availability and correctness
 * 
 * @param checkID ID to be evaluated
 * @return int Evaluation status. If valid returns LAYER_OK, else return corresponding error code
 */
int ladPipeline::isValidName(std::string checkName){
    if (checkName.empty()) return LAYER_INVALID_NAME; //!< First, we check is a positive ID value

    // Now we must test that the name follows the target convention: alphanumeric with {-_} as special characters
    // Use regex to determine if any foreing character is present
//    std::regex rgx("/^[a-zA-Z0-9]/g",std::regex_constants::egrep); //!< Regex list: will retrieve invalid characters

    if (Layers.empty()) //!< If Layers vector is empty, then given name is definitelty available
        return LAYER_OK;

    // TODO complete string based name match against all the other names
    for (auto layer:Layers){
        if (!checkName.compare(layer->layerName)){ //!< The checkID is already taken, return correspoding error code
            return LAYER_DUPLICATED_NAME;
        }
    }
    // If we reach this point, then the checkID is available. Return ok    
    return LAYER_OK;
}

/**
 * @brief Remove a Layer fully identified by its name
 * 
 * @param name name of the layer to be removed
 * @return int returns success if layer was found and removed. Otherwise, it will send the error code
 */
int ladPipeline::RemoveLayer (std::string name){
    // First we verify the stack is not empty
    if (Layers.empty()) return LAYER_EMPTY;

    int kk=0;
    //then we go through each layer
    for (auto const it:Layers){
        if (!name.compare(it->layerName)){ // found it!
            LUT_ID.at(it->getID()) = ID_AVAILABLE;
            Layers.erase(Layers.begin() + kk);
            // remove(it);
            break; // if we don't break now we will get a segfault (the vector iterator is broken)
        }
        kk++;
    }
    // we shouldn't reach this point. unless we found the target
    return LAYER_OK;
}

/**
 * @brief Remove a Layer identified by its name
 * 
 * @param name name of the layer to be removed
 * @return int returns success if layer was found and removed. Otherwise, it will send the error code
 */
int ladPipeline::RemoveLayer (int id){
    // First we verify the ID
    if (id < 0)
        return LAYER_INVALID_ID; 
    
    if (LUT_ID.at(id) == ID_AVAILABLE) // using the LUT to speed-up the search process from O(n) to 1
        return LAYER_NOT_FOUND;
    
    // Then we check the stack size
    if (Layers.empty())
        return LAYER_EMPTY;

    int kk=0;
    //then we go through each layer
    for (auto const it:Layers){
        if (it->getID() == id){ // found it!
            LUT_ID.at(it->getID()) = ID_AVAILABLE;
            Layers.erase(Layers.begin() + kk);
            break; // if we don't break now we will get a segfault (the vector iterator is broken)
        }
        kk++;
    }
    // we shouldn't reach this point. unless we found the target
    return LAYER_OK;
}

/**
 * @brief Create a new Layer (share_ptr) according to the provided type
 * 
 * @param name Valid name of the new layer
 * @param type Type of new layer
 * @return int Error code if name is invalid (LAYER_INVALID_NAME), if succesful returns LAYER_OK 
 */
int ladPipeline::CreateLayer (std::string name, int type){
    // Let's check name
    if (isValidName(name) != LAYER_OK){
        cout << "[ladPipeline] Duplicated layer name: " << name << endl;
        return LAYER_INVALID_NAME;
    }

    int newid = getValidID();

    // Type can be any of enumerated types, or any user defined
    if (type == LAYER_VECTOR){
        cout << "[ladPipeline] Creating VECTOR layer: " << name << endl;
        std::shared_ptr <lad::VectorLayer> newLayer = std::make_shared<lad::VectorLayer>(name, newid);
        Layers.push_back(newLayer);
        LUT_ID.at(newid) = ID_TAKEN;
    }
    // Type can be any of enumerated types, or any user defined
    if (type == LAYER_RASTER){
        cout << "[ladPipeline] Creating RASTER layer" << endl;
        std::shared_ptr <lad::RasterLayer> newLayer = std::make_shared<lad::RasterLayer>(name, newid);
        Layers.push_back(newLayer);
        LUT_ID.at(newid) = ID_TAKEN;
    }
    // Type can be any of enumerated types, or any user defined
    if (type == LAYER_KERNEL){
        cout << "[ladPipeline] Creating KERNEL layer" << endl;
        std::shared_ptr <lad::KernelLayer> newLayer = std::make_shared<lad::KernelLayer>(name, newid);
        Layers.push_back(newLayer);
        LUT_ID.at(newid) = ID_TAKEN;
    }

    return LAYER_OK;
}

int ladPipeline::getValidID()
{
    for (int i=0; i<LUT_ID.size(); i++){
        if (LUT_ID.at(i) == ID_AVAILABLE) //!< we return once we find an available ID
            return i; 
    }
    //!< If we reach this point it means that ALL slots(IDs) are taken, therefore, we expand our vector
    cout << red << "getValiID: vector expansion triggered" << reset << endl;
    LUT_ID.push_back(ID_AVAILABLE); // append a new empty slot
    cout << "new size: " << LUT_ID.size() << endl;
    return (LUT_ID.size()-1); //Let's return it as a new available one 
}

/**
 * @brief Show t(if any) the summary information for each layer
 * 
 * @return int LAYER_NONE if Layers vector is empty, LAYER_OK otherwise
 */
int ladPipeline::showLayers(int layer_type){
    if (!Layers.size()){
        cout << "No layer to show" << endl;
        return lad::LAYER_NONE;
    }
    for (auto it:Layers){
        if ((it->getLayerType() == layer_type) || (layer_type == LAYER_ANYTYPE))
            it->showInformation();
    }
        return lad::LAYER_OK;
} 

/**
 * @brief Upload data to the corresponding container in the layer identified by its id. 
 * Dynamic typecasting of data is performed according to the container data type
 * 
 * @param id Identifier of the target layer
 * @param data Source data to be copied
 * @return int Error code, if any. LAYER_OK if the process is completed succesfully
 */
int ladPipeline::uploadData(int id, void *data){
    if (id < 0) return LAYER_INVALID_ID;    //!< The provided ID is invalid
    if (LUT_ID.at(id) == ID_AVAILABLE) return LAYER_NOT_FOUND;  //!< No layer was created with that ID
    // cout << "Uploading data for layer #" << red << id << reset << endl;
    for (auto it:Layers){
        if (it->getID() == id){ //!< Check ID match
            int type = it->getLayerType();  //!< slight speed improve
            // WARNING: if we change these 'if' to switch , -fPermissive will trigger error 
            if (type == LAYER_VECTOR){
                auto v = std::dynamic_pointer_cast<lad::VectorLayer> (it);
                v->loadData((std::vector <cv::Point2d> *)data);
            }
            else if (type == LAYER_RASTER){
                auto r = std::dynamic_pointer_cast<lad::RasterLayer> (it);
                r->loadData((cv::Mat *) data);
            }
            else if (type == LAYER_KERNEL){
                auto r = std::dynamic_pointer_cast<lad::KernelLayer> (it);
                r->loadData((cv::Mat *) data);
            }
        }
    }
    return LAYER_OK;
}

/**
 * @brief Alternative to uploadData(int,data) where the layer is identified by its name
 * @param id Name of the target layer
 * @param data Source data to be copied
 * @return int Error code, if any. LAYER_OK if the process is completed succesfully
 */
int ladPipeline::uploadData(std::string name, void *data){
    // Check if we the name is valid (non-empty)
    if (name.empty()) return LAYER_INVALID_NAME;

    // Now, we retrieve the ID for that name
    int id = getLayerID(name);
    if ((id == LAYER_INVALID_ID) || (id == LAYER_NOT_FOUND)){
        cout << "*************Layer_INVALID!: [" << id << "]" << endl;
        return LAYER_INVALID;   // some error ocurred getting the ID of a layer with such name (double validation)
    }
    int retval = LAYER_NOT_FOUND;
    for (auto layer:Layers){
        if (layer->getID() == id){ /// there should be a match!
            // cout << "*************Layer found: [" << layer->getID() << "]" << endl;
            retval = uploadData(id,data);
            break;
        }
    }
    return retval;
}

/**
 * @brief Reads a geoTIFF file using the GDAL driver and populate the object container
 * 
 * @param inputFile Name of the geotTIFF file to be read
 * @return int Success/error code
 */
int ladPipeline::ReadTIFF (std::string inputFile){
    if (inputFile.empty()){
        cout << red << "[ReadTIFF] input filename is empty!" << reset << endl;
    }

    // Geotiff a;
//    a.

}   

/**
 * @brief Shows summary/detailed information of the pipeline object
 * 
 * @param level Level of verbosity
 * @return int Returns NO_ERROR if current pipeline is a valid (non-empty) one.
 */
int ladPipeline::showInfo(int level){
    // Geotiff input information
    int retval = NO_ERROR;
    cout << cyan << "************* Start of summary ************" << reset << endl;
    cout << green << "Geotiff:\t" << reset;
    if (apInputGeotiff == NULL){
        cout << red << "None" << reset << endl;
        retval = ERROR_GEOTIFF_EMPTY;
    }
    else
    {
        cout << yellow << apInputGeotiff->GetFileName() << reset << endl;
        apInputGeotiff->ShowInformation();
    }

    cout << "*************" << endl;
    cout << green << "Layers:\t" << reset;
    if (Layers.empty()){
        cout << yellow << "None" << reset << endl;
        retval = ERROR_LAYERS_EMPTY;
    }
    else{
        cout << reset << endl;
        showLayers();
    }
    cout << cyan << "************* End of summary *************" << reset << endl;
    return retval;        
} 

}