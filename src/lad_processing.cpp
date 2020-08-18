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
#include <CGAL/Kernel/global_functions.h>
/**
 * @brief Extend <lad> namespace with layer processing algorithms. Intended to be called by Pipeline objects 
 * Valid data is assumed to be present in the layer containers involved
 */
namespace lad
{
    /**
     * @brief Convert all non-null elements from the single-channel raster image to CGAL compatible vector of 3D points. Horizontal and vertical coordinates are derived from pixel position and scale 
     * 
     * @param matrix Input image containing the height map as a 2.5D representing the height as z = f(x,y) 
     * @param sx Horizonal pixel scale
     * @param sy Vertical pixel scale
     * @return std::vector<KPoint> 
     */
    std::vector<KPoint> convertMatrix2Vector (cv::Mat *matrix, double sx, double sy, double *acum){
        //we need to create the i,j indexing variables to compute the Point3D (X,Y) coordinates, so we go for at<T_> access mode of cvMat container        
        int cols = matrix->cols;
        int rows = matrix->rows;
        double px, py, pz;
        std::vector<KPoint> output;

        for (int x=0; x<cols; x++){
            px = x * sx;
            for (int y=0; y<rows; y++){
                py = y * sy;
                pz = matrix->at<double>(cv::Point(x,y));
                // TODO: check against cv:SparseMatrix for faster iterations and removeing the necessity to check non-NULL data
                if (pz != 0){    //only non-NULL points are included (those are assumed to be invalida data points)
                    output.push_back(KPoint(px,py,pz));
                    *acum = *acum + pz;
                }
            }
        }
        return output;
    }

    /**
     * @brief Returns the angle (slope) of a plane by measuring the minimium angle between its normal and a reference vector 
     * 
     * @param plane 4D descriptor of the plane to be analized
     * @return double Minimum angle between the plane a the reference vector ([0 0 1] as default)
     */
    double computePlaneSlope(KPlane plane, KVector reference){

        KVector normal = plane.orthogonal_vector();        
        // normal = normal /(normal*normal);
        KVector nu = normal/sqrt(normal*normal);
        KVector ru = reference/sqrt(reference*reference);
        double p = nu * ru;// / (reference*reference);
        // cout << p << " ";        
        //  angle(reference, normal);
        p = acos(p)*180/M_PI;
        if (p>90)   p = 180 - p;
       // if (p<-90)  p = 180 + p;
        // cout << " p:" << p;
        return p;
        //WARNING: fix return value
        // return acos(p/(normal*normal));
    }


    /**
     * @brief 
     * 
     * @param points Vector of 3D points to be fitted in a plane
     * @return KPlane CGAL plane described as a 4D vector: A.X + B.Y + C.Z + D = 0 
     */
    KPlane computeFittingPlane (std::vector<KPoint> points){
        KPlane plane(0,0,1,0);
        if (points.empty()) // early exit
            return plane;
        // fit plane to whole triangles
        linear_least_squares_fitting_3(points.begin(), points.end(), plane, CGAL::Dimension_tag<0>());
        return plane;
    }


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
