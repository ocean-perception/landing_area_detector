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

// pragma once is not "standard"
#ifndef _LAD_PROCESSING_CPP_
#define _LAD_PROCESSING_CPP_

#include "lad_processing.hpp"
#include "lad_layer.hpp"

/**
 * @brief Extend <lad> namespace with layer processing algorithms. Intended to be called by ladPipeline objects 
 * Valid data is assumed to be present in the layer containers involved
 */
namespace lad
{

    int convertDataSpace(vector<cv::Point2d> *inputData, vector<cv::Point2d> *outputData, int inputSpace, int outputSpace, double *apTransform)
    {
        if (inputSpace == outputSpace)
        { //!< No transformation was required
            // let's just clone the data
            cout << yellow << "[convertDataSpace] source and target coordinate space are the same. Will copy points without transformation" << reset << endl;
            for (auto it : *inputData)
            {
                outputData->push_back(it); //element wise deep-copy
            }
            return 0;
        }
        //!< Do we have a valid transformation matrix?
        if (apTransform == NULL)
        {
            cout << red << "[convertDataSpace] invalid transformation matrix received" << reset << endl;
            cout << red << "[convertDataSpace] no transformation performed" << reset << endl;
            return -1;
        }

        double sx = apTransform[1]; //!< Pixel size (X)
        double sy = apTransform[5]; //!< Pixel size (Y)
        double cx = apTransform[0]; //!< Center coordinate (X)
        double cy = apTransform[3]; //!< Center coordinate (Y)

    cv:
        Point2d p;
        // NOTE: Pixel coordinates are for the center of the pixel. Hence, the 0.5 adjustment
        switch (outputSpace)
        {
        case WORLD_COORDINATE:
            for (auto it : *inputData)
            {
                p.x = (it.x + 0.5) * sx + cx; //!< Scale & traslation 2D transformation
                p.y = (it.y + 0.5) * sy + cy; //!< No rotation implemented (yet)
                outputData->push_back(p);
            }
            break;

        case PIXEL_COORDINATE:
            for (auto it : *inputData)
            {
                p.x = -0.5 + (it.x - cx) / sx; //!< Scale & traslation 2D transformation
                p.y = -0.5 + (it.y - cy) / sy; //!< No rotation implemented (yet)
                outputData->push_back(p);
            }
            break;

        default:
            break;
        }

        return 0;
    }

} // namespace lad
#endif //_LAD_PROCESSING_CPP_