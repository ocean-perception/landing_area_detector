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
#include "lad_core.hpp"
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
     * @brief Convert all non-null elements from the single-channel raster image to CGAL compatible vector of 3D points. Horizontal and vertical coordinates are derived from pixel position and scale 
     * 
     * @param matrix Input image containing the height map as a 2.5D representing the height as z = f(x,y) 
     * @param sx Horizonal pixel scale
     * @param sy Vertical pixel scale
     * @return std::vector<KPoint> 
     */
    std::vector<pcl::PointXYZ> convertMatrix2Vector2 (cv::Mat *matrix, double sx, double sy, double *acum){
        //we need to create the i,j indexing variables to compute the Point3D (X,Y) coordinates, so we go for at<T_> access mode of cvMat container        
        int cols = matrix->cols;
        int rows = matrix->rows;
        double px, py, pz;
        std::vector<pcl::PointXYZ> output;

        for (int x=0; x<cols; x++){
            px = x * sx;
            for (int y=0; y<rows; y++){
                py = y * sy;
                pz = matrix->at<double>(cv::Point(x,y));
                // TODO: check against cv:SparseMatrix for faster iterations and removeing the necessity to check non-NULL data
                if (pz != 0){    //only non-NULL points are included (those are assumed to be invalid data points)
                    output.push_back(pcl::PointXYZ(px,py,pz));
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

    std::vector<double> computePlaneDistance(KPlane plane, std::vector<KPoint> points){
        double a = plane.a();   //for faster access, less overhead calling the methods
        double b = plane.b();
        double c = plane.c();
        double d = plane.d();
        std::vector<double> distances;
        for (auto p:points){
            double val = a*p.x() + b*p.y() + c*p.z() + d;
            distances.push_back(val);
        }
        return distances;
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
     * @brief 
     * 
     * @param points Vector of 3D points to be fitted in a plane
     * @return KPlane CGAL plane described as a 4D vector: A.X + B.Y + C.Z + D = 0 
     */
    KPlane computeFittingPlane2 (std::vector<KPoint> points){
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

    /**
     * @brief Returns the exclusion zone size (disk radius [m]) for a given obstacle height 'x'. Additionaly returns the lower and upper thershold for the height region
     * 
     * @param x Obstacle height in meters 
     * @return double exclusion disk radius in meters
     */
    double computeExclusionSize(double x){
    // exzone	 height 
    // 0.025	0.008772129409728
    // 0.05	0.019110046267601
    // 0.075	0.030975368886052
    // 0.1	0.044321160365278
    // 0.125	0.05909278542667
    // 0.15	0.075228908315143
    // 0.175	0.092662592462244
    // 0.2	0.111322460369619
    // 0.225	0.131133872530442
    // 0.25	0.152020086896598
        // Curve fitting eq:
        // f(x) = -3.948793 x*x + 2.16931 + 0.0094463
        // R2 = 0.9994
        return (-3.948793*x*x + 2.16931*x + 0.0094463);
    }


} // namespace lad

/*
from https://stackoverflow.com/questions/1400213/3d-least-squares-plane
float fitPlaneToSetOfPoints(const std::vector<cv::Point3f> &pts, cv::Point3f &p0, cv::Vec3f &nml) {
    const int SCALAR_TYPE = CV_32F;
    typedef float ScalarType;

    // Calculate centroid
    p0 = cv::Point3f(0,0,0);
    for (int i = 0; i < pts.size(); ++i)
        p0 = p0 + conv<cv::Vec3f>(pts[i]);
    p0 *= 1.0/pts.size();

    // Compose data matrix subtracting the centroid from each point
    cv::Mat Q(pts.size(), 3, SCALAR_TYPE);
    for (int i = 0; i < pts.size(); ++i) {
        Q.at<ScalarType>(i,0) = pts[i].x - p0.x;
        Q.at<ScalarType>(i,1) = pts[i].y - p0.y;
        Q.at<ScalarType>(i,2) = pts[i].z - p0.z;
    }

    // Compute SVD decomposition and the Total Least Squares solution, which is the eigenvector corresponding to the least eigenvalue
    cv::SVD svd(Q, cv::SVD::MODIFY_A|cv::SVD::FULL_UV);
    nml = svd.vt.row(2);

    // Calculate the actual RMS error
    float err = 0;
    for (int i = 0; i < pts.size(); ++i)
        err += powf(nml.dot(pts[i] - p0), 2);
    err = sqrtf(err / pts.size());

    return err;
}
*/

/*
https://stackoverflow.com/questions/40589802/eigen-best-fit-of-a-plane-to-n-points
template<class Vector3>
std::pair<Vector3, Vector3> best_plane_from_points(const std::vector<Vector3> & c)
{
    // copy coordinates to  matrix in Eigen format
    size_t num_atoms = c.size();
    Eigen::Matrix< Vector3::Scalar, Eigen::Dynamic, Eigen::Dynamic > coord(3, num_atoms);
    for (size_t i = 0; i < num_atoms; ++i) coord.col(i) = c[i];

    // calculate centroid
    Vector3 centroid(coord.row(0).mean(), coord.row(1).mean(), coord.row(2).mean());

    // subtract centroid
    coord.row(0).array() -= centroid(0); coord.row(1).array() -= centroid(1); coord.row(2).array() -= centroid(2);

    // we only need the left-singular matrix here
    //  http://math.stackexchange.com/questions/99299/best-fitting-plane-given-a-set-of-points

    auto svd = coord.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV);
    Vector3 plane_normal = svd.matrixU().rightCols<1>();
    std::cout << plane_normal << std::endl;


    return std::make_pair(centroid, plane_normal);

}

void main()
{
    Eigen::Vector3f point1(2, 2, 2);
    Eigen::Vector3f point2(4, 2, 3);
    Eigen::Vector3f point3(4, 2,1);
    Eigen::Vector3f point4(2, 2,3);

    std::vector<Eigen::Vector3f> points;

    points.push_back(point1);
    points.push_back(point2);
    points.push_back(point3);
    points.push_back(point4);


    best_plane_from_points(points);

    std::cin.get();
}
*/