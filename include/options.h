/**
 * @file options.h
 * @brief Argument parser options based on args.hxx. Extended to accomodate multiple modules using similar parsers
 * @version 1.1
 * @date 18/06/2020
 * @author Jose Cappelletto
 */

#ifndef _PROJECT_OPTIONS_H_

#define _PROJECT_OPTIONS_H_

#include "headers.h"
#include "lad_enum.hpp"
#include "../external/args.hxx"
#include <iostream>

args::ArgumentParser argParser("","");
args::HelpFlag 	     argHelp(argParser, "help", "Display this help menu", {'h', "help"});
args::CompletionFlag completion(argParser, {"complete"});	//TODO: figure out why is missing in current version of args.hxx

args::ValueFlag <std::string> 	argInput(argParser, "input", "Input bathymetry map. TIFF file or XYZ point collection", {"input"});
// args::Positional<std::string> 	argInput(argParser,     "input",    "Input bathymetry map. TIFF file or XYZ point collection");
args::ValueFlag	<std::string> 	argOutput(argParser,    "output",   "Output file basename that will be used as prefix of all exported layers",{'o',"output"});
args::ValueFlag	<std::string> 	argOutputPath(argParser,"path",   "(NOT-IMPLEMENTED) Output folder path that will contain files will be exported",{'p',"outpath"});
args::ValueFlag	<int> 	        argVerbose(argParser,   "verbose",  "Define verbosity level, 0 - 3", {"verbose"});

args::Flag	         	        argNoWait(argParser,   "Disable wait for user input when finishing. Use when batch-processing single images","", {"nowait"});
args::ValueFlag <int>  	        argSaveIntermediate(argParser, "value", "Define if to export intermediate rotation-identependent maps (Lane A & B)", {"saveintermediate"});
args::Flag	         	        argTerrainOnly(argParser,   "",     "Run terrain only calculations (maps for Lane A & B which are AUV independent)", {"terrainonly"});
args::ValueFlag	<int> 	        argNThreads(argParser,  "number",   "Define max number of threads",  {"nthreads"});
args::ValueFlag <std::string>   argConfig(argParser,    "file.yaml","Provides path to file with user defied configuration", {"config"});
args::ValueFlag	<double>        argMetacenter(argParser,"ratio",    "Recompute metacenter distance from vehicle height",  {"meta"});

args::ValueFlag	<double> 		argAlphaRadius(argParser, "alpha",  "Search radius for alpha Shape concave hull algorithm",                     {"alpharadius"});
args::ValueFlag	<double>        argRotation(argParser,  "rotation", "Vehicle rotation in degrees. Defined as ZERO heading NORTH, positive CCW", {"rotation"});

// Free parameters for debugging
args::ValueFlag	<int> 	argIntParam(argParser,  "param",    "User defined parameter INTEGER for testing purposes",  {"int"});
args::ValueFlag	<float> argFloatParam(argParser,"param",    "User defined parameter FLOAT for testing purposes",    {"float"});

// Robot dimensions
args::ValueFlag	<double> argRobotHeight(argParser,"height",  "User defined robot height in meters",    {"robotheight"});
args::ValueFlag	<double> argRobotWidth (argParser,"width",   "User defined robot width in meters",     {"robotwidth"});
args::ValueFlag	<double> argRobotLength(argParser,"length",  "User defined robot length in meters",    {"robotlength"});

// Threshold parameters
args::ValueFlag	<double> argProtrusionSize (argParser,"size", "Size threshold [cm] to consider a protrusion an obstacle", {"prot_size"});
args::ValueFlag	<double> argHeightThreshold(argParser,"height", "Height threshold [m] to determine high obstacles",    {"height_th"});
args::ValueFlag	<double> argSlopeThreshold (argParser,"slope",  "Slope threshold [deg] to determine high slope areas", {"slope_th"});
args::ValueFlag	<double> argGroundThreshold(argParser,"length", "Minimum height [m] to consider an obstacle",          {"ground_th"});
args::ValueFlag	<double> argValidThreshold(argParser,"ratio", "Minimum ratio of required valid pixels to generate PNG",{"valid_th"});

//*************************************** tiff2png specific parser
args::ArgumentParser argParserT2P("","");
args::HelpFlag 	     argHelpT2P(argParserT2P, "help", "Display this help menu", {'h', "help"});
args::CompletionFlag completionT2P(argParserT2P, {"complete"});	//TODO: figure out why is missing in current version of args.hxx

args::ValueFlag <std::string> 	argInputT2P(argParserT2P, "input", "Input geoTIFF image, typ bathymetry map",   {'i', "input"});
args::ValueFlag	<std::string> 	argOutputT2P(argParserT2P,    "filename", "Output file",                         {'o', "output"});
args::ValueFlag	<int> 	        argVerboseT2P(argParserT2P,   "verbose",  "Define verbosity level",              {'v', "verbose"});
args::ValueFlag	<std::string> 	argExportTiffT2P(argParserT2P,"filename", "GeoTIFF copy of the exported image",  {'e', "export_tiff"});

