/**
 * @file lad_config.cpp
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief  Landing Area Detection algorithm core header
 * @version 0.1
 * @date 2020-08-24
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "lad_enum.hpp"
#include "lad_config.hpp"

using namespace lad;

/**
 * @brief Print on screen the parameters stored in the structure p
 * 
 * @param p Structure containing the parameter values to be printed
 */
void lad::printParams(parameterStruct_ *p){
    cout << "Parameters:" << endl;
    cout << "\talphaRadius:    \t" << p->alphaShapeRadius << "\t[m]" << endl;
    cout << "\trobotLength:    \t" << p->robotLength << "\t[m]" << endl;
    cout << "\trobotWidth:     \t" << p->robotWidth << "\t[m]" << endl;
    cout << "\trobotHeight:    \t" << p->robotHeight << "\t[m]"<< endl;
    if (p->fixRotation){
        cout << "\trotation:       \t" << p->rotation  << "\t[deg]" << yellow << " [fixed]" << reset << endl;
    }
    else{
        cout << cyan;
        cout << "\tMin rotation:   \t" << p->rotationMin  << "\t[deg]"<< endl;
        cout << "\tMax rotation:   \t" << p->rotationMax  << "\t[deg]"<< endl;
        cout << "\tRotation step:  \t" << p->rotationStep  << "\t[deg]"<< endl;
        cout << reset;
    }

    cout << "\theightThreshold:\t" << p->heightThreshold  << "\t[m]" << endl;
    cout << "\tslopeThreshold: \t" <<  p->slopeThreshold  << "\t[deg]" << endl;
    cout << "\tgroundThreshold:\t" << p->groundThreshold  << "\t[m]" << endl;
    cout << "\tprotrusionSize: \t" <<  p->protrusionSize  << "\t[m]" << endl;

    cout << endl;
    cout << "\tdefaultNoData:  \t" << p->defaultNoData << endl;
    cout << "\tmaskBorder:     \t" << (p->maskBorder ? "true" : "false") << endl;
    cout << "\tuseNoDataMask:  \t" << (p->useNoDataMask ? "true" : "false") << endl;
    cout << "\tverbosity:      \t" << p->verbosity << endl;
}

/**
 * @brief Read pipeline configuration from user defined file
 * 
 * @param file Name of YAML file containing configuration parameters
 * @param p pointer to structure to store the parameters retrieved from the configuration file
 * @return YAML::Node copy of base YAML node containing all the parsed parameters 
 */
YAML::Node lad::readConfiguration(std::string file, parameterStruct *p){
    cout << "Processing user defined configuration: [" << cyan << file << reset << "]" << endl;

    YAML::Node config = YAML::LoadFile(file);

    int verb = 0;

    if (config["general"]) {
        if (config["general"]["verbosity"])
            verb = config["general"]["verbosity"].as<int>();
    }

    if (config["vehicle"]){
        if (verb > 0)
            cout << "[readConfiguration] Vehicle section present" << endl;
        if (config["vehicle"]["length"])
            p->robotLength = config["vehicle"]["length"].as<double>();
        if (config["vehicle"]["width"])
            p->robotWidth = config["vehicle"]["width"].as<double>();
        if (config["vehicle"]["height"])
            p->robotHeight = config["vehicle"]["height"].as<double>();
    }

    if (config["threshold"]){
        if (verb > 0)
            cout << "[readConfiguration] Threshold section present" << endl;
        if (config["threshold"]["slope"])
            p->slopeThreshold = config["threshold"]["slope"].as<double>();
        if (config["threshold"]["height"])
            p->heightThreshold = config["threshold"]["height"].as<double>();
        if (config["threshold"]["ground"])
            p->groundThreshold = config["threshold"]["ground"].as<double>();
        if (config["threshold"]["protrusion"])
            p->protrusionSize = config["threshold"]["protrusion"].as<double>();
    }
    if (config["map"]){
        if (verb > 0)
            cout << "[readConfiguration] Map section present" << endl;
        if (config["map"]["maskborder"])
            p->maskBorder =         config["map"]["maskborder"].as<bool>();
        if (config["map"]["alpharadius"])
            p->alphaShapeRadius =   config["map"]["alpharadius"].as<double>();
        if (config["map"]["usenodatamask"])
            p->useNoDataMask =      config["map"]["usenodatamask"].as<bool>();
        if (config["map"]["nodata"])
            p->defaultNoData =      config["map"]["nodata"].as<double>();
    }

    if (config["rotation"]){
        if (verb > 0)
            cout << "[readConfiguration] Rotation section present" << endl;
        if (config["rotation"]["fixed_rotation"]){
            p->rotation     = config["rotation"]["fixed_rotation"].as<double>();
            p->fixRotation = true;
        }
        else p->fixRotation = false;
        if (config["rotation"]["range_min"])
            p->rotationMin  = config["rotation"]["range_min"].as<double>();
        if (config["rotation"]["range_max"])
            p->rotationMax  = config["rotation"]["range_max"].as<double>();
        if (config["rotation"]["step"])
            p->rotationStep = config["rotation"]["step"].as<double>();
    }
    return config;
}

lad::parameterStruct lad::getDefaultParams(){
    lad::parameterStruct params;
    params.alphaShapeRadius = 1.0;
    params.fixRotation      = true;
    params.rotation         = 0.0;    // default no rotation (heading north)
    params.rotationMin      =-90.0;
    params.rotationMax      = 90.0;
    params.rotationStep     = 5.0;
    params.groundThreshold  = 0.02; //DEFAULT;
    params.heightThreshold  = 0.10;  //DEFAULT;
    params.slopeThreshold   = 17.7; //DEFAULT;
    params.robotHeight      = 0.8;  //DEFAULT
    params.robotLength      = 1.4;
    params.robotWidth       = 0.5;
    params.protrusionSize   = 0.04;
    params.defaultNoData    = DEFAULT_NODATA_VALUE;
    params.maskBorder       = false;
    params.useNoDataMask    = true;
    params.verbosity        = 0;
    return params;
}