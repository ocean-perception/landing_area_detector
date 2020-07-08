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

namespace lad{
    enum ConstantCodes{
        DEFAULT_STACK_SIZE  = 100,   //!< Initial Layers vector size (stack)
        ID_AVAILABLE        = 0,     //!< Flag to indicate ID is available in the LUT
        ID_TAKEN            = 1      //!< Flag to indicate ID is available in the LUT
    };

    enum ReturnCodes{
        NO_ERROR                = 0,
        ERROR_MISSING_ARGUMENT  =-1,
        ERROR_WRONG_ARGUMENT    =-2,
        ERROR_GDAL_FAILOPEN     =-3
    };

    enum TiffProcessing{
        TIFF_FILE_INVALID =-1, //!< Invalid TIFF image file
        TIFF_FILE_EMPTY   =-2  //!< Valid TIFF image metadata, with empty raster band
    };

    enum LayerTypes{
        LAYER_ANYTYPE   =-1,//!< Indicate any type off layer (user for layer queries)
        LAYER_UNDEFINED = 0,//!< Flags this layer type as undefined. Default Type when constructing
        LAYER_RASTER = 1,   //!< Layer can contain raster data
        LAYER_VECTOR = 2,   //!< Layer can contain vectorized data (typ std::vector)
        LAYER_KERNEL = 3    //!< Layer can contain raster description of a filter kernel (e.g. vehicle footprint)
    };

    enum LayerStatus{
        LAYER_INVALID = -1, //!< Layer is currently flagged as INVALID. Default Status when constructing
        LAYER_EMPTY   =  0, //!< Layer content is empty, typically when it has been recently  cleared()
        LAYER_VALID   =  1, //!< Layer contains valid non-empty data, regardless its type
    };

    enum LayerError{
        LAYER_OK                = 0, //!< Layer operation completed succesfully
        LAYER_NONE              =-1, //!< Layers <vector> is empty
        LAYER_INVALID_ID        =-2, //!< User provided ID is not valid. It must be positive integer
        LAYER_DUPLICATED_ID     =-3, //!< Provided Layer ID already taken
        LAYER_NOT_FOUND         =-4, //!< Provided Layer ID not found
        LAYER_DUPLICATED_NAME   =-5, //!< Provided new name for Layer already exists
        LAYER_INVALID_NAME      =-6, //!< Provided new name for Layer is invalid. It must be a non-NULL std::string
    };
};

#endif // _LAD_ENUM_HPP_ guard
