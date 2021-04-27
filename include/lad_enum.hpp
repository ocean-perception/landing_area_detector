/**
 * @file lad_enum.hpp
 * @author Jose Cappelletto (j.cappelletto@soton.ac.uk)
 * @brief Core and extended enumerated constants for <lad> namespace
 * @version 0.1
 * @date 2020-07-03
 * @details Separate file for <lad> namespace enumerations. This is intended to
 * maintain clean <lad> definition in the core file (lad_core), while
 * retaining expansion capability in additional modules.
 * The enumerations here listed are those required for the core 
 * functionalities
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef _LAD_ENUM_HPP_
#define _LAD_ENUM_HPP_

#define SENSOR_RANGE 0.1

#include "headers.h"

namespace lad{

    const std::string DEFAULT_LAYER_NAME = "default_layer_name"; //!< Employed as fallback name during construction time of a new Layer

    /**
     * @brief General constant codes
     * 
     */
    enum ConstantCodes{
        DEFAULT_STACK_SIZE  = 100,   //!< Initial Layer vector size (stack or map). Deprecated
        ID_AVAILABLE        = 0,     //!< Flag to indicate ID is available in the LUT
        ID_TAKEN            = 1,      //!< Flag to indicate ID is available in the LUT
        DEFAULT_NODATA_VALUE= -9999 //!< Default value for NODATA field in raster layers
    };

    /**
     * @brief General function return codes
     * 
     */
    enum ReturnCodes{
        NO_ERROR                = 0,    //!< No error code to report
        ERROR_MISSING_ARGUMENT  =-1,    //!< Missing argument when calling function
        ERROR_WRONG_ARGUMENT    =-2,    //!< Incorrect type of argument
        ERROR_GDAL_FAILOPEN     =-3,    //!< GDAL Driver failed to open geoTIFF file
        ERROR_GEOTIFF_EMPTY     =-4,    //!< Provided geoTIFF file is empty
        ERROR_LAYERS_EMPTY      =-5,    //!< No Layer is present in the current stack
        ERROR_CONTOURS_NOTFOUND =-6     //!< No contour could be found
    };

    /**
     * @brief Levels of verbosity used inside of Pipeline object
     * 
     */
    enum VerbosityLevels{
        NO_VERBOSE      = 0,    //!< No verbose output (default)
        VERBOSITY_0     = 0,    //!< Minimum verbose level, equivalent to NO_VERBOSE
        VERBOSITY_1     = 1,    //!< Low verbose level, show summary information.
        VERBOSITY_2     = 2     //!< High verbose level, show detailed information.
    };

    /**
     * @brief Possible return codes for functions processing geoTIFF files
     * 
     */
    enum TiffProcessing{
        TIFF_FILE_INVALID =-1, //!< Invalid TIFF image file
        TIFF_FILE_EMPTY   =-2  //!< Valid TIFF image metadata, with empty raster band
    };

    /**
     * @brief Layer type identifier, used in Layer base and derived classes
     * 
     */
    enum LayerTypes{
        LAYER_ANYTYPE   = 0,//!< Indicate any type off layer (user for layer queries)
        LAYER_UNDEFINED =-1,//!< Flags this layer type as undefined. Default Type when constructing
        LAYER_RASTER = 1,   //!< Layer can contain raster data (RasterLayer)
        LAYER_VECTOR = 2,   //!< Layer can contain vectorized data (typ std::vector)
        LAYER_KERNEL = 3    //!< Layer can contain raster description of a filter kernel (KernelLayer)
    };

    /**
     * @brief Possible Layer status flags used in the Pipeline
     * 
     */
    enum LayerStatus{
        LAYER_INVALID = -1, //!< Layer is currently flagged as INVALID. Default Status when constructing
        LAYER_EMPTY   =  0, //!< Layer content is empty, typically when it has been recently  cleared()
        LAYER_VALID   =  1, //!< Layer contains valid non-empty data, regardless its type
    };

    /**
     * @brief Defines if the coordinates of vector or raster layers are dimensionless, in pixel coordinates, or in world coordinates
     * 
     */
    enum CoordinateSpace{
        NO_COORDINATE    =-1, //!< Coordinate system has not been specified or it is not required for this layer
        PIXEL_COORDINATE = 0, //!< The Layer coordinates are in pixel space (OpenCV image equivalent)
        WORLD_COORDINATE = 1  //!< The Layer coordinates are in world space (e.g. LAT/LON)
    };

    /**
     * @brief List of supported file formats for exporting Layer content
     * 
     */
    enum ExportFormat{
        EXPORT_OK   = 0,    //!< No error exporting file
        FMT_CSV     = 1,    //!< Export data as CSV file using built-in engine
        FMT_SHP     = 2,    //!< Export as ESRI Shapefile using GDAL
        FMT_TIFF    = 3,    //!< Export as geoTIFF using GDAL
        FMT_PNG     = 4     //!< Export as flat PNG image
    };

    /**
     * @brief Error return codes for functions processing layers (e.g. Pipeline steps)
     * 
     */
    enum LayerError{
        LAYER_OK                = 0, //!< Layer operation completed succesfully
        LAYER_NONE              =-1, //!< Layers <vector> is empty
        LAYER_INVALID_ID        =-2, //!< User provided ID is not valid. It must be positive integer
        LAYER_DUPLICATED_ID     =-3, //!< Provided Layer ID already taken
        LAYER_NOT_FOUND         =-4, //!< Provided Layer ID not found
        LAYER_DUPLICATED_NAME   =-5, //!< Provided new name for Layer already exists
        LAYER_INVALID_NAME      =-6, //!< Provided new name for Layer is invalid. It must be a non-NULL std::string
    };

    /**
     * @brief Stats type codes, used when generating layer specific stats
     * 
     */
    enum LayerStats{
        LAYER_MIN = 0,  //!< Minimum value of valid raster data
        LAYER_MAX = 1,  //!< Maximum value of valid raster data
        LAYER_MEAN = 2, //!< Average value of valid raster data
        LAYER_STDEV = 3,//!< Stadard deviation valid raster data
    };

    /**
     * @brief List of available filter options when using applyWindowFilter()
     * 
     */
    enum FilterType{
        FILTER_MEAN     = 0, //!< Computes the mean value of the masked points in the sliding window
        FILTER_SLOPE    = 1, //!< Computes the slope of the plane that fits the points in the sliding window
        FILTER_DISTANCE = 2, //!< Computes the normal distance between a point cloud and its fitting plane
        FILTER_GEOTECH  = 3, //!< Computes the normal distance between a pointclod and the true-landing plane within a circular window
    };
};

#endif // _LAD_ENUM_HPP_ guard
