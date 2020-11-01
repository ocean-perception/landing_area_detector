#!/bin/bash
# 2020-10-31
# Map postprocessing tool that organizes all the intermediate maps generated with mad_test module. 
# It also generates merged gif for rotation dependant layers. Statistics are computed from the resulting merged layers usin statiff
# separate module. Optional configuration.yaml can be parsed to generate UUID paths and summary files. 

# if no argument is provided, the print basic usage
if [ -z "$1" ]; then 
	echo Usage: \n
	echo batch_process -i input
	echo "************************************************************************************************"
	echo -e "Missing mandatory input argument: path to folder containing the maps to be processed"
    echo -e "Example: collect_maps -i /directory/with/maps"
	echo -e '\t' "The directory will be scanned and the PNG files (exported colormapped images) will be merged onto animated GIFs"
	echo -e '\t' "The raster geotiff images (TIF/TIFF) will be analized using statiff module to produce summary stats per layer"
	echo -e '\t' "Maps will be sorted onto newly created subfolders, following the pipeline structure and naming convention"
	echo -e '\t' "A global summary statistics file is generated for the project and a summary report printed on screen"
	exit
fi

#######################################################################################################################
# Parsing method extracted from http://wiki.bash-hackers.org/howto/getopts_tutorial
#######################################################################################################################
PATH_BASE='.'
IMG_FMT='jpg'
#NUM_FOLDERS=1
OUTPUT_FMT='jpg'
NAME_PREFIX="CC_"

while getopts "i:v:p" opt; do
  case $opt in
    i)
	PATH_BASE=$OPTARG 
	;;
    v)
	IMG_FMT=$OPTARG 
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
echo -e "File format:\t $FILE_FMT" >&2
#echo -e "Subfolders:\t $NUM_FOLDERS" >&2
#echo -e "Output format:\t $OUTPUT_FMT" >&2
echo -e "Output prefix:\t $NAME_PREFIX" >&2

# Retrieves the list of all video files with $VIDEO_FMT extension

shopt -s nullglob

FILE_LIST=$(find $PATH_BASE -name '*.'$IMG_FMT)

#echo $FILE_LIST

for file in $FILE_LIST; do
	filename=$(basename "$file")	#extract file name with extension
	DIR_PATH=$(dirname "$file")	#extract directory path
	ID=${filename%.$IMG_FMT}		#strip extension for file name, and use it as ID
	FULL_PATH=$DIR_PATH		#construct absolute path
	echo "File found -> $ID.$IMG_FMT"
#	echo "Absolute path -> $FULL_PATH"
#	echo "**************************************************"
	INPUT=$file

	# Now, we check for the '-c' flag, in order to create the required folder
#	OUTPUT=$FULL_PATH"/"$NAME_PREFIX$ID"."$OUTPUT_FMT	
	OUTPUT=$NAME_PREFIX$ID"."$OUTPUT_FMT	
	# TODO: maybe we could check if ffmpeg is installed instead of avconv. Or provide a feature with smart selection between both 	
	COMMAND_LINE="ccorrect -m=L -show=0 -cuda=0 -time=0 $INPUT $OUTPUT"
#	echo $COMMAND_LINE
#	$($COMMAND_LINE)
	
	ccorrect -m=L -show=0 -cuda=0 -time=0 $INPUT $OUTPUT
	
done
