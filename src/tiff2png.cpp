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
    cv::Mat dst;// = apLayer->rasterData.clone();
    cv::Mat mask = apLayer->rasterMask.clone();

    double _min, _max, _mean, _sum;
    apLayer->rasterData.copyTo(dst, mask); //copy only valid pixels, the rest should be zero

    // 2.1) Compute the mean of the valid pixels. We need the number of valid pixels
    int totalPixels = mask.rows * mask.cols;
    int totalValids = countNonZero(mask);
    int totalZeroes = totalPixels - totalValids;

    cv::minMaxLoc (dst, &_min, &_max, 0, 0, mask); //masked min max of the input bathymetry
    _sum  = cv::sum(dst).val[0]; // checked in QGIS< ok
    _mean = _sum / (float)totalValids;

    // 2.2) Shift the whole map to the mean (=0)
    dst = dst - _mean; // MEAN centering: As a consequence, non-valid data points (converted to ZERO) have been shifted. Let's erase them

    // 2.3) Mask invalid pixels 
    // For this, we copy a masked matrix containing zeros
    cv::Mat zero_mask = cv::Mat::zeros(mask.size(), CV_64FC1);
    zero_mask.copyTo(dst, ~mask); // we copy zeros to the invalid points (negated mask)
    
    if (verbosity > 0){
        cout << yellow << "RAW bathymetry -     " << reset << "MIN / MEAN / MAX = [" << _min << " / " << _mean << " / " << _max << endl;
        cout << green  << "Adjusted bathymetry - " << reset << "MIN / MEAN / MAX = [" << _min - _mean << " / " << _mean - _mean << " / " << _max  - _mean<< endl;
    }
    
    // 2.4) Scale to 128/max_value
    double alfa = 128.0 / fParam; //fParam is the expected max value (higher, will be clipped)
    dst = dst * alfa;   // we rescale the bathymetry onto 0-255, where 255 is reached when height = fParam
    
    // 2.5) Shift (up) to 127.0
    dst = dst + 127.0; // 1-bit bias. The new ZERO should be in 127
    cv::minMaxLoc (dst, &_min, &_max, 0, 0, apLayer->rasterMask); //debug
    if (verbosity>0)
        cout << "PNG Min/Max: [" << _min << "\t" << _max << "]" << endl;
    
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

    // Also we need the LAT LON in decimale degree to match oplab-pipeline and LGA input format
    //Target HEADER (CSV)
    // relative_path	northing [m]	easting [m]	depth [m]	roll [deg]	pitch [deg]	heading [deg]	altitude [m]	timestamp [s]	latitude [deg]	longitude [deg]	x_velocity [m/s]	y_velocity [m/s]	z_velocity [m/s]
    // relative_path    ABSOLUT OR RELATIVE URI
    // northing [m]     UTM northing (easy to retrieve from geoTIFF)
    // easting [m]      UTM easting (easy to retrieve from geoTIFF)
    // depth [m]        Mean B0 bathymetry patch depth
    // roll [deg]       zero, orthografically projected depthmap
    // pitch [deg]      zero, same as roll
    // heading [deg]    default zero, can be modified by rotating the croping window during gdal_retile.py
    // altitude [m]     fixed to some typ posituve value (e.g. 6). Orthographic projection doesn't need image-like treatment
    // timestamp [s]    faked data
    // latitude [deg]   decimal degree latitude, calculated from geotiff metadata
    // longitude [deg]  decimal degree longitude, calculated from geotiff metadata
    // x_velocity [m/s] faked data
    // y_velocity [m/s] faked data
    // z_velocity [m/s] faked data
    // **********************************************************************************
    // This is the format required by LGA as raw input for 'lga sampling'
    // This first step will produce a prefiltered file list with this header (CSV)
    // <ID>,relative_path,altitude [m],roll [deg],pitch [deg],northing [m],easting [m],depth [m],heading [deg],timestamp [s],latitude [deg],longitude [deg]
    // >> filename: [sampled_images.csv] let's create a similar file using the exported data from this file, and merged in the bash caller


    cout << "cx/cy: [" << cx << "\t" << cy << "\t" << _mean << "]" << endl; // we export de CX/CY coordinates, and also the MEAN val of raw bathymetry

    
    return NO_ERROR;
}
