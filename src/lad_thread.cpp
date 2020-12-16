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
#include "helper.h"

int lad::processRotationWorker (lad::Pipeline *ap, parameterStruct *p){

    parameterStruct params = *p;    // local copy, to avoid accident
    // lad::Pipeline pipeline = *ap;
    ostringstream s;
    double currRotation = params.rotation;
    string suffix = "_r" + makeFixedLength((int) currRotation, 3);
    // #pragma omp critical
    // {
    s << "Creating KernelAUV" << suffix;
    logc.debug("prW", s);
    ap->createKernelTemplate("KernelAUV" + suffix, params.robotWidth, params.robotLength, cv::MORPH_RECT);
    auto ptrLayer = dynamic_pointer_cast<KernelLayer>(ap->getLayer("KernelAUV" + suffix));
    if (ptrLayer == nullptr){
        s << blue << "+++++++++++++++++++++++++++++++++++++++++++++ nullptr when dyncast KernelAUV" << suffix;
        logc.error("pRW", s); 
    }
    ptrLayer->setRotation(currRotation);
    // }
    // logc.info("processRotationWorker", s);

    std::thread threadLaneD (&lad::processLaneD, ap, &params, suffix);
    s << "Lane D dispatched for orientation [" << blue << currRotation << reset << "] degrees";
    logc.info("pRW", s);

    std::thread threadLaneC (&lad::processLaneC, ap, &params, suffix);
    s << "Lane C dispatched for orientation [" << blue << currRotation << reset << "] degrees";
    logc.info("pRW", s);

    std::thread threadLaneX (&lad::processLaneX, ap, &params, suffix);
    s << "Lane X dispatched for orientation [" << blue << currRotation << reset << "] degrees";
    logc.info("pRW", s);

    // #pragma omp sections
    // {
    //     #pragma omp section
    //     {
    //         ostringstream s;
    //         s << "Current orientation [" << blue << currRotation << reset << "] degrees";
    //         logc.info("laneD-worker", s);
    //         lad::processLaneD(ap, &params, suffix);
    //         logc.info("laneD-worker", "------------------> done");
    //     }

    //     #pragma omp section
    //     {
    //         ostringstream s;
    //         s << "Current orientation [" << blue << currRotation << reset << "] degrees";
    //         logc.info("laneC-worker", s);
    //         lad::processLaneC(ap, &params, suffix);
    //         logc.info("laneC-worker", "------------------> done");
    //     }

    //     #pragma omp section
    //     {
    //         ostringstream s;
    //         s << "Current orientation [" << blue << currRotation << reset << "] degrees" << endl;
    //         logc.info("laneX-worker", s);
    //         lad::processLaneX(ap, &params, suffix);
    //         logc.info("laneX-worker", "------------------> done");
    //     }

        // TODO: Add validation for copyemask when src is missing
        // logc.info("processRotationWorker", "Recomputing lanes C & D done");
        // logc.debug("processRotationWorker", "Computing M3_LandabilityMap");
        // Final map: M3 = C3_MeanSlope x D2_LoProtExl x D4_HiProtExcl (logical AND)

        // logc.debug("processRotationWorker", "Waiting for thread X");
        // logc.debug("processRotationWorker", "Waiting for thread X... done\ncomputeBlendMeasurability");
    // }
    threadLaneC.join();
    threadLaneD.join();
    s << "Lane C & D done for orientation [" << green << currRotation << reset << "] degrees";
    logc.info("pRW", s);
    ap->computeLandabilityMap ("C3_MeanSlopeExcl" + suffix, "D2_LoProtExcl" + suffix, "D4_HiProtExcl" + suffix, "M3_LandabilityMap" + suffix);
    ap->copyMask("C1_ExclusionMap","M3_LandabilityMap" + suffix);
    threadLaneX.join();
    s << "Lane X blending for orientation [" << green << currRotation << reset << "] degrees";
    logc.info("pRW", s);
    ap->computeBlendMeasurability("M3_LandabilityMap" + suffix, "X1_MeasurabilityMap" + suffix, "M4_FinalMeasurability" + suffix);

    // here we should ask if we need to export every intermediate layer (rotated)
    if (p->exportRotated){
        ap->saveImage("M3_LandabilityMap" + suffix, "M3_LandabilityMap" + suffix + ".png");
        ap->exportLayer("M3_LandabilityMap" + suffix, "M3_LandabilityMap" + suffix + ".tif", FMT_TIFF, WORLD_COORDINATE);
        ap->saveImage("M4_FinalMeasurability" + suffix, "M4_FinalMeasurability" + suffix + ".png");
        ap->exportLayer("M4_FinalMeasurability" + suffix, "M4_FinalMeasurability" + suffix + ".tif", FMT_TIFF, WORLD_COORDINATE);
    }    
    return NO_ERROR;

}

