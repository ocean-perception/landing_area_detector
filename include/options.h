/**
 * @file options.h
 * @brief Argument parser options based on args.hxx
 * @version 1.0
 * @date 11/06/2020
 * @author Jose Cappelletto
 */

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <iostream>
#include "../common/args.hxx"

args::ArgumentParser 	argParser("","");
args::HelpFlag 	argHelp(argParser, "help", "Display this help menu", {'h', "help"});
//CompletionFlag completion(cliParser, {"complete"});	//TODO: figure out why is missing in current version of args.hxx
//args::ValueFlag	<int> 		argLinearMatch(argParser, "", "Enable linear matching mode", {"linearmatch"});
//args::ValueFlag	<int> 		argLinearMatchLen(argParser, "kWindow", "Window size for searching matches across adjacent images", {"linearmatchlen"});
args::Positional<std::string> 	argInput(argParser, "input", "Input bathymetry map. TIFF file or XYZ point collection");
args::ValueFlag	<std::string> 	argOutput(argParser, "output", "Output file",{'o',"output"});

#endif

const string green("\033[1;32m");
const string yellow("\033[1;33m");
const string cyan("\033[1;36m");
const string red("\033[1;31m");
const string reset("\033[0m");
