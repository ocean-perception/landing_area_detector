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

export CONFIG_FILE="config_lauv.yaml"

fn_compute_derivatives (){
    input_file=$2
    filename=$(basename "$input_file")  # extract file name with extension
    filename_noext=${filename%."tif"*}  # trim extension (wildcard for tif/tiff)

    # Add suffix per derivative
    output_mad=${OUTPUT_FOLDER}"/"${filename_noext}"_mad_"
    # Process derivative per folder
    # echo per derivative
    echo -e "Processing "$light_green $input_file $colour_reset

    echo -e "> Exporting MAD to "$light_blue $output_mad $colour_reset
    mad_test --input ${input_file} --output ${output_mad}  --verbose ${VERBOSITY_LEVEL} --config ${CONFIG_FILE} --rotstep=${ROTATION_ANGLE} ${TERRAIN_FLAG}
}
export -f fn_compute_derivatives


# if no argument is provided, the print basic usage
if [ -z "$1" ]; then 
	echo Usage: \n
	echo batch.landability -i input_directory -o output_directory
	echo "************************************************************************************************"
	echo "Multithreaded batch calculation of landability and measurability maps from of geoTIFF bathymetry "
    echo "maps contained inside of [input_folder]"
    exit
fi

export OUTPUT_FOLDER=${PWD}"/mad_derivative"
#######################################################################################################################
# Parsing method extracted from http://wiki.bash-hackers.org/howto/getopts_tutorial
#######################################################################################################################
export ROTATION_ANGLE=15    # default rotation step angle
export TERRAIN_FLAG=""  # empty terrainOnly flag
export VERBOSITY_LEVEL=1
JOBS=8
while getopts "i:o:j:c:r:v:pt" opt; do
  case $opt in
    i)
	INPUT_FOLDER=$OPTARG 
	;;
    o)
	OUTPUT_FOLDER=$OPTARG 
	;;
    j)
    JOBS=$OPTARG
    ;;
    c)
    CONFIG_FILE=$OPTARG
    ;;
    r)
    ROTATION_ANGLE=$OPTARG
    ;;
    v)
    VERBOSITY_LEVEL=$OPTARG
    ;;
    t)
    TERRAIN_FLAG="--terrainonly"
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
    # mkdir -p ${OUTPUT_FOLDER}"/mad" # Roughness (1px)
fi

# shopt -s nullglob
# list all tiff* files inside the input folder
FILE_LIST=$(find $INPUT_FOLDER -type f -iname '*.tif*')
# word count > number of files (tokes) in the array
NUM_FILES=$(echo $FILE_LIST | wc -w)
echo -e "Total files found:"$NUM_FILES

# sudo apt install parallel
parallel --bar --jobs ${JOBS}  fn_compute_derivatives ${OUTPUT_FOLDER} {} ::: ${FILE_LIST}