int lad::processRotationWorker (lad::Pipeline *ap, parameterStruct *p, std::string gsuffix){

    parameterStruct params = *p;    // local copy, to avoid accident
    // lad::Pipeline pipeline = *ap;

    int nRot = (params.rotationMax - params.rotationMin) / params.rotationStep;

    #pragma omp for nowait 
    for (int r=0; r<=nRot; r++){
        std::ostringstream s;
        double currRotation = params.rotationMin + r*params.rotationStep;
        s << "Current orientation [" << blue << currRotation << reset << "] degrees" << endl;
        logc.info("processRotationWorker", s);
        // params.rotation = currRotation;
        string suffix = "_r" + makeFixedLength((int) currRotation, 3);
        // s << "creating KernelAUV" << suffix;
        // logc.debug ("forLaneD", s);
        ap->createKernelTemplate("KernelAUV" + suffix, params.robotWidth, params.robotLength, cv::MORPH_RECT);
        dynamic_pointer_cast<KernelLayer>(ap->getLayer("KernelAUV" + suffix))->setRotation(currRotation);
        // compute the rotation dependent layers
        // C3_MeanSlopeExcl
        lad::processLaneD(ap, &params, suffix);
        // std::thread threadLaneD (&lad::processLaneD, ap, &params, suffix);
        //D2_LoProtExcl & D4_HiProtExcl
        // threadLaneD.join();
    }

    #pragma omp for nowait 
    for (int r=0; r<=nRot; r++){
        std::ostringstream s;
        double currRotation = params.rotationMin + r*params.rotationStep;
        s << "Current orientation [" << blue << currRotation << reset << "] degrees" << endl;
        logc.info("processRotationWorker", s);
        // params.rotation = currRotation;
        string suffix = "_r" + makeFixedLength((int) currRotation, 3);
        // ap->createKernelTemplate("KernelAUV" + suffix, params.robotWidth, params.robotLength, cv::MORPH_RECT);
        // dynamic_pointer_cast<KernelLayer>(ap->getLayer("KernelAUV" + suffix))->setRotation(currRotation);
        // compute the rotation dependent layers
        // C3_MeanSlopeExcl
        lad::processLaneC(ap, &params, suffix);
        // std::thread threadLaneC (&lad::processLaneC, ap, &params, suffix);
        // threadLaneC.join();
    }

    #pragma omp for nowait 
    for (int r=0; r<=nRot; r++){
        std::ostringstream s;
        double currRotation = params.rotationMin + r*params.rotationStep;
        s << "Current orientation [" << blue << currRotation << reset << "] degrees" << endl;
        logc.info("processRotationWorker", s);
        // params.rotation = currRotation;
        string suffix = "_r" + makeFixedLength((int) currRotation, 3);
        // ap->createKernelTemplate("KernelAUV" + suffix, params.robotWidth, params.robotLength, cv::MORPH_RECT);
        // dynamic_pointer_cast<KernelLayer>(ap->getLayer("KernelAUV" + suffix))->setRotation(currRotation);
        // X1_Measurability map
        lad::processLaneX(ap, &params, suffix);
        // std::thread threadLaneX (&lad::processLaneX, ap, &params, suffix);
        // threadLaneX.join();
    }

    #pragma omp for nowait 
    for (int r=0; r<=nRot; r++){
        double currRotation = params.rotationMin + r*params.rotationStep;
        // s << "Current orientation [" << blue << currRotation << reset << "] degrees" << endl;
        // logc.info("processRotationWorker", s);
        // params.rotation = currRotation;
        string suffix = "_r" + makeFixedLength((int) currRotation, 3);
        // TODO: Add validation for copyemask when src is missing
        logc.info("processRotationWorker", "Recomputing lanes C & D done");
        logc.debug("processRotationWorker", "Computing M3_LandabilityMap");
        // Final map: M3 = C3_MeanSlope x D2_LoProtExl x D4_HiProtExcl (logical AND)
        ap->computeLandabilityMap ("C3_MeanSlopeExcl" + suffix, "D2_LoProtExcl" + suffix, "D4_HiProtExcl" + suffix, "M3_LandabilityMap" + suffix);
        ap->copyMask("C1_ExclusionMap","M3_LandabilityMap" + suffix);

        logc.debug("processRotationWorker", "Waiting for thread X");
        logc.debug("processRotationWorker", "Waiting for thread X... done\ncomputeBlendMeasurability");
        ap->computeBlendMeasurability("M3_LandabilityMap" + suffix, "X1_MeasurabilityMap" + suffix, "M4_FinalMeasurability" + suffix);

        // here we should ask if we need to export every intermediate layer (rotated)
        if (p->exportRotated){
            ap->saveImage("M3_LandabilityMap" + suffix, "M3_LandabilityMap" + suffix + ".png");
            ap->exportLayer("M3_LandabilityMap" + suffix, "M3_LandabilityMap" + suffix + ".tif", FMT_TIFF, WORLD_COORDINATE);
            ap->saveImage("M4_FinalMeasurability" + suffix, "M4_FinalMeasurability" + suffix + ".png");
            ap->exportLayer("M4_FinalMeasurability" + suffix, "M4_FinalMeasurability" + suffix + ".tif", FMT_TIFF, WORLD_COORDINATE);
        }
    }
    
    return NO_ERROR;

}

