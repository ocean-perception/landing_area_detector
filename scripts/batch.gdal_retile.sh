#!/bin/bash
# 2019-09-11
# Batch processing
# Based on extract_frames.sh v0.5 original script

light_red="\e[0;31m"
light_green="\e[0;32m"
light_yellow="\e[0;33m"
light_blue="\e[0;34m"
light_purple="\e[0;35m"
light_cyan="\e[0;36m"
colour_reset="\e[0m"

fn_retile_file (){
    filename=$(basename "$2")    #extract file name with extension
    echo -e "Processing "$light_blue $filename $colour_reset
    gdal_retile.py -ps 227 227 -overlap 20 -levels 1 -r near -ot Float32 -co COMPRESS=DEFLATE -co PREDICTOR=2 -co ZLEVEL=9 -targetDir $1 $2
}
export -f fn_retile_file


# if no argument is provided, the print basic usage
if [ -z "$1" ]; then 
	echo Usage: \n
	echo batch.gdal_retile -i input_directory -o output_directory -r -c
	echo "************************************************************************************************"
	echo "Search for geoTIFF (tif,tiff) files inside [input_folder] and applies gdal_retile to every single map"
	echo -e '\t' "patch. The generated tiles will be exported to the [output_folder]. If enabled, the exported"
	echo -e '\t' "tiles can be sorted into row-based subfolder"
	exit
fi

#######################################################################################################################
# Parsing method extracted from http://wiki.bash-hackers.org/howto/getopts_tutorial
#######################################################################################################################
ROW_MODE=0      # ask 1-folder per row
CLEAN_MODE=0    # erase the content (clear) if OUTPUT_FOLDER already exists
while getopts "i:o:pc" opt; do
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
    mkdir -p $OUTPUT_FOLDER
elif [[ $CLEAN_MODE -eq 1 ]]; then
    # Directory already exists, and we were asked to clear it
    echo -e "\tWarning, erasing the content of $OUTPUT_FOLDER (clear flag -c provided)"
    rm -R $OUTPUT_FOLDER/*
fi

shopt -s nullglob
# list all tiff* files inside the input folder
FILE_LIST=$(find $INPUT_FOLDER -type f -iname '*.tif*')
# word count > number of files (tokes) in the array
NUM_FILES=$(echo $FILE_LIST | wc -w)
echo -e "Total files found:"$NUM_FILES

# sudo apt install parallel
parallel --bar --jobs 8  fn_retile_file $OUTPUT_FOLDER {} ::: $FILE_LIST
