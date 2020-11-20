#!/bin/bash
# 2020-10-31
# Map postprocessing tool that organizes all the intermediate maps generated with mad_test module. 
# It also generates merged gif for rotation dependant layers. Statistics are computed from the resulting merged layers usin statiff
# separate module. Optional configuration.yaml can be parsed to generate UUID paths and summary files. 
function show_usage(){
	echo -e Usage: \n
	echo tiff.convert2png.sh -i input [-o output] [-l output_list] [-p prefix] [-u sufix] [-x scale] [-r] 
	echo "************************************************************************************************"
    echo -e "Example: tiff.convert2png -i /directory/with/tiffs -o "
	echo -e '\t' "Converts all the TIFF images contained in the given directory into grayscale PNG. It uses tiff2png tool" 
	echo -e '\t' "It also extracts the coordinates for the center of each image using the georef information contained in the TIFF"
	echo -e "-r \t Enable uniform randomized sampling (0 to 360 degrees, X/Y shift)"
}
export show_usage

# if no argument is provided, then print basic usage
if [ -z "$1" ]; then 
	echo -e "Missing mandatory arguments"
	show_usage
	exit
fi

export light_red="\e[0;31m"
export light_green="\e[0;32m"
export light_yellow="\e[0;33m"
export light_blue="\e[0;34m"
export light_purple="\e[0;35m"
export light_cyan="\e[0;36m"
export colour_reset="\e[0m"

#######################################################################################################################
# Parsing method extracted from http://wiki.bash-hackers.org/howto/getopts_tutorial
#######################################################################################################################
RANDOM=$(date +%N | cut -b4-9)
INPUT_PATH="."
OUTPUT_PATH=$(pwd)
OUTPUT_LIST="t2p_filelist.csv"
export PREFIX=""
export SUFIX=""
export SCALE=1.0			# bathymery range scale (default 1.0 m) It is passed as scaling factor when calling tiff2png
BLOCK_SIZE="4"
export RANDOM_SAMPLE="0"
export SEP=','
while getopts "i:o:u:p:x:l:rh" opt; do
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
    u)
	SUFIX=$OPTARG
	;;
    l)
	OUTPUT_LIST=$OPTARG
	;;
    p)
	PREFIX=$OPTARG 
	;;
    r)
	RANDOM_SAMPLE=1 
	;;
    h)
	show_usage
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
echo -e "Random sampling:\t $RANDOM_ROT" >&2
echo -e "Min file size:\t $BLOCK_SIZE blocks" >&2

fn_convert_file (){
	file=$1
	echo -e "$colour_reset Converting: $light_green" $file 
	# let's generate the desired output filename: $output_path + prefix + original filename + .png
	filename=$(basename "$file")	#extract file name with extension
	onlyname=$(basename "$filename" | sed 's/\(.*\)\..*/\1/')
	fullname_png=$4/$onlyname$SUFIX".png"
	fullname_tiff=$4/$onlyname$SUFIX".tif"
	echo -e "$colour_reset to PNG: $light_yellow $fullname_png" 
	echo -e "$colour_reset to TIF: $light_yellow $fullname_tiff"
	# fullname=$OUTPUT_PATH/$PREFIX$onlyname$SUFIX".png"
	local_rotation=0
	x_offset=0
	y_offset=0
	if [ $RANDOM_SAMPLE -eq '1' ]; then
		# local_rotation=20
		local_rotation=$(($RANDOM%360))
		x_offset=$(($RANDOM%20-10))
		y_offset=$(($RANDOM%20-10))
	fi
	echo -e "$colour_reset Rotation: $local_rotation \tOffset: $x_offset / $y_offset"
	RESULT=$(tiff2png --csv --input=$file --output=$fullname_png --export_tiff=$fullname_tiff --max_z=$SCALE --valid_th=0.8 --rotation=$local_rotation --offset_x=$x_offset --offset_y=$y_offset)
	# RESULT=$(  --valid_th=0.9 --rotation=$local_rotation)
	# use sed to retrieve what is inside of the brackets []
	echo -e -n "$fullname_png${SEP}$fullname_tiff${SEP}$RESULT" >> $2
	# "${SEP}altitude [m]${SEP}roll [deg]${SEP}pitch [deg]${SEP}heading [deg]${SEP}timestamp [s]" >> $OUTPUT_LIST
	echo -e "6.0${SEP}0.0${SEP}0.0${SEP}0.0${SEP}1" >> $2
}
export -f fn_convert_file

shopt -s nullglob
mkdir -p $OUTPUT_PATH

# STEP 1: find all tif/tiff files in the input path
FILE_LIST=$(find $INPUT_PATH -name '*.tif*') ## careful, should case insensitive
echo -n -e "relative_path${SEP}relative_path_tiff${SEP}valid_ratio${SEP}northing [m]${SEP}easting [m]${SEP}depth [m]${SEP}latitude [deg]${SEP}longitude [deg]" > $OUTPUT_LIST
echo -e "${SEP}altitude [m]${SEP}roll [deg]${SEP}pitch [deg]${SEP}heading [deg]${SEP}timestamp [s]" >> $OUTPUT_LIST
# TODO: add UUID counter per row (fully compatible with Takaki's LGA)
# dispatch for each file in FILE_LIST
parallel --bar --jobs 8  	 fn_convert_file {} $OUTPUT_LIST $SCALE $OUTPUT_PATH $PREFIX ::: $FILE_LIST