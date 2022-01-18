#!/bin/bash

: '
 @file    		:   finder.sh
 @brief   		:   Script to find all occurences of a particular string in a directory
 
 @author  		:   Sanish Kharade
 @date    		:   January 17, 2022
  
 @references    :   find - https://devconnected.com/how-to-count-files-in-directory-on-linux/
					grep - https://stackoverflow.com/questions/6135065/how-to-count-occurrences-of-a-word-in-all-the-files-of-a-directory/6135712
'

# Check if 2 arguments are passed
if [ $# -lt 2 ] 
then
	echo "Failed"
	echo "Total number of arguments should be 2"
	echo "Order of arguments should be -"
	echo "	1) Path of Directory"
	echo "	2) String to be searched"
	echo
	exit 1
fi

# Store the recieved arguments in varaibles
filesdir=$1
searchstr=$2

# Check if path provided is a directory
if [ ! -d $filesdir ]
then
	echo "$filesdir is not a directory"
	exit 1
fi

# Find the number of files in the directory 
X=$( find $filesdir -type f | wc -l )

# Find the occurences of string in the directory
Y=$( grep -roh $searchstr $filesdir | wc -l)

#Print the output
echo The number of files are $X and the number of matching lines are $Y







