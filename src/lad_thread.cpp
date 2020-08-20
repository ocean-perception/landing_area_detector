/**
 * @file lad_thread.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief  Landing Area Detection algorithm thread based pipeline decomposition
 * @version 0.1
 * @date 2020-09-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "lad_core.hpp"
#include "lad_thread.hpp"

int lad::processLaneC(lad::Pipeline *ap, int slopeThreshold){

    lad::tictac tt;
    tt.start();
    ap->computeMeanSlopeMap("M1_RAW_Bathymetry", "KernelAUV", "M1_VALID_DataMask", "C2_MeanSlopeMap");
    // ap->showImage("C2_MeanSlopeMap");
    ap->saveImage("C2_MeanSlopeMap", "C2_MeanSlopeMap.png");
    ap->exportLayer("C2_MeanSlopeMap", "C2_MeanSlopeMap.tif", FMT_TIFF, WORLD_COORDINATE);
    tt.lap("Lane C: C2_MeanSlopeMap");

    ap->compareLayer("C2_MeanSlopeMap", "C3_MeanSlopeExclusion", slopeThreshold, CMP_GT);
    // ap->showImage("C3_MeanSlopeExclusion");
    ap->saveImage("C3_MeanSlopeExclusion", "C3_MeanSlopeExclusion.png");
    ap->exportLayer("C3_MeanSlopeExclusion", "C3_MeanSlopeExclusion.tif", FMT_TIFF, WORLD_COORDINATE);
    tt.lap("Lane C: C2_MeanSlopeMapExclusion");
    return 0;
}

int lad::processLaneB(lad::Pipeline *ap){
    lad::tictac tt;
    tt.start();
    ap->lowpassFilter ("M1_RAW_Bathymetry", "KernelDiag", "M1_VALID_DataMask", "B0_FILT_Bathymetry");
    // ap->showImage("B0_FILT_Bathymetry", COLORMAP_JET);
    ap->saveImage("B0_FILT_Bathymetry", "B0_FILT_Bathymetry.png", COLORMAP_JET);
    ap->exportLayer("B0_FILT_Bathymetry", "B0_FILT_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
    tt.lap("Lane B: BO_FILT_Bathymetry");

    ap->computeHeight("M1_RAW_Bathymetry", "B0_FILT_Bathymetry", "B1_HEIGHT_Bathymetry");
    // ap->showImage("B1_HEIGHT_Bathymetry", COLORMAP_TWILIGHT_SHIFTED);
    ap->saveImage("B1_HEIGHT_Bathymetry", "B1_HEIGHT_Bathymetry.png", COLORMAP_TWILIGHT_SHIFTED);
    ap->exportLayer("B1_HEIGHT_Bathymetry", "B1_HEIGHT_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
    tt.lap("Lane B: B1_HEIGHT_Bathymetry");
    return 0;
}

int lad::processLaneA(lad::Pipeline *ap, int slopeThreshold){
    lad::tictac tt;
    tt.start();
    ap->computeMeanSlopeMap("M1_RAW_Bathymetry", "KernelSlope", "M1_VALID_DataMask", "A1_DetailedSlope");
    // ap->showImage("A1_DetailedSlope",COLORMAP_JET);
    ap->saveImage("A1_DetailedSlope", "A1_DetailedSlope.png", COLORMAP_JET);
    ap->exportLayer("A1_DetailedSlope", "A1_DetailedSlope.tif", FMT_TIFF, WORLD_COORDINATE);
    tt.lap("Lane A: A1_DetailedSlope");

    ap->compareLayer("A1_DetailedSlope", "A2_HiSlopeExclusion", slopeThreshold, CMP_GT);
    // ap->showImage("A2_HiSlopeExclusion",COLORMAP_JET);
    ap->saveImage("A2_HiSlopeExclusion", "A2_HiSlopeExclusion.png", COLORMAP_JET);
    ap->exportLayer("A2_HiSlopeExclusion", "A2_HiSlopeExclusion.tif", FMT_TIFF, WORLD_COORDINATE);
    tt.lap("Lane A: A2_HiSlopeExclusion");

    return 0;
}
