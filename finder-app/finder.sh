#!/bin/bash

filesdir=$1
searchstr=$2

#echo Path is $filesdir
#echo Search string is $searchstr

#echo

# Check if 2 arguments are passed
if [ $# -lt 2 ] 
then
	echo "Failed"
	echo "Total number of arguments should be 2"
	echo "Order of arguments should be -"
	echo "	1) Path of Directory"
	echo "	2) String to be searched"
	exit 1
fi

# Check if path provided is a directory
if [ -d $filesdir ]
then
	echo "$filesdir is a directory"
else
	echo "$filesdir is not a directory"
	exit 1
fi

X=$( ls $filesdir | wc -l )
#grep -rnw $filesdir -e $searchstr
Y=$( grep -roh $searchstr $filesdir | wc -w)
echo The number of files are $X and the number of matching lines are $Y

