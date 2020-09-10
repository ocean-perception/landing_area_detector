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

#include <geotiff.hpp>

#include "lad_layer.hpp"
#include "lad_core.hpp"
#include "lad_processing.hpp"

#include <iostream>

#include "ogrsf_frmts.h"

namespace lad
{

    /**
 * @brief Copy the content of a Layer into the current one. Derived classes must perform container specific deep-copy
 * 
 * @param layer Input layer to be copied
 * @return int Error code, if any.
 */
    int Layer::copy(Layer layer)
    {
        filePath = layer.filePath;
        fileName = layer.fileName;
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
    int Layer::copy(Layer *layer)
    {
        filePath = layer->filePath;
        fileName = layer->fileName;
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
    int Layer::clear()
    {
        layerID = LAYER_INVALID_ID;
        layerType = LAYER_INVALID;
        layerName = "";
        fileName = "";
        return NO_ERROR;
    }
    /**
 * @brief Return the ID of the given layer
 * 
 * @return int ID value
 */
    int Layer::getID()
    {
        return layerID;
    }
    /**
 * @brief Update the ID value of the layer.
 * 
 * @param newID New ID value of the value. It must be a valid ID
 * @return int Error code, if any. If the provided ID is valid it will return LAYER_OK
 */
    int Layer::setID(int newID)
    {
        if (newID < 0)
            return LAYER_INVALID_ID; // If invalid, return error code
        layerID = newID;             // if newID is valid update layerID.
        return LAYER_OK;             // return succesful code
    }
    /**
 * @brief get the Layer Status object
 * 
 * @return int Layer status value, from enumerated list
 */
    int Layer::getStatus()
    {
        return layerStatus;
    }

    /**
 * @brief Update the Layer status
 * 
 * @param newStatus New value of layer status. It should be any of possible values in the enumerated list. No validation is enforced
 * @return int return a copy of the new status value
 */
    int Layer::setStatus(int newStatus)
    {
        layerStatus = newStatus;
        return layerStatus;
    }

    /**
 * @brief get the Layer Type object
 * 
 * @return int Return the layer type, from enumerated list (RASTER, KERNEL, VECTOR, UNDEFINED)
 */
    int Layer::getType()
    {
        return layerType;
    }

    /**
 * @brief Update the Layer type
 * 
 * @param newType New value of layer type. It is user's responsability to correctly recast (if necessary) the data container accordingly
 * @return int return a copy of the new layer type
 */
    int Layer::setType(int newType)
    {
        layerType = newType;
    }
    /**
 * @brief Prints summary information of the layer
 */
    void Layer::showInformation()
    {
        cout << "Name: [" << green << layerName << reset << "]\t ID: [" << layerID << "]\tType: [" << layerType << "]\tStatus: [" << layerStatus << "]" << endl;
    }

    /**
     * @brief Populates a user provided array of double with the locally computed raster layer stats (min, max, mean, stdev)
     * 
     * @param array 
     */
    void RasterLayer::getStats(double *array){
        for (int i=0; i<4; i++)
            array[i] = rasterStats[i];
    }

    /**
     * @brief Updates the content of rasterMask matrix by comparing rasterData content with explicit NO-DATA value provided as argument
     * @details The resulting mask is a single channel 8-bit matrix containing 0 for invalid pixels in rasterData and 255 for valid pixels
     * @param nd NO-DATA scalar value to be used for comparison. 
     */
    void RasterLayer::updateMask(double nd){
        cv::compare(rasterData, nd, rasterMask, CMP_NE);
    }

    /**
     * @brief Updates the content of rasterMask by comparing rasterData content with implicit NO-DATA value of the layer
     * @details The NO-DATA value is assigned when importint a geoTIFF file, or during any construction methods. Can be modified using setNoDataVal
     */
    void RasterLayer::updateMask(){
        updateMask(noDataValue);
    }

    /**
     * @brief Recompute the stats of rasterData using the correspoding valid data mask
     * 
     */
    void RasterLayer::updateStats(){
        // first, let's find min, max using opencv methods
        double min, max;
        cv::minMaxLoc(rasterData, &min, &max, nullptr, nullptr, rasterMask);
        rasterStats[LAYER_MIN] = min;
        rasterStats[LAYER_MAX] = max;
        // now, let's calculate the mean value of the valid data
        Scalar mean, stdev;
        cv::meanStdDev(rasterData, mean, stdev, rasterMask);
        rasterStats[LAYER_MEAN] = mean[0];
        rasterStats[LAYER_STDEV] = stdev[0];
    }

    /**
     * @brief Copy geoTIFF specific properties from a source layer to the current layer (this)
     * 
     * @param src Pointer to the source layer to be copied
     */
    void RasterLayer::copyGeoProperties(shared_ptr<RasterLayer> src){
        layerProjection = src->layerProjection;
        // also, let's clone transformMatrix which contains pixel size and origin
        for (int i=0; i<6; i++)
            transformMatrix[i] = src->transformMatrix[i];
    }

    /**
     * @brief 
     * 
     * @param name 
     */
    int RasterLayer::readTIFF(std::string name){

    // create the container and the open input file
    Geotiff inputGeotiff(name.c_str());
    if (!inputGeotiff.isValid())
    { // check if nothing wrong happened with the constructor
        cout << red << "[readTIFF] Error opening Geotiff file: " << reset << name << endl;
        return lad::ERROR_GDAL_FAILOPEN;
    }
    //**************************************
    // Get/print summary information of the TIFF
    GDALDataset *poDataset;
    poDataset = inputGeotiff.GetDataset(); //pull the pointer to the main GDAL dataset structure
    // store a copy of the geo-transormation matrix
    poDataset->GetGeoTransform(transformMatrix);
    inputGeotiff.GetDimensions(layerDimensions);
    layerProjection = std::string(inputGeotiff.GetProjection());

    float **apData; //pull 2D float matrix containing the image data for Band 1
    apData = inputGeotiff.GetRasterBand(1);
    if (apData == nullptr)
    {
        cout << red << "[processGeotiff]: Error reading input geoTIFF data: NULL" << reset << endl;
        return ERROR_GDAL_FAILOPEN;
    }

    cv::Mat tiff(layerDimensions[1], layerDimensions[0], CV_64FC1); // cv container for tiff data . WARNING: cv::Mat constructor is failing to initialize with apData
    // cout << "Dim: [" << layerDimensions[0] << "x" << layerDimensions[1] << endl;
    for (int i = 0; i < layerDimensions[1]; i++)
    {
        for (int j = 0; j < layerDimensions[0]; j++)
        {
            tiff.at<double>(cv::Point(j, i)) = (double)apData[i][j]; // swap row/cols from matrix to OpenCV container
        }
    }
    tiff.copyTo(rasterData);

    setNoDataValue(inputGeotiff.GetNoDataValue());
    updateMask();
    updateStats();
    return NO_ERROR;
    }

    /**
 * @brief Extended method that prints general and raster specific information
 * 
 */
    void RasterLayer::showInformation()
    {
        updateStats();
        cout << "Name: [" << green << layerName << reset << "]\t ID: [" << getID() << "]\tType: [RASTER]\tStatus: [" << green << getStatus() << reset << "]" << endl;
        cout << "\t> Raster data container size: " << yellow << rasterData.size() << reset << "\t NoDataVal: [" << yellow << noDataValue << reset << "]" << endl;
        cout << "\t> Stats:\tMin [" << yellow << rasterStats[LAYER_MIN] << reset << "] Max [" << yellow << rasterStats[LAYER_MAX] << reset << "]" << endl; 
        cout << "\t> Mean [" << yellow << rasterStats[LAYER_MEAN] << reset << "] Stdev ["<< yellow  << rasterStats[LAYER_STDEV] << reset << "]" << endl;
    }

    /**
 * @brief Extended method that prints general and vector specific information
 * 
 */
    void VectorLayer::showInformation()
    {
        cout << "Name: [" << green << layerName << reset << "]\t ID: [" << getID() << "]\tType: [VECTOR]\tStatus: [" << green << getStatus() << reset << "]\tCoordinates: [";
        if (coordinateSpace == PIXEL_COORDINATE)
        {
            cout << yellow << "PIXEL]" << reset << endl;
        }
        else
        {
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
    int VectorLayer::convertSpace(int newSpace, double *apTransform)
    {
        if (newSpace = coordinateSpace)
        {
            cout << yellow << "[convertSpace] Source and target coordinate space are the same when trying to convert [" << layerName << "]. No operation was performed" << reset << endl;
            // it is the same space! nothing to change
            return 0;
        }
        if (apTransform == nullptr)
        {
            cout << red << "[convertSpace] Wrong 6D transforation matrix when trying to convert [" << layerName << "]. No operation was performed" << endl;
            return -1;
        }
        // ok we have a valid 6D transformation (somewhat...)
        int retval;
        vector<cv::Point2d> newData;
        retval = convertDataSpace(&vectorData, &newData, coordinateSpace, newSpace, apTransform);
        vectorData = newData;
        coordinateSpace = newSpace; // Update to the new coordinate space
        return retval;
    }

    /**
 * @brief Export vectorData to fileName file using fileFmt format
 * 
 * @param fileFmt Output file format. It must be a valid value from enum ExportFormat
 */
    int VectorLayer::writeLayer(std::string exportName, int fileFmt, std::string strWKTSpatialRef, int outputCoordinate, double *apGeoTransform)
    {
        //*************************************************************
        if (fileFmt == FMT_TIFF)
        {
            cout << red << "[v.writeLayer] Error, vector layer [" << layerName << "] cannot be exported as TIFF. Please convert it to raster first" << reset << endl;
            return ERROR_WRONG_ARGUMENT;
        }
        //*************************************************************
        if (fileFmt == FMT_SHP)
        {
            std::vector<cv::Point2d> transformedData;

            //we need to check if the data points need a space transformation
            if (coordinateSpace != outputCoordinate)
            {
                cout << "[v.writeLayer] Converting coordinate space of [" << exportName << "] to [";
                if (outputCoordinate == PIXEL_COORDINATE)
                    cout << yellow << "PIXEL" << reset << "]" << endl;
                else
                    cout << yellow << "WORLD" << reset << "]" << endl;
                convertDataSpace(&vectorData, &transformedData, coordinateSpace, outputCoordinate, apGeoTransform);
            }
            //vectorData
            int i = exportShapefile(exportName, layerName, transformedData, strWKTSpatialRef);
            if (i != NO_ERROR)
            {
                cout << "\tSome error ocurred while exporting [" << yellow << layerName << reset << "] to [" << yellow << exportName << "]" << reset << endl;
                return ERROR_GDAL_FAILOPEN;
            }
            return NO_ERROR;
            ;
        }
        //*************************************************************
        if (fileFmt == FMT_CSV)
        {
            // check if default filename has been already defined, if not what?

            std::vector<cv::Point2d> transformedData;

            //we need to check if the data points need a space transformation
            if (coordinateSpace != outputCoordinate)
            {
                cout << "[v.writeLayer] Converting coordinate space of [" << exportName << "] to [";
                if (outputCoordinate == PIXEL_COORDINATE)
                {
                    cout << yellow << "PIXEL" << reset << "]" << endl;
                }
                else
                {
                    cout << yellow << "WORLD" << reset << "]" << endl;
                }
                convertDataSpace(&vectorData, &transformedData, coordinateSpace, outputCoordinate, apGeoTransform);
            }

            if (exportName.empty())
            {
                if (fileName.empty())
                {
                    cout << "[writeLayer] " << yellow << "Layer filename not defined, will try to use layer name as export file" << reset << endl;
                    if (layerName.empty())
                    {
                        cout << "[writeLayer] " << red << "ERROR: Layer name not defined. Won't export layer" << reset << endl;
                        return ERROR_MISSING_ARGUMENT;
                    }
                    exportName = layerName;
                }
            }
            cout << reset << "[writeLayer] Exporting " << yellow << layerName << reset << " as CSV file: " << yellow << exportName << reset << endl;
            cout << "\tVector layer size: " << vectorData.size() << endl;

            ofstream outfile(exportName, ios::out);
            if (!outfile.good())
            {
                cout << "[writeLayer] " << red << "Error creating output file: " << exportName << reset << endl;
                return ERROR_WRONG_ARGUMENT;
            }
            std::string separator = ", ";
            outfile << "X" << separator << "Y" << endl;
            // Now we can export the content of the vector data (X,Y)
            for (auto element : transformedData)
            {
                outfile << element.x << separator << element.y << endl;
            }
            cout << "\tVector layer exported to: " << exportName << endl;
            outfile.close();
            return EXPORT_OK;
        }
        //*************************************************************
        else
        {
            cout << yellow << "[writeLayer] Unknown format: " << fileFmt << reset << endl;
            return ERROR_WRONG_ARGUMENT;
        }
    }

    int RasterLayer::writeLayer(std::string outputFilename, int fileFmt, int outputCoordinate){
        if (outputFilename.empty()){
            cout << red <<"[writeLayer] Empty output filename provided" << reset << endl;
            return ERROR_WRONG_ARGUMENT;
        }
        double noData = getNoDataValue();
        // temporary matrix that will hold the data to be exported
        // Created with the same size, and filled with the currently defined NODATA value
        cv::Mat tempData(rasterData.size(), CV_64FC1, noData);
        // before exporting, we need to verify if the data to be exported is already CV_64F
        if (rasterData.depth() != CV_64F){
            cv::Mat raster64;
            rasterData.convertTo(raster64, CV_64F);
            cout << "[r.writeLayer] Converted [" << yellow << layerName << reset << "] to CV_64F" << endl; 
            raster64.copyTo(tempData, rasterMask);
        }
        else{
            rasterData.copyTo(tempData, rasterMask);
        }

        // exporting as CSV in the pixel domain
        if (fileFmt == FMT_CSV){
            cout << "[r.writeLayer] exporting [" << yellow << layerName << reset << "] as CSV" << endl;
            std::ofstream ofs;
            ofs.open(outputFilename, std::ofstream::out); //overwrite if exist            
            for (int row = 0; row < tempData.rows; row++){
                for (int col = 0; col < tempData.cols; col++){
                    double value = tempData.at<double>(row,col);
                    ofs << value << "\t";
                }
                ofs << endl;
            }
            ofs.close();
            return NO_ERROR;
        }

        GDALDataset *geotiffDataset;
        GDALDriver *driverGeotiff;
        GDALRasterBand *geotiffBand; // also declare pointers for Geotiff

        int nrows  = rasterData.rows; //layerDimensions[1]
        int ncols  = rasterData.cols; //layerDimensions[0]

        // cout << "[r.writeLayer] Dataset dimensions (COL x ROW): [" << ncols << "] x [" << nrows << "]\tNoData = [" << noData << "]" << endl; 
 
        char **optionsForTIFF = NULL;
        optionsForTIFF = CSLSetNameValue(optionsForTIFF, "COMPRESS", "LZW");
        driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
        geotiffDataset = driverGeotiff->Create(outputFilename.c_str(), ncols, nrows, 1, GDT_Float64, optionsForTIFF);
        geotiffDataset->SetGeoTransform(transformMatrix);
        // cout << "[r.writeLayer] Projection string:" << endl;
        // cout << layerProjection.c_str() << endl;
        geotiffDataset->SetProjection(layerProjection.c_str());
        // \todo figure out if we need to convert/cast the cvMat to float/double for all layers
        int errcode;
        double *rowBuff = (double*) CPLMalloc(sizeof(double)*ncols);
        geotiffDataset->GetRasterBand(1)->SetNoDataValue (noData);       
        for(int row=0; row<nrows; row++) {
            for(int col=0; col<ncols; col++) {
                rowBuff[col] = (double) tempData.at<double>(cv::Point(col,row)); // tempData should be CV_64F
            }
            errcode = geotiffDataset->GetRasterBand(1)->RasterIO(GF_Write, 0, row,ncols, 1, rowBuff, ncols, 1, GDT_Float64, 0, 0);
        }

        GDALClose(geotiffDataset) ;
        return NO_ERROR;
    }

    /**
     * @brief Returns the size (pixel) of the diagonal of the structuring element. Currently, is just the diag of the bounding box 
     * 
     * @return double 
     */
    double RasterLayer::getDiagonalSize(){
        double x = (double) rasterData.cols;
        double y = (double) rasterData.rows;
        return (sqrt(x*x + y*y));
    }        

    /**
 * @brief Extended method that prints general and kernel specific information
 * 
 */
    void KernelLayer::showInformation()
    {
        updateStats();
        cout << "Name: [" << green << layerName << reset << "]\t ID: [" << getID() << "]\tType: [KERNEL]\tStatus: [" << green << getStatus() << reset << "]" << endl;
        cout << "\t> Kernel data container size: " << yellow << rasterData.size() << reset << endl;
        cout << "\t> Rotated container size:     " << yellow << rotatedData.size() << reset << "\tKernel rotation: " << yellow << dRotation << reset << endl;
    }

    /**
     * @brief Update the rotation value of the Kernel layer
     * 
     * @param rotation new rotation value (radians)
     */
    void KernelLayer::setRotation(double rotation){
        dRotation = rotation;
        if (rasterData.empty()) return; // Empty kernel, nothing to update

        // From: https://stackoverflow.com/questions/22041699/rotate-an-image-without-cropping-in-opencv-in-c
        // get rotation matrix for rotating the image around its center in pixel coordinates
        cv::Mat r = cv::getRotationMatrix2D(cv::Point2f((rasterData.cols-1)/2.0, (rasterData.rows-1)/2.0), dRotation, 1.0);
        // determine bounding rectangle, center not relevant
        cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), rasterData.size(), dRotation).boundingRect2f();
        // adjust transformation matrix
        r.at<double>(0,2) += bbox.width/2.0 - rasterData.cols/2.0;
        r.at<double>(1,2) += bbox.height/2.0 - rasterData.rows/2.0;

        cv::warpAffine(rasterData, rotatedData, r, bbox.size());
    }

    /**
     * @brief Returns the rotation value for the Kernel layer
     * 
     * @return double Rotation value, in radians.
     */
    double KernelLayer::getRotation(){
        return dRotation;
    }

    /**
 * @brief Loads a vector of Point2d points in the layer container. It replaces the existing data.
 * 
 * @param inputData New data to be stored in the container
 * @return int size of the new stored data vector
 */
    int VectorLayer::loadData(vector<Point2d> *inputData)
    {
        // We can not avoid a deep copy of the input vector. We could iterate through each element and assign it, or just use = operator
        vectorData = *inputData;
        setStatus(LAYER_OK);
        return vectorData.size();
    }

    /**
 * @brief Import and copy the content from an input cvMat to the internal storage rasterData
 * 
 * @param input A valid cvMat matrix (yet, no validation is performed)
 * @return int 
 */
    int RasterLayer::loadData(cv::Mat *input)
    {
        input->copyTo(rasterData); // deep copy of the Mat content and header to avoid original owner to accidentally overwrite the data
        setStatus(LAYER_OK);
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
    int exportShapefile(string filename, string layerName, vector<Point2d> data, std::string strWKTSpatialRef)
    {

        const char *pszDriverName = "ESRI Shapefile";
        GDALDriver *poDriver;
        GDALAllRegister();

        poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName);
        if (poDriver == NULL)
        {
            cout << red << "Error w/GDAL: " << pszDriverName << " not available" << endl;
            return ERROR_GDAL_FAILOPEN;
        }

        GDALDataset *poDS;
        poDS = poDriver->Create(filename.c_str(), 0, 0, 0, GDT_Unknown, NULL);
        if (poDS == NULL)
        {
            cout << red << "Error creating output file: [" << filename << "]" << reset << endl;
            return ERROR_GDAL_FAILOPEN;
        }

        OGRLayer *poLayer;
        OGRSpatialReference spatialRef(strWKTSpatialRef.c_str());
        poLayer = poDS->CreateLayer(layerName.c_str(), &spatialRef, wkbPoint, NULL);
        if (poLayer == NULL)
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
        char szName[33] = "szName";

        for (auto it : data)
        {
            OGRFeature *poFeature;
            poFeature = OGRFeature::CreateFeature(poLayer->GetLayerDefn());
            //        poFeature->SetField( "Name", szName );

            OGRPoint pt;
            pt.setX(it.x);
            pt.setY(it.y);

            poFeature->SetGeometry(&pt);

            if (poLayer->CreateFeature(poFeature) != OGRERR_NONE)
            {
                cout << yellow << "Error GDAL: Failed to create feature in shapefile." << endl;
                return ERROR_GDAL_FAILOPEN;
            }

            OGRFeature::DestroyFeature(poFeature);
        }

        GDALClose(poDS);
        return NO_ERROR;
    }
 
     RasterLayer RasterLayer::operator+(const RasterLayer& b){
         RasterLayer r(this->layerName, this->getID());
         r.rasterData = this->rasterData + b.rasterData;
         return r;
     }
 
} // namespace lad
