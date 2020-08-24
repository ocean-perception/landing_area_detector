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

#include <yaml-cpp/yaml.h>

#include <thread>

using namespace std;
using namespace cv;
using namespace lad;

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

    // Now we proceed to optional parameters. When a variable is defined, we override the default value.
    parameterStruct params;

    float alphaShapeRadius = 1.0;
    if (argAlphaRadius) alphaShapeRadius = args::get(argAlphaRadius);

    float fParam = 1.0;
    if (argFloatParam) fParam = args::get(argFloatParam);

    int  iParam = 1;
    if (argIntParam)   iParam = args::get(argIntParam);

    int nThreads = DEFAULT_NTHREADS;
    if (argNThreads)    nThreads = args::get(argNThreads);
    if (nThreads < 3)   cout << "[main] Info: number of used threads will be always 3 or higher. Asked for [" << yellow << nThreads << reset << "]" << endl;

    params.rotation         = 0; // default no rotation (heading north)
    if (argRotation)        params.rotation         = args::get(argRotation);
    params.groundThreshold  = 0.02; //DEFAULT;
    if (argGroundThreshold) params.groundThreshold  = args::get(argGroundThreshold);
    params.heightThreshold  = 0.1; //DEFAULT;
    if (argHeightThreshold) params.heightThreshold  = args::get(argHeightThreshold);
    params.slopeThreshold   = 17.7; //DEFAULT;
    if (argSlopeThreshold)  params.slopeThreshold   = args::get(argSlopeThreshold);
    params.robotHeight      = 0.8;     //DEFAULT
    if (argRobotHeight)     params.robotHeight      = args::get(argRobotHeight);
    params.robotLength      = 1.4;
    if (argRobotLength)     params.robotLength      = args::get(argRobotLength);
    params.robotWidth       = 0.5;
    if (argRobotWidth)      params.robotWidth       = args::get(argRobotWidth);
    params.protrusionSize   = 10.0;
    if (argProtrusionSize)  params.protrusionSize   = args::get(argProtrusionSize);

    //**************************************************************************
    /* Summary list parameters */
    cout << yellow << "****** Summary **********************************" << reset << endl;
    cout << "Input file:\t\t" << inputFileName << endl;
    cout << "Output file:\t\t" << outputFileName << endl;
    cout << "fParam:\t" << fParam << endl;
    cout << "iParam:\t" << iParam << endl;
    lad::printParams(&params);

    lad::tictac tt, tic;
    int verboseLevel = 0;
    lad::Pipeline pipeline;
    if (argVerbose)
    {
        verboseLevel = args::get(argVerbose);
        cout << "Verbose level:\t\t" << verboseLevel << endl;
        pipeline.verbosity = verboseLevel;
    }
    cout << "Multithreaded version, max concurrent threads: [" << yellow << nThreads << reset << "]" << endl;
    cout << yellow << "*************************************************" << reset << endl << endl;

    tic.start();
    tt.start();
    
    pipeline.useNodataMask = true;
    pipeline.verbosity = verboseLevel;
    pipeline.readTIFF(inputFileName, "M1_RAW_Bathymetry", "M1_VALID_DataMask");
    pipeline.setTemplate("M1_RAW_Bathymetry");
    // pipeline.showImage("M1_VALID_DataMask");
    pipeline.extractContours("M1_VALID_DataMask", "M1_CONTOUR_Mask", verboseLevel);
        pipeline.exportLayer("M1_RAW_Bathymetry", "M1_RAW_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
        pipeline.exportLayer("M1_CONTOUR_Mask", "M1_CONTOUR_Mask.shp", FMT_SHP, WORLD_COORDINATE);

    pipeline.createKernelTemplate("KernelAUV",   params.robotWidth, params.robotLength, cv::MORPH_RECT);
    pipeline.createKernelTemplate("KernelSlope", 0.1, 0.1, cv::MORPH_ELLIPSE);
    pipeline.createKernelTemplate("KernelDiag",  1.0, 1.0, cv::MORPH_ELLIPSE);

    dynamic_pointer_cast<KernelLayer>(pipeline.getLayer("KernelAUV"))->setRotation(params.rotation);

    pipeline.computeExclusionMap("M1_VALID_DataMask", "KernelAUV", "C1_ExclusionMap");
        pipeline.exportLayer("C1_ExclusionMap", "C1_ExclusionMap.tif", FMT_TIFF, WORLD_COORDINATE);

    tt.lap("Load M1, C1");

    std::thread threadLaneC (&lad::processLaneC, &pipeline, &params);
    std::thread threadLaneB (&lad::processLaneB, &pipeline, &params);
    std::thread threadLaneA (&lad::processLaneA, &pipeline, &params);

    threadLaneA.join();
    threadLaneB.join();
    threadLaneC.join();

    pipeline.showImage("M1_RAW_Bathymetry", COLORMAP_TWILIGHT_SHIFTED);
    pipeline.showImage("A1_DetailedSlope");

    pipeline.maskLayer("B1_HEIGHT_Bathymetry", "A2_HiSlopeExcl", "M2_Protrusions");
    // pipeline.showImage("M2_Protrusions", COLORMAP_TWILIGHT_SHIFTED);
        pipeline.saveImage("M2_Protrusions", "M2_Protrusions.png", COLORMAP_TWILIGHT_SHIFTED);
        pipeline.exportLayer("M2_Protrusions", "M2_Protrusions.tif", FMT_TIFF, WORLD_COORDINATE);

    tt.lap("** Lanes A,B & C completed -> M2_Protrusion map done");

    //now we proceed with final LoProt/HiProt exclusion calculation
    std::thread threadLaneD (&lad::processLaneD, &pipeline, &params);
    threadLaneD.join();
    pipeline.showImage("D2_LoProtExcl");
    pipeline.showImage("D4_HiProtExcl");
    
    tic.lap("***\tPipeline completed");

    if (argVerbose)
        pipeline.showInfo(); // show detailed information if asked for


    waitKey(0);
    return lad::NO_ERROR;
}
