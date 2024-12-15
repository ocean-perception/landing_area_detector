**Note:** parallel to https://github.com/ocean-perception/landing_area_detector/

# landing_area_detector
Implementation of underwater landability calculation pipeline. Computes seafloor landability map from vehicle (AUV / ROV) charasteristics and hi-resolution bathymetry geoTIFF or XYZ non-gridded maps, based on the algorithm described in "Autonomous Landing of Underwater Vehicles Using High-Resolution Bathymetry" [https://ieeexplore.ieee.org/document/8897695]

# Requirements
* GDAL for geoTIFF metadata parsing
* OpenCV for raster image processing (preferred over GDAL specifics)
* CGAL for planet fitting, convex hull (3D) and alpha-shape (2D) extraction
* OpenMP for multithreading optimization

# Installation

# Usage

