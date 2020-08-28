/**
 * @file lad_test.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief Sandbox module for testing core and extended functionalities and integration of Geotiff, OpenCV, CGAL & GDAL
 * @version 0.2
 * @date 2020-07-03
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
    //**************************************************************************
    /* Summary list parameters */
    cout << yellow << "****** Summary **********************************" << reset << endl;
    cout << "Input file:   \t" << inputFileName << endl;
    cout << "Input path:   \t" << inputFilePath << endl;
    cout << "Output prefix:\t" << outputFilePrefix << endl;
    cout << "Output path:  \t" << outputFilePath << endl;
    cout << "fParam:       \t" << fParam << endl;
    cout << "iParam:       \t" << iParam << endl;
    lad::printParams(&params);
    lad::tictac tt, tic;

    lad::Pipeline pipeline;    
    cout << "Verbose level:\t\t" << pipeline.verbosity << endl;    
    cout << "Multithreaded version, max concurrent threads: [" << yellow << nThreads << reset << "]" << endl;
    cout << yellow << "*************************************************" << reset << endl << endl;

    tic.start();
    tt.start();
    
    pipeline.useNodataMask = params.useNoDataMask;
    pipeline.readTIFF(inputFileName, "M1_RAW_Bathymetry", "M1_VALID_DataMask");
    pipeline.setTemplate("M1_RAW_Bathymetry");  // M1 will be used as internal template for the pipeline
    pipeline.extractContours("M1_VALID_DataMask", "M1_CONTOUR_Mask", params.verbosity);
        pipeline.exportLayer("M1_RAW_Bathymetry", "M1_RAW_Bathymetry.tif", FMT_TIFF, WORLD_COORDINATE);
        pipeline.exportLayer("M1_CONTOUR_Mask", "M1_CONTOUR_Mask.shp", FMT_SHP, WORLD_COORDINATE);

    pipeline.createKernelTemplate("KernelAUV",   params.robotWidth, params.robotLength, cv::MORPH_RECT);
    pipeline.createKernelTemplate("KernelSlope", 0.1, 0.1, cv::MORPH_ELLIPSE);
    pipeline.createKernelTemplate("KernelDiag",  1.0, 1.0, cv::MORPH_ELLIPSE);
    dynamic_pointer_cast<KernelLayer>(pipeline.getLayer("KernelAUV"))->setRotation(params.rotation);

    pipeline.computeExclusionMap("M1_VALID_DataMask", "KernelAUV", "C1_ExclusionMap");
        pipeline.exportLayer("C1_ExclusionMap", "C1_ExclusionMap.tif", FMT_TIFF, WORLD_COORDINATE);

    tt.lap("Load M1, C1");

    std::thread threadLaneC (&lad::processLaneC, &pipeline, &params, "");
    std::thread threadLaneB (&lad::processLaneB, &pipeline, &params, "");
    std::thread threadLaneA (&lad::processLaneA, &pipeline, &params, "");

    threadLaneA.join();
    threadLaneB.join();
    threadLaneC.join();

    pipeline.showImage("M1_RAW_Bathymetry", COLORMAP_TWILIGHT_SHIFTED);
    pipeline.showImage("A1_DetailedSlope");
    pipeline.maskLayer("B1_HEIGHT_Bathymetry", "A2_HiSlopeExcl", "M2_Protrusions");
    // pipeline.showImage("M2_Protrusions", COLORMAP_TWILIGHT_SHIFTED);
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
    pipeline.computeFinalMap ("C3_MeanSlopeExcl", "D2_LoProtExcl", "D4_HiProtExcl", "M3_FinalMap");
        // pipeline.showImage("M3_FinalMap");
        pipeline.copyMask("C1_ExclusionMap","M3_FinalMap");
        pipeline.saveImage("M3_FinalMap", "M3_FinalMap.png", COLORMAP_TWILIGHT_SHIFTED);
        pipeline.exportLayer("M3_FinalMap", "M3_FinalMap.tif", FMT_TIFF, WORLD_COORDINATE);

    tic.lap("***\tBase pipeline completed");

    if (argVerbose)
        pipeline.showInfo(); // show detailed information if asked for

    if (params.fixRotation == true){
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
    for (int w=0; w<nWorkers; w++){
        double minR = params.rotationMin + w * blockSize * params.rotationStep;
        double maxR = params.rotationMin + ((w+1) * blockSize - 1) * params.rotationStep;// - params.rotationStep;
        if (w == nWorkers-1) maxR += params.rotationStep;

        cout << "[main] Dispatching range: [" << minR << "," << maxR << "]" << endl; 
        workerParam[w] = params;
        workerParam[w].rotationMin = minR;
        workerParam[w].rotationMax = maxR;

        workerThreads[w] = std::thread (lad::processRotationWorker, &pipeline, &workerParam[w], "");
        cout << cyan << "[main] Dispatched for execution" << reset << endl;
    }

    for (int w=0; w<nWorkers; w++){
        cout << "[main] Waiting to finish worker ["<< cyan << w << reset << "]" << endl;
        workerThreads[w].join();
        cout << "[main] Worker ["<< cyan << w << reset << "] finished!" << endl;
    }

    cout << endl;    
    tt.lap("\t\t+++++++++++++++Complete pipeline +++++++++++++++");
    return NO_ERROR;
}
