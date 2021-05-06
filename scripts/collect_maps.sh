#!/bin/bash
# 2020-10-31
# Map postprocessing tool that organizes all the intermediate maps generated with mad_test module. 
# It also generates merged gif for rotation dependant layers. Statistics are computed from the resulting merged layers usin statiff
# separate module. Optional configuration.yaml can be parsed to generate UUID paths and summary files. 

# if no argument is provided, the print basic usage
if [ -z "$1" ]; then 
	echo Usage: \n
	echo collect_maps -i input -u uuid
	echo "************************************************************************************************"
	echo -e "Missing mandatory input argument: path to folder containing the maps to be processed"
    echo -e "Example: collect_maps -i /directory/with/maps"
	echo -e '\t' "The directory will be scanned and the PNG files (exported colormapped images) will be merged onto animated GIFs"
	echo -e '\t' "The raster geotiff images (TIF/TIFF) will be analized using statiff module to produce summary stats per layer"
	echo -e '\t' "Maps will be sorted onto newly created subfolders, following the pipeline structure and naming convention"
	echo -e '\t' "A global summary statistics file is generated for the project and a summary report printed on screen"
	exit
fi

# List of maps (standard pipeline with multiple rotation)
# A1_DetailedSlope.tif
# A2_HiSlopeExcl.tif
# B0_FILT_Bathymetry.tif
# B1_HEIGHT_Bathymetry.tif
# C1_ExclusionMap.tif
# C2_MeanSlopeMap_r000.tif
# C2_MeanSlopeMap_r180.tif
# C2_MeanSlopeMap.tif
# C3_MeanSlopeExcl_r000.tif
# C3_MeanSlopeExcl_r180.tif
# C3_MeanSlopeExcl.tif
# D1_LoProtElev.tif
# D1_LoProtMask.tif
# D2_LoProtExcl.tif
# D3_HiProtMask.tif
# D4_HiProtExcl_r000.tif
# D4_HiProtExcl_r180.tif
# D4_HiProtExcl.tif
# M1_RAW_Bathymetry.tif
# M2_Protrusions.tif
# M3_LandabilityMap_BLEND.tif
# M3_LandabilityMap_r000.tif
# M3_LandabilityMap_r180.tif
# M4_FinalMeasurability_r000.tif
# M4_FinalMeasurability_r180.tif
# M4_FinalMeasurability.tif
# X1_MeasurabilityMap_r000.tif
# X1_MeasurabilityMap_r180.tif
# X1_MeasurabilityMap.tif

# The root folder will contain the analysis results, animation, summary reports and main datafiles
# All the intermediate rotation-specific layers are moved into their own directories


#######################################################################################################################
# Parsing method extracted from http://wiki.bash-hackers.org/howto/getopts_tutorial
#######################################################################################################################
PATH_BASE='.'
UUID=""

while getopts "i:u:p" opt; do
  case $opt in
    i)
	PATH_BASE=$OPTARG 
	;;
    u)
	UUID=$OPTARG 
	;;
    p)
	echo "NAME PREFIX ARG PROVIDED"
	NAME_PREFIX=$OPTARG 
	;;
    \?)
	echo "Invalid option: -$OPTARG" >&2
	exit 1
	;;
    :)
	echo "Option -$OPTARG requires an argument." >&2
	exit 1
	;;
  esac
done

echo -e "Input path:\t $PATH_BASE" >&2
echo -e "Unique identifier:\t $UUID" >&2

shopt -s nullglob

# 1) Create subdirectories
mkdir C2_MeanSlopeMap C3_MeanSlopeExcl D4_HiProtExcl M3_LandabilityMap M4_FinalMeasurability X1_MeasurabilityMap
# 1.1) Create subdirectory for main base maps
mkdir BaseMaps

# 2) For each subset, we move the corresponding rotation specific images
mv C2_MeanSlopeMap_r* C2_MeanSlopeMap/
mv C3_MeanSlopeExcl_r* C3_MeanSlopeExcl/
mv D4_HiProtExcl_r* D4_HiProtExcl/
mv M3_LandabilityMap_r* M3_LandabilityMap/
mv M4_FinalMeasurability_r* M4_FinalMeasurability/
mv X1_MeasurabilityMap_r* X1_MeasurabilityMap/
mv *.tif BaseMaps
mv *.png BaseMaps
mv M1_* BaseMaps

# 3) Create the GIFs for each subset using ffmpeg
ffmpeg -pattern_type glob -i 'C2_MeanSlopeMap/*.png' C2_MeanSlopeMap.gif
ffmpeg -pattern_type glob -i 'C2_MeanSlopeExcl/*.png' C2_MeanSlopeExcl.gif
ffmpeg -pattern_type glob -i 'D4_HiProtExcl/*.png' D4_HiProtExcl.gif
ffmpeg -pattern_type glob -i 'M3_LandabilityMap/*.png' M3_LandabilityMap.gif
ffmpeg -pattern_type glob -i 'M4_FinalMeasurability/*.png' M4_FinalMeasurability.gif
ffmpeg -pattern_type glob -i 'X1_MeasurabilityMap/*.png' X1_MeasurabilityMap.gif

# 4) Next, let's compute the stats & histograms for the resulting fixed and blended maps
#### Fixed layers
# A1_DetailedSlope: 0 , 90
statiff --hmin 0 --hmax 90 --nbins 100 --input BaseMaps/A1_DetailedSlope.tif --output $UUID'A1_DetailedSlope.csv'
# B1_HEIGHT_Bathymetry: -0.5 +0.5
statiff --hmin -0.5 --hmax 0.5 --nbins 100 --input BaseMaps/B1_HEIGHT_Bathymetry.tif --output $UUID'B1_HEIGHT_Bathymetry.csv'
# C2_MeanSlopeMap: 0 , 90
statiff --hmin 0 --hmax 90 --nbins 100 --input BaseMaps/A1_DetailedSlope.tif --output $UUID'A1_DetailedSlope.csv'

#### Blended layers
# M3_LandabilityMap_BLEND: 0 , 1
statiff --hmin 0 --hmax 1 --nbins 100 --input BaseMaps/M3_LandabilityMap_BLEND.tif --output $UUID'M3_LandabilityMap_BLEND.csv'
# M4_FinalMeasurability: 0 , 1
statiff --hmin 0 --hmax 1 --nbins 100 --input BaseMaps/M4_FinalMeasurability_BLEND.tif --output $UUID'M4_FinalMeasurability_BLEND.csv'
# X1_MeasurabilityMap: 0 , 1
statiff --hmin 0 --hmax 1 --nbins 100 --input BaseMaps/X1_MeasurabilityMap.tif --output $UUID'X1_MeasurabilityMap.csv'
