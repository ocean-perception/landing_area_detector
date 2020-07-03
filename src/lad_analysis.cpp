/********************************************************************/
/* Project: Landing Area Detection algorithm						*/
/* Module: 	lad_analysis												*/
/* File: 	lad_analysis.cpp                                            */
/* Created:		02/07/2020                                          */
/* Description
*/

/********************************************************************/
/* Created by:                                                      */
/* Jose Cappelletto - j.cappelletto@soton.ac.uk		                */
/********************************************************************/

#include "headers.h"
#include "lad_analysis.h"
#include "helper.h"
// Temporal preprocesing function

using namespace std;
using namespace cv;

/**
 * @brief Integrated pre-processing pipeline from Geotiff object to boundary mask
 * 
 * @param apGeotiff 
 * @return int 
 */
int processGeotiff(Geotiff *apGeotiff){
        
    int *dimensions;
    dimensions = apGeotiff->GetDimensions();
    //**************************************
    float **apData; //pull 2D float matrix containing the image data for Band 1
    apData = apGeotiff->GetRasterBand(1);

    cv::Mat tiff(dimensions[0], dimensions[1], CV_32FC1); // cv container for tiff data . WARNING: cv::Mat constructor is failing to initialize with apData
    for (int i=0; i<dimensions[0]; i++){
        for (int j=0; j<dimensions[1]; j++){
            tiff.at<float>(cv::Point(j,i)) = (float)apData[i][j];   // swap row/cols from matrix to OpenCV container
        }
    }
    cv::Mat matDataMask;    // Now, using the NoData field from the Geotiff/GDAL interface, let's obtain a binary mask for valid/invalid pixels
    cv::compare(tiff, apGeotiff->GetNoDataValue(), matDataMask, CMP_NE); // check if NOT EQUAL to GDAL NoData field

    cv::Mat tiff_colormap = Mat::zeros( tiff.size(), CV_8UC1 ); // colour mapped image for visualization purposes
    cv::normalize(tiff, tiff_colormap, 0, 255, NORM_MINMAX, CV_8UC1, matDataMask); // normalize within the expected range 0-255 for imshow
    // apply colormap for enhanced visualization purposes
    cv::applyColorMap(tiff_colormap, tiff_colormap, COLORMAP_TWILIGHT_SHIFTED);

    imshow("tiff colormap", tiff_colormap); // this will show nothing, as imshow needs remapped images
    waitKey(0);


    double minValue, maxValue;

    cv::minMaxLoc(tiff, &minValue, &maxValue, NULL, NULL, matDataMask);
	// cout << "(tiff) data type: " << type2str(tiff.type()) << endl;
    cout << "(tiff) Min: " << minValue << endl;
    cout << "(tiff) Max: " << maxValue << endl;

    cv::minMaxLoc(matDataMask, &minValue, &maxValue, NULL, NULL);
	// cout << "(mask) data type: " << type2str(matDataMask.type()) << endl;
    cout << "(matDataMask) Min: " << minValue << endl;
    cout << "(matDataMask) Max: " << maxValue << endl;

    vector< vector<Point> > contours;   // find contours of the DataMask layer
    findContours(matDataMask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE); // obtaining only 1st level contours, no children

    cout << "# Contours detected: " << contours.size() << endl;
    if (!contours.size()){
        cout << red << "No contour line was detected! Exitting" << endl;
        return -1;
    }

    // WARNING: contours may provide false shapes when valida data mask reaches any image edge.
    // SOLUTION: expand +1px the image canvas on every direction, or remove small bathymetry section (by area or number of points)
    // See: copyMakeBorder @ https://docs.opencv.org/3.4/dc/da3/tutorial_copyMakeBorder.html UE: BORDER_CONSTANT (set to ZERO)
    Mat boundingLayer = Mat::zeros(matDataMask.size(), CV_8UC1);   // empty mask
    int n =0;
    for (const auto &contour: contours){
        cout << "Contour["<< n++ <<"] size: " << contour.size() << endl;
        drawContours(boundingLayer, contours, -1, Scalar(255*n/contours.size()), 1); // overlay contours in new mask layer, 1px width line, white
    }
    cv::Mat mask_colormap(boundingLayer.size(), CV_8UC3);
    cv::applyColorMap(boundingLayer, mask_colormap, COLORMAP_TURBO);

    Mat erode_output = Mat::zeros(matDataMask.rows, matDataMask.cols, CV_8UC1);
    Mat erode_kernel = Mat::ones(20, 50, CV_8UC1);
    cv::erode(matDataMask, erode_output, erode_kernel); // erode kernel to valid data mask

    // contours basically contains the minimum bounding polygon down to 1-pixel resolution
    // WARNING: CV_FILLED fills holes inside of the polygon. Contours may return a collection of shapes (list of list of points)

    imshow ("Contour", mask_colormap);
    imshow ("Eroded mask", erode_output * 255);
    waitKey(0);
    imwrite("contours.tif",boundingLayer);
    imwrite("contours_color.tif",mask_colormap);
    return 0;
}


/*
    TODO: FOR MEMCPY OF GDAL GEOTIFF DATA STREAM TO CVMAT

std:vector<std::vector<float> > OrigSamples;
...
... // Here OrigSamples is filled with some rows with the same row size
...

// Create a new, _empty_ cv::Mat with the row size of OrigSamples
cv::Mat NewSamples(0, OrigSamples[0].size(), cv::DataType<float>::type);

for (unsigned int i = 0; i < OrigSamples.size(); ++i)
{
  // Make a temporary cv::Mat row and add to NewSamples _without_ data copy
  cv::Mat Sample(1, OrigSamples[0].size(), cv::DataType<float>::type, OrigSamples[i].data());

  NewSamples.push_back(Sample);
}

*/