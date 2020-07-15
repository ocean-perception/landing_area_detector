/**
 * @file options.h
 * @brief Argument parser options based on args.hxx
 * @version 1.0
 * @date 18/06/2020
 * @author Jose Cappelletto
 */

#ifndef _OPTIONS_H_

#define _OPTIONS_H_

#include <iostream>
#include "../external/args.hxx"

args::ArgumentParser 	argParser("","");
args::HelpFlag 	argHelp(argParser, "help", "Display this help menu", {'h', "help"});
args::CompletionFlag completion(argParser, {"complete"});	//TODO: figure out why is missing in current version of args.hxx

args::ValueFlag	<double> 		argAlphaRadius(argParser, "alpha", "Search radius for alpha Shape concave hull algorithm", {"alpharadius"});
args::Positional<std::string> 	argInput(argParser, "input", "Input bathymetry map. TIFF file or XYZ point collection");
args::ValueFlag	<std::string> 	argOutput(argParser, "output", "Output file",{'o',"output"});
args::ValueFlag	<int> 	        argVerbose(argParser, "verbose", "Show verbose information",{"verbose"});

#endif