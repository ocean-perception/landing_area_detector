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

int lad::processLaneD(lad::Pipeline *ap, parameterStruct *p){

    lad::tictac tt;
    tt.start();

    auto apSrc  = dynamic_pointer_cast<RasterLayer> (ap->getLayer("M2_Protrusions"));
    //first step is to create the LoProt map (h < hcrit)
    ap->compareLayer("M2_Protrusions", "D3_HiProtMask", p->heightThreshold, cv::CMP_GE);
    ap->compareLayer("M2_Protrusions", "D2_tempLO", p->heightThreshold, cv::CMP_LT);
    // the LoProt must be masked against the valid protrusion mask: 
    // create a mask to remove those below h_ground
    ap->compareLayer("M2_Protrusions", "D2_tempGR", p->groundThreshold, CMP_GE);
    //need a logical OP between D2_tempLT AND D2_tempGT
    ap->maskLayer("D2_tempLO", "D2_tempGR", "D2_LoProtMask");

    ap->showImage("D2_tempGR");
    ap->removeLayer("D2_tempGR");
    ap->removeLayer("D2_tempLO");

    // ap->showImage("D2_LoProtMask");

    auto apLoProt  = dynamic_pointer_cast<RasterLayer> (ap->getLayer("D2_LoProtMask"));
    auto apHiProt  = dynamic_pointer_cast<RasterLayer> (ap->getLayer("D3_HiProtMask"));
    auto auvKernel = dynamic_pointer_cast<KernelLayer> (ap->getLayer("KernelAUV"));
    // now, we create the Exclusion map, for the current vehicle heading (stored in KernelAUV)
    cv::Mat excl(apHiProt->rasterData.size(), CV_8UC1); //same size and type as original mask
    cv::dilate(apHiProt->rasterData, excl, auvKernel->rotatedData);
    ap->createLayer("D4_HiProtExcl", LAYER_RASTER);
    auto apHiProtExcl  = dynamic_pointer_cast<RasterLayer> (ap->getLayer("D4_HiProtExcl"));

    excl.copyTo(apHiProtExcl->rasterData); // transfer the data, now the config & georef
    apHiProtExcl->setNoDataValue(DEFAULT_NODATA_VALUE);
    apHiProtExcl->copyGeoProperties(apSrc);
    apHiProt->copyGeoProperties(apSrc);
    apLoProt->copyGeoProperties(apSrc);

    ap->saveImage("D2_LoProtMask", "D2_LoProtMask.png");
    ap->exportLayer("D2_LoProtMask", "D2_LoProtMask.tif", FMT_TIFF, WORLD_COORDINATE);
    ap->saveImage("D3_HiProtMask", "D3_HiProtMask.png");
    ap->exportLayer("D3_HiProtMask", "D3_HiProtMask.tif", FMT_TIFF, WORLD_COORDINATE);
    ap->saveImage("D4_HiProtExcl", "D4_HiProtExcl.png");
    ap->exportLayer("D4_HiProtExcl", "D4_HiProtExcl.tif", FMT_TIFF, WORLD_COORDINATE);

    tt.lap("Lane D: D2_LoProt, D3_HiProt");

    // now, the final step is to comput the D2_LoProt exclusion using a disk(ei) as structuring element
    // the radius of the disk (ei) depends on the obstacle height
    // all the obstacles below ground_threshold must be neglected (Mehul's implementation consider them as noise, but the threshold doesn't match the reported one) 
    // typ threshold: 2cm
    // the target set must contain points where h_ground < h_i < h_crit --> D2_LoProt_Heights
    // M2_Protrusions contains those where (slope > slope_crit)

    return 0;
}

int lad::processLaneC(lad::Pipeline *ap, parameterStruct *p){

    lad::tictac tt;
    tt.start();
    ap->computeMeanSlopeMap("M1_RAW_Bathymetry", "KernelAUV", "M1_VALID_DataMask", "C2_MeanSlopeMap");
    // ap->showImage("C2_MeanSlopeMap");
    ap->saveImage("C2_MeanSlopeMap", "C2_MeanSlopeMap.png");
    ap->exportLayer("C2_MeanSlopeMap", "C2_MeanSlopeMap.tif", FMT_TIFF, WORLD_COORDINATE);
    tt.lap("Lane C: C2_MeanSlopeMap");

    ap->compareLayer("C2_MeanSlopeMap", "C3_MeanSlopeExclusion", p->slopeThreshold, CMP_GT);
    // ap->showImage("C3_MeanSlopeExclusion");
    ap->saveImage("C3_MeanSlopeExclusion", "C3_MeanSlopeExclusion.png");
    ap->exportLayer("C3_MeanSlopeExclusion", "C3_MeanSlopeExclusion.tif", FMT_TIFF, WORLD_COORDINATE);
    tt.lap("Lane C: C2_MeanSlopeMapExclusion");
    return 0;
}

int lad::processLaneB(lad::Pipeline *ap, parameterStruct *p){
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

int lad::processLaneA(lad::Pipeline *ap, parameterStruct *p){
    lad::tictac tt;
    tt.start();
    ap->computeMeanSlopeMap("M1_RAW_Bathymetry", "KernelSlope", "M1_VALID_DataMask", "A1_DetailedSlope");
    // ap->showImage("A1_DetailedSlope",COLORMAP_JET);
    ap->saveImage("A1_DetailedSlope", "A1_DetailedSlope.png", COLORMAP_JET);
    ap->exportLayer("A1_DetailedSlope", "A1_DetailedSlope.tif", FMT_TIFF, WORLD_COORDINATE);
    tt.lap("Lane A: A1_DetailedSlope");

    ap->compareLayer("A1_DetailedSlope", "A2_HiSlopeExclusion", p->slopeThreshold, CMP_GT);
    // ap->showImage("A2_HiSlopeExclusion",COLORMAP_JET);
    ap->saveImage("A2_HiSlopeExclusion", "A2_HiSlopeExclusion.png", COLORMAP_JET);
    ap->exportLayer("A2_HiSlopeExclusion", "A2_HiSlopeExclusion.tif", FMT_TIFF, WORLD_COORDINATE);
    tt.lap("Lane A: A2_HiSlopeExclusion");
    return 0;
}
