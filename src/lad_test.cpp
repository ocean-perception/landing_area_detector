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
    //**************************************************************************
    /* Summary list parameters */
    cout << yellow << "****** Summary **********************************" << reset << endl;
    cout << "Input file:\t\t" << inputFileName << endl;
    cout << "Output file:\t\t" << outputFileName << endl;
    cout << "alphaShapeRadius:\t" << alphaShapeRadius << endl;
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

    // create the container and the open input file
    Geotiff inputGeotiff(inputFileName.c_str());
    if (!inputGeotiff.isValid())
    { // check if nothing wrong happened with the constructor
        cout << red << "Error opening Geotiff file: " << reset << inputFileName << endl;
        return lad::ERROR_GDAL_FAILOPEN;
    }
    //**************************************
    // Get/print summary information of the TIFF
    GDALDataset *poDataset;
    poDataset = inputGeotiff.GetDataset(); //pull the pointer to the main GDAL dataset structure

    Pipeline.apInputGeotiff = &inputGeotiff;
    Pipeline.processGeotiff("RAW_Bathymetry", "VALID_DataMask", argVerbose);
    Pipeline.extractContours("VALID_DataMask", "CONTOUR_Mask", argVerbose);

    Pipeline.createKernelTemplate("KernelAUV", 0.5, 1.4);
    Pipeline.createKernelTemplate("KernelSlope", 0.1, 0.1);
  
//    Pipeline.rotateLayer("KernelAUV", -10);

    shared_ptr<KernelLayer> apKernel = dynamic_pointer_cast<KernelLayer>(Pipeline.getLayer("KernelAUV"));
    if (apKernel == nullptr){
        cout << red << "Error creating AUV footprint layer " << reset << endl;
        return -1;
    }
    apKernel->setRotation(-10);
    
    Pipeline.useNodataMask = true;
    Pipeline.computeExclusionMap("VALID_DataMask", "KernelAUV", "ExclusionMap");
    Pipeline.computeMeanSlopeMap("RAW_Bathymetry", "KernelAUV", "VALID_DataMask", "SlopeMap");
    Pipeline.maskLayer ("SlopeMap", "ExclusionMap", "SlopeMap-masked");
    Pipeline.compareLayer("SlopeMap-masked", "P3-SlopeExclMap", 17.7, CMP_LE); // flag as valid those points that are LOWER THAN

    Pipeline.computeMeanSlopeMap("RAW_Bathymetry", "KernelSlope", "VALID_DataMask", "SlopeMapHIRES");
//    Pipeline.maskLayer ("SlopeMapHIRES", "ExclusionMap", "SlopeMapHIRES-masked");
    Pipeline.compareLayer("SlopeMapHIRES", "P1-LoSlopeExclMap", 17.7, CMP_LE); // flag as valid those points that are LOWER THAN

    int k = iParam;
    Pipeline.lowpassFilter("RAW_Bathymetry", "FILT_Bathymetry", cv::Size(k, k));

    // WARNING RESULTING LAYER CONATINES SOME DEAD PIXELS BECAUSE DIFF = 0 (MATCHES NO-DATA VALUE)
    Pipeline.computeHeight("RAW_Bathymetry", "HEIGHT_Bathymetry", cv::Size(k, k));
    // fix *********************

    if (argVerbose)
        Pipeline.showInfo(); // show detailed information if asked for

    Pipeline.showImage("RAW_Bathymetry",COLORMAP_JET);
    Pipeline.showImage("FILT_Bathymetry",COLORMAP_JET);
    Pipeline.showImage("HEIGHT_Bathymetry",COLORMAP_JET);

    // Pipeline.showImage("SlopeMap", COLORMAP_JET);
    // Pipeline.showImage("SlopeMapHIRES", COLORMAP_JET);
    // Pipeline.showImage("P1-LoSlopeExclMap", COLORMAP_JET);
    // Pipeline.showImage("ExclusionMap", COLORMAP_JET);
    // Pipeline.showImage("P3-SlopeExclMap", COLORMAP_JET);

    waitKey(0);

    Pipeline.exportLayer("FILT_Bathymetry", "FILT_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
    Pipeline.exportLayer("HEIGHT_Bathymetry", "HEIGHT_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
    Pipeline.exportLayer("RAW_Bathymetry", "RAW_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);


    return lad::NO_ERROR;
}
