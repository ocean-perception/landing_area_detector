/**
 * @file options.h
 * @brief Argument parser options based on args.hxx
 * @version 1.1
 * @date 18/06/2020
 * @author Jose Cappelletto
 */

#ifndef _PROJECT_OPTIONS_H_

#define _PROJECT_OPTIONS_H_

#include <iostream>
#include "../external/args.hxx"

args::ArgumentParser argParser("","");
args::HelpFlag 	     argHelp(argParser, "help", "Display this help menu", {'h', "help"});
args::CompletionFlag completion(argParser, {"complete"});	//TODO: figure out why is missing in current version of args.hxx

args::Positional<std::string> 	argInput(argParser,     "input",    "Input bathymetry map. TIFF file or XYZ point collection");
args::ValueFlag	<std::string> 	argOutput(argParser,    "output",   "Output file",{'o',"output"});
args::ValueFlag	<int> 	        argVerbose(argParser,   "verbose",  "Define verbosity level",                                                   {"verbose"});

args::ValueFlag	<double> 		argAlphaRadius(argParser, "alpha",  "Search radius for alpha Shape concave hull algorithm",                     {"alpharadius"});
args::ValueFlag	<float>         argRotation(argParser,  "rotation", "Vehicle rotation in degrees. Defined as ZERO heading NORTH, positive CCW", {"rotation"});

// Free parameters for debugging
args::ValueFlag	<int> 	argIntParam(argParser,  "param",    "User defined parameter INTEGER for testing purposes",  {"int"});
args::ValueFlag	<float> argFloatParam(argParser,"param",    "User defined parameter FLOAT for testing purposes",    {"float"});

// Robot dimensions
args::ValueFlag	<float> argRobotHeight(argParser,"height",  "User defined robot height in meters",    {"robotheight"});
args::ValueFlag	<float> argRobotWidth (argParser,"width",   "User defined robot width in meters",     {"robotwidth"});
args::ValueFlag	<float> argRobotLength(argParser,"length",  "User defined robot length in meters",    {"robotlength"});

// Threshold parameters
args::ValueFlag	<float> argProtrusionSize (argParser,"size", "Size threshold [cm] to consider a protrusion an obstacle", {"prot_size"});
args::ValueFlag	<float> argHeightThreshold(argParser,"height", "Height threshold [m] to determine high obstacles",    {"height_th"});
args::ValueFlag	<float> argSlopeThreshold (argParser,"slope",  "Slope threshold [deg] to determine high slope areas", {"slope_th"});
args::ValueFlag	<float> argGroundThreshold(argParser,"length", "Minimum height [m] to consider an obstacle",          {"ground_th"});


#endif //_PROJECT_OPTIONS_H_