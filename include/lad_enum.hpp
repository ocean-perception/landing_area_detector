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
#define _LAD_ENUM_HPP_

namespace lad{
    enum ReturnCodes{
        NO_ERROR                = 0,
        ERROR_MISSING_ARGUMENT  = 1,
        ERROR_WRONG_ARGUMENT    = 2,
        ERROR_GDAL_FAILOPEN     = 3,
    };

    enum TiffProcessing{
        TIFF_FILE_INVALID = 1, //!< Invalid TIFF image file
        TIFF_FILE_EMPTY   = 2, //!< Valid TIFF image metadata, with empty raster band
    };
}

#endif // _LAD_ENUM_HPP_ guard