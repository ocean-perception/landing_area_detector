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
// #include <opencv2/core/eigen.hpp>
/**
 * @brief Extend <lad> namespace with layer processing algorithms. Intended to be called by Pipeline objects 
 * Valid data is assumed to be present in the layer containers involved
 */
namespace lad
{

    float fitPlaneToSetOfPoints(const cv::Mat &pts, cv::Point3f &p0, cv::Vec3f &nml, double sx, double sy) {
        const int SCALAR_TYPE = CV_64F;
        typedef float ScalarType;

        // Calculate centroid
        p0 = cv::Point3f(0,0,0);
        int nPix = 0;
        // Calculate centroid
        for(int r = 0; r < pts.rows; r++) {
            // We obtain a pointer to the beginning of row r
            const double* ptr = pts.ptr<double>(r);
            //compute centroid
            for(int c = 0; c < pts.cols; c++) {
                if (ptr[c] !=0){
                    p0 = p0 + cv::Point3f(c*sx, r*sy, ptr[c]);
                    ++nPix; //increase the numebr of valid pixels
                }
            }
        }
        p0 *= 1.0/nPix;
        // Compose data matrix subtracting the centroid from each point
        cv::Mat Q(nPix, 3, SCALAR_TYPE);
        // Calculate centroid
        int i=0;
        for(int r = 0; r < pts.rows; r++) {
            // We obtain a pointer to the beginning of row r
            const double* ptr = pts.ptr<double>(r);
            //compute centroid
            for(int c = 0; c < pts.cols; c++) {
                if (ptr[c] !=0){
                    Q.at<ScalarType>(i,0) = c*sx   - p0.x;
                    Q.at<ScalarType>(i,1) = r*sy   - p0.y;
                    Q.at<ScalarType>(i,2) = ptr[c] - p0.z;
                    i++;
                }
            }
        }
        // Compute SVD decomposition and the Total Least Squares solution, which is the eigenvector corresponding to the least eigenvalue
        cv::SVD svd(Q, cv::SVD::MODIFY_A|cv::SVD::FULL_UV);
        nml = svd.vt.row(2);
        return 0;
    }

    /**
     * @brief Convert all non-null elements from the single-channel raster image to CGAL compatible vector of 3D points. Horizontal and vertical coordinates are derived from pixel position and scale 
     * 
     * @param matrix Input image containing the height map as a 2.5D representing the height as z = f(x,y) 
     * @param sx Horizonal pixel scale
     * @param sy Vertical pixel scale
     * @return std::vector<KPoint> 
     */
    std::vector<KPoint> convertMatrix2Vector (const cv::Mat &matrix, double sx, double sy, double *acum){
        //we need to create the i,j indexing variables to compute the Point3D (X,Y) coordinates, so we go for at<T_> access mode of cvMat container        
        int cols = matrix.cols;
        int rows = matrix.rows;
        std::vector<KPoint> master; // preallocating space does not improve it, maybe we are not triggering resize

        size_t total_elem = cols*rows; // expected input vector size

        // #pragma omp parallel
        {
            // TODO: rollback to for x,y or use contiguous pointer format (row,col) to SIMD px,py calculation
            // Also, it will remove the i->row,col modulo and division operation
            // std::vector<KPoint> slave; //preallocating space does not improve it
            // #pragma omp for nowait
            for (int i=0; i < total_elem; i++)  // single index iteration allows using omp parallel
            {
                double px, py, pz;
                //let's calculate the index
                int row, col;
                row = i / cols;
                col = (i % cols);
                // now, let's retrieve the pixel value and its spatial coordinates            
                pz = matrix.at<double>(row,col);
                // pz = matrix->at<double>(cv::Point(col,row));
                if (pz != 0.0f){    //only non-NULL points are included (those are assumed to be invalid data points)
                    px = col * sx;
                    py = row * sy;
                    master.push_back(KPoint(px,py,pz)); // we could ignore the scale and correct it AFTER plane-fitting
                    *acum = *acum + pz;
                }
            }
            // #pragma omp critical
            // {
            //     master.insert(master.end(), 
            //                         std::make_move_iterator(slave.begin()), 
            //                         std::make_move_iterator(slave.end()));
            // }
        }
        // *acum = 1;

        CGAL_PROFILER("calls to convertMatrix2Vector");

        return master;
    }