int lad::processLaneX(lad::Pipeline *ap, parameterStruct *p, std::string suffix){

    lad::tictac tt;
    tt.start();
    ostringstream s;
    // we create an unique name using the rotation angle
    // s << "computeMeasurability -> X1_MeasurabilityMap for " << blue << suffix; 
    // logc.debug("laneX", s);
    ap->computeMeasurabilityMap("M1_RAW_Bathymetry", "KernelAUV" + suffix, "M1_VALID_DataMask", "X1_MeasurabilityMap" + suffix);
    // ap->showImage("C2_MeanSlopeMap");
    if (p->exportRotated){
        ap->saveImage("X1_MeasurabilityMap" + suffix, "X1_MeasurabilityMap" + suffix + ".png");
        ap->exportLayer("X1_MeasurabilityMap" + suffix, "X1_MeasurabilityMap" + suffix + ".tif", FMT_TIFF, WORLD_COORDINATE);
    }
    s << "processLaneX for suffix: [" << blue << suffix << reset << "]";
    logc.debug("laneX", s);
    tt.lap("\tLane X: X1_Measurability");
    return 0;
}

int lad::processLaneD(lad::Pipeline *ap, parameterStruct *p, std::string suffix){

    lad::tictac tt;
    tt.start();
    ostringstream s;

    auto apSrc  = dynamic_pointer_cast<RasterLayer> (ap->getLayer("M2_Protrusions"));
    //first step is to create the LoProt map (h < hcrit)
    ap->compareLayer("M2_Protrusions", "D3_HiProtMask" + suffix, p->heightThreshold, cv::CMP_GE);
    ap->compareLayer("M2_Protrusions", "D1_tempLO" + suffix, p->heightThreshold, cv::CMP_LT);
    // the LoProt must be masked against the valid protrusion mask: 
    // create a mask to remove those below h_ground
    ap->compareLayer("M2_Protrusions", "D1_tempGR" + suffix, p->groundThreshold, CMP_GE);
    // need a logical op: D1_tempLT AND D1_tempGT
    ap->maskLayer("D1_tempLO" + suffix, "D1_tempGR" + suffix, "D1_LoProtMask" + suffix);
    // s << "lpElev try for " << suffix;
    // logc.debug("laneD", s);
    ap->maskLayer("M2_Protrusions", "D1_LoProtMask" + suffix, "D1_LoProtElev" + suffix);
    // s << "lpElev done for " << suffix;
    // logc.debug("laneD", s);
    ap->removeLayer("D1_tempGR" + suffix);
    ap->removeLayer("D1_tempLO" + suffix);

    // Final step, iterate through different LO obstacle heights and compute correpsonding exlusion area (disk) 
    // we need a vector containing the partitioned thresholds/disk size pairs.
    // Starting from ground_threshold (lowest) to height_threshold (highest) LoProt elevation value
    auto apElev = dynamic_pointer_cast<RasterLayer> (ap->getLayer("D1_LoProtElev" + suffix));
    if (apElev == nullptr){
        s << "Nullptr when casting for " << "D1_LoProtElev" << suffix;
        logc.error("laneD", s);
    }
    cv::Mat D3_Excl = cv::Mat::zeros(apElev->rasterData.size(), CV_8UC1); // empty mask
    cv::Mat D3_layers[LO_NPART], temp;
    int diskSize[LO_NPART];
    double sx = fabs(ap->geoTransform[1]);  // we need the pixel scale to generate scale-aware structuring element
    double sy = fabs(ap->geoTransform[5]);
    for (int i=0; i<LO_NPART; i++){// 5 partitions default, it can be any positive integer value (too fine, and it won't make any difference)
        double h = (p->heightThreshold - p->groundThreshold)*(i+1)/LO_NPART + p->groundThreshold; // (i+1) for conservative approximation (obstacle height range rounded-up) 
        // double h = (p->heightThreshold - p->groundThreshold)*(i)/LO_NPART + p->groundThreshold;
        double e = computeExclusionSize(2*h); // fitted curve that estimate the disk size (radius) according to the obstacle height
        diskSize[i] = 2*round(e/sx); // diameter instead of radius
        cv::compare(apElev->rasterData, h, D3_layers[i], CMP_GE);
    }
    // we filter (remove) small protrusion clusters
   // warning: filter size cannot be zero (ceiling to 1)
    cv::Mat open_disk = cv::getStructuringElement(MORPH_ELLIPSE, cv::Size(ceil(p->protrusionSize/sx), ceil(p->protrusionSize/sy)));
    for (int i=0; i<LO_NPART-1; i++){
        temp = D3_layers[i] - D3_layers[i+1];
        cv::morphologyEx(temp, temp, MORPH_OPEN, open_disk); //remove the small protrusions
        cv::Mat dilate_disk = cv::getStructuringElement(MORPH_ELLIPSE, cv::Size(diskSize[i], diskSize[i])); // disk e(h) 
        cv::morphologyEx(temp, D3_layers[i], MORPH_DILATE, dilate_disk); //dilate e(h) disk
        // Final step: blend into the final exclusion map
        D3_Excl = D3_Excl | D3_layers[i];
    }

    // s << "Creating D2_LoProtExcl " << suffix;
    // logc.debug("laneD", s);

    ap->createLayer("D2_LoProtExcl" + suffix, LAYER_RASTER);
    auto apLoProtExcl  = dynamic_pointer_cast<RasterLayer> (ap->getLayer("D2_LoProtExcl" + suffix));
    if (apLoProtExcl == nullptr)
    {
        s << "Failed to retrieve RasterLayer: D2_LoProtExcl" << suffix;
        logc.error("laneD", s);
    }
    D3_Excl.copyTo(apLoProtExcl->rasterData); // transfer the data, now the config & georef
    // TODO: use Pipeline method uploadData
    apLoProtExcl->setNoDataValue(DEFAULT_NODATA_VALUE);
    apLoProtExcl->copyGeoProperties(apSrc);

    // s << "Created D2_LoProtExcl " << suffix;
    // logc.debug("laneD", s);

    //*******************************
    auto apLoProt  = dynamic_pointer_cast<RasterLayer> (ap->getLayer("D1_LoProtMask" + suffix));
    auto apHiProt  = dynamic_pointer_cast<RasterLayer> (ap->getLayer("D3_HiProtMask" + suffix));
    auto auvKernel = dynamic_pointer_cast<KernelLayer> (ap->getLayer("KernelAUV" + suffix));
    // now, we create the Exclusion map, for the current vehicle heading (stored in KernelAUV)
    cv::Mat excl(apHiProt->rasterData.size(), CV_8UC1); //same size and type as original mask
    cv::dilate(apHiProt->rasterData, excl, auvKernel->rotatedData);
    ap->createLayer("D4_HiProtExcl" + suffix, LAYER_RASTER);

    // s << "Created D4_HiProtExcl " << suffix;
    // logc.debug("laneD", s);

    auto apHiProtExcl  = dynamic_pointer_cast<RasterLayer> (ap->getLayer("D4_HiProtExcl" + suffix));
    // construction time upload method?
    excl.copyTo(apHiProtExcl->rasterData); // transfer the data, now the config & georef
    apHiProtExcl->setNoDataValue(DEFAULT_NODATA_VALUE);
    apHiProtExcl->copyGeoProperties(apSrc);
    apHiProt->copyGeoProperties(apSrc);
    apLoProt->copyGeoProperties(apSrc);

    // ap->copyMask("C1_ExclusionMap", "D1_LoProtMask");
    // ap->saveImage("D1_LoProtMask" + suffix, "D1_LoProtMask"  + suffix + ".png");
    // ap->exportLayer("D1_LoProtMask" + suffix, "D1_LoProtMask" + suffix + ".tif", FMT_TIFF, WORLD_COORDINATE);

    // ap->copyMask("C1_ExclusionMap", "D2_LoProtExcl");
    // ap->saveImage("D2_LoProtExcl" + suffix, "D2_LoProtExcl" + suffix + ".png");
    // ap->exportLayer("D2_LoProtExcl" + suffix, "D2_LoProtExcl" + suffix + ".tif", FMT_TIFF, WORLD_COORDINATE);

    // ap->copyMask("C1_ExclusionMap", "D1_LoProtElev");
    // ap->saveImage("D1_LoProtElev" + suffix, "D1_LoProtElev" + suffix + ".png");
    // ap->exportLayer("D1_LoProtElev" + suffix, "D1_LoProtElev" + suffix + ".tif", FMT_TIFF, WORLD_COORDINATE);

    // ap->copyMask("C1_ExclusionMap", "D3_HiProtMask" + suffix);
    // ap->saveImage("D3_HiProtMask" + suffix, "D3_HiProtMask" + suffix + ".png");
    // ap->exportLayer("D3_HiProtMask" + suffix, "D3_HiProtMask" + suffix + ".tif", FMT_TIFF, WORLD_COORDINATE);
    s << "Lane D for " << blue << suffix << reset << " completed";
    logc.debug("laneD", s);

    ap->copyMask("C1_ExclusionMap", "D4_HiProtExcl" + suffix);
    if (p->exportRotated){
        ap->saveImage("D4_HiProtExcl" + suffix, "D4_HiProtExcl" + suffix + ".png");
        ap->exportLayer("D4_HiProtExcl" + suffix, "D4_HiProtExcl" + suffix + ".tif", FMT_TIFF, WORLD_COORDINATE);
    }
    tt.lap("\tLane D: D1_LoProt, D3_HiProt, D3_HiProtExcl");
    return 0;
}

