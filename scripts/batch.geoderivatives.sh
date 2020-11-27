#!/bin/bash
# 2019-09-11
# Batch processing
# Based on extract_frames.sh v0.5 original script

export light_red="\e[0;31m"
export light_green="\e[0;32m"
export light_yellow="\e[0;33m"
export light_blue="\e[0;34m"
export light_purple="\e[0;35m"
export light_cyan="\e[0;36m"
export colour_reset="\e[0m"

fn_compute_derivatives (){
    input_file=$2
    filename=$(basename "$input_file")  # extract file name with extension
    filename_noext=${filename%."tif"*}  # trim extension (wildcard for tif/tiff)

    # Add suffix per derivative
    output_tri=${OUTPUT_FOLDER}"/tri/"${filename_noext}"_tri.tif"
    output_tpi=${OUTPUT_FOLDER}"/tpi/"${filename_noext}"_tpi.tif"
    output_slo=${OUTPUT_FOLDER}"/slo/"${filename_noext}"_slo.tif"
    output_rou=${OUTPUT_FOLDER}"/rou/"${filename_noext}"_rou.tif"
    # Process derivative per folder
    # echo per derivative
    echo -e "Processing "$light_green $input_file $colour_reset

    echo -e "> Exporting TRI to "$light_blue $output_tri $colour_reset
    gdaldem TRI ${input_file} ${output_tri} -b 1 -compute_edges -co COMPRESS=DEFLATE -co PREDICTOR=2 -co ZLEVEL=9

    echo -e "> Exporting TPI to "$light_blue $output_tpi $colour_reset
    gdaldem TPI ${input_file} ${output_tpi} -b 1 -compute_edges -co COMPRESS=DEFLATE -co PREDICTOR=2 -co ZLEVEL=9

    echo -e "> Exporting SLO to "$light_blue $output_slo $colour_reset
    gdaldem slope ${input_file} ${output_slo} -of GTiff -b 1 -s 1.0 -co COMPRESS=DEFLATE -co PREDICTOR=2 -co ZLEVEL=9

    echo -e "> Exporting ROU to "$light_blue     $output_rou $colour_reset
    gdaldem roughness ${input_file} ${output_rou} -of GTiff -b 1 -compute_edges -co COMPRESS=DEFLATE -co PREDICTOR=2 -co ZLEVEL=9

}
export -f fn_compute_derivatives


# if no argument is provided, the print basic usage
if [ -z "$1" ]; then 
	echo Usage: \n
	echo batch.geoderivatives -i input_directory -o output_directory
	echo "************************************************************************************************"
	echo "Multithreaded batch processing of geoTIFF bathymetry maps contained inside of [input_folder], generating"
    echo "geoTIFFs containing typical derivatives such as: Slope, Roughness, TRI (rugosity), TPI using gdaldem."
    echo "Generated maps are sorted into subfolders inside of [output_folder]. If clean [-c] flag is provided, any"
    echo "preexisintg content in the target output will be removed"
    exit
fi

export OUTPUT_FOLDER=${PWD}"/derivative"
#######################################################################################################################
# Parsing method extracted from http://wiki.bash-hackers.org/howto/getopts_tutorial
#######################################################################################################################
CLEAN_MODE=0    # erase the content (clear) if OUTPUT_FOLDER already exists
JOBS=8
while getopts "i:o:j:pc" opt; do
  case $opt in
    i)
	INPUT_FOLDER=$OPTARG 
	;;
    o)
	OUTPUT_FOLDER=$OPTARG 
	;;
    r)
	ROW_MODE=1
	;;
    c)
    CLEAN_MODE=1
    ;;
    j)
    JOBS=$OPTARG
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

echo -e "Input folder:\t $INPUT_FOLDER" >&2
# Let's test if output folder exist
echo -e "Output folder:\t $OUTPUT_FOLDER" >&2
if [ ! -d "$OUTPUT_FOLDER" ]; then
    echo -e "\tFolder does not exist. Creating ..."
    mkdir -p ${OUTPUT_FOLDER}
    mkdir -p ${OUTPUT_FOLDER}"/tpi" # container for Topographical Position Index
    mkdir -p ${OUTPUT_FOLDER}"/tri" # container for Terrain Ruggedness Index
    mkdir -p ${OUTPUT_FOLDER}"/slo" # Slope (1px)
    mkdir -p ${OUTPUT_FOLDER}"/rou" # Roughness (1px)
elif [[ $CLEAN_MODE -eq 1 ]]; then
    # Directory already exists, and we were asked to clear it
    echo -e "\tWarning, erasing the content of $OUTPUT_FOLDER (clear flag -c provided)"
    rm -R $OUTPUT_FOLDER/*
fi

# shopt -s nullglob
# list all tiff* files inside the input folder
FILE_LIST=$(find $INPUT_FOLDER -type f -iname '*.tif*')
# word count > number of files (tokes) in the array
NUM_FILES=$(echo $FILE_LIST | wc -w)
echo -e "Total files found:"$NUM_FILES

# sudo apt install parallel
parallel --bar --jobs ${JOBS}  fn_compute_derivatives ${OUTPUT_FOLDER} {} ::: ${FILE_LIST}