    int convertMatrix2Vector_Points (const cv::Mat &matrix, double sx, double sy, std::vector<KPoint> &master, double *acum, std::vector<KPoint> &sensor, double diameter){

        //we need to create the i,j indexing variables to compute the Point3D (X,Y) coordinates, so we go for at<T_> access mode of cvMat container        
        int cols = matrix.cols;
        int rows = matrix.rows;
        // std::vector<KPoint> master; // preallocating space does not improve it, maybe we are not triggering resize

        size_t total_elem = cols*rows; // expected input vector size
        double diam_th = 0.25f * diameter * diameter;   // precompute it once, we do not need to square it every iteration
        int r=0;
        // #pragma omp parallel
        {
            // TODO: rollback to for x,y or use contiguous pointer format (row,col) to SIMD px,py calculation
            // Also, it will remove the i->row,col modulo and division operation
            // std::vector<KPoint> slave; //preallocating space does not improve it
            // #pragma omp for nowait
            for (int i=0; i < total_elem; i++)  // single index iteration allows using omp parallel
            {
                double px, py, pz;

                //let's calculate the index
                int row, col;
                row = i / cols;
                col = (i % cols);
                // now, let's retrieve the pixel value and its spatial coordinates            
                pz = matrix.at<double>(row,col);
                // pz = matrix->at<double>(cv::Point(col,row));
                if (pz != 0.0f){    //only non-NULL points are included (those are assumed to be invalid data points)
                    px = (col - cols/2) * sx;   // Centering the points
                    py = (row - rows/2) * sy;   // This is necessary to speed-up the geotech sensor diameter-based masking
                    KPoint newPoint(px,py,pz);
                    master.push_back(newPoint); // we could ignore the scale and correct it AFTER plane-fitting
                    *acum = *acum + pz;

                    // snippet from pointsInSensor
                    double _d = px*px + py*py; 
                    if (_d < diam_th)  // no need to extract sqrt, just squared both sides
                        {
                            r++;    //we keep track of total of inserted points, as safe check return value
                            [[unlikely]] sensor.push_back(newPoint);
                        }
                }
            }
        }
        CGAL_PROFILER("calls to convertMatrix2Vector_Points");
        return r;
    }

    int computePointsInSensor  (const std::vector<KPoint> &inpoints, std::vector<KPoint> &outpoints, double diameter){
        // iterate through all the points contained in vector
        double diam_th = 0.25f * diameter * diameter;   // precompute it once, we do not need to square it every iteration
        double _x, _y;
        int r=0;
        for (auto it:inpoints){ // typ. inpoints value: 200~1000. Maybe using omp w/reductor loop for vector can help
            _x = it.x();
            _y = it.y();
            double _d = _x*_x + _y*_y; 
            // check if within coordinates (center)
            if (_d < diam_th)  // no need to extract sqrt, just squared both sides
                {
                    r++;    //we keep track of total of inserted points, as safe check return value
                    [[unlikely]] outpoints.push_back(it);
                }
        }
        return r;
    }

