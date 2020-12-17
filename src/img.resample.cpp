/**
 * @file img.resample.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com / j.cappelletto@soton.ac.uk)
 * @brief  Quick OpenCV based image resampling module (upsampling/downsapling). 
 * The output resolution remains unchanged (unless defined otherwise) but the image is resampled to an intermediate
 * resolution. The default resampling method is Bicubic interpolation. Extended implementation can use any other
 * available method from OpenCV collection (excpet superresolution using dnn) 
 * @version 0.1
 * @date 2020-11-20
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "headers.h"
// #include "helper.h"

#include "options.h"
#include "geotiff.hpp" // Geotiff class definitions
#include "lad_core.hpp"
#include "lad_config.hpp"
// // #include "lad_analysis.h"
#include "lad_enum.hpp"
#include <limits>

using namespace std;
using namespace cv;
using namespace lad;
//we recycle the backbone code from tiff2png module
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

    // Mandatory arguments
    if (argInputT2P) inputFileName = args::get(argInputT2P); //input file is mandatory positional argument.
    if (inputFileName.empty()){ //not defined as command line argument? let's use config.yaml definition
            logc.error ("main", "Input file missing. Please define it using --input='filename'");
            return -1;
    }
    if (argInputT2P) outputFileName = args::get(argOutputT2P); //input file is mandatory positional argument
    if (outputFileName.empty()){
            logc.error ("main", "Output file missing. Please define it using --output='filename'");
            return -1;
    }

    // these are the intermediate dimension. We could use ratios to define intermediate sizes (TOD)
    unsigned int xSize = 227; // width of intermediate image (columns), 0 means use the same as input
    if (argXSizeT2P) xSize = args::get(argXSizeT2P); // any value is valid. No validation is required
    unsigned int ySize = 227; // heigh of the intermediate image
    if (argYSizeT2P) ySize = args::get(argYSizeT2P); // any value is valid. No validation is required

    // exported image size can be any positive value. if zero any of the dimensions, the it is assumed it will inherit the input image size for that dimension
    // potential silent bugs? maybe, if careless arg parsing is done during batch call from bash
    if (!(xSize * ySize)){
        s << "Intermediate image dimension must be positive integer (non-zero). Provided : [" << yellow << xSize << " x " << ySize << reset << "]";
        logc.error("args", s);
        return -1;
    }


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

    // let's open the input image
    // cv::Mat input = imread(inputFileName, IMREAD_ANYCOLOR);
    cv::Mat input = original.clone();

    cv::Mat output;
    /* Summary list parameters */
    if (verbosity >= 1){
        cout << yellow << "****** Summary **********************************" << reset << endl;
        cout << "Input file:       \t" << yellow << inputFileName << reset << "\tSize: " << yellow << input.size() << reset << endl; 
        cout << "Intermediate size:\t" << green << xSize << " x " << ySize << reset << endl;
        cout << "Output file:      \t" << green << outputFileName << reset << endl;
    }

    cv::resize(input, output, cv::Size(xSize, ySize), 0.0, 0.0, INTER_CUBIC);
    cv::resize(output, output, input.size(), 0.0, 0.0, INTER_CUBIC);

    if (verbosity >= 2){
        namedWindow("input");
        imshow("input", input);
        namedWindow("output");
        imshow("output", output);
        waitKey(0);
    }

    double nodata = apLayer->getNoDataValue();
    // imwrite(outputFileName, output);
    output.copyTo(apLayer->rasterData, mask);
    pipeline.exportLayer("M1_RAW_Bathymetry", outputFileName, FMT_TIFF);

    if (verbosity >= 1){
        s << "[" << yellow << inputFileName << reset << "] resampled to [" << blue << outputFileName << reset << "]. Size: " << output.size();
        logc.info ("img.resample", s);
    }
    return 0;
}
