#!/bin/bash
# 2019-09-11
# Batch processing, apply resampling filter to a batch of images (given input folder)
# 
light_red="\e[0;31m"
light_green="\e[0;32m"
light_yellow="\e[0;33m"
light_blue="\e[0;34m"
light_purple="\e[0;35m"
light_cyan="\e[0;36m"
colour_reset="\e[0m"

fn_resize_file (){
    filename=$(basename "$1")    #extract file name with extension
    echo -e "Processing "$light_blue $filename $colour_reset 
	output_file=${OUTPUT_FOLDER}/$filename
	# echo -e "output file: "$output_file
	# echo -e "xsize: "$XSIZE
	# echo -e "ysize: "$YSIZE
	ARG="--input $1 --output $output_file --size_x $XSIZE --size_y $YSIZE"
	img.resample $ARG
}
export -f fn_resize_file


# if no argument is provided, the print basic usage
if [ -z "$1" ]; then 
	echo Usage: \n
	echo img.resample -i input_directory [-o output_directory] [-x image_width] [-y image_height] [-r] [-f image_format]
	echo "************************************************************************************************"
	echo "Search for image files in input_directory and applies batch resampling filter. The intermediate image size"
	echo -e '\t' "can be defined using [image_width] and [image_height] parameters. If not, internal default values will be used"
	echo -e '\t' "If no [output_directory] is provided, then the same [input_directory] is used overwritting input images"
	echo -e '\t' "Image format can be defined using the [-f ext] parameter, where is the file extension being searched "
	exit
fi

#######################################################################################################################
# Parsing method extracted from http://wiki.bash-hackers.org/howto/getopts_tutorial
#######################################################################################################################
export OUTPUT_FOLDER=""
export XSIZE=227
export YSIZE=227
FORMAT="png"
while getopts "i:o:x:y:f:" opt; do
  case $opt in
    i)
	INPUT_FOLDER=$OPTARG 
	;;
    o)
	OUTPUT_FOLDER=$OPTARG
	;;
    x)
	XSIZE=$OPTARG
	;;
    f)
	FORMAT=$OPTARG
	;;
    y)
	YSIZE=$OPTARG
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
fi

# shopt -s nullglob
# list all tiff* files inside the input folder
# FILE_LIST=$(find $INPUT_FOLDER -type f -iname '*.${FORMAT}')
FILE_LIST=$(ls $INPUT_FOLDER/*.${FORMAT})
echo -e "Filelist: "$FILE_LIST
# word count > number of files (tokes) in the array
NUM_FILES=$(echo $FILE_LIST | wc -w)
echo -e "Total files found: "$NUM_FILES

if [ $NUM_FILES -lt 1 ]; then
	echo -e $light_red "No '"$FORMAT$"' image file found in '"$INPUT_FOLDER"'\nExiting..."
	exit
fi

# sudo apt install parallel
parallel --bar --jobs 8  fn_resize_file {} ::: $FILE_LIST
