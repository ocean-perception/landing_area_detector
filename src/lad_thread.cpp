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
    ap->compareLayer("M2_Protrusions", "D1_tempLO", p->heightThreshold, cv::CMP_LT);
    // the LoProt must be masked against the valid protrusion mask: 
    // create a mask to remove those below h_ground
    ap->compareLayer("M2_Protrusions", "D1_tempGR", p->groundThreshold, CMP_GE);
    // need a logical op: D1_tempLT AND D1_tempGT
    ap->maskLayer("D1_tempLO", "D1_tempGR", "D1_LoProtMask");
    ap->maskLayer("M2_Protrusions", "D1_LoProtMask", "D1_LoProtElev");
    ap->removeLayer("D1_tempGR");
    ap->removeLayer("D1_tempLO");

    // Final step, iterate through different LO obstacle heights and compute correpsonding exlusion area (disk) 
    // we need a vector containing the partitioned thresholds/disk size pairs.
    // Starting from ground_threshold (lowest) to height_threshold (highest) LoProt elevation value
    auto apElev = dynamic_pointer_cast<RasterLayer> (ap->getLayer("D1_LoProtElev"));
    cv::Mat D3_Excl = cv::Mat::zeros(apElev->rasterData.size(), CV_8UC1); // empty mask
    cv::Mat D3_layers[LO_NPART], temp;
    int diskSize[LO_NPART];
    double sx = fabs(ap->geoTransform[1]);
    double sy = fabs(ap->geoTransform[5]);
    for (int i=0; i<LO_NPART; i++){// 5 partitions
        double h = (p->heightThreshold - p->groundThreshold)*i/LO_NPART + p->groundThreshold;
        double e = computeExclusionSize(h);
        diskSize[i] = 2*round(e/sx);
        cv::compare(apElev->rasterData, h, D3_layers[i], CMP_GE);
    }
    // we filter (remove) small protrusion clusters
    cv::Mat open_disk = cv::getStructuringElement(MORPH_ELLIPSE, cv::Size(p->protrusionSize/sx, p->protrusionSize/sy));
    for (int i=0; i<LO_NPART-1; i++){
        temp = D3_layers[i] - D3_layers[i+1];
        cv::morphologyEx(temp, temp, MORPH_OPEN, open_disk);
        cv::Mat dilate_disk = cv::getStructuringElement(MORPH_ELLIPSE, cv::Size(diskSize[i], diskSize[i]));
        cv::morphologyEx(temp, D3_layers[i], MORPH_DILATE, dilate_disk);
        // Final step: blend into the final exclusion map
        D3_Excl = D3_Excl | D3_layers[i];
    }

    ap->createLayer("D2_LoProtExcl", LAYER_RASTER);
    auto apLoProtExcl  = dynamic_pointer_cast<RasterLayer> (ap->getLayer("D2_LoProtExcl"));
    D3_Excl.copyTo(apLoProtExcl->rasterData); // transfer the data, now the config & georef
    apLoProtExcl->setNoDataValue(DEFAULT_NODATA_VALUE);
    apLoProtExcl->copyGeoProperties(apSrc);

    //*******************************
    auto apLoProt  = dynamic_pointer_cast<RasterLayer> (ap->getLayer("D1_LoProtMask"));
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

    ap->saveImage("D1_LoProtMask", "D1_LoProtMask.png");
    ap->exportLayer("D1_LoProtMask", "D1_LoProtMask.tif", FMT_TIFF, WORLD_COORDINATE);
    ap->saveImage("D2_LoProtExcl", "D2_LoProtExcl.png");
    ap->exportLayer("D2_LoProtExcl", "D2_LoProtExcl.tif", FMT_TIFF, WORLD_COORDINATE);
    ap->saveImage("D1_LoProtElev", "D1_LoProtElev.png");
    ap->exportLayer("D1_LoProtElev", "D1_LoProtElev.tif", FMT_TIFF, WORLD_COORDINATE);
    ap->saveImage("D3_HiProtMask", "D3_HiProtMask.png");
    ap->exportLayer("D3_HiProtMask", "D3_HiProtMask.tif", FMT_TIFF, WORLD_COORDINATE);
    ap->saveImage("D4_HiProtExcl", "D4_HiProtExcl.png");
    ap->exportLayer("D4_HiProtExcl", "D4_HiProtExcl.tif", FMT_TIFF, WORLD_COORDINATE);

    tt.lap("\tLane D: D1_LoProt, D3_HiProt, D3_HiProtExcl");

    // now, the final step is to comput the D1_LoProt exclusion using a disk(ei) as structuring element
    // the radius of the disk (ei) depends on the obstacle height
    // all the obstacles below ground_threshold must be neglected (Mehul's implementation consider them as noise, but the threshold doesn't match the reported one) 
    // typ threshold: 2cm
    // the target set must contain points where h_ground < h_i < h_crit --> D1_LoProt_Heights
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
    // tt.lap("Lane C: C2_MeanSlopeMap");

    ap->compareLayer("C2_MeanSlopeMap", "C3_MeanSlopeExcl", p->slopeThreshold, CMP_GT);
    // ap->showImage("C3_MeanSlopeExclusion");
    ap->saveImage("C3_MeanSlopeExcl", "C3_MeanSlopeExcl.png");
    ap->exportLayer("C3_MeanSlopeExcl", "C3_MeanSlopeExcl.tif", FMT_TIFF, WORLD_COORDINATE);
    tt.lap("\tLane C: C2_MeanSlope, C2_MeanSlopeMapExcl");
    return 0;
}

