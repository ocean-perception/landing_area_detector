/**
 * @file lad_processing.hpp
 * @author Jose Cappelletto (j.cappelletto@soton.ac.uk)
 * @brief Core module of Landing Area Detection (lad) algorithm
 * @version 0.1
 * @date 2020-07-05
 * 
 * @copyright Copyright (c) 2020
 * 
 */

// pragma once is not "standard"
#ifndef _LAD_PROCESSING_HPP_
#define _LAD_PROCESSING_HPP_

#include "headers.h"
#include "lad_enum.hpp"
#include "lad_config.hpp"
#include "lad_core.hpp"

// #include <pcl/point_types.h>

using namespace std; // STL
using namespace cv;  // OpenCV
using namespace lad;  // OpenCV

/**
 * @brief Extend <lad> namespace with major processing tools for every type of layer 
 * 
 */
namespace lad
{ // landing area detection algorithm namespace
    int processGeotiff(std::string dataName, std::string maskName, int showImage = false); // Process Geotiff object and generate correspondig data and mask raster layers
    int extractContours(std::string rasterName, std::string contourName, int showImage = false);

    int convertDataSpace(vector<cv::Point2d> *inputData, vector<cv::Point2d> *outputData, int inputSpace, int outputSpace, double *apTransform = nullptr);

    // CGAL implementations associated to convexHull / alphaShape // Terrain slope
    // double computeMeanSlope (std::vector<cv::Point2d> inputPoints = NULL, cv::Vec3d normalVector = NULL);
    double computeMeanSlope ();

    double computePlaneSlope(KPlane plane, KVector reference = KVector(0,0,-1));

    std::vector<double> computePlaneDistance(KPlane plane, std::vector<KPoint> points);

    KPlane computeFittingPlane (std::vector<KPoint> points);
    int computePointsInSensor  (const std::vector<KPoint> &inpoints, std::vector<KPoint> &outpoints, double diameter);

    // std::vector<pcl::PointXYZ> convertMatrix2Vector2 (cv::Mat *matrix, double sx, double sy, double *acum);
    // std::vector<KPoint> convertMatrix2Vector (cv::Mat *matrix, double sx, double sy, double *acum);
    std::vector<KPoint> convertMatrix2Vector (const cv::Mat &matrix, double sx, double sy, double *acum);
    int                 convertMatrix2Vector (const cv::Mat &matrix, double sx, double sy, std::vector<KPoint> &master, double *acum);
    int convertMatrix2Vector_Points (const cv::Mat &matrix, double sx, double sy, std::vector<KPoint> &master, double *acum, std::vector<KPoint> &sensor, double diameter);
    int convertMatrix2Vector_Masked (const cv::Mat &matrix, const cv::Mat &mask1, const cv::Mat &mask2, double sx, double sy, std::vector<KPoint> &master, double *acum, std::vector<KPoint> &sensor, double diameter);

    float fitPlaneToSetOfPoints(const cv::Mat &pts, cv::Point3f &p0, cv::Vec3f &nml, double sx, double sy);

    // Pixel to World coordinates transformation
    // World to Pixel coordinates transformation
    // Already internally implemented with convertSpace + geoTransform matrix
    double computeExclusionSize(double x);

} // namespace lad

#endif // _LAD_PROCESSING_HPP_