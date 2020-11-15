#!/bin/bash
# 2020-10-31
# Map postprocessing tool that organizes all the intermediate maps generated with mad_test module. 
# It also generates merged gif for rotation dependant layers. Statistics are computed from the resulting merged layers usin statiff
# separate module. Optional configuration.yaml can be parsed to generate UUID paths and summary files. 

# if no argument is provided, then print basic usage
if [ -z "$1" ]; then 
	echo Usage: \n
	echo tiff.convert2png.sh -i input -o output -p prefix -s sufix -x scale
	echo "************************************************************************************************"
	echo -e "Missing mandatory input argument: path to folder containing the maps to be processed"
    echo -e "Example: tiff.convert2png -i /directory/with/maps"
	echo -e '\t' "Converts all the TIFF images contained in the given directory into grayscale PNG. It uses tiff2png tool" 
	echo -e '\t' "It also extracts the coordinates for the center of each image using the georef information contained in the TIFF"
	echo -e '\t' "The output images will have the prefix (if provided) added to the original TIFF image filename"
	exit
fi

light_red="\e[0;31m"
light_green="\e[0;32m"
light_yellow="\e[0;33m"
light_blue="\e[0;34m"
light_purple="\e[0;35m"
light_cyan="\e[0;36m"
colour_reset="\e[0m"

#######################################################################################################################
# Parsing method extracted from http://wiki.bash-hackers.org/howto/getopts_tutorial
#######################################################################################################################
INPUT_PATH="."
OUTPUT_PATH=$(pwd)
OUTPUT_LIST="tiff2png_filelist.csv"
PREFIX=""
SUFIX=""
SCALE=1			# bathymery range scale (default 1.0 m) It is passed as scaling factor when calling tiff2png
BLOCK_SIZE="4"
while getopts "i:o:s:p:x:l:h" opt; do
  case $opt in
    i)
	INPUT_PATH=$OPTARG 
	;;
    o)
	OUTPUT_PATH=$OPTARG 
	;;
    x)
	SCALE=$OPTARG
	;;
    s)
	SUFIX=$OPTARG
	;;
    l)
	OUTPUT_LIST=$OPTARG
	;;
    p)
	PREFIX=$OPTARG 
	;;
    h)
	echo -e "Usage: tiff.convert2png.sh -i INPUT -o OUTPUT -s SCALE -p PREFIX -l OUTPUT_LIST "
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
echo -e "Output sufix:\t $SUFIX" >&2
echo -e "Output scale:\t $SCALE" >&2
echo -e "Exported list:\t $OUTPUT_LIST" >&2
echo -e "Min file size:\t $BLOCK_SIZE blocks" >&2

fn_convert_file (){
	file=$1
	echo -e "$colour_reset Converting: $light_green" $file 
	# let's generate the desired output filename: $output_path + prefix + original filename + .png
	filename=$(basename "$file")	#extract file name with extension
	onlyname=$(basename "$filename" | sed 's/\(.*\)\..*/\1/')
	fullname=$4/$5$onlyname$6".png"
	# fullname=$OUTPUT_PATH/$PREFIX$onlyname$SUFIX".png"
	RESULT=$(tiff2png --input=$file --output=$fullname --float=$3 --verbose=0 --valid_th=0.9) # | awk -F'[][]' '{print $2}')
	# RESULT=$(tiff2png --input=$file --output=$fullname --float=$SCALE --verbose=0 --valid_th=0.9) # | awk -F'[][]' '{print $2}')
	# use sed to retrieve what is inside of the brackets []
	echo -e "$fullname\t$RESULT" >> $2
	# echo -e "$fullname\t$RESULT" >> $OUTPUT_LIST
	# i=$((i+1))
    # filename=$(basename "$2")    #extract file name with extension
    # echo -e "Processing "$light_blue $filename $colour_reset
    # gdal_retile.py -ps 227 227 -overlap 20 -levels 1 -r near -ot Float32 -co COMPRESS=DEFLATE -co PREDICTOR=2 -co ZLEVEL=9 -targetDir $1 $2
}
export -f fn_convert_file

shopt -s nullglob
mkdir -p $OUTPUT_PATH

# STEP 1: find all tif/tiff files in the input path
FILE_LIST=$(find $INPUT_PATH -name '*.tif*') ## careful, should case insensitive
echo -e "relative_path\tvalid_ratio\tnorthing [m]\teasting [m]\tdepth [m]\tlatitude [deg]\tlongitude [deg]" > $OUTPUT_LIST

# TODO: add UUID counter per row (fully compatible with Takaki's LGA)
# dispatch for each file in FILE_LIST
parallel --bar --jobs 8  	 fn_convert_file {} $OUTPUT_LIST $SCALE $OUTPUT_PATH $PREFIX $SUFIX ::: $FILE_LIST

# Old for-loop serial format
# for file in $FILE_LIST; do
# 	fn_convert_file $file
# done
# echo -e "$colour_reset Exported [$light_blue$i$colour_reset] images" 
