#!/bin/bash
# 2020-10-31
# Map postprocessing tool that organizes all the intermediate maps generated with mad_test module. 
# It also generates merged gif for rotation dependant layers. Statistics are computed from the resulting merged layers usin statiff
# separate module. Optional configuration.yaml can be parsed to generate UUID paths and summary files. 

# if no argument is provided, the print basic usage
if [ -z "$1" ]; then 
	echo Usage: \n
	echo collect_maps -i input -o output -p prefix
	echo "************************************************************************************************"
	echo -e "Missing mandatory input argument: path to folder containing the maps to be processed"
    echo -e "Example: tiff.convert2png -i /directory/with/maps"
	echo -e '\t' "Converts all the TIFF images contained in the given directory into grayscale PNG. It uses tiff2png tool" 
	echo -e '\t' "It also extracts the coordinates for the center of each image using the georef information contained in the TIFF"
	echo -e '\t' "The output images will have the prefix (if provided) added to the original TIFF image filename"
	exit
fi

#######################################################################################################################
# Parsing method extracted from http://wiki.bash-hackers.org/howto/getopts_tutorial
#######################################################################################################################
INPUT_PATH="."
OUTPUT_PATH=$(pwd)
OUTPUT_LIST="png_filelist.csv"
SCALE="1.0"			# bathymery range scale (default 1.0 m) It is passed as scaling factor when calling tiff2png

echo -e "INPUT_PATH: "$INPUT_PATH
echo -e "OUTPUT_PATH: "$OUTPUT_PATH
echo -e "ScaleX: "$SCALE

while getopts "i:o:s:p:l:h" opt; do
	echo "OPT:"$opt
  case $opt in
    i)
	INPUT_PATH=$OPTARG 
	;;
    o)
	OUTPUT_PATH=$OPTARG 
	;;
    s)
	SCALE=$OPTARG
	;;
    l)
	OUTPUT_LIST=$OPTARG
	;;
    p)
	PREFIX=$OPTARG 
	;;
    h)
	echo -e "Usage: tiff.convert2png.sh -i INPUT -o OUTPUT -s SCALE -p PREFIX -l OUTPUT_LIST"
	echo -e "\tConvert TIFF images contained in INPUT into grayscale PNG images after rescaling up to SCALE [meters]"
	echo -e "\tInput images filename will be propagated to destination filename with the given PREFIX added"
	echo -e "\tThe list of filenames and coordinates for each image will be exported to OUTPUT_LIST as csv/tsv"
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

echo -e "Input path:\t $INPUT_PATH" >&2
echo -e "Output path:\t $OUTPUT_PATH" >&2
echo -e "Output prefix:\t $PREFIX" >&2
echo -e "Output scale:\t $SCALE" >&2
echo -e "Exported list:\t $OUTPUT_LIST" >&2

shopt -s nullglob

# STEP 1: find all tif/tiff files in the input path