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
    //**************************************************************************
    /* Summary list parameters */
    cout << yellow << "****** Summary **********************************" << reset << endl;
    cout << "Input file:\t\t" << inputFileName << endl;
    cout << "Output file:\t\t" << outputFileName << endl;
    cout << "alphaShapeRadius:\t" << alphaShapeRadius << endl;
    cout << "footprintRotation (degrees):\t" << footprintRotation << endl;
    cout << "fParam:\t" << fParam << endl;
    cout << "iParam:\t" << iParam << endl;

    int verboseLevel = 0;
    lad::Pipeline Pipeline;
    if (argVerbose)
    {
        verboseLevel = args::get(argVerbose);
        cout << "Verbose level:\t\t" << verboseLevel << endl;
        Pipeline.verbosity = verboseLevel;
    }
    cout << "*************************************************" << endl
         << endl;

    Pipeline.useNodataMask = true;
    Pipeline.verbosity = verboseLevel;
    Pipeline.readTIFF(inputFileName, "M1_RAW_Bathymetry", "M1_VALID_DataMask");
    Pipeline.setTemplate("M1_RAW_Bathymetry");
    Pipeline.extractContours("M1_VALID_DataMask", "M1_CONTOUR_Mask", verboseLevel);

    // Pipeline.generatePlaneMap("plane", KPlane(0,0.2,0.2,15), "M1_RAW_Bathymetry");
    // Pipeline.showImage("plane",COLORMAP_JET);
    // Pipeline.exportLayer("plane","plane.tif", FMT_TIFF, WORLD_COORDINATE);

    Pipeline.createKernelTemplate("KernelAUV", 0.5, 1.4, cv::MORPH_RECT);
    Pipeline.createKernelTemplate("KernelSlope", 0.06, 0.06, cv::MORPH_ELLIPSE);
    Pipeline.createKernelTemplate("KernelDiag", 1.0, 1.0, cv::MORPH_ELLIPSE);
  
    auto apKernel = dynamic_pointer_cast<KernelLayer>(Pipeline.getLayer("KernelAUV"));
    if (apKernel == nullptr){
        cout << red << "Error creating AUV footprint layer " << reset << endl;
        return -1;
    }
    apKernel->setRotation(footprintRotation);
    Pipeline.computeExclusionMap("M1_VALID_DataMask", "KernelAUV", "C1_ExclusionMap");

    Pipeline.computeMeanSlopeMap("M1_RAW_Bathymetry", "KernelAUV", "M1_VALID_DataMask", "C2_MeanSlopeMap");
    Pipeline.showImage("C2_MeanSlopeMap");
    Pipeline.exportLayer("C2_MeanSlopeMap", "C2_MeanSlopeMap.tif", FMT_TIFF, WORLD_COORDINATE);
    // Pipeline.maskLayer("C2_MeanSlopeMap", "C1_ExclusionMap", "C2_MeanSlopeMap_Clip");

    double slopeThreshold = 17.7;

    Pipeline.compareLayer("C2_MeanSlopeMap", "C3_MeanSlopeExclusion", slopeThreshold, CMP_GT);
    Pipeline.showImage("C3_MeanSlopeExclusion");
    Pipeline.exportLayer("C3_MeanSlopeExclusion", "C3_MeanSlopeExclusion.tif", FMT_TIFF, WORLD_COORDINATE);

    int k = iParam;
    Pipeline.lowpassFilter ("M1_RAW_Bathymetry", "KernelDiag", "M1_VALID_DataMask", "B0_FILT_Bathymetry");
    Pipeline.showImage("B0_FILT_Bathymetry", COLORMAP_JET);
    Pipeline.exportLayer("B0_FILT_Bathymetry", "B0_FILT_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);

    Pipeline.computeHeight("M1_RAW_Bathymetry", "B0_FILT_Bathymetry", "B1_HEIGHT_Bathymetry");
    Pipeline.showImage("B1_HEIGHT_Bathymetry", COLORMAP_TWILIGHT_SHIFTED);
    Pipeline.exportLayer("B1_HEIGHT_Bathymetry", "B1_HEIGHT_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
    
    Pipeline.computeMeanSlopeMap("M1_RAW_Bathymetry", "KernelSlope", "M1_VALID_DataMask", "A1_DetailedSlope");
    Pipeline.showImage("A1_DetailedSlope",COLORMAP_JET);
    Pipeline.exportLayer("A1_DetailedSlope", "A1_DetailedSlope.tif", FMT_TIFF, WORLD_COORDINATE);

    // Pipeline.compareLayer("A1_DetailedSlope", "A2_HiSlopeExclusion", slopeThreshold, CMP_GT);
    // Pipeline.showImage("A2_HiSlopeExclusion",COLORMAP_JET);
    // Pipeline.exportLayer("A2_HiSlopeExclusion", "A2_HiSlopeExclusion.tif", FMT_TIFF, WORLD_COORDINATE);

    // Pipeline.maskLayer("B1_HEIGHT_Bathymetry", "A2_HiSlopeExclusion", "M2_Protrusions");
    // Pipeline.showImage("M2_Protrusions", COLORMAP_TWILIGHT_SHIFTED);

    if (argVerbose)
        Pipeline.showInfo(); // show detailed information if asked for

    waitKey(0);

    Pipeline.exportLayer("M1_RAW_Bathymetry", "M1_RAW_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
    // Pipeline.exportLayer("M1_VALID_DataMask", "M1_VALID_DataMask.tif", FMT_TIFF, WORLD_COORDINATE);
    // Pipeline.exportLayer("M2_Protrusions","M2_Protrusions.tif", FMT_TIFF, WORLD_COORDINATE);
    // Pipeline.exportLayer("B0_FILT_Bathymetry", "B0_FILT_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
    // Pipeline.exportLayer("B1_HEIGHT_Bathymetry", "B1_HEIGHT_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
    // Pipeline.exportLayer("A1_DetailedSlope","A1_DetailedSlope.tif", FMT_TIFF, WORLD_COORDINATE);
    // Pipeline.exportLayer("A2_HiSlopeExclusion","A2_HiSlopeExclusion.tif", FMT_TIFF, WORLD_COORDINATE);
    // Pipeline.exportLayer("C2_MeanSlopeMap", "C2_MeanSlopeMap.tif", FMT_TIFF, WORLD_COORDINATE);
    // Pipeline.exportLayer("C3_MeanSlopeExclusion", "C3_MeanSlopeExclusion.tif", FMT_TIFF, WORLD_COORDINATE);

    return lad::NO_ERROR;
}
