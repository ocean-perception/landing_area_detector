# landing_area_detector
Computes seafloor landability map from hi-resolution bathymetry geoTIFF or XYZ non-gridded maps.


C++ based implementation for *Landing Area Detection* pipeline. Current requirements:

* GDAL for geoTIFF metadata parsing
* OpenCV for raster image processing (preferred over GDAL specifics)
* CGAL for improved alphaShape detection
* OpenMP for multithreading optimization

GIS specific solutions does not provide a seamless connection between different pipeline modules. QGIS plugins for SAGA, GDAL and GRASS fail to provide interoperability. The goal is to provide a low-level C++ modular implementation, and -if necessary- provides an interface with QGIS via Python plugins w/QT.
