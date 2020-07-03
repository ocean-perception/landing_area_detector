/********************************************************************/
/* Project: Landing Area Detection algorithm						*/
/* Module: 	lad_test												*/
/* File: 	lad_test.cpp                                            */
/* Created:		18/06/2020                                          */
/* Description
*/

/********************************************************************/
/* Created by:                                                      */
/* Jose Cappelletto - j.cappelletto@soton.ac.uk		                */
/********************************************************************/
///Basic C and C++ libraries
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cmath>
#include <stdexcept>
#include <vector>

/// OpenCV libraries. May need review for the final release
#include <opencv2/core.hpp>
#include "opencv2/core/ocl.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <opencv2/video.hpp>
#include <opencv2/features2d.hpp>
#include "opencv2/calib3d.hpp"
#include <opencv2/xfeatures2d.hpp>

/// CUDA specific libraries
#if USE_GPU
    #include <opencv2/cudafilters.hpp>
    #include "opencv2/cudafeatures2d.hpp"
    #include "opencv2/xfeatures2d/cuda.hpp"
#endif

using namespace std;
using namespace cv;
//using namespace cv::cuda;     //prefer explicit definitions rather than risking name mangling

#include "../include/options.h"
#include <geotiff.hpp>      // Geotiff class definitions
#include "helper.cpp"

const string green("\033[1;32m");
const string yellow("\033[1;33m");
const string cyan("\033[1;36m");
const string red("\033[1;31m");
const string reset("\033[0m");

#define DEFAULT_OUTPUT_FILE "output.tif"

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
    cout << "\tOpenCV version:\t" << yellow << CV_VERSION << reset << endl;
    cout << "\tGit commit:\t" << yellow << GIT_COMMIT << reset << endl;
    // cout << "\tBuilt:\t" << __DATE__ << " - " << __TIME__ << endl;

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

    float alphaShapeRadius = 1.0;
//    int CUDA = 0;                                       //Default option (running with CPU)
    /*
     * Start parsing mandatory arguments
     */

    if (!argInput){
        cerr << "Mandatory <input> file name missing" << endl;
        cerr << "Use -h, --help command to see usage" << endl;
        return 1;
    }

    string inputFileName = args::get(argInput);	//String containing the input file path+name from cvParser function
    string outputFileName = DEFAULT_OUTPUT_FILE;

    if (!argOutput){
        cerr << "Using default [output] filename: " << reset << "output.tif" << endl;
        // cerr << "Use -h, --help command to see usage" << endl;
        // return 1;
    }
    else
    {
        outputFileName = args::get(argOutput);	//String containing the output file template from cvParser function
        cout << "Output filename: " << outputFileName << endl;
    }
    
    /*
     * These were the mandatory arguments. Now we proceed to optional parameters.
     * When each variable is defined, we assign the default value.
     */
    if (argAlphaRadius) cout << "Using user-defined [argAlphaRadius]: " << (alphaShapeRadius = args::get(argAlphaRadius)) << endl;
    else cout << yellow << "Using default value for [argAlphaRadius]: " << alphaShapeRadius << reset << endl;

    //**************************************************************************
    /* Summary list parameters */
    cout << yellow << "Summary" << reset << endl;
    cout << "Input file:\t" << inputFileName << endl;
    cout << "Output file:\t" << outputFileName << endl;
    cout << "alphaShapeRadius:\t" << alphaShapeRadius << endl;
    cout << "***********************************" << endl;

    // create the container and the open input file
    Geotiff geoContainer (inputFileName.c_str());
    if (!geoContainer.isValid()){ // check if nothing wrong happened with the constructor
        cout << red << "Error opening Geotiff file: " << reset << inputFileName << endl;
        return -1;
    }

    //**************************************
    // Get/print summary information of the TIFF 
    GDALDataset *poDataset;
    poDataset = geoContainer.GetDataset(); //pull the pointer to the main GDAL dataset structure

    // geoContainer.ShowInformation();    
    int *dimensions;
    dimensions = geoContainer.GetDimensions();
    //**************************************
    float **apData; //pull 2D float matrix containing the image data for Band 1
    apData = geoContainer.GetRasterBand(1);

    cv::Mat tiff(dimensions[0], dimensions[1], CV_32FC1); // cv container for tiff data . WARNING: cv::Mat constructor is failing to initialize with apData
    for (int i=0; i<dimensions[0]; i++){
        for (int j=0; j<dimensions[1]; j++){
            tiff.at<float>(cv::Point(j,i)) = (float)apData[i][j];   // swap row/cols from matrix to OpenCV container
        }
    }

    cv::Mat matDataMask;    // Now, using the NoData field from the Geotiff/GDAL interface, let's obtain a binary mask for valid/invalid pixels
    cv::compare(tiff, geoContainer.GetNoDataValue(), matDataMask, CMP_NE); // check if NOT EQUAL to GDAL NoData field

    cv::Mat tiff_colormap = Mat::zeros( tiff.size(), CV_8UC1 ); // colour mapped image for visualization purposes
    cv::normalize(tiff, tiff_colormap, 0, 255, NORM_MINMAX, CV_8UC1, matDataMask); // normalize within the expected range 0-255 for imshow
    // apply colormap for enhanced visualization purposes
    cv::applyColorMap(tiff_colormap, tiff_colormap, COLORMAP_TWILIGHT_SHIFTED);

    imshow("tiff colormap", tiff_colormap); // this will show nothing, as imshow needs remapped images
    waitKey(0);


    double minValue, maxValue;

    cv::minMaxLoc(tiff, &minValue, &maxValue, NULL, NULL, matDataMask);
	cout << "(tiff) data type: " << type2str(tiff.type()) << endl;
    cout << "(tiff) Min: " << minValue << endl;
    cout << "(tiff) Max: " << maxValue << endl;

    cv::minMaxLoc(matDataMask, &minValue, &maxValue, NULL, NULL);
	cout << "(mask) data type: " << type2str(matDataMask.type()) << endl;
    cout << "(matDataMask) Min: " << minValue << endl;
    cout << "(matDataMask) Max: " << maxValue << endl;

    vector< vector<Point> > contours;   // find contours of the DataMask layer
    findContours(matDataMask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    Mat boundingLayer = Mat::zeros(matDataMask.size(), CV_8UC1);   // empty mask

    drawContours(boundingLayer, contours, -1, Scalar(255), 1); // overlay contours in new mask layer, 1px width line, white

    Mat erode_output = Mat::zeros(matDataMask.rows, matDataMask.cols, CV_8UC1);
    Mat erode_kernel = Mat::ones(20, 50, CV_8UC1);
    cv::erode(matDataMask, erode_output, erode_kernel); // erode kernel to valid data mask

    // contours basically contains the minimum bounding polygon down to 1-pixel resolution
    // WARNING: CV_FILLED fills holes inside of the polygon. Contours may return a collection of shapes (list of list of points)
    
    imshow ("Contour", boundingLayer);
    imshow ("Eroded mask", erode_output * 255);
    waitKey(0);
    cout << "Contours[N]: " << contours.size() << endl;
    return 0;
}
