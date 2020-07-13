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
    if (Layers.size() <=0) return LAYER_EMPTY;
    if (isValid(name) == false) return LAYER_INVALID_NAME;

    // Check each raster in the array, compare its ID against search index
    for (auto layer:Layers){
        cout << cyan << "compare names: [" << layer->layerName << "] ["<< name << "]" << endl;
        if (!name.compare(layer->layerName)) return layer->getID();
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
    if (isValid(id) ==false) return LAYER_INVALID_ID;    // Provided ID is invalid
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
    if (isValid(checkName) == false) return LAYER_INVALID_NAME; //!< First, we check is a positive ID value

    // Now we must test that the name follows the target convention: alphanumeric with {-_} as special characters
    // Use regex to determine if any foreing character is present
//    std::regex rgx("/^[a-zA-Z0-9]/g",std::regex_constants::egrep); //!< Regex list: will retrieve invalid characters

    if (Layers.empty()) //!< If Layers vector is empty, then given name is definitelty available
        return LAYER_OK;

    // TODO complete string based name match against all the other names
    for (auto layer:Layers){
        if (checkName == layer->layerName){ //!< The checkID is already taken, return correspoding error code
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

    int i=0;
    //then we go through each layer
    for (auto const it:Layers){
        if (!name.compare(it->layerName)){ // found it!
            LUT_ID.at(it->getID()) = ID_AVAILABLE;
            Layers.erase(Layers.begin() + i);
            // remove(it);
            break; // if we don't break now we will get a segfault (the vector iterator is broken)
        }
        i++;
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

    int i=0;
    //then we go through each layer
    for (auto const it:Layers){
        if (it->getID() == id){ // found it!
            LUT_ID.at(it->getID()) = ID_AVAILABLE;
            Layers.erase(Layers.begin() + i);
            break; // if we don't break now we will get a segfault (the vector iterator is broken)
        }
        i++;
    }
    // we shouldn't reach this point. unless we found the target
    return LAYER_OK;
}

/**
 * @brief Create a new Layer (share_ptr) according to the provided type
 * 
 * @param name Valid name of the new layer
 * @param type Type of new layer
 * @return int new ID if valid, error code otherwise 
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
        // cout << "[ladPipeline] Creating VECTOR layer: " << name << endl;
        std::shared_ptr <lad::VectorLayer> newLayer = std::make_shared<lad::VectorLayer>(name, newid);
        Layers.push_back(newLayer);
        LUT_ID.at(newid) = ID_TAKEN;
    }
    // Type can be any of enumerated types, or any user defined
    if (type == LAYER_RASTER){
        // cout << "[ladPipeline] Creating RASTER layer" << endl;
        std::shared_ptr <lad::RasterLayer> newLayer = std::make_shared<lad::RasterLayer>(name, newid);
        Layers.push_back(newLayer);
        LUT_ID.at(newid) = ID_TAKEN;
    }
    // Type can be any of enumerated types, or any user defined
    if (type == LAYER_KERNEL){
        // cout << "[ladPipeline] Creating KERNEL layer" << endl;
        std::shared_ptr <lad::KernelLayer> newLayer = std::make_shared<lad::KernelLayer>(name, newid);
        Layers.push_back(newLayer);
        LUT_ID.at(newid) = ID_TAKEN;
    }

    return newid;
}

/**
 * @brief Retrieve the first valid available ID from the Look-up table
 * 
 * @return int valid ID ready to be consumed in a CreateLayer call
 */
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
    if (isValid(id) == false){
        return LAYER_INVALID_ID; //!< The provided ID is invalid
    
    } if (isAvailable(id) == true){
        return LAYER_NOT_FOUND;  //!< No layer was found with that ID
    }

    // cout << cyan;
    // cout << "++++++++++++++++++++++ DATA RECEIVED" << endl;
    // cv::Mat *pdata = (cv::Mat *)data;
    // for (int i=0; i<pdata->rows;i++){
    //     for (int j=0; j<pdata->cols; j++){
    //         cout << pdata->at<float>(i,j) << " ";
    //     }
    //     cout << endl;
    // }
    // cout << "++++++++++++++++++++++ DUMPED DATA" << endl;
    // cout << reset;



    cout << "Uploading data for layer #" << red << id << reset << endl;
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
    if ((id == LAYER_INVALID_ID) || (id == LAYER_NOT_FOUND)){ //TODO: Should we create it?
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
    cout << red << "NOT IMPLEMENTED YET ***********************************" << endl;
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

/**
 * @brief Process Geotiff object & data and generate data raster and valid-data mask raster
 * 
 * @return int error code, if any
 */
int ladPipeline::processGeotiff(std::string rasterName, std::string maskName, int showImage){
    //First, check if have any valid Geotiff object loaded in memory
    int *dimensions;
    dimensions = apInputGeotiff->GetDimensions();
    float **apData; //pull 2D float matrix containing the image data for Band 1

    apData = apInputGeotiff->GetRasterBand(1);
    if (apData == NULL){
        cout << red << "[processGeotiff]: Error reading input geoTIFF data: NULL" << reset << endl;
        return ERROR_GDAL_FAILOPEN;
    }

    cv::Mat tiff(dimensions[0], dimensions[1], CV_32FC1); // cv container for tiff data . WARNING: cv::Mat constructor is failing to initialize with apData
    for (int i=0; i<dimensions[0]; i++){
        for (int j=0; j<dimensions[1]; j++){
            tiff.at<float>(cv::Point(j,i)) = (float)apData[i][j];   // swap row/cols from matrix to OpenCV container
        }
    }

    // we need check if the raster layer exist
    int id = -1;
    id = getLayerID(rasterName);
    if (id == LAYER_INVALID_NAME){  //!< Provided rasterName is invalid
        cout << "[processGeotiff]" << red << "Invalid raster name: [" << rasterName << "]" << reset << endl;
        return LAYER_INVALID_NAME;
    }
    // TIFF name is VALID

    if ((id == LAYER_EMPTY)||(id == LAYER_NOT_FOUND)){ //!< Layer was not found, we have been asked to create it
        id = CreateLayer(rasterName, LAYER_RASTER);
        uploadData(id, (void *) &tiff); //upload cvMat tiff for deep-copy into the internal container
    }
   //*********************************************************
    // Check DATA mask layer
    //*********************************************************

    cv::Mat matDataMask;    // Now, using the NoData field from the Geotiff/GDAL interface, let's obtain a binary mask for valid/invalid pixels
    cv::compare(tiff, apInputGeotiff->GetNoDataValue(), matDataMask, CMP_NE); // check if NOT EQUAL to GDAL NoData field

    id = getLayerID(maskName);
    if (id == LAYER_INVALID_NAME){  //!< Provided maskName is invalid
        cout << "[processGeotiff]" << red << "Invalid data mask name: [" << maskName << "]" << reset << endl;
        return LAYER_INVALID_NAME;
    }

    if ((id == LAYER_EMPTY)||(id == LAYER_NOT_FOUND)){ //!< Layer was not found, we have been asked to create it
        id = CreateLayer(maskName, LAYER_RASTER);
        uploadData(id, (void *) &matDataMask); //upload cvMat tiff for deep-copy into the internal container
    }

    if (showImage){
        cv::Mat tiff_colormap = Mat::zeros( tiff.size(), CV_8UC1 ); // colour mapped image for visualization purposes
        cv::normalize(tiff, tiff_colormap, 0, 255, NORM_MINMAX, CV_8UC1, matDataMask); // normalize within the expected range 0-255 for imshow
        // apply colormap for enhanced visualization purposes
        cv::applyColorMap(tiff_colormap, tiff_colormap, COLORMAP_TWILIGHT_SHIFTED);
        namedWindow(maskName, WINDOW_NORMAL);
        imshow(maskName, matDataMask);
        resizeWindow(maskName, 800, 800);

        namedWindow(rasterName, WINDOW_NORMAL);
        imshow(rasterName, tiff_colormap);
        resizeWindow(rasterName, 800, 800);
    }
}

/**
 * @brief Extract contours from rasterName layer and export it as vector to countourName
 * 
 * @param rasterName Input binary raster layer to be analized 
 * @param contourName Output vector layer that will contain the largest found contour
 * @param showImage flag indicating if we need to show the images
 * @return int Error code - if any.
 */
int ladPipeline::extractContours(std::string rasterName, std::string contourName, int showImage){
 
    vector< vector<Point> > contours;   // find contours of the DataMask layer
    //pull access to rasterMask
    std::shared_ptr<RasterLayer> apRaster;
    apRaster = dynamic_pointer_cast<RasterLayer>(getLayer(rasterName));

    cv::findContours(apRaster->rasterData, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE); // obtaining only 1st level contours, no children
    cout << "[extractContours] #contours detected: " << contours.size() << endl;
    if (contours.empty()){
        cout << red << "No contour line was detected!" << endl;
        return ERROR_CONTOURS_NOTFOUND;
    }

    // We need to extract the largest polygon, which should provide the main data chunk boundary
    // Disconected bathymetries are not expected
    int largest = 0;
    vector< vector<Point> > good_contours;   // find contours of the DataMask layer

    int k=0;
    for (auto it:contours){
        if (it.size() > largest){
            largest = it.size();
            if (!good_contours.empty()) good_contours.pop_back();
            good_contours.push_back(it); // hack to keep it at a single element
        }
    }
    cout << "[extractContours] Largest contour: " << yellow << largest << reset << endl;

    // WARNING: contours may provide false shapes when valid data mask reaches any image edge.
    // SOLUTION: expand +1px the image canvas on every direction, or remove small bathymetry section (by area or number of points)
    // See: copyMakeBorder @ https://docs.opencv.org/3.4/dc/da3/tutorial_copyMakeBorder.html UE: BORDER_CONSTANT (set to ZERO)
    if (showImage){
        Mat boundingLayer = Mat::zeros(apRaster->rasterData.size(), CV_8UC1);   // empty mask
        int n =1;

        for (const auto &contour: good_contours){
            drawContours(boundingLayer, contours, -1, Scalar(255*n/good_contours.size()), 1); // overlay contours in new mask layer, 1px width line, white
            n++;
        }
        namedWindow(contourName, WINDOW_NORMAL);
        imshow (contourName, boundingLayer);
        resizeWindow(contourName, 800, 800);
    }

    // COMPLETE: now we export the data to the target vector layer contourName
    // if it already exist, overwrite data?
    //pull access to rasterMask
    std::shared_ptr<VectorLayer> apVector;
    if (getLayerID(contourName)>=0){
        // it already exist!
        apVector = dynamic_pointer_cast<VectorLayer>(getLayer(contourName));
    }
    else{
        // nope, we must create it
        int newid = CreateLayer(contourName,LAYER_VECTOR);
        apVector = dynamic_pointer_cast<VectorLayer>(getLayer(contourName));
        cout << "\tCreated new vector layer: [" << contourName << "] with ID [" << newid << "]" << endl;
    }

    vector<Point> contour = good_contours.at(0); //a single element is expected because we force it
    for (auto it:contour){  //deep copy by iterating through the vector. = operator non-existent por Point to Point2d (blame OpenCV?)
        apVector->vectorData.push_back(it);

    }
    // cout << "Vector data: " << apVector->vectorData.size() << endl;
    return NO_ERROR;
}

/**
 * @brief Search a Layer by its ID and return a shared_ptr to it. If the provided ID is invalid or non-existent a NULL pointer is returned
 * 
 * @param id ID of the layer to be retrieved
 * @return std::shared_ptr<Layer> pointer to the Layer with the given id
 */
std::shared_ptr<Layer> ladPipeline::getLayer(int id){
    if (isValid(id) == false) return NULL;
    if (Layers.empty()) return NULL;
    if (LUT_ID[id] == ID_AVAILABLE) return NULL;
    // now, it is safe to assume that such ID exists
    for (auto it:Layers){
        if (it->getID() == id){
            return it;
        }
    }
    return NULL; //!< Curious thing, nothing was found!
}

/**
 * @brief Search a Layer by its ID and return a shared_ptr to it. If the provided ID is invalid or non-existent a NULL pointer is returned
 * 
 * @param name name of the layer to be retrieved
 * @return std::shared_ptr<Layer> pointer to the Layer with the given id
 */
std::shared_ptr<Layer> ladPipeline::getLayer(std::string name){
    if (Layers.empty()) return NULL;

    if (!isValid(name)) return NULL;

    int id = getLayerID(name);

    if (!isValid(id)) return NULL;   //some error ocurred while searching that name in the list of layers and returned an invalid ID

    return (getLayer(id));  //now we search it by ID
}

/**
 * @brief Returs true if the provided ID is valid. It does not check whether it is available in the current stack
 * 
 * @param id ID to be tested 
 * @return int true id ID is valid, false otherwise
 */
int ladPipeline::isValid(int id){
    if ((id < 0) || (id > (LUT_ID.size()-1))) return false;
    return true;
}

int ladPipeline::isValid(std::string str){
    // TODO: Add regexp for alphanumeric +set of special characters as valid set
    if (str.empty()) return false;
    return true;
}  //!< Returs true if the provided NAME is valid. It does not check whether it is available in the current stack

/**
 * @brief Returns true if the provided ID is available. It also checks validity, for sanity reasons
 * 
 * @param id ID to be tested 
 * @return int true if ID is available, false otherwise
 */
int ladPipeline::isAvailable(int id){
    if (id < 0) return false; // careful, the ID is valid, therefore we prefer to flag it as unavailable too
    if (id >= LUT_ID.size()) return false; // this is to avoid throwing an out-of-range exception
    if (LUT_ID[id] == ID_TAKEN) return false; // taken!
    return true;    // default, it didn't fail therefore is available
}

/**
 * @brief Verify if the tested name is already taken by any existing layer in the current stack. This is used to avoid name duplication
 * 
 * @return int true if the name is available, false otherwise
 */
int ladPipeline::isAvailable(std::string str){
    //iterate through all the existing layers, and compare against them
    if (isValid(str) == false){
        return false;   // sanity check of its validity
    }
    if (Layers.empty()){    //nothing stored in the stack, so any valid name is available
        return true;
    }
    for (auto it:Layers){
        if (str == it->layerName){ //that name is already taken
            return false;
        }
    }
    return true;
}

}