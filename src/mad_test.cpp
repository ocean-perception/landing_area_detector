/**
 * @file mad_test.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief Measurability Area Detector, extended version of LAD test
 *        Sandbox module for testing core and extended functionalities and integration of Geotiff, OpenCV, CGAL & GDAL
 * @version 0.1
 * @date 2020-10-03
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "headers.h"
#include "helper.h"

#include "options.h"
#include "geotiff.hpp" // Geotiff class definitions
#include "lad_core.hpp"
#include "lad_config.hpp"
#include "lad_analysis.h"
#include "lad_enum.hpp"
#include "lad_processing.hpp"
#include "lad_thread.hpp"

using namespace std;
using namespace cv;
using namespace lad;

/*!
	@fn		int main(int argc, char* argv[])
	@brief	Main function
*/
int main(int argc, char *argv[])
{
    int retval = initParser(argc, argv);   // initial argument validation, populates arg parsing structure args
    if (retval != 0)  // some error ocurred, we have been signaled to stop
        return retval;
    // Parameters hierarchy
    // ARGS > CONFIG > DEFAULT (this)
    parameterStruct params = getDefaultParams(); // structure to hold configuration (populated with defaults).
    // They will be updated if config file or command line arguments are provided
    YAML::Node config;
    if (argConfig)     // check if config YAML file is provided
        config = lad::readConfiguration(args::get(argConfig), &params); // populates params structure with content of the YAML file
    // Input file priority: must be defined either by the config.yaml or --input argument
    string inputFileName    = ""; // command arg or config defined
    string inputFilePath    = ""; // can be retrieved from the fully qualified inputFileName 
    string outputFilePrefix = ""; // none, output filenames will be the same as the standard
    string outputFilePath   = ""; // same relative folder

    if (argInput) inputFileName = args::get(argInput); //input file is mandatory positional argument. Overrides any definition in configuration.yaml

    if (inputFileName.empty()){ //not defined as command line argument? let's use config.yaml definition
        if (config["input"]["filename"])
            inputFileName = config["input"]["filename"].as<std::string>();
        else{ // ERROR! We do not have any definition of the input file
            cout << red << "[main] Input file missing. Please define it using --input='filename' or inside a YAML configuration file (see --config option)";
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
        if (nThreads < 3)  cout << "[main] Info: number of used threads will be always 3 or higher. Asked for [" << yellow << nThreads << reset << "]" << endl;
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
    if (argMetacenter)      params.ratioMeta        = args::get(argMetacenter);

    if (params.updateThreshold){
        // let's recompute the slope and height thresholds according to the vehicle geometry
        cout << cyan << "[main] Recomputing slope and height thresholds" << reset << endl;
        cout << "Height" << yellow << params.robotHeight << reset << endl;
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
    //**************************************************************************
    /* Summary list parameters */
    cout << yellow << "****** Summary **********************************" << reset << endl;
    cout << "Input file:   \t" << inputFileName << endl;
    cout << "Input path:   \t" << inputFilePath << endl;
    cout << "Output prefix:\t" << outputFilePrefix << endl;
    cout << "Output path:  \t" << outputFilePath << endl;
    cout << "fParam:       \t" << fParam << endl;
    cout << "iParam:       \t" << iParam << endl;
    params.robotDiagonal = sqrt(params.robotWidth*params.robotWidth + params.robotLength*params.robotLength); 
    lad::printParams(&params);
    lad::tictac tt, tic;

    lad::Pipeline pipeline;    
    cout << "Verbose level:\t\t" << pipeline.verbosity << endl;    
    cout << "Multithreaded version, max concurrent threads: [" << yellow << nThreads << reset << "]" << endl;
    cout << yellow << "*************************************************" << reset << endl << endl;

    tic.start();
    tt.start();
    
    pipeline.useNodataMask = true;//params.useNoDataMask;
    pipeline.readTIFF(inputFileName, "M1_RAW_Bathymetry", "M1_VALID_DataMask");
    pipeline.setTemplate("M1_RAW_Bathymetry");  // M1 will be used as internal template for the pipeline
    pipeline.extractContours("M1_VALID_DataMask", "M1_CONTOUR_Mask", params.verbosity);
        pipeline.exportLayer("M1_RAW_Bathymetry", "M1_RAW_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
        pipeline.exportLayer("M1_CONTOUR_Mask", "M1_CONTOUR_Mask.shp", FMT_SHP, WORLD_COORDINATE);

    pipeline.createKernelTemplate("KernelAUV",   params.robotWidth, params.robotLength, cv::MORPH_RECT);
    pipeline.createKernelTemplate("KernelSlope", 0.1, 0.1, cv::MORPH_ELLIPSE);
    pipeline.createKernelTemplate("KernelDiag",  params.robotDiagonal, params.robotDiagonal, cv::MORPH_ELLIPSE);
    dynamic_pointer_cast<KernelLayer>(pipeline.getLayer("KernelAUV"))->setRotation(params.rotation);

    pipeline.computeExclusionMap("M1_VALID_DataMask", "KernelAUV", "C1_ExclusionMap");
        pipeline.exportLayer("C1_ExclusionMap", "C1_ExclusionMap.tif", FMT_TIFF, WORLD_COORDINATE);

    tt.lap("Load M1, C1");

    std::thread threadLaneC (&lad::processLaneC, &pipeline, &params, "");
    std::thread threadLaneB (&lad::processLaneB, &pipeline, &params, "");
    std::thread threadLaneA (&lad::processLaneA, &pipeline, &params, ""); //no suffix, nill-rotation sample

    threadLaneA.join();
    threadLaneB.join();
    threadLaneC.join();

    pipeline.showImage("M1_RAW_Bathymetry", COLORMAP_TWILIGHT_SHIFTED);
    pipeline.showImage("A1_DetailedSlope");
    pipeline.maskLayer("B1_HEIGHT_Bathymetry", "A2_HiSlopeExcl", "M2_Protrusions");
    pipeline.showImage("B1_HEIGHT_Bathymetry", COLORMAP_TWILIGHT_SHIFTED);
        pipeline.saveImage("M2_Protrusions", "M2_Protrusions.png", COLORMAP_TWILIGHT_SHIFTED);
        pipeline.exportLayer("M2_Protrusions", "M2_Protrusions.tif", FMT_TIFF, WORLD_COORDINATE);

    tt.lap("** Lanes A,B & C completed -> M2_Protrusions map done");

    //now we proceed with final LoProt/HiProt exclusion calculation
    std::thread threadLaneD (&lad::processLaneD, &pipeline, &params, "");
    threadLaneD.join();

    pipeline.copyMask("C1_ExclusionMap", "D1_LoProtMask");
    pipeline.saveImage("D1_LoProtMask", "D1_LoProtMask.png");
    pipeline.exportLayer("D1_LoProtMask", "D1_LoProtMask.tif", FMT_TIFF, WORLD_COORDINATE);

    pipeline.copyMask("C1_ExclusionMap", "D2_LoProtExcl");
    pipeline.saveImage("D2_LoProtExcl", "D2_LoProtExcl.png");
    pipeline.exportLayer("D2_LoProtExcl", "D2_LoProtExcl.tif", FMT_TIFF, WORLD_COORDINATE);

    pipeline.copyMask("C1_ExclusionMap", "D1_LoProtElev");
    pipeline.saveImage("D1_LoProtElev", "D1_LoProtElev.png");
    pipeline.exportLayer("D1_LoProtElev", "D1_LoProtElev.tif", FMT_TIFF, WORLD_COORDINATE);

    pipeline.copyMask("C1_ExclusionMap", "D3_HiProtMask");
    pipeline.saveImage("D3_HiProtMask", "D3_HiProtMask.png");
    pipeline.exportLayer("D3_HiProtMask", "D3_HiProtMask.tif", FMT_TIFF, WORLD_COORDINATE);

    if (params.verbosity> 0){
        pipeline.showImage("D2_LoProtExcl");
        pipeline.showImage("D4_HiProtExcl");
    }
    
    // Final map: M3 = C3_MeanSlope x D2_LoProtExl x D4_HiProtExcl (logical AND)
    if (params.fixRotation == true){
        pipeline.computeLandabilityMap ("C3_MeanSlopeExcl", "D2_LoProtExcl", "D4_HiProtExcl", "M3_LandabilityMap");
        pipeline.copyMask("C1_ExclusionMap","M3_LandabilityMap");
        pipeline.showImage("M3_LandabilityMap");
        pipeline.saveImage("M3_LandabilityMap", "M3_LandabilityMap.png");
        pipeline.exportLayer("M3_LandabilityMap", "M3_LandabilityMap.tif", FMT_TIFF, WORLD_COORDINATE);

        pipeline.computeBlendMeasurability("M3_LandabilityMap", "X1_MeasurabilityMap", "M4_FinalMeasurability");
        pipeline.copyMask("C1_ExclusionMap","M4_FinalMeasurability");
        pipeline.showImage("M4_FinalMeasurability");
        pipeline.saveImage("M4_FinalMeasurability", "M4_FinalMeasurability.png");
        pipeline.exportLayer("M4_FinalMeasurability", "M4_FinalMeasurability.tif", FMT_TIFF, WORLD_COORDINATE);

        if (argVerbose)
            pipeline.showInfo(); // show detailed information if asked for

        tic.lap("***\tBase pipeline completed");
        cout << endl << green << "Press any key to exit..." << endl;
        waitKey(0);
        return NO_ERROR;
    }

    // if fixRotation = false, we iterate from rotationMin to rotationMax
    cout << cyan << "[main] Calculating landability maps for every rotation:" << reset << endl;
    cout << "\tRange:  [" << params.rotationMin << ", " << params.rotationMax << "]\t Steps: " << params.rotationStep << endl;
    int nIter = (params.rotationMax - params.rotationMin)/params.rotationStep;

    // let's define the set of rotation values to be tested 
    // TODO: STEP MUST BE POSITIVE
    // TODO: MIN MUST BE LOWER THAN MAX
    // TODO: nRot should result as a positive number
    // split the workload in nThreads (nWorkers)
    int    nWorkers = (nThreads > 3)? nThreads : 4;
    vector <parameterStruct> workerParam(nWorkers);
    vector <thread>          workerThreads(nWorkers);
    // define job for each worker
    int nSteps = ceil((params.rotationMax - params.rotationMin) / params.rotationStep); // user defined range/step should match
    int blockSize = nSteps / nWorkers;
    int blockRemain = nSteps % nWorkers;
    cout << "[main] Iterating for [" << yellow << (nSteps) << reset << "] orientation combinations" << endl;
    cout << "[main] nSteps/nWorkers: " << yellow << nSteps << "/" << nWorkers << reset << endl;
    // cout << "[main] blockSize: " << yellow << blockSize << reset << endl;
    // cout << "[main] blockRemain: " << yellow << blockRemain << reset << endl;
    for (int w=0; w<nWorkers; w++){
        double minR = params.rotationMin + w * blockSize * params.rotationStep;
        double maxR = params.rotationMin + ((w+1) * blockSize - 1) * params.rotationStep;// - params.rotationStep;
        if (w == nWorkers-1) maxR = params.rotationMax;

        cout << "[main] Dispatching range: [" << minR << "," << maxR << "]" << endl; 
        workerParam[w] = params;
        workerParam[w].rotationMin = minR;
        workerParam[w].rotationMax = maxR;

        workerThreads[w] = std::thread (lad::processRotationWorker, &pipeline, &workerParam[w], "");
        // sleep(10);
        cout << cyan << "[main] Dispatched for execution: [" << yellow << w << cyan << "]" << reset << endl;
    }

    for (int w=0; w<nWorkers; w++){
        cout << "[main] Waiting to finish worker ["<< green << w << reset << "]" << endl;
        workerThreads[w].join();
        cout << "[main] Worker ["<< cyan << w << reset << "] finished!\t\t\t\t\t\t******" << endl;
    }

    // now we need to merge all the intermediate rotated binary layers (M3) into a single M3_Final layer
    // every landability rotation map is a binary map indicating "landable or no-landable"
    // This can be used to describe the landing process as a Bernoulli one (binary distribution). However, as it is rotation dependent
    // and our question is "can we land?" regardless the orientation (which is something that the LAUV can determine in -situ), we proceed
    // to generate composite map as the sum/average of each map for every tested rotation.
    // This produces the equivalent of a probability map, where 0 is NO_LANDABLE and 1 is FULLY_LANDABLE

    // Step 1: Create base empty container that will hold the final value
    // pipeline.showInfo();
    cout << endl << red <<  "*************************************************" << reset << endl;

    pipeline.createLayer("M3_LandabilityMap_BLEND", LAYER_RASTER);
    pipeline.copyMask("M1_RAW_Bathymetry", "M3_LandabilityMap_BLEND");
    pipeline.createLayer("M4_FinalMeasurability", LAYER_RASTER);
    pipeline.copyMask("M1_RAW_Bathymetry", "M4_FinalMeasurability");
    auto apBase    = dynamic_pointer_cast<RasterLayer>(pipeline.getLayer("M1_RAW_Bathymetry"));
    auto apFinal   = dynamic_pointer_cast<RasterLayer>(pipeline.getLayer("M3_LandabilityMap_BLEND"));
    auto apMeasure = dynamic_pointer_cast<RasterLayer>(pipeline.getLayer("M4_FinalMeasurability"));

    apFinal->copyGeoProperties(apBase);
    apFinal->setNoDataValue(DEFAULT_NODATA_VALUE);
    apMeasure->copyGeoProperties(apBase);
    apMeasure->setNoDataValue(DEFAULT_NODATA_VALUE);

    apFinal->rasterData = cv::Mat(apBase->rasterData.size(), CV_64FC1, DEFAULT_NODATA_VALUE); // NODATA raster, then we upload the values
    apMeasure->rasterData = cv::Mat(apBase->rasterData.size(), CV_64FC1, DEFAULT_NODATA_VALUE); // NODATA raster, then we upload the values
    cv::Mat acum        = cv::Mat::zeros(apBase->rasterData.size(), CV_64FC1); // acumulator matrix

    // pipeline.showInfo();
    // Step 2: iterate through every  
    // Layer name: "M3_LandabilityMap" + suffix
    cout << "[main] Blending all rotation-depending maps (M3)..." << endl;

    for (int r=0; r<=nIter; r++){
        double currRotation = params.rotationMin + r*params.rotationStep;
        cout << "[main] Current orientation [" << cyan << currRotation << reset << "] degrees. Blending [" << yellow << r << "/" << nIter << reset << "]" << endl;
        // params.rotation = currRotation;
        string suffix = "_r" + makeFixedLength((int) currRotation, 3);
        string currentname = "M3_LandabilityMap" + suffix;
        // cout << "\tName: " << currentname << endl;
        // let's retrieve the rasterData for the current orientation layer
        auto apCurrent = dynamic_pointer_cast<RasterLayer>(pipeline.getLayer(currentname));
        // let's convert to a CV64FC1 normalized matrix
        cv::Mat currentmat;
        apCurrent->rasterData.convertTo(currentmat, CV_64FC1,(double) 1.0/255.0);

        acum = acum + currentmat; // sum to the acum
    }
    cout << "[main] Normalizing..." << endl;
    // normalizing
    acum = acum / (nIter+1);
    cout << "[main] Exporting M3_LandabilityMap_BLEND" << endl;
    // transfer, via mask
    acum.copyTo(apFinal->rasterData, apFinal->rasterMask); // dst.rasterData use non-null values as binary mask ones

    pipeline.saveImage("M3_LandabilityMap_BLEND", "M3_LandabilityMap_BLEND.png");
    pipeline.exportLayer("M3_LandabilityMap_BLEND", "M3_LandabilityMap_BLEND.tif", FMT_TIFF, WORLD_COORDINATE);
    pipeline.showImage("M3_LandabilityMap_BLEND");
    cout << endl;    

//*******************************************************//
    acum = cv::Mat::zeros(apBase->rasterData.size(), CV_64FC1); // acumulator matrix
    for (int r=0; r<=nIter; r++){
        double currRotation = params.rotationMin + r*params.rotationStep;
        cout << "[main] Current orientation [" << cyan << currRotation << reset << "] degrees. Blending [" << yellow << r << "/" << nIter << reset << "]" << endl;
        // params.rotation = currRotation;
        string suffix = "_r" + makeFixedLength((int) currRotation, 3);
        string currentname = "M4_FinalMeasurability" + suffix;
        if (params.exportRotated)
            pipeline.saveImage(currentname, currentname + ".png");
        // cout << "\tName: " << currentname << endl;
        // let's retrieve the rasterData for the current orientation layer
        auto apCurrent = dynamic_pointer_cast<RasterLayer>(pipeline.getLayer(currentname));
        // let's convert to a CV64FC1 normalized matrix
        cv::Mat currentmat;
        apCurrent->rasterData.convertTo(currentmat, CV_64FC1);

        acum = acum + currentmat; // sum to the acum
    }
    cout << "[main] Blending all rotation-depending MAD-maps (M4)..." << endl;
    cout << "[main] Normalizing..." << endl;
    // normalizing
    acum = acum / (nIter+1);
    cout << "[main] Exporting M4_FinalMeasurability" << endl;
    // transfer, via mask
    acum.copyTo(apMeasure->rasterData, apFinal->rasterMask); // dst.rasterData use non-null values as binary mask ones

    pipeline.saveImage("M4_FinalMeasurability", "M4_FinalMeasurability.png");
    pipeline.exportLayer("M4_FinalMeasurability", "M4_FinalMeasurability.tif", FMT_TIFF, WORLD_COORDINATE);
    pipeline.showImage("M4_FinalMeasurability");
    cout << endl;    

    tt.lap("+++++++++++++++Complete pipeline +++++++++++++++");
    if (params.verbosity > 0)
        pipeline.showInfo();
    tt.stop();
    tt.show();
//    cout << "... press any key to exit" << endl;
    return NO_ERROR;
}
