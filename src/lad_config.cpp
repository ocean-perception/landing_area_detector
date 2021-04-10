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
    cout << "Algorithm parameters:" << endl;
    cout << "\talphaRadius:    \t" << p->alphaShapeRadius << "\t[m]" << endl;
    cout << "\trobotLength:    \t" << p->robotLength << "\t[m]" << endl;
    cout << "\trobotWidth:     \t" << p->robotWidth << "\t[m]" << endl;
    cout << "\trobotDiagonal:  \t" << p->robotDiagonal << "\t[m]" << endl;
    cout << "\trobotHeight:    \t" << p->robotHeight << "\t[m]"<< endl;
    cout << "\tCG Ratio:       \t" << p->ratioCg << "\t[m/m]"<< endl;
    cout << "\tMC Ratio:       \t" << p->ratioMeta << "\t[m/m]"<< endl;
    cout << "\tForce ratio:    \t" << p->forceRatio << "\t[N/N]"<< endl;
    cout << "\tGravity force:  \t" << p->gravityForce << "\t[N]"<< endl;
    cout << "\tBuoyancy force: \t" << p->buoyancyForce << "\t[N]"<< endl;
    cout << "\tNet force:      \t" << (p->gravityForce - p->buoyancyForce) << "\t[N]"<< endl;

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
    if (p->updateThreshold) cout << yellow;
    cout << "\theightThreshold:\t" << p->heightThreshold  << "\t[m]" << endl;
    cout << "\tslopeThreshold: \t" <<  p->slopeThreshold  << "\t[deg]" << reset << endl;
    cout << "\tgroundThreshold:\t" << p->groundThreshold  << "\t[m]" << endl;
    cout << "\tprotrusionSize: \t" <<  p->protrusionSize  << "\t[m]" << endl;

    cout << "Sensor parameters" << endl;
    cout << "\tdiameter:\t" << p->geotechSensor.diameter  << "\t[m]" << endl;
    cout << "\tz_optimal:\t" << p->geotechSensor.z_optimal  << "\t[m]" << endl;
    cout << "\tz_suboptimal:\t" << p->geotechSensor.z_suboptimal  << "\t[m]" << endl;

    cout << "Map options" << endl;
    cout << "\tdefaultNoData:  \t" << p->defaultNoData << endl;
    cout << "\tmaskBorder:     \t" << (p->maskBorder ? "true" : "false") << endl;
    cout << "\tuseNoDataMask:  \t" << (p->useNoDataMask ? "true" : "false") << endl;
    cout << "\tverbosity:      \t" << p->verbosity << endl;

    cout << "Export options" << endl;
    cout << "\texportIntermediate:\t" << (p->exportIntermediate ? "true" : "false") << endl;
    cout << "\texportRotated:     \t" << (p->exportRotated ? "true" : "false") << endl;
}

/**
 * @brief Read pipeline configuration from user defined file
 * 
 * @param file Name of YAML file containing configuration parameters
 * @param p pointer to structure to store the parameters retrieved from the configuration file
 * @return YAML::Node copy of base YAML node containing all the parsed parameters 
 */
YAML::Node lad::readConfiguration(std::string file, parameterStruct *p){
    // cout << "Processing user defined configuration: [" << cyan << file << reset << "]" << endl;

    YAML::Node config = YAML::LoadFile(file);
    int verb = 0;

    if (config["general"]) {
        if (config["general"]["verbosity"])
            verb = config["general"]["verbosity"].as<int>();
        if (config["general"]["export"]["intermediate"])
            p->exportIntermediate = config["general"]["export"]["intermediate"].as<bool>();
        if (config["general"]["export"]["rotated"])
            p->exportRotated      = config["general"]["export"]["rotated"].as<bool>();
        if (config["general"]["recomputethresh"])
            p->updateThreshold = config["general"]["recomputethresh"].as<bool>();
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
        if (config["vehicle"]["cg_ratio"])
            p->ratioCg = config["vehicle"]["cg_ratio"].as<double>();
        if (config["vehicle"]["meta_ratio"])
            p->ratioMeta = config["vehicle"]["meta_ratio"].as<double>();
        if (config["vehicle"]["force_ratio"])
            p->forceRatio = config["vehicle"]["force_ratio"].as<double>();
        if (config["vehicle"]["forces"]){
            p->gravityForce = config["vehicle"]["forces"]["gravity"].as<double>();
            p->buoyancyForce = config["vehicle"]["forces"]["buoyancy"].as<double>();
        }
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
            p->protrusionSize  = config["threshold"]["protrusion"].as<double>();
        // if (config["threshold"]["geosensor"])
        //     p->geotecThreshold = config["threshold"]["geosensor"].as<double>();
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

    if (config["geotechsensor"]){   // explicit definition of geotechnical sensor parameters
        if (verb > 0)
            cout << "[readConfiguration] geotechSensor section present" << endl;

        if (config["geotechsensor"]["diameter"]){
            p->geotechSensor.diameter = config["geotechsensor"]["diameter"].as<double>();
        }
        // else{
        //     cout << "using default diameter for sensor" << endl;
        //     p->geotechSensor.diameter = DEFAULT_G_DIAM; // use default diameter for geotech sensor
        // }
        if (config["geotechsensor"]["z_optimal"]){
            p->geotechSensor.z_optimal = config["geotechsensor"]["z_optimal"].as<double>();
        }
        // else{
        //     cout << "using default z_opt for sensor" << endl;
        //     p->geotechSensor.z_optimal = DEFAULT_Z_OPT; // use default sensor optimal range
        // }
        if (config["geotechsensor"]["z_suboptimal"]){
            p->geotechSensor.z_suboptimal = config["geotechsensor"]["z_suboptimal"].as<double>();
        }
        // else{
        //     cout << "using default z_sub for sensor" << endl;
        //     p->geotechSensor.z_suboptimal = DEFAULT_Z_SUB; // use default sensor suboptimal range
        // }
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
    params.ratioMeta        = 0.2;
    params.ratioCg          = 0.5;
    params.forceRatio       = 0.05;
    params.updateThreshold  = false;

    params.protrusionSize   = 0.04;
    params.defaultNoData    = DEFAULT_NODATA_VALUE;
    params.maskBorder       = false;
    params.useNoDataMask    = true;
    params.verbosity        = 0;
    params.exportIntermediate = true;
    params.exportRotated    = false;

    params.geotechSensor.diameter     = DEFAULT_G_DIAM;
    params.geotechSensor.z_optimal    = DEFAULT_Z_OPT;
    params.geotechSensor.z_suboptimal = DEFAULT_Z_SUB;
    return params;
}