int lad::processLaneB(lad::Pipeline *ap, parameterStruct *p){
    lad::tictac tt;
    tt.start();
    ap->lowpassFilter ("M1_RAW_Bathymetry", "KernelDiag", "M1_VALID_DataMask", "B0_FILT_Bathymetry");
    // ap->showImage("B0_FILT_Bathymetry", COLORMAP_JET);
    ap->saveImage("B0_FILT_Bathymetry", "B0_FILT_Bathymetry.png", COLORMAP_JET);
    ap->exportLayer("B0_FILT_Bathymetry", "B0_FILT_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
    // tt.lap("Lane B: BO_FILT_Bathymetry");

    ap->computeHeight("M1_RAW_Bathymetry", "B0_FILT_Bathymetry", "B1_HEIGHT_Bathymetry");
    // ap->showImage("B1_HEIGHT_Bathymetry", COLORMAP_TWILIGHT_SHIFTED);
    ap->saveImage("B1_HEIGHT_Bathymetry", "B1_HEIGHT_Bathymetry.png", COLORMAP_TWILIGHT_SHIFTED);
    ap->exportLayer("B1_HEIGHT_Bathymetry", "B1_HEIGHT_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
    tt.lap("\tLane B: BO_FILT_Bathymetry, B1_HEIGHT_Bathymetry");
    return 0;
}

int lad::processLaneA(lad::Pipeline *ap, parameterStruct *p){
    lad::tictac tt;
    tt.start();
    ap->computeMeanSlopeMap("M1_RAW_Bathymetry", "KernelSlope", "M1_VALID_DataMask", "A1_DetailedSlope");
    // ap->showImage("A1_DetailedSlope",COLORMAP_JET);
    ap->saveImage("A1_DetailedSlope", "A1_DetailedSlope.png", COLORMAP_JET);
    ap->exportLayer("A1_DetailedSlope", "A1_DetailedSlope.tif", FMT_TIFF, WORLD_COORDINATE);
    // tt.lap("Lane A: A1_DetailedSlope");

    ap->compareLayer("A1_DetailedSlope", "A2_HiSlopeExcl", p->slopeThreshold, CMP_GT);
    // ap->showImage("A2_HiSlopeExcl",COLORMAP_JET);
    ap->saveImage("A2_HiSlopeExcl", "A2_HiSlopeExcl.png", COLORMAP_JET);
    ap->exportLayer("A2_HiSlopeExcl", "A2_HiSlopeExcl.tif", FMT_TIFF, WORLD_COORDINATE);
    tt.lap("\tLane A: A1_DetailedSlope, A2_HiSlopeExcl");
    return 0;
}
