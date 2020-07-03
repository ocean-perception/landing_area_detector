/********************************************************************/
/* Project: Landing Area Detection algorithm						*/
/* Module: 	lad_core												*/
/* File: 	lad_core.hpp                                            */
/* Created:		03/07/2020                                          */
/* Description
*/

/********************************************************************/
/* Created by:                                                      */
/* Jose Cappelletto - j.cappelletto@soton.ac.uk		                */
/********************************************************************/
// pragma once is not "standard"
#ifndef _LAD_CORE_HPP_ 
#define _LAD_CORE_HPP_

#include "geotiff.hpp"
#include "headers.h"

using namespace std;    // STL
using namespace cv;     // OpenCV

namespace lad{      //!< landing area detection algorithm namspace

    cv::Mat Layer;


}


#endif // _LAD_CORE_HPP_ 