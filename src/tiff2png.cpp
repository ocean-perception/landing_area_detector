/**
 * @file tif2png.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief geoTIFF to PNG converter. Part of the data preparation pipeline to generate the PNG training dataset for LG Autoencoder
 *        and Bayesian Neural Network inference framework
 * @version 0.1
 * @date 2020-11-09
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "headers.h"
#include "helper.h"

#include "options.h"
#include "geotiff.hpp" // Geotiff class definitions
#include "lad_core.hpp"
#include "lad_config.hpp"
#include "lad_analysis.h"
#include "lad_enum.hpp"
// #include "lad_processing.hpp"
// #include "lad_thread.hpp"

using namespace std;
using namespace cv;
using namespace lad;

logger::ConsoleOutput logc;

/*!
    @fn     int main(int argc, char* argv[])
    @brief  Main function
*/
int main(int argc, char *argv[])
{
    int retval = initParser(argc, argv);    // initial argument validation, populates arg parsing structure args
    if (retval != 0)                        // some error ocurred, we have been signaled to stop
        return retval;
    std::ostringstream s;
    // Parameters hierarchy
    // ARGS > CONFIG > DEFAULT (this)

    int verbosity=0; // default: no verbose output
    if (argVerbose) verbosity = args::get(argVerbose); //input file is mandatory positional argument. Overrides any definition in configuration.yaml

    // Input file priority: must be defined either by the config.yaml or --input argument
    string inputFileName    = ""; // command arg or config defined
    string outputFileName    = ""; // command arg or config defined

    if (argInput) inputFileName = args::get(argInput); //input file is mandatory positional argument. Overrides any definition in configuration.yaml
    if (inputFileName.empty()){ //not defined as command line argument? let's use config.yaml definition
            logc.error ("main", "Input file missing. Please define it using --input='filename'");
            return -1;
    }

    if (argInput) outputFileName = args::get(argOutput); //input file is mandatory positional argument. Overrides any definition in configuration.yaml
    if (outputFileName.empty()){ //not defined as command line argument? let's use config.yaml definition
            logc.error ("main", "Output file missing. Please define it using --output='filename'");
            return -1;
    }

    // Now we proceed to optional parameters. When a variable is defined, we override the default value.
    float fParam = 1.0;
    if (argFloatParam) fParam = args::get(argFloatParam);
    int  iParam = 1;
    if (argIntParam)   iParam = args::get(argIntParam);

    /* Summary list parameters */
    if (verbosity>0){
        cout << yellow << "****** Summary **********************************" << reset << endl;
        cout << "Input file:   \t" << inputFileName << endl;
        cout << "Output file:  \t" << outputFileName << endl;
        cout << "fParam:       \t" << fParam << endl;

    }
    lad::tictac tic;
    tic.start();

    lad::Pipeline pipeline;    

    // Step 1: Read input TIFF file
    pipeline.useNodataMask = true;//params.useNoDataMask;
    pipeline.readTIFF(inputFileName, "M1_RAW_Bathymetry", "M1_VALID_DataMask");

    // Step 2: Determine bathymetry range for the loaded image before rescaling
    // We know the bathymetry map is stored as 64 bit float 
    String layer = "M1_RAW_Bathymetry";
    shared_ptr<RasterLayer> apLayer = dynamic_pointer_cast<RasterLayer> (pipeline.getLayer(layer));
    if (apLayer == nullptr){
        s << "Unexpected error when downcasting RASTER layer [" << yellow << layer << "]";
        logc.error("saveImage", s);
        cout << cyan << "at" << __FILE__ << ":" << __LINE__ << reset << endl;
        return ERROR_WRONG_ARGUMENT;
    }
    if (apLayer->rasterData.empty()){
        s << "rasterData in raster layer [" << yellow << layer << reset << "] is empty. Nothing to save";
        logc.error("saveImage", s);
        return NO_ERROR;                
    }
    // correct data range to improv
    cv::Mat dst = apLayer->rasterData.clone();

    // how do we deal with NO-DATA parts of the image?
    // we could use IMREAD_LOAD_GDAL from OpenCV imread: https://docs.opencv.org/3.4/d7/d73/tutorial_raster_io_gdal.html
    
    double _min, _max;
    cv::minMaxLoc (dst, &_min, &_max, 0, 0, apLayer->rasterMask);
    if (verbosity>0)
        cout << "TIF Min/Max: [" << _min << "\t" << _max << "]" << endl;
    dst = dst - _min;   // shift by _min, now every value must be positive
    _max = _max - _min; // shift also the _max

    // now, we must replace NODATA pixels with ZERO. We use the rasterMask, as the actual pixels have been modified
    cv::Mat zz = cv::Mat::zeros(dst.size(), CV_64FC1); // zeros of 64b float (same as loaded image)
    zz.copyTo(dst,~apLayer->rasterMask);

    // Step 3: rescale accordingly. The min must match zero, and the max must be clipped if exceeds the (pre)defined expected max for (relative) bathymetry
    // e.g.: if raw bathymetry range is [12 .. 20]m, and the expect max diff is 4 meters then,
    // z = z - min(z) // shift to zero  [12..20] --> [0..8]
    // z = alfa * z (alfa = 255/z_max)  [0..8]   --> [0..2], in this case the depth/atitude range of 8.0 exceed the expected norm for 4.0m. 
    // Typ the output range should fall within [0..1] 
    double alfa = 255.0 / fParam;
    dst = dst * alfa;   // we rescale the bathymetry onto 0-255, where 255 is reached when height = fParam
    cv::minMaxLoc (dst, &_min, &_max, 0, 0, apLayer->rasterMask); //debug
    if (verbosity>0)
        cout << "PNG Min/Max: [" << _min << "\t" << _max << "]" << endl;
    
    // cv::normalize(dst, dst, 0, 255, NORM_MINMAX, CV_8UC1, apLayer->rasterMask); // normalize within the expected range 0-255 for imshow
    cv::imwrite(outputFileName, dst);
    if (verbosity>1){
        cv::normalize(dst, dst, 0, 255, NORM_MINMAX, CV_8UC1, apLayer->rasterMask); // normalize within the expected range 0-255 for imshow
        namedWindow("test");
        imshow("test", dst);
        tic.lap("");
        waitKey(0);
    }
        // todo correct normalization to given fparam
    // Step 3: use geoTransform matrix to retrieve center of map image
    // let's use the stored transformMatrix coefficients retrieved by GDAL
    // coordinates are given as North-East positive. Vertical resolution sy (coeff[5]) can be negative
    // as long as the whole dataset is self-consistent, any offset can be ignored, as the LGA autoencoder uses the relative distance 
    // between image centers (it could also be for any corner)
    double cx = apLayer->transformMatrix[0] + apLayer->transformMatrix[1]*apLayer->rasterData.cols/2;
    double cy = apLayer->transformMatrix[3] + apLayer->transformMatrix[5]*apLayer->rasterData.rows/2;

    cout << "cx/cy: [" << cx << "\t" << cy << "]" << endl;

    return NO_ERROR;
}
