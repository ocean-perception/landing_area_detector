#!/bin/bash
# 2019-09-11
# Batch processing
# Based on extract_frames.sh v0.5 original script

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

#// for each file
#// paralel job dispatcher? max of instances?

for file in $FILE_LIST; do
	filename=$(basename "$file")	#extract file name with extension

#	call gdal with def params, just change input argument and output folder
    echo -e "Processing "$filename
    gdal_retile.py -ps 227 227 -overlap 20 -levels 1 -r near -ot Float32 -co COMPRESS=DEFLATE -co PREDICTOR=2 -co ZLEVEL=9 -pyramidOnly -csv "gdal_retile.filelist.csv" -csvDelim ";" -targetDir $OUTPUT_FOLDER $file

    # list of generated tiles exported as CSV:
    # 0004000_depth_map_01_01.tif;16.456370;20.996370;-165.705434;-161.165434
    # filename;  COORDINATES OF UTM LOCAL EXTENT <- not useful for oplab-pipeline
done
