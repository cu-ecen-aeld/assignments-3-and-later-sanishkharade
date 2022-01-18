#!/bin/bash

: '
 @file    		:   writer.sh
 @brief   		:   Script to write a particular string to a file
 
 @author  		:   Sanish Kharade
 @date    		:   January 17, 2022
  
 @references    :   get directory name - https://www.cyberciti.biz/faq/unix-get-directory-name-from-path-command/
'

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

# Store the recieved arguments in varaibles
writefile=$1
writestr=$2

# Get directory name from the path
directoryname="${writefile%/*}"

# Check if directory exists
if [ ! -d $directoryname ]
then
	echo "$directoryname doesn't exist. Creating the directory"
	mkdir $directoryname
fi	

# Write the string to the file
echo $writestr > $writefile

: '
Check if the file was written successfully.
It could happen that the file exists but does not have write permission
Hence -w is used to check that
'
if [ -w $writefile ]
then
	exit 0	
else
	echo "File could not be written"
	exit 1
fi



