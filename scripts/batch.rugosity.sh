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

fn_compute_rugosity (){
    input_file=$2
    filename=$(basename "$input_file")  # extract file name with extension
    filename_noext=${filename%."tif"*}  # trim extension (wildcard for tif/tiff)

    # Add suffix per derivative
    output_rug=${OUTPUT_FOLDER}"/rug/"${filename_noext}"_rug.tif"
    # Process derivative per folder
    # echo per derivative
    echo -e "Processing "$light_green $input_file $colour_reset

    echo -e "> Exporting RUG to "$light_blue     $output_rug $colour_reset
    tiff2rugosity --input ${input_file} --output ${output_rug}

}
export -f fn_compute_rugosity


# if no argument is provided, the print basic usage
if [ -z "$1" ]; then 
	echo Usage: \n
	echo batch.rugosity -i input_directory -o output_directory -f
	echo "************************************************************************************************"
	echo "Multithreaded batch processing of geoTIFF bathymetry maps contained inside of [input_folder], generating"
    echo "RUGOSITY derivative inside the [output_folder]. The Rugosity is calculated as the ratio between the 3D"
    echo "representation of a 2x2 px window, and it's projected planar area."
    exit
fi

export OUTPUT_FOLDER=${PWD}
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
if [ ! -d "${OUTPUT_FOLDER}/rug" ]; then
    echo -e "\tFolder does not exist. Creating ..."
    mkdir -p ${OUTPUT_FOLDER}"/rug" # container for Topographical Position Index
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
parallel --bar --jobs ${JOBS}  fn_compute_rugosity ${OUTPUT_FOLDER} {} ::: ${FILE_LIST}
