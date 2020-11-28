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

double calculateTriArea (vector<Point3d> points){
    Point3d u,v;    // base vectors to be used for triangle area calculation
    u = points[1] - points[0];
    v = points[2] - points[0];
    register double a = u.y*v.z - u.z*v.y;
    register double b = u.z*v.x - u.x*v.z;
    register double c = u.x*v.y - u.y*v.x;
    return sqrt(a*a + b*b + c*c)/2;
}

double calculateWindowArea (vector<Point3d> p){
    // iterate the 4 corners to generate 4 triangles
    vector<Point3d> tri(3);

    Point3d center = (p[0] + p[1] + p[2] + p[3])/4; // center of the shape
    tri[0] = center; // we maintain the pivot vertex

    tri[1] = p[0];
    tri[2] = p[1];
    double a1 = calculateTriArea(tri);
    tri[1] = p[1];
    tri[2] = p[2];
    double a2 = calculateTriArea(tri);
    tri[1] = p[2];
    tri[2] = p[3];
    double a3 = calculateTriArea(tri);
    tri[1] = p[3];
    tri[2] = p[0];
    double a4 = calculateTriArea(tri);
    return a1 + a2 + a3 + a4;
}



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
    // Mandatory arguments: only the input filename
    if (argInputT2P) inputFileName = args::get(argInputT2P); //input file is mandatory positional argument.
    if (inputFileName.empty()){ //not defined as command line argument? let's use config.yaml definition
            logc.error ("main", "Input file missing. Please define it using --input='filename'");
            return -1;
    }
    //Optional arguments
    if (argOutputT2P) outputFileName = args::get(argOutputT2P); //output file is optional positional argument
    /* Summary list parameters */
    if (verbosity >= 1){
        cout << yellow << "****** Summary **********************************" << reset << endl;
        cout << "Input file:    \t" << green << inputFileName << reset << endl;
        if (outputFileName.empty()){
            cout << "No output map will be generated" << endl;
        }
        else{
            cout << "Output file:   \t" << yellow << outputFileName << reset << endl;
        }
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

    double sx = fabs(apLayer->transformMatrix[1]);    // pixel width
    double sy = fabs(apLayer->transformMatrix[5]);    // pixel height

    Point3d a (0,  0, 0);   // the surface area is traslation independent
    Point3d b (sx, 0, 0);   // so we can fix the x/y coordinates
    Point3d c (sx,sy, 0);   // and only update the z coordinate
    Point3d d (0, sy, 0);

    Mat dst = Mat::zeros(apLayer->rasterData.size(), CV_64FC1); // empty container, same size as input
    double totalArea = 0;
    double rugosity = 0;
    int validPixels = 0;
    double nd =  apLayer->noDataValue;

    for (int j=0; j<dst.rows-1; j++){   // last row is replicated
        for (int i=0; i<dst.cols-1; i++){ // last column is replicated
            a.z = apLayer->rasterData.at<double>(Point2i(i,  j));
            b.z = apLayer->rasterData.at<double>(Point2i(i+1,j));
            c.z = apLayer->rasterData.at<double>(Point2i(i+1,j+1));
            d.z = apLayer->rasterData.at<double>(Point2i(i  ,j+1)); // four corners
            // if (any of the additional points) is (nodata), invalidate calculation
            if ((a.z==nd) || (b.z==nd) || (c.z==nd) || (d.z==nd)){ // nodata, invalidate calculation
                dst.at<double>(Point2i(i,j)) = nd; //
            }
            else{
                vector<Point3d> v;
                // now we must compute the total area for the 4 triangles around the center point 
                v.push_back(a);
                v.push_back(b);
                v.push_back(c);
                v.push_back(d);
                double area = calculateWindowArea (v); 
                rugosity = area / (sx*sy);
                dst.at<double>(Point2i(i,j)) = rugosity; // we store the area, it should be divide by the planar area, but it is constant, can be done by the end
                dst.at<double>(Point2i(i,j+1)) = rugosity; // repeat for next row (it will be updated for all except last row)
                validPixels++;
                // dst.at<double>(Point2i(i,j)) = area; // we store the area, it should be divide by the planar area, but it is constant, can be done by the end
                totalArea += area;
            }                
        }
        // for the last column we repeat our value (mirror-edge)
        dst.at<double>(Point2i(dst.cols,j)) = rugosity;
    }
    // also, we have the totalArea (non-scaled). We can extract the rugosiry value for the whole image
    double meanRugosity = totalArea / (validPixels*sx*sy);
    cout << meanRugosity << endl; // this is the value that can be used by the caller

    if (verbosity>=2){
        cv::Mat original;
        apLayer->rasterData.copyTo(original);
        namedWindow ("original");
        cv::normalize(original, original, 0, 255, NORM_MINMAX, CV_8UC1, apLayer->rasterMask); // normalize within the expected range 0-255 for imshow
        imshow("original", original);

        cv::Mat normalized;
        namedWindow ("rugosity");
        cv::normalize(dst, normalized, 0, 255, NORM_MINMAX, CV_8UC1); // normalize within the expected range 0-255 for imshow
        imshow("rugosity", normalized);
        waitKey(0);
    }

    if (!outputFileName.empty()){ // let's export the data
        dst.copyTo(apLayer->rasterData); // overwrite the memory copy of the bathymetry
        pipeline.exportLayer(layer, outputFileName, FMT_TIFF);
    }

    if (verbosity > 0)
        tic.lap("");
    return NO_ERROR;
}