int lad::processLaneC(lad::Pipeline *ap, parameterStruct *p, std::string suffix){

    lad::tictac tt;
    tt.start();
    ostringstream s;
    // s << "computeMeanSlopeMap -> C2_MeanSlopeMap for " << blue << suffix;
    // logc.debug("laneC", s);
    // we create an unique name using the rotation angle
    ap->computeMeanSlopeMap("M1_RAW_Bathymetry", "KernelAUV" + suffix, "M1_VALID_DataMask", "C2_MeanSlopeMap" + suffix);
    // ap->showImage("C2_MeanSlopeMap");
    if (p->exportRotated){
        ap->saveImage("C2_MeanSlopeMap" + suffix, "C2_MeanSlopeMap" + suffix + ".png");
        ap->exportLayer("C2_MeanSlopeMap" + suffix, "C2_MeanSlopeMap" + suffix + ".tif", FMT_TIFF, WORLD_COORDINATE);
    }
    // logc.debug("laneC", "compareLayer -> C2_MeanSlopeMapExcl");
    ap->compareLayer("C2_MeanSlopeMap" + suffix, "C3_MeanSlopeExcl" + suffix, p->slopeThreshold, CMP_GT);
    // ap->showImage("C3_MeanSlopeExcl");
    if (p->exportRotated){
        ap->saveImage("C3_MeanSlopeExcl" + suffix, "C3_MeanSlopeExcl" + suffix + ".png");
        ap->exportLayer("C3_MeanSlopeExcl" + suffix, "C3_MeanSlopeExcl" + suffix + ".tif", FMT_TIFF, WORLD_COORDINATE);
    }
    // tt.lap("Lane C: C2_MeanSlopeMap");
    // logc.debug("laneC", "computeMeasurability -> X1_MeasurabilityMap");
    ap->computeMeasurabilityMap("M1_RAW_Bathymetry", "KernelAUV" + suffix, "M1_VALID_DataMask", "X1_MeasurabilityMap" + suffix);
    // ap->showImage("C2_MeanSlopeMap");
    if (p->exportRotated){
        ap->saveImage("X1_MeasurabilityMap" + suffix, "X1_MeasurabilityMap" + suffix + ".png");
        ap->exportLayer("X1_MeasurabilityMap" + suffix, "X1_MeasurabilityMap" + suffix + ".tif", FMT_TIFF, WORLD_COORDINATE);
    }
    s << "processLaneC for suffix: [" << blue << suffix << reset << "]";
    logc.debug("laneC", s);

    tt.lap("\tLane C: C2_MeanSlope, C3_MeanSlopeMapExcl, X1_Measurability");
    return 0;
}

