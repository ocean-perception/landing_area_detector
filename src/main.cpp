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
using namespace cv::cuda;     //prefer explicit definitions rather than risking name mangling

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

cv::Point2d pixel2world( const int&, const int&, const cv::Size& );
cv::Point2d lerp( const cv::Point2d&, const cv::Point2d&, const double& );

/*
 * Linear Interpolation
 * p1 - Point 1
 * p2 - Point 2
 * t  - Ratio from Point 1 to Point 2
*/
cv::Point2d lerp( cv::Point2d const& p1, cv::Point2d const& p2, const double& t ){
    return cv::Point2d( ((1-t)*p1.x) + (t*p2.x),
                        ((1-t)*p1.y) + (t*p2.y));
}

/*
 * Convert a pixel coordinate to world coordinates
*/
/*cv::Point2d pixel2world( const int& x, const int& y, const cv::Size& size ){
    // compute the ratio of the pixel location to its dimension
    double rx = (double)x / size.width;
    double ry = (double)y / size.height;
    // compute LERP of each coordinate
    cv::Point2d rightSide = lerp(tr, br, ry);
    cv::Point2d leftSide  = lerp(tl, bl, ry);
    // compute the actual Lat/Lon coordinate of the interpolated coordinate
    return lerp( leftSide, rightSide, rx );
}//*/

string type2str(int type) {
  string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}

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
    //  1- Read input file (path/filename must be accepted)
    //  2- Test/open all the mandatory fields in the input
    //  3- Extract extent, resolution, no_data and image statistics

    // load the image (note that we don't have the projection information.  You will
    // need to load that yourself or use the full GDAL driver.  The values are pre-defined
    // at the top of this file
//    cv::Mat image = cv::imread(inputFileName, cv::IMREAD_LOAD_GDAL | cv::IMREAD_COLOR );
    cv::Mat image = cv::imread(inputFileName, cv::IMREAD_LOAD_GDAL | cv::IMREAD_GRAYSCALE );
 
    //cv::imshow("Input image", image);
    cout << yellow << "Input image loaded" << endl;

    //char key = (char)waitKey(0);
    char key;

    cout << "Image depth: " << image.depth() << endl;

	string ty =  type2str( image.type() );
	cout << "Image data type: " << ty << endl;

    cout << "Channels: " << image.channels() << endl;
//    cv::Vec3b min, max, pixel;
    double min = 200000, max , pixel;
    if (image.at<float>(0,0) > 0) min = max = image.at<float>(0,0);
    double acum = 0;

    for( int y = 0; y < image.rows; y++ ) {
        for( int x = 0; x < image.cols; x++ ) {
		    pixel = image.at<float>(y,x);
//		    if (pixel[0] > 0) cout << pixel << " ";
			if (pixel < min){
				if (pixel > 0) min = pixel; //get rid of no-data zero fields
			}
			if (pixel > max) max = pixel;
			acum += pixel;
/*            for( int c = 0; c < image.channels(); c++ ) {
			    cout << pixel << " ";
            }*/
        }
    }//*/

    cout << "Acc: " << acum << endl;
    cout << "Mean: " << acum /(image.rows*image.cols) << endl;
    cout << "Max: " << max << endl;
    cout << "Min: " << min << endl;
    float alfa = 255/(max - min);
    cout << "Alfa: " << alfa << endl;

    for( int y = 0; y < image.rows; y++ ) {
        for( int x = 0; x < image.cols; x++ ) {
//		    image.at<float>(y,x) = 50 + (x+y+1)/100;
        	pixel = image.at<float>(y,x);
        	if (pixel <1) image.at<float>(y,x) = min;
//		    image.at<float>(y,x) = (image.at<float>(y,x) - min)*alfa;
        }
    }//*/

    acum = 0;
    max = 0;
    min = 99999;

    for( int y = 0; y < image.rows; y++ ) {
        for( int x = 0; x < image.cols; x++ ) {
		    pixel = image.at<float>(y,x);
//		    if (pixel[0] > 0) cout << pixel << " ";
			if (pixel < min){
				if (pixel > 0) min = pixel; //get rid of no-data zero fields
			}
			if (pixel > max) max = pixel;
			acum += pixel;
        }
    }//*/

    cout << "Acc: " << acum << endl;
    cout << "Mean: " << acum /(image.rows*image.cols) << endl;
    cout << "Max: " << max << endl;
    cout << "Min: " << min << endl;

    Mat new_image = Mat::zeros( image.size(), CV_8UC1 );

    normalize(image, new_image,0 , 255, NORM_MINMAX, CV_8UC1);

    imshow("Src Image", new_image);
	key = (char)waitKey(0);

//	imwrite("salida.jpg", image);
    return 0;
}
