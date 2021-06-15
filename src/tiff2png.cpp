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
// #include "lad_analysis.h"
#include "lad_enum.hpp"
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
    int verbosity=0; // default: no verbose output
    if (argVerboseT2P) verbosity = args::get(argVerboseT2P); //input file is mandatory positional argument. Overrides any definition in configuration.yaml
    // Input file priority: must be defined either by the config.yaml or --input argument
    string inputFileName  = ""; // command arg or config defined
    string outputFileName = ""; // command arg or config defined
    string outputTIFF     = ""; // command arg or config defined
    // Mandatory arguments
    if (argInputT2P) inputFileName = args::get(argInputT2P); //input file is mandatory positional argument.
    if (inputFileName.empty()){ //not defined as command line argument? let's use config.yaml definition
            logc.error ("main", "Input file missing. Please define it using --input='filename'");
            return -1;
    }
    if (argOutputT2P) outputFileName = args::get(argOutputT2P); //input file is mandatory positional argument
    if (outputFileName.empty()){
            logc.error ("main", "Output file missing. Please define it using --output='filename'");
            return -1;
    }
    if (argExportTiffT2P) outputTIFF = args::get(argExportTiffT2P); //extra geotiff copy to be exported

    //Optional arguments
    //validity threshold. Default pass all (th=0)
    double validThreshold = 0.0; // default, we export any image regardless the proportion of valid pixels
    if (argValidThresholdT2P) validThreshold = args::get(argValidThresholdT2P);
    // let's validate
    if (validThreshold < 0.0 || validThreshold > 1.0){
        s << "Invalid value for validThreshold [" << red << validThreshold << reset << "]. Valid range [0.0, 1.0]. Check --valid_th argument";
        logc.error("main",  s);
        return -1;
    }
    //rotation, default = 0 degress
    double rotationAngle = 0;
    if (argRotationT2P) rotationAngle = args::get(argRotationT2P); // any value is valid. No validation is required
    // max depth range (both positive and negative), default = +1.0
    double maxDepth = 1.0;
    if (argZMaxT2P) maxDepth = args::get(argZMaxT2P); // any value is valid. No validation is required
    // horizontal offset, default = 0
    int xOffset = 0; // horizontal, row wise (positive right)
    if (argXOffsetT2P) xOffset = args::get(argXOffsetT2P); // any value is valid. No validation is required
    // vertical offset, default = 0
    int yOffset = 0; // vertical, column wise (positive down)
    if (argYOffsetT2P) yOffset = args::get(argYOffsetT2P); // any value is valid. No validation is required
    // output size, default same as input
    unsigned int xSize = 227; // width of output image (columns), 0 means use the same as input
    if (argXSizeT2P) xSize = args::get(argXSizeT2P); // any value is valid. No validation is required
    unsigned int ySize = 227; // vertical, column wise (positive down)
    if (argYSizeT2P) ySize = args::get(argYSizeT2P); // any value is valid. No validation is required

    // exported image size can be any positive value. if zero any of the dimensions, the it is assumed it will inherit the input image size for that dimension
    // potential silent bugs? maybe, if careless arg parsing is done during batch call from bash
    // minDepth < maxDepth
    // xOffset, yOffset may fall out-of-boudary. We check that after reading the input image

    /* Summary list parameters */
    if (verbosity >= 1){
        cout << yellow << "****** Summary **********************************" << reset << endl;
        cout << "Input file:    \t" << yellow << inputFileName << reset << endl;
        cout << "Output file:   \t" << green << outputFileName << reset << endl;
        if (argExportTiffT2P) 
            cout << "outputTIFF:    \t" << green << outputTIFF << reset << endl;; //extra geotiff copy to be exported
        cout << "validThreshold:\t" << yellow << validThreshold << reset << endl;
        cout << "ROI Offset:    \t(" << xOffset << ", " << yOffset << ")\tRotation: \t" << rotationAngle << "deg" << endl; 
        if (xSize * ySize > 0)
            cout << "ROI Size:      \t(" << xSize << ", " << ySize << ")" << endl;
        else
            cout << "ROI Size: " << light_green << "<same as input>" << reset << endl;
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
    cv::Mat original;
    cv::Mat mask = apLayer->rasterMask.clone();
    apLayer->rasterData.copyTo(original, mask); //copy only valid pixels, the rest should remain zero
    // now, we need to extract ROI for the given rotation and displacement (offset)
    // 0/verify if sampling is necessary and validate sizes (update if not provided with input image params)
    if (xSize > mask.cols){
        s << "Desired image output width larger than image input width (" << red << xSize << " > " << mask.cols << reset << ")";
        logc.error("validation", s);
        return -1;
    }
    else if (xSize == 0) xSize = mask.cols; // asked for autodetection of image width
    if (ySize > mask.rows){
        s << "Desired image output height larger than image input height (" << red << ySize << " > " << mask.rows << reset << ")";
        logc.error("validation", s);
        return -1;
    }
    else if (ySize == 0) ySize = mask.rows; // asked for autodetection of image height

    // 1/compute the new center
    int cx = original.cols/2;
    int cy = original.rows/2;    // center of the source image
    int nx = cx + xOffset;
    int ny = cy + yOffset; // to be used for the lat/lon or UTM coordinates and to determine the corners of the ROI defining the large bbox
    int nx_east = nx;
    int ny_north = ny;
    // 2/compute the max radius of the ROI (factoring rotation)
    int diag = ceil(sqrt(xSize*xSize + ySize*ySize) / 2.0) + 1; // radius of internal bounding box. The size of the external bbox is twice this radius
    // 2.1/verify crop extent still falls withint the src image
    int tlx = nx - diag;    // top left corner
    int tly = ny - diag;
    int brx = nx + diag;    // bottom right corner
    int bry = ny + diag;
    if (tlx < 0){
        logc.error("rect", "top left corner X out of range (negative");
        return -1;
    }
    if (tly < 0){
        logc.error("rect", "top left corner Y out of range (negative");
        return -1;
    }
    if (brx >= original.cols){
        s << "bottom right corner X out of range: " << brx << " > " << original.cols;
        logc.error("rect", s);
        return -1;
    }
    if (bry >= original.rows){
        s << "bottom right corner Y out of range: " << bry << " > " << original.rows;
        logc.error("rect", s);
        return -1;
    }
    // 3/crop large extent
    cv::Mat large_roi (original, cv::Rect2d(tlx, tly, 2*diag, 2*diag)); // the bbox size is twice the diagonal
    cv::Mat large_crop;
    large_roi.copyTo (large_crop); 
    // 4/rotate given angle
    cv::Mat r = cv::getRotationMatrix2D(cv::Point2f((large_crop.cols-1)/2.0, (large_crop.rows-1)/2.0), rotationAngle, 1.0);
    // determine bounding rectangle, center not relevant
    cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), large_crop.size(), rotationAngle).boundingRect2f(); //this was a bit overkill
    // adjust transformation matrix
    r.at<double>(0,2) += bbox.width/2.0 - large_crop.cols/2.0;
    r.at<double>(1,2) += bbox.height/2.0 - large_crop.rows/2.0;
    cv::Mat rotatedROI;
    cv::warpAffine(large_crop, rotatedROI, r, bbox.size(), cv::INTER_NEAREST); // using nearest: faster and we avoid interpolation of nodata field
    // 5/crop small extent (xSize, ySize)
    tlx = rotatedROI.cols/2 - xSize/2; // center - width 
    tly = rotatedROI.rows/2 - ySize/2; // center - height
    bbox = cv::Rect2d(tlx, tly, xSize, ySize);
    cv::Mat final = rotatedROI(bbox); // crop the final size image, already rotated    
    // 6/update mask: compare against nodata field
    double nodata = apLayer->getNoDataValue();
        // let's inspect the 'final' matrix and compare against  'nodata'
    cv::Mat final_mask;
    // nodata was replaced by ZERO when loading into the original matrix
    cv::compare(final, 0, final_mask, CMP_NE);
    // 7/normalize the mask. The actual PNG output must be scaled according to the bathymetry range param
    // cv::normalize(final_mask, final_mask, 0, 255, NORM_MINMAX, CV_8UC1); // normalize within the expected range 0-255 for imshow
    if (verbosity>=2){
        namedWindow ("original");
        cv::normalize(original, original, 0, 255, NORM_MINMAX, CV_8UC1, mask); // normalize within the expected range 0-255 for imshow
        imshow("original", original);
        namedWindow ("final_mask");// already normalized
        imshow("final_mask", final_mask);
    }

    // 2.1) Compute the mean of the valid pixels. We need the number of valid pixels
    double _min, _max, _mean, _sum;
    int totalPixels = final.rows * final.cols;    // max total pixels, should be the same as xSize * ySize
    int totalValids = countNonZero(final_mask);       // total valid pixels = non zero pixels
    int totalZeroes = totalPixels - totalValids;// total invalid pxls = total - valids
    double proportion = (double)totalValids/(double)totalPixels;

    cv::Mat zero_mask = cv::Mat::zeros(final_mask.size(), CV_64FC1); // float zero mask
    zero_mask.copyTo(final, ~final_mask); // we copy zeros to the invalid points (negated mask). Cheaper to peform memory copy than multiplying by the mask 
    _sum  = cv::sum(final).val[0]; // checked in QGIS< ok. Sum all pixels including the invalid ones, which have been already converted to zero
    _mean = _sum / (float)totalValids;

    if (verbosity >= 2){
        cv::minMaxLoc (final, &_min, &_max, 0, 0, final_mask); //masked min max of the input bathymetry
        cout << light_yellow << "RAW bathymetry - \t" << reset << "MIN / MEAN / MAX = [" << _min << " / " << _mean << " / " << _max << "]" << endl;
    }
    // 2.2) Shift the whole map to the mean (=0)
    cv::subtract(final, _mean, final, final_mask); // MEAN centering of only valida data points (use mask)
    // show debug
    if (verbosity >= 2){
        // cv::normalize(final, final, 0, 255, NORM_MINMAX, CV_8UC1, final_mask); // normalize within the expected range 0-255 for imshow
        namedWindow ("final");
        cv::Mat temp;
        cv::normalize(final, temp, 0, 255, NORM_MINMAX, CV_8UC1, final_mask); // normalize within the expected range 0-255 for imshow
        imshow("final", temp);
        // recompute min/max
        cv::minMaxLoc (final, &_min, &_max, 0, 0, final_mask); //masked min max of the input bathymetry
        _sum  = cv::sum(final).val[0]; // checked in QGIS< ok. Sum all pixels including the invalid ones, which have been already converted to zero
        _mean = _sum / (float)totalValids;
        cout << light_green  << "Adjusted bathymetry - \t" << reset << "MIN / MEAN / MAX = [" << _min << " / " << _mean << " / " << _max << "]" << endl;
        waitKey(0);
    }
    // duplicate for export
    cv::Mat final_png;
    final.copyTo(final_png); //copy for normalization to 0-255. The source can be used to be exported as local bathymetry geoTIFF
    // 2.4) Scale to 128/max_value
    double alfa = 128.0 / maxDepth; //fParam is the expected max value (higher, will be clipped)
    final_png = final_png * alfa;   // we rescale the bathymetry onto 0-255, where 255 is reached when height = fParam
    final_png = final_png + 127.0; // 1-bit bias. The new ZERO should be in 127
    if (verbosity >= 2){
        double png_mean = (double) cv::sum(final_png).val[0] / (double) (final_png.cols * final_png.rows); 
        cv::minMaxLoc (final_png, &_min, &_max, 0, 0, final_mask); //debug
        // fancy colour to indicate if out of range [0, 255]. his is a symptom of depth range saturation for the lcoal bathymetry patch
        cout << light_blue << "Exported PNG image - \t" << reset << "MIN / MEAN / MAX = [" << ((_min < 0.0) ? red : green) << _min << reset << " / " << png_mean;
        cout << " / " << ((_min > 255.0) ? red : green) << _max << reset << "]" << endl;
    }
    // Step 3: use geoTransform matrix to retrieve center of map image
    // let's use the stored transformMatrix coefficients retrieved by GDAL
    // coordinates are given as North-East positive. Vertical resolution sy (coeff[5]) can be negative
    // as long as the whole dataset is self-consistent, any offset can be ignored, as the LGA autoencoder uses the relative distance 
    // between image centers (it could also be for any corner when rotation is neglected)
    // before exporting the geoTIFF, we need to correct the geotransformation matrix to reflect the x/y offset
    // it should be mapped as a northing/easting displacemen (scaled by the resolution)
    // easting_offset -> transforMatrix[0]
    // northing_offset -> transforMatrix[3]

    // this is the northing/easting in the original reference system
    double easting  = apLayer->transformMatrix[0] + apLayer->transformMatrix[1]*nx_east; // easting
    double northing = apLayer->transformMatrix[3] + apLayer->transformMatrix[5]*ny_north; // northing
    // easily, we can add those UTM coordinates as the new offset (careful: center ref vs corner ref)
    apLayer->transformMatrix[0] = easting - (final.cols/2)*apLayer->transformMatrix[1];
    apLayer->transformMatrix[3] = northing - (final.rows/2)*apLayer->transformMatrix[5];


    if (proportion >= validThreshold){  // export inly if it satisfies the minimum proportion of valid pixels. Set threshold to 0.0 to esport all images 
        // before exporting, we check the desired number of image channels (grayscale or RGB)
        if (!argGrayscaleT2P){ // we need to convert to RGB
            final_png.convertTo(final_png, CV_8UC3);
            cv::cvtColor(final_png,final_png, COLOR_GRAY2RGB);

        }
        cv::imwrite(outputFileName, final_png);
        final.copyTo(apLayer->rasterData); //update layer with extracted patch
        final_mask.copyTo(apLayer->rasterMask); //update layer mask with extracted patch
        pipeline.exportLayer("M1_RAW_Bathymetry", outputTIFF, FMT_TIFF);
    }

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
    // altitude [m]     fixed to some typ. positive value (e.g. 6). Orthographic projection doesn't need image-like treatment
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

    String separator = "\t"; 
    if (argCsvT2P) separator = ",";

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
    return NO_ERROR;
}