int lad::processLaneB(lad::Pipeline *ap, parameterStruct *p, std::string affix){
    lad::tictac tt;
    tt.start();
    ap->lowpassFilter ("M1_RAW_Bathymetry", "KernelDiag", "M1_VALID_DataMask", "B0_FILT_Bathymetry");
    // ap->showImage("B0_FILT_Bathymetry", COLORMAP_JET);
    if (p->exportIntermediate){
        ap->saveImage("B0_FILT_Bathymetry", affix + "B0_FILT_Bathymetry.png", COLORMAP_JET);
        ap->exportLayer("B0_FILT_Bathymetry", affix + "B0_FILT_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
    }
    // tt.lap("Lane B: BO_FILT_Bathymetry");

    ap->computeHeight("M1_RAW_Bathymetry", "B0_FILT_Bathymetry", "B1_HEIGHT_Bathymetry");
    // ap->showImage("B1_HEIGHT_Bathymetry", COLORMAP_TWILIGHT_SHIFTED);
    ap->copyMask("M1_RAW_Bathymetry", "B1_HEIGHT_Bathymetry");
    if (p->exportIntermediate){
        ap->saveImage("B1_HEIGHT_Bathymetry", "B1_HEIGHT_Bathymetry.png", COLORMAP_TWILIGHT_SHIFTED);
        ap->exportLayer("B1_HEIGHT_Bathymetry", "B1_HEIGHT_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
    }
    tt.lap("\tLane B: BO_FILT_Bathymetry, B1_HEIGHT_Bathymetry");
    return 0;
}

int lad::processLaneA(lad::Pipeline *ap, parameterStruct *p, std::string affix){
    lad::tictac tt;
    tt.start();
    // we are using affix as prefix 
    ap->computeMeanSlopeMap("M1_RAW_Bathymetry", "KernelSlope", "M1_VALID_DataMask", "A1_DetailedSlope");
    // ap->showImage("A1_DetailedSlope",COLORMAP_JET);
    if (p->exportIntermediate){
        ap->saveImage("A1_DetailedSlope", affix + "A1_DetailedSlope.png", COLORMAP_JET);
        ap->exportLayer("A1_DetailedSlope", affix + "A1_DetailedSlope.tif", FMT_TIFF, WORLD_COORDINATE);
    }
    // tt.lap("Lane A: A1_DetailedSlope");

    ap->compareLayer("A1_DetailedSlope", "A2_HiSlopeExcl", p->slopeThreshold, CMP_GT);
    // ap->showImage("A2_HiSlopeExcl",COLORMAP_JET);
    // ap->saveImage("A2_HiSlopeExcl", "A2_HiSlopeExcl.png", COLORMAP_JET);
    if (p->exportIntermediate){
        ap->exportLayer("A2_HiSlopeExcl", affix + "A2_HiSlopeExcl.tif", FMT_TIFF, WORLD_COORDINATE);
    }
    tt.lap("\tLane A: A1_DetailedSlope, A2_HiSlopeExcl");
    return 0;
}
