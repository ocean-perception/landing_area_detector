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
// #include "headers.h"
#include "helper.cpp"

namespace lad
{

    /**
 * @brief Returns (if present) the name of the vector layer that matches provided ID
 * 
 * @param id Layer ID number to be searched
 * @return std::string 
 */
    std::string Pipeline::getLayerName(int id)
    {
        if (mapLayers.empty())
            return "EMPTY_VECTOR";
        if (id < 0)
            return "INVALID_ID";
        // Check each raster in the array, compare its ID against search index
        for (auto layer : mapLayers)
        {
            if (layer.second->getID() == id)
                return layer.first;
        }
        return "NO_LAYER";
    }

    /**
 * @brief Returns (if present) the ID of the vector layer that matches provided name
 * 
 * @param name Layer name to be searched
 * @return int ID of the layer, if found any. Returns LAYER_NOTFOUN 
 */
    int Pipeline::getLayerID(std::string name)
    {
        if (mapLayers.empty())
            return LAYER_EMPTY;
        if (isValid(name) == false)
            return LAYER_INVALID_NAME;
        // Check each raster in the array, compare its ID against search index
        auto layer = mapLayers.find(name);

        if (layer != mapLayers.end())
            return layer->second->getID();

        return LAYER_NOT_FOUND;
    }

    int Pipeline::setLayerID(std::string name, int id)
    {
        if (mapLayers.empty())
            return LAYER_EMPTY;
        if (isValid(name) == false)
            return LAYER_INVALID_NAME;
        if (isAvailable(id) == false)
            return ID_TAKEN;

        // Check each raster in the array, compare its ID against search index
        auto layer = mapLayers.find(name);
        if (layer != mapLayers.end())
            return layer->second->setID(id);

        return LAYER_NOT_FOUND;
    }

