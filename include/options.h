/**
 * @file options.h
 * @brief Argument parser options based on args.hxx
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
args::ValueFlag	<std::string> 	argOutput(argParser,    "output",   "Output file",{'o',"output"});
args::ValueFlag	<int> 	        argVerbose(argParser,   "verbose",  "Define verbosity level",                                                   {"verbose"});

args::Flag	         	        argTerrainOnly(argParser,   "",  "Run terrain only calculations (maps for Lane A & B which are AUV independent)", {"terrainonly"});
args::ValueFlag	<int> 	        argNThreads(argParser,  "number",   "Define max number of threads",  {"nthreads"});
args::ValueFlag <std::string>   argConfig(argParser,    "file.yaml","Provides path to file with user defied configuration", {"config"});
args::ValueFlag	<double>        argMetacenter(argParser,  "ratio",   "Recompute metacenter distance from vehicle height",  {"meta"});

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
    cout << cyan << "lad_test" << reset << endl; // CREATE OUTPUT TEMPLATE STRING
    cout << "\tOpenCV version:\t" << yellow << CV_VERSION << reset << endl;
    cout << "\tGit commit:\t" << yellow << GIT_COMMIT << reset << endl;
    // cout << "\tBuilt:\t" << __DATE__ << " - " << __TIME__ << endl;   // TODO: solve, make is complaining about this
    return 0;
}

#endif //_PROJECT_OPTIONS_H_