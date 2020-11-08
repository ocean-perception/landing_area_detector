#!/bin/bash
# 2019-09-11
# Batch processing
# Based on extract_frames.sh v0.5 original script

# if no argument is provided, the print basic usage
if [ -z "$1" ]; then 
	echo "Usage: \n"
	echo "tiff.remove.sh -i input -o REPORT -r BLOCK_SIZE"
	echo "************************************************************************************************"
	echo "Example: batch_process -i /directory/with/tiff "
	echo -e '\t' "This script reads the proportion of valid points of each geoTIFF file using gdalinfo a export it to a report file"
	echo -e '\t' "Such report can be used to remove files with not enoug valid data points, by comparing against a threshold."
	echo -e "\tSpecial option -r can be called to enable fast culling using filesize. All files with a reported size (du)"
	echo -e "\tlower than BLOCK_SIZE will be removed"
	exit
fi

#######################################################################################################################
# Parsing method extracted from http://wiki.bash-hackers.org/howto/getopts_tutorial
#######################################################################################################################
OUT_NAME="tiff_report.txt"
BLOCK_SIZE="0"			# Any non-empty file wil have size greater than zero. This is to avoid removing any if option is not called
INPUT_PATH=$(cwd)

while getopts "i:o:r:h" opt; do
  case $opt in
    i)
	INPUT_PATH=$OPTARG 
	;;
    o)
	echo "Output report filename provided"
	OUT_NAME=$OPTARG 
	;;
	r)
	echo "Remove flag enable. Those files below the threshold will be deleted"
	BLOCK_SIZE=$OPTARG
	;;
	h)
	echo "Usage: \n"
	echo "tiff.remove.sh -i input -o REPORT -r BLOCK_SIZE"
	echo "************************************************************************************************"
	echo -e '\t' "This script reads the proportion of valid points of each geoTIFF file using gdalinfo a export it to a report file"
	echo -e '\t' "Such report can be used to remove files with not enoug valid data points, by comparing against a threshold."
	echo -e "\tSpecial option -r can be called to enable fast culling using filesize. All files with a reported size (du)"
	echo -e "\tlower than BLOCK_SIZE will be removed"
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
echo -e "Output report:\t $OUT_NAME" >&2
echo -e "Block size:\t $BLOCK_SIZE" >&2

# Retrieves the list of all video files with $VIDEO_FMT extension

if [ $BLOCK_SIZE -gt 0 ]
then
	echo -e "Applying pre-culling..."
	# find those file within the input folder whose du-size is lower than block_size, and remove
	find $INPUT_PATH -maxdepth 2 -name "*.tif*" | xargs du  | awk {'if ($1 < '$BLOCK_SIZE') print $2'} | xargs rm
fi


shopt -s nullglob

FILE_LIST=$(find $INPUT_PATH -name '*.tif*')

#echo $FILE_LIST
rm $OUT_NAME
for file in $FILE_LIST; do
	# filename=$(basename "$file")	#extract file name with extension
	# DIR_PATH=$(dirname "$file")	#extract directory path
	# ID=${filename%.$IMG_FMT}		#strip extension for file name, and use it as ID
	# FULL_PATH=$DIR_PATH		#construct absolute path
	echo "File found -> $file"
#	echo "Absolute path -> $FULL_PATH"
#	echo "**************************************************"
	# INPUT=$file

	COMMAND_RESPONSE=$(gdalinfo -stats $file | grep VALID_PERCENT | sed 's/^.*=//')
	echo -e $file"\t"$COMMAND_RESPONSE >> $OUT_NAME
done

rm *.xml

# Create the report file, now pipe it to awk to remove those below the thresold
# This can be done in the same for-loop that calls gdalinfo, but we separate it in case 
# we do not need to remove, just create the list

# if [[ $REMOVE_FLAG -eq 1 ]]; then
	# cat $OUT_NAME | awk {'if ($2 > $THRESHOLD) print $1'}
# fi

# cat tiff.report.txt | awk {'if ($2 < $THRESHOLD) {print $1}'} | xargs rm