    /**
 * @brief Overwrite the name of a layer (if present) with a unique string. 
 * @details If provided string 
 * @param id 
 * @return std::string 
 */
    int Pipeline::setLayerName(int id, std::string newName)
    {
        if (mapLayers.empty())
            return LAYER_NONE; // Layers vector is empty
        if (isValid(id) == false)
            return LAYER_INVALID_ID; // Provided ID is invalid
        // Check each raster in the array, compare its ID against search index
        // WARNING: TODO: Check if newName is already taken
        for (auto layer : mapLayers)
        {
            if (layer.second->getID() == id)
            {
                layer.second->layerName = newName;
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
    int Pipeline::getTotalLayers(int type)
    {
        if (type == LAYER_ANYTYPE)
            return mapLayers.size();
        return -1;
    }

    /**
 * @brief Determine if a given layer ID is valid. It checks both availability and correctness
 * 
 * @param checkID ID to be evaluated
 * @return int Evaluation status. If valid returns LAYER_OK, else return corresponding error code
 */
    int Pipeline::isValidID(int checkID)
    {
        if (checkID < 0)
            return LAYER_INVALID_ID; // First, we check is a positive ID value

        if (mapLayers.empty()) // If Layers vector is empty, then chckID is definitelty available
            return LAYER_OK;

        for (auto layer : mapLayers)
        {
            if (layer.second->getID() == checkID) // The checkID is already taken, return correspoding error code
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
    int Pipeline::isValidName(std::string checkName)
    {
        if (isValid(checkName) == false)
            return LAYER_INVALID_NAME; // First, we check is a positive ID value
        // Now we must test that the name follows the target convention: alphanumeric with {-_} as special characters
        // Use regex to determine if any foreing character is present
        //    std::regex rgx("/^[a-zA-Z0-9]/g",std::regex_constants::egrep); // Regex list: will retrieve invalid characters
        if (mapLayers.empty()) // If Layers vector is empty, then given name is definitelty available
            return LAYER_OK;
        // TODO complete string based name match against all the other names
        for (auto layer : mapLayers)
        {
            if (checkName == layer.second->layerName)
            { // The checkID is already taken, return correspoding error code
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
    int Pipeline::removeLayer(std::string name)
    {
        // First we verify the stack is not empty
        if (mapLayers.empty())
            return LAYER_EMPTY;
        //then we go through each layer
        mapLayers.erase(name);

        return NO_ERROR;
    }

    /**
 * @brief Remove a Layer identified by its name
 * 
 * @param name name of the layer to be removed
 * @return int returns success if layer was found and removed. Otherwise, it will send the error code
 */
    int Pipeline::removeLayer(int id)
    {
        // First we verify the ID
        if (id < 0)
            return LAYER_INVALID_ID;
        // Then we check the stack size
        if (mapLayers.empty())
            return LAYER_EMPTY;
        //then we go through each layer
        for (auto it : mapLayers)
        {
            if (it.second->getID() == id)
            { // found it!
                // LUT_ID.at(it.second->getID()) = ID_AVAILABLE;
                mapLayers.erase(it.first);
                break; // if we don't break now we will get a segfault (the vector iterator is broken)
            }
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
    int Pipeline::createLayer(std::string name, int type)
    {
        // Let's check name
        if (isValidName(name) != LAYER_OK)
        {
            cout << "[Pipeline] Invalid layer name: " << name << endl;
            return LAYER_INVALID_NAME;
        }
        int newid = getValidID();
        // Type can be any of enumerated types, or any user defined
        if (type == LAYER_VECTOR)
        {
            // cout << "[Pipeline] Creating VECTOR layer: " << name << endl;
            std::shared_ptr<lad::VectorLayer> newLayer = std::make_shared<lad::VectorLayer>(name, newid);
            // Layers.push_back(newLayer);
            mapLayers.insert(make_pair(name, newLayer));
        }
        // Type can be any of enumerated types, or any user defined
        if (type == LAYER_RASTER)
        {
            // cout << "[Pipeline] Creating RASTER layer" << endl;
            std::shared_ptr<lad::RasterLayer> newLayer = std::make_shared<lad::RasterLayer>(name, newid);
            // Layers.push_back(newLayer);
            mapLayers.insert(make_pair(name, newLayer));
        }
        // Type can be any of enumerated types, or any user defined
        if (type == LAYER_KERNEL)
        {
            // cout << "[Pipeline] Creating KERNEL layer" << endl;
            std::shared_ptr<lad::KernelLayer> newLayer = std::make_shared<lad::KernelLayer>(name, newid);
            // Layers.push_back(newLayer);
            mapLayers.insert(make_pair(name, newLayer));
        }
        return newid;
    }

    /**
 * @brief Export a given layer (by name) to the file 
 * 
 * @param name 
 * @param outfile 
 * @param coords 
 * @return int 
 */
    int Pipeline::exportLayer(std::string name, std::string outfile, int format, int coord_sys)
    { // Export a given layer in the stack identified by its name, to
        if (name.empty())
        {
            cout << red << "[exportLayer] Error when trying to export layer, no valid name was provided" << reset << endl;
            return ERROR_MISSING_ARGUMENT;
        }
        std::string exportName;
        //now, we pull the Layer from the stack
        shared_ptr<Layer> apLayer = getLayer(name);
        // getlayer()
        if (apLayer == nullptr)
        { // failed to retrieve Layer "name" from the stack
            cout << red << "[exportLayer] Failed to retrieve Layer [" << name << "] from stack" << reset << endl;
            return ERROR_WRONG_ARGUMENT;
        }

        if (outfile.empty())// if no outfile name was provided, use internally defined fileName
            exportName = apLayer->fileName;
        else
            exportName = outfile;

        int type = apLayer->getType();
        shared_ptr<VectorLayer> apVector;
        shared_ptr<RasterLayer> apRaster;
        shared_ptr<KernelLayer> apKernel;

        // 6D matrix geotransformation is assumed to be internally stored in the geotiff object
        // double *adfGeoTransform;
        // adfGeoTransform = apInputGeotiff->GetGeoTransform();
        // if (adfGeoTransform == nullptr)
        // {
        //     cout << red << "[exportLayer] some error ocurred while calling GetGeoTransform() " << reset << endl;
        //     return ERROR_WRONG_ARGUMENT;
        // }

        // now, depending on the type of Layer and target format, we operate
        switch (type)
        {
        case LAYER_RASTER:
            if (verbosity > VERBOSITY_0)
                cout << "[exportLayer] Exporting RasterLayer [" << yellow << name << reset << "] to file [" << yellow << outfile << reset << "]" << endl; 
            apRaster = dynamic_pointer_cast<RasterLayer>(apLayer);
            apRaster->writeLayer(exportName, format, coord_sys);
            break;

        case LAYER_VECTOR:
            if (verbosity > VERBOSITY_0)
                cout << "[exportLayer] Exporting VectorLayer [" << yellow << name << reset << "] to file [" << yellow << outfile << reset << "]" << endl; 
            // cout << "Export VECTOR" << endl;
            apVector = dynamic_pointer_cast<VectorLayer>(apLayer);
            apVector->writeLayer(exportName, format, geoProjection.c_str(), coord_sys, geoTransform);
            break;

        case LAYER_KERNEL:
            cout << "Export RASTER" << endl;
            cout << yellow << "KERNEL_RASTER export feature not implemented yet from stack pipeline" << reset << endl;
            // apKernel = dynamic_pointer_cast<KernelLayer>(apLayer);
            break;

        default:
            cout << red << "[exportLayer] Error layer [" << name << " is of unknown type [" << type << "]" << reset << endl;
            return ERROR_WRONG_ARGUMENT;
            break;
        }
        return NO_ERROR;
    }

    /**
 * @brief Retrieve the first valid available ID from the Look-up table
 * 
 * @return int valid ID ready to be consumed in a createLayer call
 */
    int Pipeline::getValidID()
    {
        // use internal counter as valid ID generator
        return (currentAvailableID++);
    }

    /**
 * @brief Show t(if any) the summary information for each layer
 * 
 * @return int LAYER_NONE if Layers vector is empty, LAYER_OK otherwise
 */
    int Pipeline::showLayers(int layer_type)
    {
        if (!mapLayers.size())
        {
            cout << "No layer to show" << endl;
            return LAYER_NONE;
        }
        for (auto it : mapLayers)
        {
            if ((it.second->getType() == layer_type) || (layer_type == LAYER_ANYTYPE))
                it.second->showInformation();
        }
        return LAYER_OK;
    }

    /**
     * @brief Creates and insert a new Kernel layer which contains a rectangular shape as template in the image container
     * @details The horizontal and vertical pixel resolutions are retrieved from the Pipeline geotiff container
     * @param name Name of the new layer to be inserted
     * @param width width of the pattern in spatial units. It will define the number of columns of the image
     * @param length length of the pattern in spatial units. It will define the numer of rows of the image
     * @return int Error code, if any
     */
    int Pipeline::createKernelTemplate (std::string name, double width, double length, int morphtype){
        double sx = geoTransform[GEOTIFF_PARAM_SX];
        if (sx == 0) sx = 1;
        double sy = geoTransform[GEOTIFF_PARAM_SY];
        if (sy == 0) sy = 1;
        return (createKernelTemplate (name, width, length, sx, sy, morphtype));
    }


    /**
     * @brief Creates and insert a new Kernel layer which contains a rectangular shape as template in the image container
     * 
     * @param name Name of the new layer to be inserted
     * @param width width of the pattern in spatial units. It will define the number of columns of the image
     * @param length length of the pattern in spatial units. It will define the numer of rows of the image
     * @param sx Horizontal resolution per pixel, in spatial units 
     * @param sy Vertical resolution per pixel, in spatial units
     * @return int Error code, if any
     */
    int Pipeline::createKernelTemplate (std::string name, double width, double length, double sx, double sy, int morphtype){
        // first, we verify that the layer name is available
        if (!isValid(name)){
            cout << red << "[createKernelTemplate] Error when creating new layer, name [" << name << "] invalid" << reset << endl; 
            return LAYER_INVALID_NAME;
        }
        auto layer = mapLayers.find(name);
        if (layer != mapLayers.end()){ //we had a match! we exit
            cout << red << "[createKernelTemplate] Error when creating new layer, name [" << name << "] is already taken" << reset << endl; 
            return LAYER_DUPLICATED_NAME;
        }
        // now, we check that the input arguments are valid (basically, they must be positive)
        // Pixel resolution can be negative is inherited from the geoTIFF metadata, we can correct that
        if ((width <= 0) || (length <= 0)){
            cout << red << "[createKernelTemplate] Invalid dimensions: [" << width << " x " << length << "]. They must positive." << reset << endl; 
            return ERROR_WRONG_ARGUMENT;
        }
        sx = fabs(sx);
        sy = fabs(sy);
        if ((sx * sy) == 0){
            cout << red << "[createKernelTemplate] Invalid pixel resolution: [" << sx << " x " << sy << "]. They must non-zero." << reset << endl; 
            return ERROR_WRONG_ARGUMENT;
        }

        int ncols = ceil (width / sx); // each pixel is of sx horizontal size
        int nrows = ceil (length / sy); // each pixel is of sy vertical size
        // create the template 
        cv::Mat A = cv::getStructuringElement(morphtype,cv::Size(ncols,nrows));
        // create a new KernelLayer
        createLayer(name, LAYER_KERNEL);
        uploadData(name, (void *) &A);
        // apLayer->setRotation(apLayer->getRotation()); // DIRTY HACK TO FORCE RECOMPUTING THE INTERNAL rotatedData rasterLayer;
        if (verbosity > 0){
            shared_ptr<KernelLayer> apLayer = dynamic_pointer_cast<KernelLayer>(getLayer(name));
            namedWindow(name);
            imshow(name, apLayer->rasterData * 255);
            resizeWindow(name, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
        }
        return NO_ERROR;
    }

    /**
 * @brief Upload data to the corresponding container in the layer identified by its id. 
 * Dynamic typecasting of data is performed according to the container data type
 * 
 * @param id Identifier of the target layer
 * @param data Source data to be copied
 * @return int Error code, if any. LAYER_OK if the process is completed succesfully
 */
    int Pipeline::uploadData(int id, void *data)
    {
        if (isValid(id) == false)
        {
            return LAYER_INVALID_ID; // The provided ID is invalid
        }
        if (isAvailable(id) == true)
        {
            return LAYER_NOT_FOUND; // No layer was found with that ID
        }

        for (auto it : mapLayers)
        {
            if (it.second->getID() == id)
                uploadData(it.second->layerName, data);
        }
        return LAYER_OK;
    }

    /**
 * @brief Alternative to uploadData(int,data) where the layer is identified by its name
 * @param id Name of the target layer
 * @param data Source data to be copied
 * @return int Error code, if any. LAYER_OK if the process is completed succesfully
 */
    int Pipeline::uploadData(std::string name, void *data)
    {
        // Check if we the name is valid (non-empty)
        if (name.empty())
            return LAYER_INVALID_NAME;
        int retval = NO_ERROR;        
        auto layer = getLayer(name);
        if (layer == nullptr){
            cout << red << "[uploadData] Error when getting layer: [" << name << "]" << reset << endl;
            return LAYER_NOT_FOUND;
        }
        
        int type = layer->getType(); // let's operate according to the layer type
        // WARNING: if we change these 'if' to switch , -fPermissive will trigger error
        if (type == LAYER_VECTOR)
        {
            auto v = std::dynamic_pointer_cast<lad::VectorLayer>(layer);
            retval = v->loadData((std::vector<cv::Point2d> *)data);
        }
        else if (type == LAYER_RASTER)
        {
            auto r = std::dynamic_pointer_cast<lad::RasterLayer>(layer);
            retval = r->loadData((cv::Mat *)data);
        }
        else if (type == LAYER_KERNEL)
        {
            auto k = std::dynamic_pointer_cast<lad::KernelLayer>(layer);
            retval = k->loadData((cv::Mat *)data);
            // now we trigger an update in the rotatedData matrix
            k->setRotation(k->getRotation());
        }
        return retval;
    }

    /**
 * @brief Shows summary/detailed information of the pipeline object
 * 
 * @param level Level of verbosity
 * @return int Returns NO_ERROR if current pipeline is a valid (non-empty) one.
 */
    int Pipeline::showInfo(int level)
    {
        // Geotiff input information
        int retval = NO_ERROR;
        cout << endl << cyan << "****** Geotiff Summary *****************" << reset << endl;
        // \todo print geotransformation (if available) from the template
        cout << cyan
             << "++++++ Layers ++++++++++++++++++++++++++" << reset << endl;
        if (mapLayers.empty())
        {
            cout << yellow << "None" << reset << endl;
            retval = ERROR_LAYERS_EMPTY;
        }
        else
        {
            cout << "Total of Layers: " << mapLayers.size() << endl;
            showLayers();
        }
        cout << cyan << "****** End of Summary ******************" << reset << endl;
        return retval;
    }

    /**
     * @brief Reads a geoTIFF file, imports the raster data and also creates a valida data mask 
     * 
     * @param inputFile Name of the geoTIFF file
     * @param rasterLayer Name of the raster layer that will hold the valid raster data
     * @param maskLayer Name of the raster layer that will hold the valida data mask
     * @return int Eror code, if any
     */
    int Pipeline::readTIFF(std::string inputFile, std::string rasterLayer, std::string maskLayer){
        // first we create the destination layers, if missing
        if (isAvailable(rasterLayer)){
            createLayer(rasterLayer, LAYER_RASTER);
        }
        if (isAvailable(maskLayer)){
            createLayer(maskLayer, LAYER_RASTER);
        }
        auto apRaster = dynamic_pointer_cast<RasterLayer>(getLayer(rasterLayer));
        if (apRaster == nullptr){
            cout << red << "[readTIFF] Error retrieving layer [" << rasterLayer << "]" << endl;
        }
        auto apMask = dynamic_pointer_cast<RasterLayer>(getLayer(maskLayer));
        if (apMask == nullptr){
            cout << red << "[readTIFF] Error retrieving layer [" << maskLayer << "]" << endl;
        }

        if (apRaster->readTIFF(inputFile) != NO_ERROR){
            cout << red << "[readTIFF] Error reading file [" << inputFile << "]" << endl;
            return ERROR_GDAL_FAILOPEN;
        }
        // transfer the recently computed mask layer from the source raster layer
        apRaster->rasterMask.copyTo(apMask->rasterData);
        apRaster->rasterMask.copyTo(apMask->rasterMask);
        // apMask->rasterMask = cv::Mat::ones(apMask->rasterData.size(), CV_8UC1);
        // update layerDimensions array, as they are needed when exporting as geoTIFF
        // \todo use actual size from the raster container?
        apMask->layerDimensions[1] = apMask->rasterData.rows;
        apMask->layerDimensions[0] = apMask->rasterData.cols;
        apMask->copyGeoProperties(apRaster);
        apMask->setNoDataValue(DEFAULT_NODATA_VALUE);
        return NO_ERROR;
    }

    /**
 * @brief Extract contours from rasterName layer and export it as vector to countourName
 * 
 * @param rasterName Input binary raster layer to be analized 
 * @param contourName Output vector layer that will contain the largest found contour
 * @param showImage flag indicating if we need to show the images
 * @return int Error code - if any.
 */
    int Pipeline::extractContours(std::string rasterName, std::string contourName, int showImage)
    {

        vector<vector<Point>> contours; // find contours of the DataMask layer
        //pull access to rasterMask
        std::shared_ptr<RasterLayer> apRaster;
        apRaster = dynamic_pointer_cast<RasterLayer>(getLayer(rasterName));

        cv::findContours(apRaster->rasterData, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE); // obtaining only 1st level contours, no children
        cout << "[extractContours] #contours detected: " << contours.size() << endl;
        if (contours.empty())
        {
            cout << red << "No contour line was detected!" << endl;
            return ERROR_CONTOURS_NOTFOUND;
        }

        // We need to extract the largest polygon, which should provide the main data chunk boundary
        // Disconected bathymetries are not expected
        int largest = 0;
        vector<vector<Point>> good_contours; // find contours of the DataMask layer

        int k = 0;
        for (auto it : contours)
        {
            if (it.size() > largest)
            {
                largest = it.size();
                if (!good_contours.empty())
                    good_contours.pop_back();
                good_contours.push_back(it); // hack to keep it at a single element
            }
        }
        cout << "[extractContours] Largest contour: " << yellow << largest << reset << endl;

        // WARNING: contours may provide false shapes when valid data mask reaches any image edge.
        // SOLUTION: expand +1px the image canvas on every direction, or remove small bathymetry section (by area or number of points)
        // See: copyMakeBorder @ https://docs.opencv.org/3.4/dc/da3/tutorial_copyMakeBorder.html UE: BORDER_CONSTANT (set to ZERO)
        if (showImage)
        {
            Mat boundingLayer = Mat::zeros(apRaster->rasterData.size(), CV_8UC1); // empty mask
            int n = 1;
            for (const auto &contour : good_contours)
            {
                drawContours(boundingLayer, contours, -1, Scalar(255 * n / good_contours.size()), 1); // overlay contours in new mask layer, 1px width line, white
                n++;
            }
            namedWindow(contourName, WINDOW_NORMAL);
            imshow(contourName, boundingLayer);
            resizeWindow(contourName, 800, 800);
        }

        // COMPLETE: now we export the data to the target vector layer contourName
        // if it already exist, overwrite data?
        //pull access to rasterMask
        std::shared_ptr<VectorLayer> apVector;
        if (getLayerID(contourName) >= 0)
        {   // it already exist!
            apVector = dynamic_pointer_cast<VectorLayer>(getLayer(contourName));
        }
        else
        {   // nope, we must create it
            int newid = createLayer(contourName, LAYER_VECTOR);
            apVector = dynamic_pointer_cast<VectorLayer>(getLayer(contourName));
            // cout << "\tCreated new vector layer: [" << contourName << "] with ID [" << newid << "]" << endl;
        }

        vector<Point> contour = good_contours.at(0); //a single element is expected because we force it
        for (auto it : contour)
        { //deep copy by iterating through the vector. = operator non-existent por Point to Point2d (blame OpenCV?)
            apVector->vectorData.push_back(it);
        }
        // Finally, we set the correct coordinate flag to PIXEL SPACE (we processed everything as an image)
        apVector->coordinateSpace = PIXEL_COORDINATE;
        // cout << "Vector data: " << apVector->vectorData.size() << endl;
        return NO_ERROR;
    }

    /**
 * @brief Search a Layer by its ID and return a shared_ptr to it. If the provided ID is invalid or non-existent a NULL pointer is returned
 * 
 * @param id ID of the layer to be retrieved
 * @return std::shared_ptr<Layer> pointer to the Layer with the given id
 */
    std::shared_ptr<Layer> Pipeline::getLayer(int id)
    {
        if (isValid(id) == false)
            return nullptr;
        if (mapLayers.empty())
            return nullptr;
        // now, it is safe to assume that such ID exists
        for (auto it : mapLayers)
        {
            if (it.second->getID() == id)
                return it.second;
        }
        return nullptr; // Curious thing, nothing was found!
    }

    /**
 * @brief Search a Layer by its ID and return a shared_ptr to it. If the provided ID is invalid or non-existent a NULL pointer is returned
 * 
 * @param name name of the layer to be retrieved
 * @return std::shared_ptr<Layer> pointer to the Layer with the given id
 */
    std::shared_ptr<Layer> Pipeline::getLayer(std::string name)
    {
        auto layer = mapLayers.find(name);
        if (layer == mapLayers.end())
            return nullptr;

        return (layer->second); //now we search it by ID
    }

    /**
 * @brief Returs true if the provided ID is valid. It does not check whether it is available in the current stack
 * 
 * @param id ID to be tested 
 * @return int true id ID is valid, false otherwise
 */
    int Pipeline::isValid(int id)
    {
        if (id < 0)
            return false;
        return true;
    }

    /**
     * @brief Returns true if the provided string is a valid layer name
     * 
     * @param str string to be tested as valid layer name
     * @return int true if valid, else return false
     */
    int Pipeline::isValid(std::string str)
    {
        // TODO: Add regexp for alphanumeric +set of special characters as valid set
        if (str.empty())
            return false;
        return true;
    } 

    /**
 * @brief Returns true if the provided ID is available. It also checks validity, for sanity reasons
 * 
 * @param id ID to be tested 
 * @return int true if ID is available, false otherwise
 */
    int Pipeline::isAvailable(int id)
    {
        if (id < 0)
            return false; // ID sanity check, we prefer to flag it as unavailable if invalid

        for (auto it : mapLayers){
            if (it.second->getID() == id)   // is already taken?
                return false;           
        }
        return true;    // none found, therefore it must be available
    }

    /**
 * @brief Verify if the tested name is already taken by any existing layer in the current stack. This is used to avoid name duplication
 * 
 * @return int true if the name is available, false otherwise
 */
    int Pipeline::isAvailable(std::string str)
    {
        //iterate through all the existing layers, and compare against them
        if (isValid(str) == false)
            return false; // sanity check of its validity

        if (mapLayers.find(str) == mapLayers.end()) // not found? is available
            return true;
        else
            return false;
    }

    /**
     * @brief Compute the exclusion map (P3) using morphological operators (erode) from a vehicle footprint kernel against the bathymetry valida data map.
     * 
     * @param raster Raster layer indicating available valid bathymetry data (non-zero)
     * @param kernel Vehicle footprint described as a kernel layer
     * @param dst Name of the resulting raster layer that will containg the exclusion map. If not present in the stack, it wil be created
     * @return int 
     */

    int Pipeline::computeExclusionMap(std::string raster, std::string kernel, std::string dstLayer){
        // *****************************************
        // Validating input raster layer
        if (raster.empty()){
            cout << red << "[computeExclusionMap] Input raster layer name is empty" << reset << endl;
            return ERROR_WRONG_ARGUMENT;
        }
        auto apBase = mapLayers.find(raster);
        if (apBase == mapLayers.end()){
            cout << "[computeExclusionMap] Input raster [" << raster << "] not found in the stack" << endl;
            return LAYER_NOT_FOUND;
        }
        if (apBase->second->getType() != LAYER_RASTER){
            cout << "[computeExclusionMap] Input layer [" << raster << "] must be of type LAYER_RASTER" << endl;
            return LAYER_NOT_FOUND;
        }
        // *****************************************
        // Validating input kernel layer
        if (kernel.empty()){
            cout << red << "[computeExclusionMap] Input kernel layer name is empty" << reset << endl;
            return ERROR_WRONG_ARGUMENT;
        }
        auto apKernel = mapLayers.find(kernel);
        if (apKernel == mapLayers.end()){
            cout << "[computeExclusionMap] Input raster [" << kernel << "] not found in the stack" << endl;
            return LAYER_NOT_FOUND;
        }
        if (apKernel->second->getType() != LAYER_KERNEL){
            cout << "[computeExclusionMap] Input layer [" << kernel << "] must be of type LAYER_RASTER" << endl;
            return LAYER_NOT_FOUND;
        }
        // *****************************************
        // Validating output raster (exclusion map) layer
        if (dstLayer.empty()){
            cout << red << "[computeExclusionMap] Output raster layer name is empty" << reset << endl;
            return ERROR_WRONG_ARGUMENT;
        }
        // cout << "Searching [" << dstLayer << "] +++++++++++++++++++++" << endl; 
        auto apOutput = mapLayers.find(dstLayer); // not found? let's create it
        if (apOutput == mapLayers.end()){
            cout << green << "[computeExclusionMap] Output raster [" << dstLayer << "] not found in the stack. Creating" << reset << endl;
            createLayer(dstLayer, LAYER_RASTER);
            apOutput =  mapLayers.find(dstLayer);    //we get the pointer, it should appear now in the stack!
        }
        else if (apOutput->second->getType() != LAYER_RASTER){
            cout << "[computeExclusionMap] Output layer [" << dstLayer << "] must be of type LAYER_RASTER" << endl;
            return ERROR_WRONG_ARGUMENT;
        }
        
        // *****************************************
        // Applying erode
        shared_ptr<RasterLayer> apLayerR = dynamic_pointer_cast<RasterLayer>(apBase->second);
        shared_ptr<KernelLayer> apLayerK = dynamic_pointer_cast<KernelLayer>(apKernel->second);
        shared_ptr<RasterLayer> apLayerO = dynamic_pointer_cast<RasterLayer>(apOutput->second);
        // output is a binary image
        cv::erode(apLayerR->rasterData, apLayerO->rasterData, apLayerK->rotatedData);
        // we do not need to set nodata field for destination layer if we use it as mask
        // if we use it for other purposes (QGIS related), we can use a negative value to flag it
        apLayerO->copyGeoProperties(apLayerR); //let's copy the geoproperties
        apLayerO->setNoDataValue(DEFAULT_NODATA_VALUE);
        apLayerO->rasterMask = cv::Mat::ones(apLayerO->rasterData.size(), CV_8UC1);
        if (verbosity > 0){
            namedWindow (dstLayer);
            imshow (dstLayer, apLayerO->rasterData);
            resizeWindow(dstLayer, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
        }
        // return no error
        return NO_ERROR;
    }

    /**
     * @brief Show the Layer data container as an image. Use color mapping to improve visibility.
     * 
     * @param layer Layer (raster or kernel) to be shown
     * @param colormap OpenCV valid colormap ID
     * @return int Error code, if any
     */
    int Pipeline::showImage(std::string layer, int colormap){
        // first, we check the layer is available and is of Raster or Kernel type (vector plot not available yet)
        if (getLayer(layer) == nullptr){
            cout << "[showImage] layer [" << yellow << layer << reset << "] not found..." << endl;
            return LAYER_NOT_FOUND;
        }
        int type = getLayer(layer)->getType();  // being virtual, every derived class must provide a run-time solution for getType
        if (type == LAYER_VECTOR){
            cout << "[showImage] layer [" << yellow << layer << reset << "] is of type LAYER_VECTOR. Visualization mode not supported yet." << endl;
            return ERROR_WRONG_ARGUMENT;
        }
        // no we operate according to the layer type. Both RASTER and KERNEL layer have the rasterData matrix as basic container.
        // we must check if the container is non-empty
        // WARNING: As KERNEL is a derived class from RASTER we could downcast to RASTER without risking object slicing, and still be able to retrieve the rasterData
        if (type == LAYER_RASTER){
            shared_ptr<RasterLayer> apLayer = dynamic_pointer_cast<RasterLayer> (getLayer(layer));
            if (apLayer == nullptr){
                cout << red << "[showImage] Unexpected error when downcasting RASTER layer [" << yellow << layer << "]" << reset << endl;
                cout << cyan << "at" << __FILE__ << ":" << __LINE__ << reset << endl;
                return ERROR_WRONG_ARGUMENT;
            }
            if (apLayer->rasterData.empty()){
                cout << "[showImage] rasterData in raster layer [" << yellow << layer << reset << "] is empty. Nothing to show" << endl;
                return NO_ERROR;                
            }
            // correct data range to improv
            cv::Mat dst = apLayer->rasterData.clone();
            if (useNodataMask){
                apLayer->updateMask();
                cv::normalize(dst, dst, 0, 255, NORM_MINMAX, CV_8UC1, apLayer->rasterMask); // normalize within the expected range 0-255 for imshow
            }
            else{
                cv::normalize(dst, dst, 0, 255, NORM_MINMAX, CV_8UC1); // normalize within the expected range 0-255 for imshow
            }
            // apply colormap for enhanced visualization purposes
            cv::applyColorMap(dst, dst, colormap);
            namedWindow(apLayer->layerName);
            imshow(apLayer->layerName, dst);
            resizeWindow(apLayer->layerName, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
        }

        if (type == LAYER_KERNEL){
            shared_ptr<KernelLayer> apLayer = dynamic_pointer_cast<KernelLayer> (getLayer(layer));
            if (apLayer == nullptr){
                cout << red << "[showImage] Unexpected error when downcasting RASTER layer [" << yellow << layer << "]" << reset << endl;
                cout << cyan << "at" << __FILE__ << ":" << __LINE__ << reset << endl;
                return ERROR_WRONG_ARGUMENT;
            }
            if (apLayer->rasterData.empty()){
                cout << "[showImage] rasterData in kernel layer [" << yellow << layer << reset << "] is empty. Nothing to show" << endl;
                return NO_ERROR;                
            }
            namedWindow(apLayer->layerName);
            imshow(apLayer->layerName, apLayer->rasterData);
            resizeWindow(apLayer->layerName, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
            // resizeWindow(apLayer->layerName, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
            namedWindow(apLayer->layerName + "_rotated");
            imshow(apLayer->layerName + "_rotated", apLayer->rotatedData);
            resizeWindow(apLayer->layerName + "_rotated", DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
            // resizeWindow(apLayer->layerName + "_rotated", DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
        }

        return NO_ERROR;
    }

    
    /**
     * @brief Use geoTIFF related structures from a reference raster layer as template for the whole stack
     * 
     * @param reference Name of the RasterLayer that will be used as template
     * @return int Error code, if any
     */
    int Pipeline::setTemplate (std::string reference){
        if (isAvailable(reference)){
            cout << red << "[setTemplate] Template layer does not exist: [" << reference << "]" << endl;
            return ERROR_WRONG_ARGUMENT;
        }
        auto ap = dynamic_pointer_cast<RasterLayer> (getLayer(reference));
        if (ap == nullptr){
            cout << red << "[setTemplate] Provided layer [" << reference << "] must be of type LAYER_RASTER" << endl;
            return ERROR_WRONG_ARGUMENT;
        }
        // Now we start copying the parameters from the raster layer to the stack
        for (int i=0; i<6; i++)
            geoTransform[i] = ap->transformMatrix[i];
        geoProjection = ap->layerProjection;     // copy the WKT projection string
        return NO_ERROR;
    }

    /**
     * @brief Apply a raster/kernel mask to a raster input layer and store the result in another layer.
     * 
     * @param src Name of an existing raster or kernel layer that will be used as the source
     * @param mask Name of an existing raster or kernel layer that will be used as mask. If layer is of type kernel, rotatedData can be used as mask
     * @param dst Name of the destination layer. If it doesn't exist it is created and inserted into the stack
     * @param useRotated Flag indicating if rotatedData must be used insted of rasterData. Only valid if mask layer is of type kernel
     * @return int Error code, if any
     */
    int Pipeline::maskLayer(std::string src, std::string mask, std::string dst, int useRotated){
        // check that both src and mask layers exist. If not, return with error
        if (isAvailable(src)){
            cout << red << "[maskLayer] source layer ["  << src << "] does not exist" << reset << endl;
            return LAYER_NOT_FOUND;
        }
        if (isAvailable(mask)){
            cout << red << "[maskLayer] mask layer ["  << mask << "] does not exist" << reset << endl;
            return LAYER_NOT_FOUND;
        }
        if (isAvailable(dst)){
            cout << "[maskLayer] destination layer ["  << yellow << dst << yellow<< "] does not exist. Creating..." << reset << endl;
            createLayer(dst, LAYER_RASTER);
        }

        shared_ptr<RasterLayer> apSrc = dynamic_pointer_cast<RasterLayer> (getLayer(src));
        shared_ptr<RasterLayer> apDst = dynamic_pointer_cast<RasterLayer> (getLayer(dst));

        apDst->rasterData = cv::Mat::ones(apSrc->rasterData.size(), CV_64FC1) * apSrc->getNoDataValue();
        apDst->copyGeoProperties (apSrc);
        apDst->setNoDataValue(apSrc->getNoDataValue());

        int type = getLayer(mask)->getType();
        if (type == LAYER_RASTER){
            auto apMask = dynamic_pointer_cast<RasterLayer>(getLayer(mask));
            apSrc->rasterData.copyTo(apDst->rasterData, apMask->rasterData); // dst.rasterData use non-null values as binary mask ones
       }
        else if (type == LAYER_KERNEL){
            // we may or may not use rotatedData depending on the input flag
            auto apMask = dynamic_pointer_cast<KernelLayer>(getLayer(mask));
            if (useRotated)
                apSrc->rasterData.copyTo(apDst->rasterData, apMask->rotatedData);
            else
                apSrc->rasterData.copyTo(apDst->rasterData, apMask->rasterData);
        }
        else{
            cout << red << "[maskLayer] mask layer [" << getLayer(mask)->layerName << "] must be either raster or kernel" << reset << endl;
            return ERROR_WRONG_ARGUMENT;
        }
        apDst->updateMask();

        return NO_ERROR;
    } 

    /**
     * @brief 
     * 
     * @param src 
     * @param dst 
     * @param angleThreshold 
     * @return int 
     */
    int Pipeline::compareLayer(std::string src, std::string dst, double threshold, int cmp){
        // check that both src and mask layers exist. If not, return with error
        if (isAvailable(src)){
            cout << red << "[thresholdLayer] source layer ["  << src << "] does not exist" << reset << endl;
            return LAYER_NOT_FOUND;
        }
        if (isAvailable(dst)){
            cout << "[thresholdLayer] destination layer ["  << yellow << dst << yellow<< "] does not exist. Creating..." << reset << endl;
            createLayer(dst, LAYER_RASTER);
        }

        shared_ptr<RasterLayer> apSrc = dynamic_pointer_cast<RasterLayer> (getLayer(src));
        shared_ptr<RasterLayer> apDst = dynamic_pointer_cast<RasterLayer> (getLayer(dst));
        apDst->copyGeoProperties(apSrc);
        apDst->setNoDataValue(DEFAULT_NODATA_VALUE);
        cv::compare(apSrc->rasterData, threshold, apDst->rasterData, cmp);  // create a no-data mask
        return NO_ERROR;
    }

    /**
     * @brief Rotates a kernel layer by modifying its rotation parameter and triggering an update in the rotatedData
     * 
     * @param src Name of the source KernelLayer
     * @param angle Angle in degrees
     * @return int Error code, if any.
     */
    int Pipeline::rotateLayer(std::string src, double angle){
        // first we check if the layer exist in the stack
        if (isAvailable(src)){
            cout << red << "[rotateLayer] Error layer [" << src << "] not found" << reset << endl;
            return LAYER_NOT_FOUND;
        }
        if (getLayer(src)->getType() != LAYER_KERNEL){
            cout << red << "[rotateLayer] Error layer [" << src << "] is not of type KERNEL" << reset << endl;
            return ERROR_WRONG_ARGUMENT;
        }
        shared_ptr<KernelLayer> apLayer = dynamic_pointer_cast<KernelLayer>(getLayer(src));
        if (apLayer == nullptr){
            cout << red << "[rotateLayer] Unknown error retrieving [" << src << "] from the stack. Returned a nullptr" << reset << endl;
            return LAYER_INVALID;
        }
        apLayer->setRotation(angle);
        return NO_ERROR;
    } 

    /**
     * @brief Apply a low pass filter to the input raster Layer
     * 
     * @param src Input raster layer
     * @param dst Destination raster Layer
     * @return int 
     */
    int Pipeline::lowpassFilter(std::string src, std::string dst, cv::Size filterSize, int filterType){
        // we ignore filter type for the preliminary implementation. Future dev may include bilateral, box, user-defined, or raster defined kernel for filtering purposes
        // first we check if the layer exist in the stack
        if (isAvailable(src)){
            cout << red << "[lowpassFilter] Error layer [" << src << "] not found" << reset << endl;
            return LAYER_NOT_FOUND;
        }
        if (isAvailable(dst)){
            cout << "[lowpassFilter] destination layer ["  << yellow << dst << reset << "] does not exist. Creating..." << reset << endl;
            createLayer(dst, LAYER_RASTER);
        }
        
        shared_ptr<RasterLayer> apSrc = dynamic_pointer_cast<RasterLayer> (getLayer(src));
        shared_ptr<RasterLayer> apDst = dynamic_pointer_cast<RasterLayer> (getLayer(dst));
        if (apSrc == nullptr){
            cout << "ApSrc error" << endl;
            return -1;
        }
        if (apDst == nullptr){
            cout << "ApDst error" << endl;
            return -1;
        }
        int cols = apSrc->rasterData.cols;
        int rows = apSrc->rasterData.rows;

        // WARNING: we asume that the output range of the filter is within the input range of the bathymetry values
        // as we are removing the non validad data point (not -defined)
        double srcNoData = apSrc->getNoDataValue(); //we inherit ource no valid data value
        apDst->setNoDataValue(srcNoData);
        if (verbosity > VERBOSITY_0){
            cout << "[p.lowpassFilter] Source NoData value: " << srcNoData << endl;
            cout << "[p.lowpassFilter] Target NoData value: " << apDst->getNoDataValue() << endl;
            cout << "[lowpassFilter] Input raster size: " << apSrc->rasterData.size() << endl; 
            cout << "[lowpassFilter] Filter size: " << filterSize.width << " x " << filterSize. height << endl; 
        }
        apDst->copyGeoProperties(apSrc);
        apDst->rasterData = cv::Mat(apSrc->rasterData.size(), CV_64FC1, srcNoData); 
        // we create a matrix with NOVALID data
        cv::Mat  roi_image = cv::Mat(apSrc->rasterData.size(), CV_8UC1); // create global valid_data mask
        cv::compare(apSrc->rasterData, srcNoData, roi_image, CMP_NE); // ROI at the source data level

        for (int row=0; row<rows; row++){
            for (int col=0; col<cols; col++){
                if (roi_image.at<unsigned char>(cv::Point(col, row))){
                    int rt = row - filterSize.height/2;
                    if (rt < 0) rt = 0;
                    int rb = row + filterSize.height/2;
                    if (rb > rows) rb = rows;
                    int cl = col - filterSize.width/2;
                    if (cl < 0) cl = 0;
                    int cr = col + filterSize.width/2;
                    if (cr > cols) cr = cols;

                    cv::Mat subImage = apSrc->rasterData(cv::Range(rt, rb), cv::Range(cl, cr));
                    cv::Mat roi_patch = roi_image(cv::Range(rt, rb), cv::Range(cl, cr));

            // TODO: failing to compute when data is partially available at the edges
                    float acum = cv::sum(subImage)[0];
                    float den  = cv::sum(roi_patch)[0] / 255;
                    apDst->rasterData.at<double>(cv::Point(col, row)) = acum/den;
                }
                else
                    apDst->rasterData.at<double>(cv::Point(col, row)) = srcNoData;
            }
        }
        // cv::Mat mask;
        // cv::compare(apSrc->rasterData, apSrc->getNoDataValue(), mask, CMP_EQ);
        if (verbosity > VERBOSITY_1){
            // now we must clip-out those points that were labelled as NODATA in the source layer
            // namedWindow(src + "_mask");
            // imshow(src + "_mask", mask);
        }
        // the mask contains true (255) for thos invalid points
        // roi_image = cv::Mat(apSrc->rasterData.size(), CV_64FC1);
        // roi_image.copyTo(apDst->rasterData, mask);

        apDst->updateMask();
        apDst->updateStats();
        return NO_ERROR;
    }

    /**
     * @brief Implementation of low-pass filter that uses applyWindowFilter
     * 
     * @param src source raster to be filtered
     * @param kernel raster containint the sliding structuring element for filtering
     * @param mask raster containing the global mask, can be used as ROI
     * @param dst name of the raster layer that will store the resulting image
     * @return int Error code, if any
     */
    int Pipeline::lowpassFilter (std::string src, std::string kernel, std::string mask, std::string dst){
        applyWindowFilter(src, kernel, mask, dst, FILTER_MEAN);
    }

    /**
     * @brief Computes the seafloor height map by direct substraction of the raw and filtered maps 
     * @details This version relies on the previous computation of a base filtered map, eliminating the duplicity when exporting the intermediate products
     * @param src raster of the raw bathymetry
     * @param filt raster containing a filtered version of the bathymetry
     * @param dst name of the layer that will contain the resulting height map
     * @return int 
     */
    int Pipeline::computeHeight (std::string src, std::string filt, std::string dst){
        // src contains the RAW bathymetry. We substract the filtered map from it to obtain the height
        if (isAvailable(dst)){
            cout << "[computeHeight] Destination layer ["  << yellow << dst << reset << "] does not exist. Creating..." << reset << endl;
            createLayer(dst, LAYER_RASTER);
        }
        auto apSrc  = dynamic_pointer_cast<RasterLayer> (getLayer(src));
        auto apFilt = dynamic_pointer_cast<RasterLayer> (getLayer(filt));
        auto apDst  = dynamic_pointer_cast<RasterLayer> (getLayer(dst));

        if (apSrc == nullptr){
            cout << "apSrc error" << endl;
            return -1;
        }
        if (apFilt == nullptr){
            cout << "apSrc error" << endl;
            return -1;
        }
        if (apDst == nullptr){
            cout << "apDst error" << endl;
            return -1;
        }

        apDst->copyGeoProperties(apSrc);
        apDst->setNoDataValue(DEFAULT_NODATA_VALUE);
        cv::Mat dest; //(apSrc->rasterData.size(), CV_64FC1, DEFAULT_NODATA_VALUE);
        dest = - apSrc->rasterData + apFilt->rasterData;

        cv::Mat mask1(apSrc->rasterData.size(), CV_8UC1);
        cv::Mat mask2(apDst->rasterData.size(), CV_8UC1); // they must have the same size
        cv::Mat maskf(apDst->rasterData.size(), CV_8UC1); // final mask        

        // apSrc->updateMask();
        cv::compare(apSrc->rasterData, apSrc->getNoDataValue(), mask1, CMP_NE);
        cv::compare(apFilt->rasterData, apFilt->getNoDataValue(), mask2, CMP_NE);
        // combine to generate final valid mask
        cv::bitwise_and(mask1, mask2, maskf);
        // use a base constant value layer labeled as NODATA
        apDst->rasterData = cv::Mat(apSrc->rasterData.size(), CV_64FC1, DEFAULT_NODATA_VALUE);
        // let's apply the resulting mask
        dest.copyTo(apDst->rasterData, maskf);
        return NO_ERROR;
    }

    int Pipeline::computeHeight(std::string src, std::string dst, cv::Size filterSize, int filterType){
        int retval = lowpassFilter(src, dst, filterSize, filterType);
        if (retval != NO_ERROR){
            cout << red << "[computeHeight] error when calling lowpassFilter" << reset << endl;
            return retval;
        }
        auto apSrc = dynamic_pointer_cast<RasterLayer> (getLayer(src));
        auto apDst = dynamic_pointer_cast<RasterLayer> (getLayer(dst));
        if (apSrc == nullptr){
            cout << "apSrc error" << endl;
            return -1;
        }
        if (apDst == nullptr){
            cout << "apDst error" << endl;
            return -1;
        }
        // cv::Mat dest(apSrc->rasterData.size(), CV_64FC1);
        cv::Mat dest; //(apSrc->rasterData.size(), CV_64FC1, DEFAULT_NODATA_VALUE);
        apDst->setNoDataValue(DEFAULT_NODATA_VALUE);
        // we flipped the order because the source is giving bathymetry depth as altitude (wrong sign)
        // dest = apSrc->rasterData - apDst->rasterData;
        dest = - apSrc->rasterData + apDst->rasterData;
        // WARNING: the expected output range can contain ZERO which is used sometimes as NOVALID data label
        // to avoid inhering invalida values from source layers, we calculate the valid data mask for the  dst layer
        // from the valid data mask of each source (src1 AND src2 -> dst) 
        // a) create src1 data mask
        cv::Mat mask1(apSrc->rasterData.size(), CV_8UC1);
        cv::Mat mask2(apDst->rasterData.size(), CV_8UC1); // they must have the same size
        cv::Mat maskf(apDst->rasterData.size(), CV_8UC1); // final mask        
        cv::compare(apSrc->rasterData, apSrc->getNoDataValue(), mask1, CMP_NE);
        cv::compare(apDst->rasterData, apDst->getNoDataValue(), mask2, CMP_NE);
        // combine to generate final valid mask
        cv::bitwise_and(mask1, mask2, maskf);
        // use a base constant value layer labeled as NODATA
        apDst->rasterData = cv::Mat(apSrc->rasterData.size(), CV_64FC1, DEFAULT_NODATA_VALUE);
        apDst->copyGeoProperties(apSrc);
        // let's apply the resulting mask
        dest.copyTo(apDst->rasterData, maskf);
        // TODO: trim the edges because the lowpassfilter cannot compute outside the filtersize box
        return NO_ERROR;
    }

    int Pipeline::generatePlaneMap (std::string dst, KPlane plane, std::string templ){
        // we ignore filter type for the preliminary implementation. Future dev may include bilateral, box, user-defined, or raster defined kernel for filtering purposes
        // first we check if the layer exist in the stack
        if (isAvailable(templ)){
            cout << red << "[generatePlaneMap] Error, template layer [" << templ << "] not found" << reset << endl;
            return LAYER_NOT_FOUND;
        }
        if (isAvailable(dst)){
            cout << "[generatePlaneMap] Destination layer ["  << yellow << dst << reset << "] does not exist. Creating..." << reset << endl;
            createLayer(dst, LAYER_RASTER);
        }
        
        auto apDst = dynamic_pointer_cast<RasterLayer> (getLayer(dst));
        auto apTemp = dynamic_pointer_cast<RasterLayer> (getLayer(templ));
        if (apDst == nullptr){
            cout << "Template layer must be of type: Raster" << endl;
            return -1;
        }
        if (apTemp == nullptr){
            cout << "Destination layer must be of type: Raster" << endl;
            return -1;
        }

        apDst->rasterData = cv::Mat(apTemp->rasterData.size(), CV_64FC1, DEFAULT_NODATA_VALUE);
        apDst->copyGeoProperties(apTemp);

        double z; // height (z) will be compute as a function from the plane equation
        double sx = geoTransform[GEOTIFF_PARAM_SX];
        double sy = geoTransform[GEOTIFF_PARAM_SY];

        // Plane eq: a.x + b.y + c.z + d =0
        // if (plane.c = 0) cannot be computed
        double planeA = plane.a();
        double planeB = plane.b();
        double planeC = plane.c();
        double planeD = plane.d();

        cout << cyan << "[planeEquation] " << plane << reset << endl;

        if (planeC == 0){
            cout << red << "[generatePlaneMap] provided plane [" << plane << "] contains NULL c() parameter" << reset << endl;
            return ERROR_WRONG_ARGUMENT;
        }

        if (verbosity > VERBOSITY_0){
            cout << "[.generatePlaneMap] Populating the target raster layer" << endl;
            cout << "\tPlane parameters: " << plane << endl;
        }

        for (int c=0; c<apDst->rasterData.cols; c++){
            double px = c * sx; // x coordinate of the pixel
            for (int r=0; r<apDst->rasterData.rows; r++){
                double py = r * sy; // x coordinate of the pixel
                z = -(planeA*px + planeB*py + planeD) / planeC;
                apDst->rasterData.at<double>(cv::Point(c,r)) = z; 
            }        
        }        
        return NO_ERROR;
    }

    /**
     * @brief Generic windowed filter that applies an specific filter to the src layer using a combination of global mask & sliding kernel
     * 
     * @param raster Source layer to be filtered
     * @param kernel Sliding kernel, typically a binary structuring element
     * @param mask Global raster mask that can be used as ROI
     * @param dst Name of the layer that will store the resulting image
     * @param filtertype type of filter to be applied: mean, slope, etc
     * @return int Error code, if any
     */
    int Pipeline::applyWindowFilter(std::string raster, std::string kernel, std::string mask, std::string dst, int filtertype){
        // first, we retrieve the raster Layer
        auto apSrc = dynamic_pointer_cast<RasterLayer> (getLayer(raster));
        if (apSrc == nullptr){
            cout << red << "[applyWindowFilter] Base bathymetry Layer [" << yellow << raster << red << "] not found..." << reset << endl;
            return LAYER_NOT_FOUND;
        }
        auto apMask = dynamic_pointer_cast<RasterLayer> (getLayer(mask));
        if (apMask == nullptr){
            cout << red << "[applyWindowFilter] Base valid mask Layer [" << yellow << mask << red << "] not found..." << reset << endl;
            return LAYER_NOT_FOUND;
        }
        auto apKernel = dynamic_pointer_cast<KernelLayer> (getLayer(kernel));
        if (apKernel == nullptr){
            cout << red << "[applyWindowFilter] Kernel layer [" << yellow << kernel << red << "] not found..." << reset << endl;
            return LAYER_NOT_FOUND;
        }
        auto apDst = dynamic_pointer_cast<RasterLayer> (getLayer(dst));
        if (apDst == nullptr){
            cout << "[applyWindowFilter] Destination layer [" << yellow << dst << reset << "] not found. Creating it.." << endl;
            createLayer(dst, LAYER_RASTER);
            apDst = dynamic_pointer_cast<RasterLayer> (getLayer(dst));
        }
        // we create the empty container for the destination layer
        apDst->rasterData = cv::Mat(apSrc->rasterData.size(), CV_64FC1, DEFAULT_NODATA_VALUE);
        // apDst->rasterData = DEFAULT_NODATA_VALUE * cv::Mat::ones(apSrc->rasterData.size(), CV_64FC1); 
        apDst->setNoDataValue(DEFAULT_NODATA_VALUE);
        double srcNoData = apSrc->getNoDataValue(); //we inherit ource no valid data value
        apDst->copyGeoProperties(apSrc);
        // second, we iterate over the source image
        int nRows = apSrc->rasterData.rows; // faster to have a local copy rather than reading it multiple times inside the for/loop
        int nCols = apSrc->rasterData.cols;
        int hKernel = apKernel->rotatedData.rows;   // height of the kernel 
        int wKernel = apKernel->rotatedData.cols;   // width of the kernel
        //on each different position, we apply the kernel as a mask <- TODO: change from RAW_Bathymetry to SparseMatrix representation of VALID Data raster Layer for speed increase
        if (verbosity > VERBOSITY_0){
            cout << "[p.applyWindowFilter] Layers created, now defining container elements" << endl;
            cout << "[nRows, nCols, hKernel, wKernel] = " << nRows << "/" << nCols << "/" <<  hKernel << "/" <<  wKernel << endl;
        }

        // WARNING: we asume that the output range of the filter is within the input range of the bathymetry values
        // as we are removing the non validad data point (not -defined)
        if (verbosity > VERBOSITY_0){
            cout << "[p.applyWindowFilter] Source NoData value: " << srcNoData << endl;
            cout << "[p.applyWindowFilter] Target NoData value: " << apDst->getNoDataValue() << endl;
            cout << "[p.applyWindowFilter] Input raster size: " << apSrc->rasterData.size() << endl; 
            // cout << "[p,computeMeanSlopeMap] Filter size: " << filterSize.width << " x " << filterSize. height << endl; 
        }
        // we create a matrix with NOVALID data
        cv::Mat  roi_image;// = cv::Mat(apSrc->rasterData.size(), CV_8UC1); // create global valid_data mask
        cv::compare(apSrc->rasterData, srcNoData, roi_image, CMP_NE); // ROI at the source data level

        int rt, lt, rb, lb;
        double cx = geoTransform[0];
        double cy = geoTransform[3];
        double sx = geoTransform[1];
        double sy = geoTransform[5];
        cv::Mat kernelMask;
        cv::Mat temp, sout;
        apKernel->rotatedData.convertTo(kernelMask, CV_64FC1);

        for (int row=0; row<nRows; row++){
            for (int col=0; col<nCols; col++){
                if (roi_image.at<unsigned char>(cv::Point(col, row))){ // we compute the slope only for those valid points
                    int cl = col - wKernel/2;
                    if (cl < 0) cl = 0;
                    int cr = col + wKernel/2;
                    if (cr > nCols) cr = nCols - 1;
                    int rt = row - hKernel/2;
                    if (rt < 0) rt = 0;
                    int rb = row + hKernel/2;
                    if (rb > nRows) rb = nRows - 1;

                    // ROI cropped selected window from the rotated kernel of the filter
                    int xi = wKernel/2 - (col - cl);
                    int yi = hKernel/2 - (row - rt);
                    int xf = cr - col + wKernel/2;
                    int yf = rb - row + hKernel/2;

                    cv::Mat subMask = kernelMask(cv::Range(yi,yf), cv::Range(xi,xf));
                    //subImage contains the raw data patch
                    cv::Mat subImage = apSrc->rasterData(cv::Range(rt, rb), cv::Range(cl, cr));
                    // roi_patch contains a binary mask of valid data
                    cv::Mat roi_patch = roi_image(cv::Range(rt, rb), cv::Range(cl, cr));
                    // apKernel contains and additional mask
                    subMask.convertTo(subMask, CV_64FC1);
                    roi_patch.convertTo(temp, CV_64FC1);

                    // cout << subImage << endl << endl << endl;
                    // cout << "roi/img/mask: " << roi_patch.size() << " " << subImage.size() << " " << subMask.size() << endl;
                    temp = subImage.mul(temp)/255;
                    temp = subMask.mul(temp);
                    // cout << temp << endl;
                    // WARNING: as we need a minimum set of valid 3D points for the plane fitting
                    // we filter using the size of pointList. For a 3x3 kernel matrix, the min number of points
                    // is n > K/2, being K = 3x3 = 9 ---> n = 5
                    std::vector<KPoint> pointList;
                    pointList = convertMatrix2Vector (&temp, sx, sy);

                    if (pointList.size() > 2){
                        if (filtertype == FILTER_SLOPE){
                            KPlane plane = computeFittingPlane(pointList);
                            double slope = computePlaneSlope(plane, KVector(0,0,1)); // returned value is the angle of the normal to the plane, in radians
                            // double slope = computePlaneSlope(plane) * 180/M_PI; // returned value is the angle of the normal to the plane, in radians
                            apDst->rasterData.at<double>(cv::Point(col, row)) = slope;
                        }
                        else if (filtertype == FILTER_MEAN){
                            double acum = 0;
                            for (auto it:pointList){
                                acum = acum + it.hz();
                            }
                            acum = acum / pointList.size();
                            // if (acum < 0) acum = 0;
                            apDst->rasterData.at<double>(cv::Point(col, row)) = acum;
                        }
                    }
                    else{ // we do not have enough points to compute a valid plane
                        apDst->rasterData.at<double>(cv::Point(col, row)) = DEFAULT_NODATA_VALUE;
                    }//*/
                }
                else
                    apDst->rasterData.at<double>(cv::Point(col, row)) = DEFAULT_NODATA_VALUE;
            }
        }
        apDst->copyGeoProperties(apSrc); //let's copy the geoproperties
        apDst->setNoDataValue(DEFAULT_NODATA_VALUE);
        apDst->updateMask();

        if (verbosity > VERBOSITY_1){
            cv::normalize(apDst->rasterData, sout, 0, 255, NORM_MINMAX, CV_8UC1, apMask->rasterData); // normalize within the expected range 0-255 for imshow
            // // apply colormap for enhanced visualization purposes
            cv::applyColorMap(sout, sout, COLORMAP_JET);
            namedWindow(apDst->layerName + "_verbose");
            imshow(apDst->layerName + "_verbose", sout); // this will show nothing, as imshow needs remapped images
            resizeWindow(apDst->layerName + "_verbose", DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
        }
        return NO_ERROR;
    }

    /**
     * @brief Compute the mean slope map using least-square fitting plane for every point of raster Layer. It uses kernel Layer as a local mask to clip the 3D point cloud used for plan estimation
     * 
     * @param raster Bathymetry Layer interpreted as a 2.5D map, where depth is defined for every pixel as Z = f(X,Y). Correspond to the union of both (V)alid and (N)on valid pixels from the RAW bathymetry map
     * @param kernel Binary mask Layer that is used to determine the subset S of points to be used for plane calculation. Such plane is used for local slope calculation
     * @param dst Resulting raster Layer containing the slope field computed for every point defined in the raster Layer
     * @return int Error code, if any
     */
    int Pipeline::computeMeanSlopeMap(std::string raster, std::string kernel, std::string mask, std::string dst){
        if (verbosity > VERBOSITY_0){
            cout << "[computeMeanSlopeMap] Calling applyWindowFilter" << endl;
        }
        return applyWindowFilter(raster, kernel, mask, dst, FILTER_SLOPE);
    }

    /**
     * @brief Starts tictac counter
     * 
     */
    void tictac::start(){
        start_time = getTickCount();
    }

    /**
     * @brief Stops tictac counter
     * 
     */
    void tictac::stop(){
        stop_time = getTickCount();
    }

    /**
     * @brief Return the number of ms elapsed between last start / stop calls
     * 
     * @return int64 Elapsed time in ms
     */
    int64 tictac::elapsed(){
        return 1000 * ((double) stop_time - start_time) / getTickFrequency();
    }

    /**
     * @brief Show elapsed time between events (start-stop pairs)
     * 
     */
    void tictac::show(){
        cout << highlight << "Elapsed time: " << elapsed() << " ms " << reset << endl;
    }

    /**
     * @brief Helper function that triggers a new lap as a stop/start pair
     * 
     * @param str Explicit string containing message to be shown  for the lap
     */
    void tictac::lap(std::string str){
        stop();
        cout << str << endl;
        show();
        start();
    }

} // namespace lad