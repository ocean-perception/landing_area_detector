/**
 * @file mad_test.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief Measurability Area Detector, extended version of LAD test
 *        Sandbox module for testing core and extended functionalities and integration of Geotiff, OpenCV, CGAL & GDAL
 *        Part of PhD project on predicting landable areas for autonomous vehicles using remotely sensed data
 *        Ocean Perception Lab. University of Southampton, UK. 
 *        Visit: https://oceans.soton.ac.uk
 * @version 3.6-DualEnv [local + Iridis5]
 * @date 2021-11-18
 * 
 * @copyright Copyright (c) 2020-2021
 * 
 */
#include "headers.h"
#include "helper.h"
#include "options.h"
#include "geotiff.hpp"
#include "lad_core.hpp"
#include "lad_config.hpp"
#include "lad_analysis.h"
#include "lad_enum.hpp"
#include "lad_processing.hpp"
#include "lad_thread.hpp"

using namespace std;
using namespace cv;
using namespace lad;

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#ifdef USE_CUDA
  #include <opencv2/cudaarithm.hpp>
  #include <opencv2/cudafeatures2d.hpp>
#endif

logger::ConsoleOutput logc;

/*!
    @fn     int main(int argc, char* argv[])
    @brief  Main function
*/
int main(int argc, char *argv[])
{
    cout << cyan << "mad_test" << reset << endl;
    cout << "\tOpenCV version:\t" << yellow << CV_VERSION << reset << endl;
    cout << "\tGit commit:\t" << yellow << GIT_COMMIT << reset << endl;
    cout << "\tMode:\t\t" << yellow << CMAKE_BUILD_TYPE << reset << endl;
    // std::cout << cv::getBuildInformation() << std::endl;

    #ifdef USE_CUDA
        cout << blue << "\tCUDA enabled" << reset << endl;
        cv::cuda::printShortCudaDeviceInfo(cv::cuda::getDevice());
        int cuda_devices_number = cv::cuda::getCudaEnabledDeviceCount();
        cout << "CUDA Device(s) Number: "<< cuda_devices_number << endl;
        cv::cuda::DeviceInfo _deviceInfo;
        bool _isd_evice_compatible = _deviceInfo.isCompatible();
        cout << "CUDA Device(s) Compatible: " << _isd_evice_compatible << endl;
        cv::cuda::setDevice(0);
    #endif

    int retval = initParser(argc, argv);    // initial argument validation, populates arg parsing structure args
    if (retval != 0)                        // some error ocurred, we have been signaled to stop
        return retval;
    std::ostringstream s;
    // Parameters hierarchy: ARGS > CONFIG > DEFAULT (this)
    parameterStruct params = getDefaultParams(); // structure to hold configuration (populated with defaults).
    // They will be updated if config file or command line arguments are provided
    YAML::Node config;
    if (argConfig)     // check if config YAML file is provided
        config = lad::readConfiguration(args::get(argConfig), &params); // populates params structure with content of the YAML file
    // Input file priority: must be defined either by the config.yaml or --input argument
    string inputFileName    = ""; // command arg or config defined
    string outputFileName   = ""; // if empty, output filenames will be the same as the standard. If non-null, will be used as prefix
    string outputFilePath   = ""; // absolut/relative folder path were output will be stored

    if (argInput)   inputFileName    = args::get(argInput);   //input file is mandatory positional argument. Overrides any definition in configuration.yaml
    if (argOutput)  outputFileName   = args::get(argOutput);  //input file is mandatory positional argument. Overrides any definition in configuration.yaml
    if (argVerbose) params.verbosity = args::get(argVerbose); // retrieve user defined verbosity level

    if (inputFileName.empty()){ //not defined as command line argument? let's use config.yaml definition
        if (config["input"]["filename"])
            inputFileName = config["input"]["filename"].as<std::string>();
        else{ // ERROR! We do not have any definition of the input file
            logc.error ("main", "Input file missing. Please define it using --input='filename' or inside a YAML configuration file (see --config option)");
            return -1;
        }
    }
    // Now we proceed to optional parameters. When a variable is defined, we override the default value.
    float fParam = 1.0;
    if (argFloatParam) fParam = args::get(argFloatParam);
    int  iParam = 1;
    if (argIntParam)   iParam = args::get(argIntParam);
    int nThreads = DEFAULT_NTHREADS;
    if (argNThreads)   nThreads = args::get(argNThreads);
    if (nThreads < 3) {
        if (params.verbosity > VERBOSITY_0)
        {
            s << "Number of used threads will be always 3 or higher. Asked for [" << yellow << nThreads << reset << "]" << endl;
            logc.warn("main", s);
        }
    }
    
    // override defaults or config file with command provided values (DEFAULT < CONFIG < ARGUMENT)
    if (argAlphaRadius)     params.alphaShapeRadius = args::get(argAlphaRadius);
    if (argGroundThreshold) params.groundThreshold  = args::get(argGroundThreshold);
    if (argHeightThreshold) params.heightThreshold  = args::get(argHeightThreshold);
    if (argSlopeThreshold)  params.slopeThreshold   = args::get(argSlopeThreshold);
    if (argRobotHeight)     params.robotHeight      = args::get(argRobotHeight);
    if (argRobotLength)     params.robotLength      = args::get(argRobotLength);
    if (argRobotWidth)      params.robotWidth       = args::get(argRobotWidth);
    if (argProtrusionSize)  params.protrusionSize   = args::get(argProtrusionSize);
    if (argRotation){
        params.rotation         = args::get(argRotation);
        params.fixRotation      = true;
    }   
    if (argRotationStep){
        if (argRotation){
                logc.warn("main", "Fixed rotation parameter provided with variable rotation. Ignoring fixed rotation parameter");
            }
        params.rotationStep     = args::get(argRotationStep);
        params.fixRotation      = false;
    }
    
    if (argSlopeAlgorithm){
        auto option = args::get(argSlopeAlgorithm);
        if (option == "CONVEX"){
            params.slopeAlgorithm   = lad::FilterType::FILTER_CONVEX_SLOPE; 
            logc.warn("main-config", "Using CONVEX HULL algorithm for slope estimation");
        }               
        else if (option == "PLANE"){
            params.slopeAlgorithm   = lad::FilterType::FILTER_SLOPE;
            logc.warn("main-config", "Using HOUGH TRANSFORM algorithm for slope estimation");
        }
        else{
            logc.error("main-config", "Unknown slope estimation algorithm");
            return -1;
        }
    }
    else{
        params.slopeAlgorithm   = lad::FilterType::FILTER_SLOPE;                
        logc.warn("main-config", "Using LMS PLANE algorithm for slope estimation");               
    }

    if (argMetacenter)      params.ratioMeta        = args::get(argMetacenter);
    if (argSaveIntermediate)    params.exportIntermediate = args::get(argSaveIntermediate);
    // TODO: allow user defined drag coefficient
    if (params.updateThreshold){
        // let's recompute the slope and height thresholds according to the vehicle geometry
        if (params.verbosity > VERBOSITY_0)
            logc.warn("main", "Recomputing slope and height thresholds");
        double dm = params.robotHeight * params.ratioMeta;
        double dg = params.robotHeight * params.ratioCg;

        // approx vehicle volume to an ellipsoid with major axis: W, L, H
        double volume = (M_PI/6.0) * params.robotHeight * params.robotLength * params.robotWidth;
        double mass = volume * WATER_DENSITY;
        params.gravityForce = mass * GRAVITY;
        params.buoyancyForce = params.gravityForce * (1 - params.forceRatio); 
        double Fb = params.buoyancyForce; // buoyancy force, vol * density * gravity [N]
        double Fg = params.gravityForce; // gravity force, mass*gravity [N]
        double Fr = Fg - Fb; //net force [N] (positive down), should be equivalent to gravity_force * force_ratio

        // recompute slopeCritical (Mehul2019, Eq[2])
        params.slopeThreshold = atan((0.5*params.robotWidth*Fr)/((dm*Fb) - (dg*Fr)));
        // recompute hCritical (Mehul2019, Eq[9])
        params.heightThreshold = params.robotWidth * sin (params.slopeThreshold);
        params.slopeThreshold *= 180.0/M_PI;
    }   
    params.robotDiagonal = sqrt(params.robotWidth*params.robotWidth + params.robotLength*params.robotLength); 

    if (params.verbosity > 0){
        //**************************************************************************
        /* Summary list parameters */
        cout << yellow << "****** Summary **********************************" << reset << endl;
        cout << "Input file:   \t" << inputFileName << endl;
        cout << "Output name:  \t" << outputFileName << endl;
        cout << "Output path:  \t" << outputFilePath << endl;
        lad::printParams(&params);
        cout << "Verbose level:\t\t" << params.verbosity << endl;    
        cout << "Multithreaded version, max concurrent threads: [" << yellow << nThreads << reset << "]" << endl;
        cout << yellow << "*************************************************" << reset << endl << endl;
    }

    lad::tictac tt, tic;
    lad::Pipeline pipeline;    
    tic.start();
    tt.start();

    s << "Threshold slope: [" << yellow << params.slopeThreshold << reset << "]" << endl;
    logc.info("main", s);
    
    pipeline.parameters = params;   // forward config/user defined parameters to the internal pipeline structure
    pipeline.useNodataMask = true;  //params.useNoDataMask;
    pipeline.readTIFF(inputFileName, "M1_RAW_Bathymetry", "M1_VALID_DataMask");

    pipeline.setTemplate("M1_RAW_Bathymetry");  // M1 will be used as internal template for the pipeline
    pipeline.extractContours("M1_VALID_DataMask", "M1_CONTOUR_Mask", params.verbosity);
    if (argSaveIntermediate){
        pipeline.exportLayer("M1_RAW_Bathymetry", outputFileName + "M1_RAW_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
        pipeline.exportLayer("M1_CONTOUR_Mask", outputFileName + "M1_CONTOUR_Mask.shp", FMT_SHP, WORLD_COORDINATE);
    }
    pipeline.createKernelTemplate("KernelAUV",   params.robotWidth, params.robotLength, cv::MORPH_RECT); // vehicle footprint: rectangular
    pipeline.createKernelTemplate("KernelSlope", 0.1, 0.1, cv::MORPH_ELLIPSE);  // TODO: convert this into a size/resolution aware method
    pipeline.createKernelTemplate("KernelDiag",  params.robotDiagonal, params.robotDiagonal, cv::MORPH_ELLIPSE);
    dynamic_pointer_cast<KernelLayer>(pipeline.getLayer("KernelAUV"))->setRotation(params.rotation);

    pipeline.computeExclusionMap("M1_VALID_DataMask", "KernelAUV", "C1_ExclusionMap");
    if (argSaveIntermediate){
        pipeline.exportLayer("C1_ExclusionMap", outputFileName + "C1_ExclusionMap.tif", FMT_TIFF, WORLD_COORDINATE);
    }
    tt.lap("Load M1, C1");

    std::thread threadLaneA (&lad::processLaneA, &pipeline, &params, ""); //no suffix, nill-rotation sample
    std::thread threadLaneB (&lad::processLaneB, &pipeline, &params, "");

    threadLaneA.join();
    threadLaneB.join();

    if (argTerrainOnly){ // terrain-only maps. No vehicle-specific maps will be calculated, useful for quick diagnostics
        if (params.verbosity > 0){
            logc.debug("main", "Exporting terrain specifics only: Lanes A & B. Finishing ...");
            tt.lap("** Lanes A & B");
        }
        pipeline.saveImage("A1_DetailedSlope", outputFileName + "A1_DetailedSlope.png", COLORMAP_TWILIGHT_SHIFTED);
        pipeline.exportLayer("A1_DetailedSlope", outputFileName + "A1_DetailedSlope.tif", FMT_TIFF, WORLD_COORDINATE);
        pipeline.saveImage("B1_HEIGHT_Bathymetry", outputFileName + "B1_HEIGHT_Bathymetry.png", COLORMAP_TWILIGHT_SHIFTED);
        pipeline.exportLayer("B1_HEIGHT_Bathymetry", outputFileName + "B1_HEIGHT_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
        return NO_ERROR;
    }

    if (params.verbosity > 0){ 
        logc.debug("main", "Lanes A & B completed -> M2_Protrusions map done. Joining queue for Lane C & X");
        tt.lap("** Lanes A & B");
    }
    std::thread threadLaneC (&lad::processLaneC, &pipeline, &params, "");
    std::thread threadLaneX (&lad::processLaneX, &pipeline, &params, "");
    threadLaneC.join();
    threadLaneX.join();

    pipeline.maskLayer("B1_HEIGHT_Bathymetry", "A2_HiSlopeExcl", "M2_Protrusions");
    if (argSaveIntermediate){
        pipeline.saveImage("M2_Protrusions", outputFileName + "M2_Protrusions.png", COLORMAP_TWILIGHT_SHIFTED);
        pipeline.exportLayer("M2_Protrusions", outputFileName + "M2_Protrusions.tif", FMT_TIFF, WORLD_COORDINATE);
    }
    tt.lap("** Lanes C & X completed...");

    //now we proceed with final LoProt/HiProt exclusion calculation
    std::thread threadLaneD (&lad::processLaneD, &pipeline, &params, "");
    threadLaneD.join();

    if (params.exportIntermediate){
        pipeline.copyMask("C1_ExclusionMap", "D1_LoProtMask");
        pipeline.saveImage("D1_LoProtMask", outputFileName + "D1_LoProtMask.png");
        pipeline.exportLayer("D1_LoProtMask", outputFileName + "D1_LoProtMask.tif", FMT_TIFF, WORLD_COORDINATE);

        pipeline.copyMask("C1_ExclusionMap", "D2_LoProtExcl");
        pipeline.saveImage("D2_LoProtExcl", outputFileName + "D2_LoProtExcl.png");
        pipeline.exportLayer("D2_LoProtExcl", outputFileName + "D2_LoProtExcl.tif", FMT_TIFF, WORLD_COORDINATE);

        pipeline.copyMask("C1_ExclusionMap", outputFileName + "D1_LoProtElev");
        pipeline.saveImage("D1_LoProtElev", outputFileName + "D1_LoProtElev.png");
        pipeline.exportLayer("D1_LoProtElev", outputFileName + "D1_LoProtElev.tif", FMT_TIFF, WORLD_COORDINATE);
    
        pipeline.copyMask("C1_ExclusionMap", "D3_HiProtMask");
        pipeline.saveImage("D3_HiProtMask", outputFileName + "D3_HiProtMask.png");
        pipeline.exportLayer("D3_HiProtMask", outputFileName + "D3_HiProtMask.tif", FMT_TIFF, WORLD_COORDINATE);
    }
  
    // Final map: M3 = C3_MeanSlope x D2_LoProtExl x D4_HiProtExcl (logical AND)
    if (params.fixRotation == true){
        s << "Calculating maps for fixed rotation [" << blue << params.rotation << reset << "]";
        logc.debug("main", s);
        pipeline.computeLandabilityMap ("C3_MeanSlopeExcl", "D2_LoProtExcl", "D4_HiProtExcl", "M3_LandabilityMap");
        pipeline.copyMask("C1_ExclusionMap","M3_LandabilityMap");
//        pipeline.showImage("M3_LandabilityMap");
        pipeline.saveImage("M3_LandabilityMap", outputFileName + "M3_LandabilityMap.png");
        pipeline.exportLayer("M3_LandabilityMap", outputFileName + "M3_LandabilityMap.tif", FMT_TIFF, WORLD_COORDINATE);

        pipeline.computeBlendMeasurability("M3_LandabilityMap", "X1_MeasurabilityMap", "M4_FinalMeasurability");
        pipeline.copyMask("C1_ExclusionMap","M4_FinalMeasurability");
//        pipeline.showImage("M4_FinalMeasurability");
        pipeline.saveImage("M4_FinalMeasurability", outputFileName + "M4_FinalMeasurability.png");
        pipeline.exportLayer("M4_FinalMeasurability", outputFileName + "M4_FinalMeasurability.tif", FMT_TIFF, WORLD_COORDINATE);

        if (params.verbosity>1)
            pipeline.showInfo(); // show detailed information if asked for

        tic.lap("***\tBase pipeline completed");

        if (!argNoWait){
            logc.info("main", "Press any key to exit...");
            waitKey(0);
        }
        return NO_ERROR;
    }

    // if fixRotation = false, we iterate from rotationMin to rotationMax
    // TODO: Add verbosity check for intermediate steps (when proc. small files, it's not necessary)
    //  hi-slope can be exported directly as it is rot-independent 
    logc.info("main", "Calculating landability maps for every rotation ...");
    s << "\tRange:  [" << params.rotationMin << ", " << params.rotationMax << "]\t Steps: " << params.rotationStep;
    logc.info("main", s);

    int nIter = (params.rotationMax - params.rotationMin)/params.rotationStep;
    int finished = 0;

    #pragma omp parallel for shared(finished) num_threads(nThreads)
    for (int nK = 0; nK <= nIter; nK++){
        ostringstream xs;
        parameterStruct localParam = params;
        localParam.rotation = params.rotationMin + nK * params.rotationStep;

        if (params.verbosity > VERBOSITY_0){
            xs << "Dispatched: [" << yellow << nK << reset << "]\t---------------------------------> rot: [" << green << localParam.rotation << reset << "]";
            logc.info("main", xs);
        }
        lad::processRotationWorker (&pipeline, &localParam);
        
        #pragma omp atomic
        finished++;

        if (params.verbosity > VERBOSITY_0){
            xs << "Executed: [" << yellow << nK << reset << "]\t---------------------------------> rot: [" << green << localParam.rotation << reset << "]    Done: " << (float)(finished / (float)nIter);
            logc.info("main", xs);
        }
    }
    // now we need to merge all the intermediate rotated binary layers (M3) into a single M3_Final layer
    // every landability rotation map is a binary map indicating "landable or no-landable"
    // This can be used to describe the landing process as a Bernoulli one (binary distribution). However, as it is rotation dependent
    // and our question is "can we land?" regardless the orientation (which is something that the LAUV can determine in -situ), we proceed
    // to generate composite map as the sum/average of each map for every tested rotation.
    // This produces the equivalent of a probability map, where 0 is NO_LANDABLE and 1 is FULLY_LANDABLE

    // Step 1: Create base empty container that will hold the final value
    // pipeline.showInfo();
    logc.warn("main", "*************************************************");

    pipeline.createLayer("M3_LandabilityMap_BLEND", LAYER_RASTER);
    pipeline.copyMask("M1_RAW_Bathymetry", "M3_LandabilityMap_BLEND");
    pipeline.createLayer("M4_FinalMeasurability_BLEND", LAYER_RASTER);
    pipeline.copyMask("M1_RAW_Bathymetry", "M4_FinalMeasurability_BLEND");
    pipeline.createLayer("C2_MeanSlope_BLEND", LAYER_RASTER);
    pipeline.copyMask("M1_RAW_Bathymetry", "C2_MeanSlope_BLEND");

    auto apBase    = dynamic_pointer_cast<RasterLayer>(pipeline.getLayer("M1_RAW_Bathymetry"));
    auto apFinal   = dynamic_pointer_cast<RasterLayer>(pipeline.getLayer("M3_LandabilityMap_BLEND"));
    auto apMeasure = dynamic_pointer_cast<RasterLayer>(pipeline.getLayer("M4_FinalMeasurability_BLEND"));
    auto apSlope   = dynamic_pointer_cast<RasterLayer>(pipeline.getLayer("C2_MeanSlope_BLEND"));

    apFinal->copyGeoProperties(apBase);
    apFinal->setNoDataValue(DEFAULT_NODATA_VALUE);
    apMeasure->copyGeoProperties(apBase);
    apMeasure->setNoDataValue(DEFAULT_NODATA_VALUE);
    apSlope->copyGeoProperties(apBase);
    apSlope->setNoDataValue(DEFAULT_NODATA_VALUE);

    apFinal->rasterData   = cv::Mat(apBase->rasterData.size(), CV_64FC1, DEFAULT_NODATA_VALUE); // NODATA raster, then we upload the values
    apMeasure->rasterData = cv::Mat(apBase->rasterData.size(), CV_64FC1, DEFAULT_NODATA_VALUE); // NODATA raster, then we upload the values
    apSlope->rasterData   = cv::Mat(apBase->rasterData.size(), CV_64FC1, DEFAULT_NODATA_VALUE); // NODATA raster, then we upload the values
    cv::Mat acum          = cv::Mat::zeros(apBase->rasterData.size(), CV_64FC1); // acumulator matrix

    // pipeline.showInfo();
    // Step 2: iterate through every  
    // Layer name: "M3_LandabilityMap" + suffix
    logc.info("main", "Blending all rotation-depending maps (M3)...");

    for (int r=0; r<=nIter; r++){
        double currRotation = params.rotationMin + r*params.rotationStep;
        s << "Current orientation [" << cyan << currRotation << reset << "] degrees. Blending [" << yellow << r << "/" << nIter << reset << "]";
        logc.info("main",s);
        // params.rotation = currRotation;
        string suffix = "_r" + makeFixedLength((int) currRotation, 3);
        string currentname = "M3_LandabilityMap" + suffix;
        // cout << "\tName: " << currentname << endl;
        // let's retrieve the rasterData for the current orientation layer
        auto apCurrent = dynamic_pointer_cast<RasterLayer>(pipeline.getLayer(currentname));
        if (apCurrent == nullptr){
            s << "Failed to retrieve layer apCurrent [ " << currentname << "], line: " << __LINE__;
            logc.error("M3-blend", s);
        }
        // let's convert to a CV64FC1 normalized matrix
        cv::Mat currentmat;
        apCurrent->rasterData.convertTo(currentmat, CV_64FC1,(double) 1.0/255.0);
        acum = acum + currentmat; // sum to the acum
    }
    logc.info("main", "Normalizing ...");
    acum = acum / (nIter+1);    // normalizing

    logc.info("main","Exporting M3_LandabilityMap_BLEND");
    // transfer, via mask
    acum.copyTo(apFinal->rasterData, apFinal->rasterMask); // dst.rasterData use non-null values as binary mask ones

    pipeline.saveImage("M3_LandabilityMap_BLEND", outputFileName + "M3_LandabilityMap_BLEND.png");
    pipeline.exportLayer("M3_LandabilityMap_BLEND", outputFileName + "M3_LandabilityMap_BLEND.tif", FMT_TIFF, WORLD_COORDINATE);
//*******************************************************//
    acum = cv::Mat::zeros(apBase->rasterData.size(), CV_64FC1); // acumulator matrix
    for (int r=0; r<=nIter; r++){
        double currRotation = params.rotationMin + r*params.rotationStep;
        s <<  "Current orientation [" << cyan << currRotation << reset << "] degrees. Blending [" << yellow << r << "/" << nIter << reset << "]";
        logc.info("main",s);
        // params.rotation = currRotation;
        string suffix = "_r" + makeFixedLength((int) currRotation, 3);
        string currentname = "M4_FinalMeasurability" + suffix;
        // if (params.exportRotated)
        //     pipeline.saveImage(currentname, currentname + ".png");
        // cout << "\tName: " << currentname << endl;
        // let's retrieve the rasterData for the current orientation layer
        auto apCurrent = dynamic_pointer_cast<RasterLayer>(pipeline.getLayer(currentname));
        if (apCurrent == nullptr){
            s << "Failed to retrieve layer apCurrent [ " << currentname << "], line: " << __LINE__;
            logc.error("M4-blend", s);
        }        // let's convert to a CV64FC1 normalized matrix. This may not be necessary if layer data already stored as CV64FC1
        cv::Mat currentmat;
        apCurrent->rasterData.convertTo(currentmat, CV_64FC1);
        acum = acum + currentmat; // sum to the acum
    }

    logc.info("main", "Blending all rotation-depending MAD-maps (M4)...");
    logc.info("main", "Normalizing...");
    acum = acum / (nIter+1);    //normalizing
    logc.info("main", "Exporting M4_FinalMeasurability_BLEND");
    // transfer, via mask
    acum.copyTo(apMeasure->rasterData, apFinal->rasterMask); // dst.rasterData use non-null values as binary mask ones

    pipeline.saveImage("M4_FinalMeasurability_BLEND", outputFileName + "M4_FinalMeasurability_BLEND.png");
    pipeline.exportLayer("M4_FinalMeasurability_BLEND", outputFileName + "M4_FinalMeasurability_BLEND.tif", FMT_TIFF, WORLD_COORDINATE);
//*******************************************************//
    acum = cv::Mat::zeros(apBase->rasterData.size(), CV_64FC1); // acumulator matrix
    for (int r=0; r<=nIter; r++){
        double currRotation = params.rotationMin + r*params.rotationStep;
        s <<  "Current orientation [" << cyan << currRotation << reset << "] degrees. Blending [" << yellow << r << "/" << nIter << reset << "]";
        logc.info("main",s);
        // params.rotation = currRotation;
        string suffix = "_r" + makeFixedLength((int) currRotation, 3);
        string currentname = "C2_MeanSlope" + suffix;
        // if (params.exportRotated)
        //     pipeline.saveImage(currentname, currentname + ".png");
        // cout << "\tName: " << currentname << endl;
        // let's retrieve the rasterData for the current orientation layer
        auto apCurrent = dynamic_pointer_cast<RasterLayer>(pipeline.getLayer(currentname));
        if (apCurrent == nullptr){
            s << "Failed to retrieve layer apCurrent [ " << currentname << "], line: " << __LINE__;
            logc.error("C2-blend", s);
        }        // let's convert to a CV64FC1 normalized matrix. This may not be necessary if layer data already stored as CV64FC1
        cv::Mat currentmat;
        apCurrent->rasterData.convertTo(currentmat, CV_64FC1);
        acum = acum + currentmat; // sum to the acum
    }

    logc.info("main", "Blending all rotation-depending Slope-maps (C2)...");
    logc.info("main", "Normalizing...");
    acum = acum / (nIter+1);    //normalizing
    logc.info("main", "Exporting C2_MeanSlope_BLEND");
    // transfer, via mask
    acum.copyTo(apSlope->rasterData, apFinal->rasterMask); // dst.rasterData use non-null values as binary mask ones

    pipeline.saveImage("C2_MeanSlope_BLEND", outputFileName + "C2_MeanSlope_BLEND.png");
    pipeline.exportLayer("C2_MeanSlope_BLEND", outputFileName + "C2_MeanSlope_BLEND.tif", FMT_TIFF, WORLD_COORDINATE);
//*******************************************************//
    if (params.verbosity > 1)
        pipeline.showInfo();

    tt.lap("+++++++++++++++Complete pipeline +++++++++++++++");
    tt.stop();
    return NO_ERROR;
}