// Free parameters for debugging
args::ValueFlag	<int> 	argIntParamT2P(argParserT2P,  "param",    "User defined parameter INTEGER for testing purposes",  {"int"});
args::ValueFlag	<float> argFloatParamT2P(argParserT2P,"param",    "User defined parameter FLOAT for testing purposes",    {"float"});
// Sampling parameters
args::ValueFlag	<double>        argRotationT2P(argParserT2P,"angle",  "Rotation angle of the ROI to be exported [degrees]",{"rotation"});
args::ValueFlag	<int>           argXOffsetT2P(argParserT2P,"pixels", "ROI horizontal (X) offset from the input image center", {"offset_x"});
args::ValueFlag	<int>           argYOffsetT2P(argParserT2P,"pixels", "ROI vertical (Y) offset from the input image center",   {"offset_y"});
args::ValueFlag	<unsigned int>  argXSizeT2P(argParserT2P,"pixels", "ROI width (X) in pixels",                                 {"size_x"});
args::ValueFlag	<unsigned int>  argYSizeT2P(argParserT2P,"pixels", "ROI height (Y) in pixels",                                {"size_y"});
args::ValueFlag	<double>        argZMaxT2P(argParserT2P,"meters", "Maximum input value (Z). It wil be mapped to 255",      {"max_z"});
// Thresholds
args::ValueFlag	<double>        argValidThresholdT2P(argParserT2P,"ratio", "Minimum ratio of required valid pixels to generate PNG",{"valid_th"});
args::Flag	         	        argGrayscaleT2P(argParserT2P,   "",  "Export single channel 8-bit PNG instead of RGB", {"grayscale"});
args::Flag	         	        argCsvT2P(argParserT2P,   "",  "Use comma ',' as column separator rather than TAB", {"csv"});

int initParser(int argc, char *argv[]){
        //*********************************************************************************
    /* PARSER section */
    std::string descriptionString =
        "lad_test - testing module part of [landing-area-detection] pipeline \
    Compatible interface with geoTIFF bathymetry datasets via GDAL + OpenCV";

    argParser.Description(descriptionString);
    argParser.Epilog("Author: J. Cappelletto (GitHub: @cappelletto)\n");
    argParser.Prog(argv[0]);
    argParser.helpParams.width = 120;

    try
    {
        argParser.ParseCLI(argc, argv);
    }
    catch (const args::Completion &e)
    {
        cout << e.what();
        return 0;
    }

    catch (args::Help)
    { // if argument asking for help, show this message
        cout << argParser;
        return lad::ERROR_MISSING_ARGUMENT;
    }
    catch (args::ParseError e)
    { //if some error ocurr while parsing, show summary
        std::cerr << e.what() << std::endl;
        std::cerr << "Use -h, --help command to see usage" << std::endl;
        return lad::ERROR_WRONG_ARGUMENT;
    }
    catch (args::ValidationError e)
    { // if some error at argument validation, show
        std::cerr << "Bad input commands" << std::endl;
        std::cerr << "Use -h, --help command to see usage" << std::endl;
        return lad::ERROR_WRONG_ARGUMENT;
    }
    // cout << "\tBuilt:\t" << __DATE__ << " - " << __TIME__ << endl;   // TODO: solve, make is complaining about this
    return 0;
}

int initParserT2P(int argc, char *argv[], string newDescription = ""){
        //*********************************************************************************
    /* PARSER section */
    std::string descriptionString =
        "tiff2png - image preprocessing tool for LGA + BNN based seafloor measurability predictor \
        Partial data augmentation on demand by resampling input image, via traslation and rotation \
        Data range linear remapping with (clip-limit) is performed beore exporting as PNG image \
    Compatible interface with geoTIFF bathymetry datasets via GDAL + OpenCV";

    if (!newDescription.empty())
        argParserT2P.Description(newDescription);
    else
        argParserT2P.Description(descriptionString);
    
    argParserT2P.Epilog("Author: J. Cappelletto (GitHub: @cappelletto)\n");
    argParserT2P.Prog(argv[0]);
    argParserT2P.helpParams.width = 120;

    try
    {
        argParserT2P.ParseCLI(argc, argv);
    }
    catch (const args::Completion &e)
    {
        cout << e.what();
        return 0;
    }

    catch (args::Help)
    { // if argument asking for help, show this message
        cout << argParserT2P;
        return lad::ERROR_MISSING_ARGUMENT;
    }
    catch (args::ParseError e)
    { //if some error ocurr while parsing, show summary
        std::cerr << e.what() << std::endl;
        std::cerr << "Use -h, --help command to see usage" << std::endl;
        return lad::ERROR_WRONG_ARGUMENT;
    }
    catch (args::ValidationError e)
    { // if some error at argument validation, show
        std::cerr << "Bad input commands" << std::endl;
        std::cerr << "Use -h, --help command to see usage" << std::endl;
        return lad::ERROR_WRONG_ARGUMENT;
    }
    // cout << "\tBuilt:\t" << __DATE__ << " - " << __TIME__ << endl;   // TODO: solve, make is complaining about this
    return 0;
}

#endif //_PROJECT_OPTIONS_H_