    /**
     * @brief Convert all non-null elements from the single-channel raster image to CGAL compatible vector of 3D points. Horizontal and vertical coordinates are derived from pixel position and scale 
     * 
     * @param matrix Input image containing the height map as a 2.5D representing the height as z = f(x,y) 
     * @param sx Horizonal pixel scale
     * @param sy Vertical pixel scale
     * @param master vector that will contain all the valid KPoints extracted from the input matrix
     * @param acum  pointer to store the sum(z) of all valid points. This value is used later to compute mean slope/height without re-scanning the vector again 
     * @return int number of valid points inserted into the vector (list) of points 
     */
    int convertMatrix2Vector (const cv::Mat &matrix, double sx, double sy, std::vector<KPoint> &master, double *acum){
        //we need to create the i,j indexing variables to compute the Point3D (X,Y) coordinates, so we go for at<T_> access mode of cvMat container        
        int cols = matrix.cols;
        int rows = matrix.rows;
        double _a = 0.0f;  // [optim] local copy to avoid aliasing + conversion from *acum (reduce fetch per cycle)
        // std::vector<KPoint> master; // preallocating space does not improve it, maybe we are not triggering resize

        size_t total_elem = cols*rows; // expected input vector size

        // #pragma omp parallel
        {
            // TODO: rollback to for x,y or use contiguous pointer format (row,col) to SIMD px,py calculation
            // Also, it will remove the i->row,col modulo and division operation
            // std::vector<KPoint> slave; //preallocating space does not improve it
            // #pragma omp for nowait
            for (int i=0; i < total_elem; i++)  // single index iteration allows using omp parallel
            {
                double px, py, pz;
                //let's calculate the index
                int row, col;
                row = i / cols;
                col = (i % cols);
                // now, let's retrieve the pixel value and its spatial coordinates            
                pz = matrix.at<double>(row,col);
                // pz = matrix->at<double>(cv::Point(col,row));
                if (pz != 0.0f){    //only non-NULL points are included (those are assumed to be invalid data points)
                    px = (col - cols/2) * sx;   // Centering the points
                    py = (row - rows/2) * sy;   // This is necessary to speed-up the geotech sensor diameter-based masking
                    master.push_back(KPoint(px,py,pz)); // we could ignore the scale and correct it AFTER plane-fitting
                    _a = _a + pz;
                }
            }
            // reduction section when slave/master omp mode is used
            // #pragma omp critical
            // {
            //     master.insert(master.end(), 
            //                         std::make_move_iterator(slave.begin()), 
            //                         std::make_move_iterator(slave.end()));
            // }
        }
        CGAL_PROFILER("calls to convertMatrix2Vector");
        *acum = _a;
        return 0;
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
     * @brief Computes the normal distance (minimum) of every KPoint provided in the vector <point> to the KPlane <plane>
     * 
     * @param plane Reference plane. The distance to points is using the closest (normal) projection onto this plane
     * @param points vector containing the 3D points to be projected against the plane
     * @return std::vector<double> vector containing the distance of <points> against <plane>. It keeps the same input <points> order
     */
    std::vector<double> computePlaneDistance(KPlane plane, std::vector<KPoint> points){
        double a = plane.a();   //for faster access, less overhead calling the methods
        double b = plane.b();
        double c = plane.c();
        double d = plane.d();
        std::vector<double> distances;

        size_t total = points.size();
        // #pragma omp parallel            // OMP parallel for vector<double>
        {
            // std::vector<double> slave; // thread local copy

            // __m256d _c;            
            // _c = _mm256_set_pd(a,b,c,0.0f); // define coefficient vector for the constant plane coeff

            // #pragma omp parallel for num_threads(8)  // thread allocation overhead appears to reduce performance
///*
            for (int i=0; i < total; i++){
                double outdata[4], val;
                auto p = points[i];                                         // can we exploit having points[i] memory aligned?
                val = a*p[0] + b*p[1] + c*p[2] + d;
                distances.push_back(val);
            }
//*/
    // AVX optimization available, but it is not the bottleneck
/*            for (int i=0; i < total; i++){
                // 64-bit double "registers"
                __m256d _p, _d, _v, _r;
                __m256d _ymm0, _ymm1, _ymm2, _ymm3, _ymm4;

                double outdata[4], val;
                auto p = points[i];       // can we exploit having points[i] memory aligned?

            // val = a*p[0] + b*p[1] + c*p[2] + d;
                _p = _mm256_set_pd(p[0],p[1],p[2],0.0f); // vector for 3D point
                _d = _mm256_set_pd(0.0,0.0,0.0, d); // vector for 'd' plane constant
                _r = _mm256_mul_pd(_c, _p); // C * P + D
                _r = _mm256_add_pd(_r, _d);
                // _r = _mm256_fmadd_pd(_c, _p, _d); // C * P + D
                // // now we need to sum all the elements of the vector '_r' (horizontal add)

                _ymm0 = _r;
                _ymm1 = _mm256_permute_pd(_ymm0, 0x05);
                _ymm2 = _mm256_add_pd(_ymm0, _ymm1);
                _ymm3 = _mm256_permute2f128_pd(_ymm2, _ymm2, 0x01);
                _ymm4 = _mm256_add_pd(_ymm2, _ymm3);
                _mm256_storeu_pd(outdata, _ymm4);
                // val   = outdata[0];

                distances.push_back(outdata[0]);
            }
*/
            // #pragma omp critical
            // {
            //     // collector / merge
            //     distances.insert(distances.end(), 
            //                 std::make_move_iterator(slave.begin()), 
            //                 std::make_move_iterator(slave.end()));
            // }

        }
        CGAL_PROFILER("computePlaneDistance (all calls)");
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
        // f(x) = -3.948793 x*x + 2.16931*x + 0.0094463
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