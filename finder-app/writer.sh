#!/bin/bash

writefile=$1
writestr=$2

#echo Path is $writefile
#echo String is $writestr

#echo

# Check if 2 arguments are passed
if [ $# -lt 2 ] 
then
	echo "Failed"
	echo "Total number of arguments should be 2"
	echo "Order of arguments should be -"
	echo "	1) Path of Directory"
	echo "	2) String to be written"
	exit 1
fi

echo $writestr > $writefile
if [ -w $writefile ] && [ ! -d $writefile ]
then
	#echo "File written successfully"
	exit 0	
else
	echo "File could not be written"
	exit 1
fi
#myvar=$( cat $writefile )
#if [ myvar != $writestr ]
#then 
#	echo "File could not be written"
#	exit 1
#else
#	echo "File written successfully"
#	exit 0
#fi
