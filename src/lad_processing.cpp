/**
 * @file lad_core.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief  Landing Area Detection algorithm core header
 * @version 0.1
 * @date 2020-07-05
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "headers.h"
#include "lad_processing.hpp"
#include "lad_layer.hpp"

/**
 * @brief Extend <lad> namespace with layer processing algorithms. Intended to be called by Pipeline objects 
 * Valid data is assumed to be present in the layer containers involved
 */
namespace lad
{

    /**
     * @brief Compute the smallest angle between normalVector and the best least-square fitting plane for a collection of 3D points 
     * 
     * @param inputPoints  3D points to be fitted with the plane
     * @param normalVector Angle reference vector (typ n={ 0 0 1})
     * @return double Smallest angle in radians
     */
    // double computeMeanSlope (std::vector<cv::Point2d> inputPoints, cv::Vec3d normalVector){
    double computeMeanSlope (){

        std::vector<KTriangle> triangles;
        KPoint a(1.0,2.0,3.0);
        KPoint b(4.0,0.0,6.0);
        KPoint c(7.0,8.0,9.0);
        KPoint d(8.0,7.0,6.0);
        KPoint e(5.0,3.0,4.0);
        triangles.push_back(KTriangle(a,b,c));
        triangles.push_back(KTriangle(a,b,d));
        triangles.push_back(KTriangle(d,e,c));
        KLine line;
        KPlane plane;
        // fit plane to whole triangles
        linear_least_squares_fitting_3(triangles.begin(),triangles.end(),plane,CGAL::Dimension_tag<2>());

        cout << "Plane:" << plane << endl;
        return 0;
    }//*/


    /**
     * @brief Converts vector of 2D points from one coordinate space to another. The valid spaces are PIXEL and WORLD coordinates 
     * 
     * @param inputData Pointer input vector of cv::Point2d points to be converted
     * @param outputData Pointer to output vector where transformed points will be stored
     * @param inputSpace Identifier of source coordinate system
     * @param outputSpace Identifier of target coordinate system
     * @param apTransform 6D transformation matrix describing the desired transformation (scale and offset)
     * @return int Error code, if any
     */
    int convertDataSpace(vector<cv::Point2d> *inputData, vector<cv::Point2d> *outputData, int inputSpace, int outputSpace, double *apTransform)
    {
        if (inputSpace == outputSpace)
        { // No transformation was required
            // let's just clone the data
            cout << yellow << "[convertDataSpace] source and target coordinate space are the same. Will copy points without transformation" << reset << endl;
            for (auto it : *inputData)
            {
                outputData->push_back(it); //element wise deep-copy
            }
            return 0;
        }
        // Do we have a valid transformation matrix?
        if (apTransform == nullptr)
        {
            cout << red << "[convertDataSpace] invalid transformation matrix received" << reset << endl;
            cout << red << "[convertDataSpace] no transformation performed" << reset << endl;
            return -1;
        }

        double sx = apTransform[1]; // Pixel size (X)
        double sy = apTransform[5]; // Pixel size (Y)
        double cx = apTransform[0]; // Center coordinate (X)
        double cy = apTransform[3]; // Center coordinate (Y)

        cv:Point2d p;
        // NOTE: Pixel coordinates are for the center of the pixel. Hence, the 0.5 adjustment
        switch (outputSpace)
        {
        case WORLD_COORDINATE:
            for (auto it : *inputData)
            {
                p.x = (it.x + 0.5) * sx + cx; // Scale & traslation 2D transformation
                p.y = (it.y + 0.5) * sy + cy; // No rotation implemented (yet)
                outputData->push_back(p);
            }
            break;

        case PIXEL_COORDINATE:
            for (auto it : *inputData)
            {
                p.x = -0.5 + (it.x - cx) / sx; // Scale & traslation 2D transformation
                p.y = -0.5 + (it.y - cy) / sy; // No rotation implemented (yet)
                outputData->push_back(p);
            }
            break;

        default:
            break;
        }
        return 0;
    }

} // namespace lad
