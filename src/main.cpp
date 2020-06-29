/********************************************************************/
/* Project: Landing Area Detection algorithm						*/
/* Module: 	lad_test												*/
/* File: 	main.cpp 	                                            */
/* Created:		18/06/2020                                          */
/* Description
*/

/********************************************************************/
/* Created by:                                                      */
/* Jose Cappelletto - j.cappelletto@soton.ac.uk		                */
/********************************************************************/

#include "../include/options.h"
#include <ctime>
#include <regex>
#include <fstream>

using namespace std;

const string green("\033[1;32m");
const string yellow("\033[1;33m");
const string cyan("\033[1;36m");
const string red("\033[1;31m");
const string reset("\033[0m");

// #cmakedefine USE_GPU

char keyboard = 0;	// keyboard input character
double t;			// Timing monitor

// General structure index:
//**** 1- Parse arguments from CLI
//**** 2- Read input TIFF file
//**** 3- Show image properties
//**** 4- Generate binary version of the input image
//**** 5- Determine the concave alphaShape
//**** 6- Export binary image as geoTIFF and the alphaShape as ESRI Shapefile

float alphaShapeRadius = 1.0;
/*!
	@fn		int main(int argc, char* argv[])
	@brief	Main function
*/
int main(int argc, char *argv[]) {

//*********************************************************************************
/*	PARSER section */
    std::string descriptionString = \
    "lad_test - testing module part of [landing-area-detection] pipeline \
    Compatible interface with geoTIFF bathymetry datasets via GDAL + OpenCV";

    argParser.Description(descriptionString);
    argParser.Epilog("Author: J. Cappelletto (GitHub: @cappelletto)\n");
    argParser.Prog(argv[0]);
    argParser.helpParams.width = 120;

    cout << cyan << "lad_test" << reset << endl; // CREATE OUTPUT TEMPLATE STRING
//    cout << "\tOpenCV version:\t" << yellow << CV_VERSION << reset << endl;
    cout << "\tGit commit:\t" << yellow << GIT_COMMIT << reset << endl;
    cout << "\tBuilt:\t" << __DATE__ << " - " << __TIME__ << endl;

    try{
        argParser.ParseCLI(argc, argv);
    }
    catch (args::Help){    // if argument asking for help, show this message
        cout << argParser;
        return 1;
    }
    catch (args::ParseError e){  //if some error ocurr while parsing, show summary
        std::cerr << e.what() << std::endl;
        std::cerr << "Use -h, --help command to see usage" << std::endl;
        return 1;
    }
    catch (args::ValidationError e){ // if some error at argument validation, show
        std::cerr << "Bad input commands" << std::endl;
        std::cerr << "Use -h, --help command to see usage" << std::endl;
        return 1;
    }

    int CUDA = 0;                                       //Default option (running with CPU)
    /*
     * Start parsing mandatory arguments
     */

    if (!argInput){
        cerr << "Mandatory <input> file name missing" << endl;
        cerr << "Use -h, --help command to see usage" << endl;
        return 1;
    }

    if (!argOutput){
        cerr << "Mandatory <output> file name missing" << endl;
        cerr << "Use -h, --help command to see usage" << endl;
        return 1;
    }

    string inputFileName = args::get(argInput);	//String containing the input file path+name from cvParser function
    string outputFileName = args::get(argOutput);	//String containing the output file template from cvParser function

    /*
     * These were the mandatory arguments. Now we proceed to optional parameters.
     * When each variable is defined, we assign the default value.
     */
    /*
     * Now, start verifying each optional argument from argParser
     */

    if (argAlphaRadius) cout << "[argAlphaRadius] value provided: " << (alphaShapeRadius = args::get(argAlphaRadius)) << endl;
        else cout << "[argAlphaRadius] using default value: " << alphaShapeRadius << endl;

    //**************************************************************************
    int nCuda = -1;    //<Defines number of detected CUDA devices. By default, -1 acting as error value
    #ifdef USE_GPU
    /* CUDA */
    // TODO: read about possible failure at runtime when calling CUDA methods in non-CUDA hardware.
    // CHECK whether it is possible with try-catch pair
    nCuda = cuda::getCudaEnabledDeviceCount();	// we try to detect any existing CUDA device
    cuda::DeviceInfo deviceInfo;

    cout << green << "CUDA mode enabled" << reset << std::endl;
    if (nCuda > 0){
        cout << green << "CUDA enabled devices detected: " << reset << deviceInfo.name() << endl;
        cuda::setDevice(0);
    }
    else {
        cout << red << "No CUDA device detected" << reset << endl;
        cout << "Exiting, use non-GPU version instead" << endl;
        return -1;
    }

    #endif

    // TODO: How to operate when multiple CUDA devices are detected?
    // So far, we work with the first detected CUDA device. Maybe, add some CUDA probe mode when called

    time_t now = time(0);
    char* dt = ctime(&now);

    //**************************************************************************
    /* FILE LIST INPUT */
    cout << yellow << "Summary" << reset << endl;
    cout << "Input file:\t" << inputFileName << endl;
    cout << "Output file:\t" << outputFileName << endl;
    cout << "alphaShapeRadius:\t" << alphaShapeRadius << endl;
    cout << "***********************************" << endl;

    //*****************************************************************************
    //***** Open the input TIFF file

    //Sequence:
    //  1- Read list the single input file (path/filename must be accepted)
    //  2- Test/open all the mandatory fields in the input
    //  3- Extract extent, resolution, no_data and image statistics

    ifstream inputFile;
    inputFile.open(inputFileName);

    ofstream outputFile;
    outputFile.open(outputFileName, std::ofstream::out);

    if (!inputFile.is_open()) {
        cout << red << "Error reading input image file: " << reset << inputFileName << endl;
        return 1;
    }


    inputFile.close();
    outputFile.close();
    return 0;
}
