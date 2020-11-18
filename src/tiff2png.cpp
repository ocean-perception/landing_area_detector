/**
 * @file tif2png.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com / j.cappelletto@soton.ac.uk)
 * @brief geoTIFF to PNG converter. Part of the data preparation pipeline to generate the PNG training dataset for LG Autoencoder
 *        and Bayesian Neural Network inference framework
 * @version 0.2
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
#include <limits>

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
    int retval = initParserT2P(argc, argv);    // initial argument validation, populates arg parsing structure args
    if (retval != 0)                        // some error ocurred, we have been signaled to stop
        return retval;
    std::ostringstream s;
    // Parameters hierarchy
    // ARGS > CONFIG > DEFAULT (this)

    int verbosity=0; // default: no verbose output
    if (argVerboseT2P) verbosity = args::get(argVerboseT2P); //input file is mandatory positional argument. Overrides any definition in configuration.yaml

    // Input file priority: must be defined either by the config.yaml or --input argument
    string inputFileName    = ""; // command arg or config defined
    string outputFileName    = ""; // command arg or config defined

    if (argInputT2P) inputFileName = args::get(argInputT2P); //input file is mandatory positional argument. Overrides any definition in configuration.yaml
    if (inputFileName.empty()){ //not defined as command line argument? let's use config.yaml definition
            logc.error ("main", "Input file missing. Please define it using --input='filename'");
            return -1;
    }

    if (argInputT2P) outputFileName = args::get(argOutputT2P); //input file is mandatory positional argument. Overrides any definition in configuration.yaml
    if (outputFileName.empty()){ //not defined as command line argument? let's use config.yaml definition
            logc.error ("main", "Output file missing. Please define it using --output='filename'");
            return -1;
    }

    double validThreshold = 0.0; // default, we export any image regardless the proportion of valid pixels
    if (argValidThresholdT2P) validThreshold = args::get(argValidThresholdT2P);
    // let's validate
    if (validThreshold < 0.0 || validThreshold > 1.0){
        s << "Invalid value for validThreshold [" << red << validThreshold << reset << "]. Valid range [0.0, 1.0]. Check --valid_th argument";
        logc.error("main",  s);
        return -1;
    }
    // Now we proceed to optional parameters. When a variable is defined, we override the default value.
    float fParam = 1.0;
    if (argFloatParamT2P) fParam = args::get(argFloatParamT2P);
    int  iParam = 1;
    if (argIntParamT2P)   iParam = args::get(argIntParamT2P);

    /* Summary list parameters */
    if (verbosity >= 2){
        cout << yellow << "****** Summary **********************************" << reset << endl;
        cout << "Input file:    \t" << inputFileName << endl;
        cout << "Output file:   \t" << outputFileName << endl;
        cout << "fParam:        \t" << fParam << endl;
        cout << "validThreshold:\t" << validThreshold << endl;
    }

    lad::tictac tic;
    tic.start();

    lad::Pipeline pipeline;    

    // Step 1: Read input TIFF file
    pipeline.useNodataMask = true;//params.useNoDataMask;
    pipeline.readTIFF(inputFileName, "M1_RAW_Bathymetry", "M1_VALID_DataMask");

    // Step 2: Determine bathymetry range for the loaded image before rescaling
    // We know the bathymetry map is stored as 32/64 bit float 
    String layer = "M1_RAW_Bathymetry";
    auto apLayer = dynamic_pointer_cast<RasterLayer> (pipeline.getLayer(layer));
    if (apLayer == nullptr){
        s << "Unexpected error when downcasting RASTER layer [" << yellow << layer << "]";
        logc.error("main:getLayer", s);
        cout << cyan << "at" << __FILE__ << ":" << __LINE__ << reset << endl;
        return ERROR_WRONG_ARGUMENT;
    }
    if (apLayer->rasterData.empty()){
        s << "rasterData in raster layer [" << yellow << layer << reset << "] is empty. Nothing to save";
        logc.error("main:getLayer", s);
        return NO_ERROR;                
    }
    // correct data range to improve
    cv::Mat dst;
    cv::Mat mask = apLayer->rasterMask.clone();

    double _min, _max, _mean, _sum;
    apLayer->rasterData.copyTo(dst, mask); //copy only valid pixels, the rest should remain zero

    // 2.1) Compute the mean of the valid pixels. We need the number of valid pixels
    int totalPixels = mask.rows * mask.cols;    // max total pixels
    int totalValids = countNonZero(mask);       // total valid pixels = non zero pixels
    int totalZeroes = totalPixels - totalValids;// total invalid pxls = total - valids
    double proportion = (double)totalValids/(double)totalPixels;

    if (verbosity >= 2){
        cv::minMaxLoc (dst, &_min, &_max, 0, 0, mask); //masked min max of the input bathymetry
        cout << light_yellow << "RAW bathymetry - \t" << reset << "MIN / MEAN / MAX = [" << _min << " / " << _mean << " / " << _max << "]" << endl;
    }
    // 2.2) Shift the whole map to the mean (=0)
    _sum  = cv::sum(dst).val[0]; // checked in QGIS< ok. Sum all pixels including the invalid ones, which have been already converted to zero
    _mean = _sum / (float)totalValids;
    dst = dst - _mean; // MEAN centering: As a consequence, non-valid data points (converted to ZERO) have been shifted. Let's erase them

    // 2.3) Mask invalid pixels 
    // For this, we copy a masked matrix containing zeros
    cv::Mat zero_mask = cv::Mat::zeros(mask.size(), CV_64FC1);
    zero_mask.copyTo(dst, ~mask); // we copy zeros to the invalid points (negated mask). Cheaper to peform memory copy than multiplying by the mask 
    if (verbosity >= 2){
        cout << light_green  << "Adjusted bathymetry - \t" << reset << "MIN / MEAN / MAX = [" << _min - _mean << " / " << _mean - _mean << " / " << _max  - _mean << "]" << endl;
    }
    
    // 2.4) Scale to 128/max_value
    double alfa = 128.0 / fParam; //fParam is the expected max value (higher, will be clipped)
    dst = dst * alfa;   // we rescale the bathymetry onto 0-255, where 255 is reached when height = fParam
    
    // 2.5) Shift (up) to 127.0
    dst = dst + 127.0; // 1-bit bias. The new ZERO should be in 127
    if (verbosity >= 2){
        double png_mean = (double) cv::sum(dst).val[0] / (double) (dst.cols * dst.rows); 
        cv::minMaxLoc (dst, &_min, &_max, 0, 0, apLayer->rasterMask); //debug
        // fancy colour to indicate if out of range [0, 255]. his is a symptom of depth range saturation for the lcoal bathymetry patch
        cout << light_blue << "Exported PNG image - \t" << reset << "MIN / MEAN / MAX = [" << ((_min < 0.0) ? red : green) << _min << reset << " / " << png_mean;
        cout << " / " << ((_min > 255.0) ? red : green) << _max << reset << "]" << endl;
    }
    
    if (proportion >= validThreshold){  // export inly if it satisfies the minimum proportion of valid pixels. Set threshold to 0.0 to esport all images 
        cv::imwrite(outputFileName, dst);
    }

    // Step 3: use geoTransform matrix to retrieve center of map image
    // let's use the stored transformMatrix coefficients retrieved by GDAL
    // coordinates are given as North-East positive. Vertical resolution sy (coeff[5]) can be negative
    // as long as the whole dataset is self-consistent, any offset can be ignored, as the LGA autoencoder uses the relative distance 
    // between image centers (it could also be for any corner when rotation is neglected)
    double easting  = apLayer->transformMatrix[0] + apLayer->transformMatrix[1]*apLayer->rasterData.cols/2; // easting
    double northing = apLayer->transformMatrix[3] + apLayer->transformMatrix[5]*apLayer->rasterData.rows/2; // northing

    // Also we need the LAT LON in decimal degree to match oplab-pipeline and LGA input format
    double latitude;
    double longitude;
    // we need to transform from northing easting to WGS84 lat lon
    OGRSpatialReference refUtm;
    refUtm.importFromProj4(apLayer->layerProjection.c_str());   // original SRS
    OGRSpatialReference refGeo;
    refGeo.SetWellKnownGeogCS("WGS84"); // target SRS
    OGRCoordinateTransformation* coordTrans = OGRCreateCoordinateTransformation(&refUtm, &refGeo); // ask for a SRS transforming object

    double x = easting;
    double y = northing;
    
    cout.precision(std::numeric_limits<double>::digits10);    // maybe it's an overkill. Just in case
    int reprojected = coordTrans->Transform(1, &x, &y);
    latitude  = x; // yes, this is not a bug, they are swapped 
    longitude = y;
    delete coordTrans; // maybe can be removed as destructor and garbage collector will take care of this after return
    // Target HEADER (CSV)
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
    // x_velocity [m/s] faked data - optional
    // y_velocity [m/s] faked data - optional
    // z_velocity [m/s] faked data - optional
    // **********************************************************************************
    // This is the format required by LGA as raw input for 'lga sampling'
    // This first step will produce a prefiltered file list with this header (CSV)
    // <ID>,relative_path,altitude [m],roll [deg],pitch [deg],northing [m],easting [m],depth [m],heading [deg],timestamp [s],latitude [deg],longitude [deg]
    // >> filename: [sampled_images.csv] let's create a similar file using the exported data from this file, and merged in the bash caller

    String separator = "\t"; // TODO: user defined separator (for more general compatibility)
    if (verbosity >= 1){
        // export header colums
        cout << "valid_ratio"        << separator;
        // cout << "relative_path"     << separator; // this information is know by the caller
        cout << "northing [m]"      << separator;
        cout << "easting [m]"       << separator;
        cout << "depth [m]"         << separator;
        cout << "latitude [deg]"    << separator;
        cout << "longitude [deg]"   << endl;
    }
    // export data columns (always)
    cout << proportion  << separator;   // proportion of valid pixels, can be used by the caller to postprocessing culling
    cout << northing    << separator;   // northing [m]
    cout << easting     << separator;   // easting [m]
    cout << _mean       << separator;   // mean depth for the current bathymety patch
    cout << latitude    << separator;   // mean depth for the current bathymety patch
    cout << longitude   << separator;   // mean depth for the current bathymety patch
    cout << endl;

    if (verbosity > 0)
        tic.lap("");
    if (verbosity >= 3){
        cv::normalize(dst, dst, 0, 255, NORM_MINMAX, CV_8UC1, apLayer->rasterMask); // normalize within the expected range 0-255 for imshow
        namedWindow("test");
        imshow("test", dst);
        waitKey(0);
    }
    return NO_ERROR;
}
