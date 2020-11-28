/**
 * @file tiff2rugosity.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com / j.cappelletto@soton.ac.uk)
 * @brief Computes the 1-px rugosity map using the 3D area vs planar area ratio as metric of rugosity. It is computed over 2x2 px patches
 * with central point tesselation (4xtriangles). We exploit the fact that rugosity metrics is traslation invariant to compute it locally
 * over the sliding window. There is no overlap among neighbouring patches. Global error spatially bound to 1/2 px around the edges
 * The global (mean) rugosity value is returned in the console, and (optionally) a rugosity map is generated preserving the input geoTIFF
 * parameters (extent, dimensions, resolution, ref)
 * @version 0.1
 * @date 2020-11-27
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
    // Mandatory arguments
    if (argInputT2P) inputFileName = args::get(argInputT2P); //input file is mandatory positional argument.
    if (inputFileName.empty()){ //not defined as command line argument? let's use config.yaml definition
            logc.error ("main", "Input file missing. Please define it using --input='filename'");
            return -1;
    }

    //Optional arguments
    if (argOutputT2P) outputFileName = args::get(argOutputT2P); //input file is mandatory positional argument
    if (outputFileName.empty()){
            logc.error ("main", "Output file missing. Please define it using --output='filename'");
            return -1;
    }
    // exported image size can be any positive value. if zero any of the dimensions, the it is assumed it will inherit the input image size for that dimension
    // potential silent bugs? maybe, if careless arg parsing is done during batch call from bash
    // minDepth < maxDepth
    // xOffset, yOffset may fall out-of-boudary. We check that after reading the input image

    /* Summary list parameters */
    if (verbosity >= 1){
        cout << yellow << "****** Summary **********************************" << reset << endl;
        cout << "Input file:    \t" << yellow << inputFileName << reset << endl;
        cout << "Output file:   \t" << green << outputFileName << reset << endl;
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
        s << "rasterData in raster layer [" << yellow << layer << reset << "] is empty. Nothing to compute";
        logc.error("main:getLayer", s);
        return 0;                
    }
    // if (verbosity>=2){
    //     namedWindow ("original");
    //     cv::normalize(original, original, 0, 255, NORM_MINMAX, CV_8UC1, mask); // normalize within the expected range 0-255 for imshow
    //     imshow("original", original);
    //     namedWindow ("final_mask");// already normalized
    //     imshow("final_mask", final_mask);
    // }

    // if (proportion >= validThreshold){  // export inly if it satisfies the minimum proportion of valid pixels. Set threshold to 0.0 to esport all images 
    //     // before exporting, we check the desired number of image channels (grayscale or RGB)
    //     if (!argGrayscaleT2P){ // we need to convert to RGB
    //         final_png.convertTo(final_png, CV_8UC3);
    //         cv::cvtColor(final_png,final_png, COLOR_GRAY2RGB);

    //     }
    //     cv::imwrite(outputFileName, final_png);
    //     final.copyTo(apLayer->rasterData); //update layer with extracted patch
    //     final_mask.copyTo(apLayer->rasterMask); //update layer mask with extracted patch
    //     pipeline.exportLayer("M1_RAW_Bathymetry", outputTIFF, FMT_TIFF);
    // }

    if (verbosity > 0)
        tic.lap("");
    return NO_ERROR;
}
