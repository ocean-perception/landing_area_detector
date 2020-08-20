/**
 * @file lad_test.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief Sandbox module for testing core and extended functionalities and integration of Geotiff, OpenCV, CGAL & GDAL
 * @version 0.2
 * @date 2020-07-03
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "headers.h"

#include "options.h"
#include "geotiff.hpp" // Geotiff class definitions
#include "lad_core.hpp"
#include "lad_analysis.h"
#include "lad_enum.hpp"
#include "lad_processing.hpp"
#include "lad_thread.hpp"

#include <thread>

using namespace std;
using namespace cv;
using namespace lad;

// General structure index:
//**** 1- Parse arguments from CLI
//**** 2- Read input TIFF file
//**** 3- Show image properties
//**** 4- Generate binary version of the input image
//**** 5- Determine the concave alphaShape
//**** 6- Export binary image as geoTIFF and the alphaShape as ESRI Shapefile

/*!
	@fn		int main(int argc, char* argv[])
	@brief	Main function
*/
int main(int argc, char *argv[])
{

    //*********************************************************************************
    /* PARSER section */
    std::string descriptionString =
        "lad_test - testing module part of [landing-area-detection] pipeline \
    Compatible interface with geoTIFF bathymetry datasets via GDAL + OpenCV";

    argParser.Description(descriptionString);
    argParser.Epilog("Author: J. Cappelletto (GitHub: @cappelletto)\n");
    argParser.Prog(argv[0]);
    argParser.helpParams.width = 120;

    cout << cyan << "lad_test" << reset << endl; // CREATE OUTPUT TEMPLATE STRING
    cout << "\tOpenCV version:\t" << yellow << CV_VERSION << reset << endl;
    cout << "\tGit commit:\t" << yellow << GIT_COMMIT << reset << endl
         << endl;
    // cout << "\tBuilt:\t" << __DATE__ << " - " << __TIME__ << endl;   // TODO: solve, make is complaining about this

    try
    {
        argParser.ParseCLI(argc, argv);
    }
    catch (const args::Completion &e)
    {
        cout << e.what();
        return 0;
    }
    catch (args::Help)
    { // if argument asking for help, show this message
        cout << argParser;
        return lad::ERROR_MISSING_ARGUMENT;
    }
    catch (args::ParseError e)
    { //if some error ocurr while parsing, show summary
        std::cerr << e.what() << std::endl;
        std::cerr << "Use -h, --help command to see usage" << std::endl;
        return lad::ERROR_WRONG_ARGUMENT;
    }
    catch (args::ValidationError e)
    { // if some error at argument validation, show
        std::cerr << "Bad input commands" << std::endl;
        std::cerr << "Use -h, --help command to see usage" << std::endl;
        return lad::ERROR_WRONG_ARGUMENT;
    }

    // Start parsing mandatory arguments
    if (!argInput)
    {
        cerr << "Mandatory <input> file name missing" << endl;
        cerr << "Use -h, --help command to see usage" << endl;
        return lad::ERROR_MISSING_ARGUMENT;
    }

    string inputFileName = args::get(argInput); //String containing the input file path+name from cvParser function
    string outputFileName = DEFAULT_OUTPUT_FILE;
    if (!argOutput)
        cout << "Using default output filename: " << yellow << outputFileName << reset << endl;
    else
        outputFileName = args::get(argOutput); //String containing the output file template from cvParser function

    /*
     * Now we proceed to optional parameters.
     * When each variable is defined, we override the default value.
     */
    float alphaShapeRadius = 1.0;
    if (argAlphaRadius)
        cout << "Using user-defined [argAlphaRadius]: " << (alphaShapeRadius = args::get(argAlphaRadius)) << endl;
    else
        cout << "Using default value for [argAlphaRadius]: " << yellow << alphaShapeRadius << reset << endl;

    float   fParam = 1.0;
    if (argFloatParam)
        fParam = args::get(argFloatParam);
    int     iParam = 1;
    if (argIntParam)
        iParam = args::get(argIntParam);
    float footprintRotation = 0; // default no rotation (heading north)
    if (argRotation)
        footprintRotation = args::get(argRotation);

    double slopeThreshold = 17.7;
    if (argThreshold)
        slopeThreshold = args::get(argThreshold);

    //**************************************************************************
    /* Summary list parameters */
    cout << yellow << "****** Summary **********************************" << reset << endl;
    cout << "Input file:\t\t" << inputFileName << endl;
    cout << "Output file:\t\t" << outputFileName << endl;
    cout << "alphaShapeRadius:\t" << alphaShapeRadius << endl;
    cout << "slopeThreshold:\t" << slopeThreshold << endl;
    cout << "footprintRotation (degrees):\t" << footprintRotation << endl;
    cout << "fParam:\t" << fParam << endl;
    cout << "iParam:\t" << iParam << endl;

    lad::tictac tt;
    int verboseLevel = 0;
    lad::Pipeline pipeline;
    if (argVerbose)
    {
        verboseLevel = args::get(argVerbose);
        cout << "Verbose level:\t\t" << verboseLevel << endl;
        pipeline.verbosity = verboseLevel;
    }
    cout << cyan << "Multithreaded version (3)" << reset << endl;
    cout << yellow << "*************************************************" << reset << endl
         << endl;

    tt.start();
    
    pipeline.useNodataMask = true;
    pipeline.verbosity = verboseLevel;
    pipeline.readTIFF(inputFileName, "M1_RAW_Bathymetry", "M1_VALID_DataMask");
    pipeline.setTemplate("M1_RAW_Bathymetry");
    pipeline.extractContours("M1_VALID_DataMask", "M1_CONTOUR_Mask", verboseLevel);
        pipeline.exportLayer("M1_RAW_Bathymetry", "M1_RAW_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
        pipeline.exportLayer("M1_CONTOUR_Mask", "M1_CONTOUR_Mask.shp", FMT_SHP, WORLD_COORDINATE);

    pipeline.createKernelTemplate("KernelAUV", 0.5, 1.4, cv::MORPH_RECT);
    pipeline.createKernelTemplate("KernelSlope", 0.1, 0.1, cv::MORPH_ELLIPSE);
    pipeline.createKernelTemplate("KernelDiag", 1.0, 1.0, cv::MORPH_ELLIPSE);

    auto apKernel = dynamic_pointer_cast<KernelLayer>(pipeline.getLayer("KernelAUV"));
    if (apKernel == nullptr){
        cout << red << "Error creating AUV footprint layer " << reset << endl;
        return -1;
    }
    apKernel->setRotation(footprintRotation);
    pipeline.computeExclusionMap("M1_VALID_DataMask", "KernelAUV", "C1_ExclusionMap");
        pipeline.exportLayer("C1_ExclusionMap", "C1_ExclusionMap.tif", FMT_TIFF, WORLD_COORDINATE);

    tt.lap("Load M1, C1");

    int k = iParam;

    std::thread threadLaneC (&lad::processLaneC, &pipeline, slopeThreshold);
    std::thread threadLaneB (&lad::processLaneB, &pipeline);
    std::thread threadLaneA (&lad::processLaneA, &pipeline, slopeThreshold);

    threadLaneA.join();
    threadLaneB.join();
    threadLaneC.join();

    pipeline.showImage("M1_RAW_Bathymetry", COLORMAP_TWILIGHT_SHIFTED);
    pipeline.showImage("A1_DetailedSlope");
    pipeline.showImage("B1_HEIGHT_Bathymetry", COLORMAP_TWILIGHT_SHIFTED);
    pipeline.showImage("C2_MeanSlopeMap");

    pipeline.maskLayer("B1_HEIGHT_Bathymetry", "A2_HiSlopeExclusion", "M2_Protrusions");
    pipeline.showImage("M2_Protrusions", COLORMAP_TWILIGHT_SHIFTED);
        pipeline.saveImage("M2_Protrusions", "M2_Protrusions.png", COLORMAP_TWILIGHT_SHIFTED);
        pipeline.exportLayer("M2_Protrusions", "M2_Protrusions.tif", FMT_TIFF, WORLD_COORDINATE);

    tt.lap("Pipeline completed");

    if (argVerbose)
        pipeline.showInfo(); // show detailed information if asked for

    waitKey(0);

    return lad::NO_ERROR;
}
