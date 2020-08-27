/**
 * @file helper.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-07-03
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef _PROJECT_HELPER_CPP_
#define _PROJECT_HELPER_CPP_

#include "headers.h"
#include "helper.h"
using namespace std;

/**
 * @brief Convert OpenCV data type into human readable format (e.g. CV32FC1)
 * 
 * @param type 
 * @return std::string 
 */
std::string type2str(int type) {
  string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}

std::string makeFixedLength(const int i, const int length)
{
    std::ostringstream ostr;

    if (i < 0)
        ostr << '-';

    ostr << std::setfill('0') << std::setw(length) << (i < 0 ? -i : i);

    return ostr.str();
}

#endif //_PROJECT_HELPER_CPP_