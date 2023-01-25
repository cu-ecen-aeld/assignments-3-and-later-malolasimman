#!/bin/sh
#File name    :  finder.h
#Author name  : Malola Simman Srinivasan Kannan

if [ $# -eq 2  ] # checking number of arguments are valid
then

	if [ -d $1 ] # checking whether directory is exists
	then
                NUMBER_OF_FILES="$(find "$1" -type f | wc -l)"  #counting number of files
                NUMBER_OF_OCCURENCE="$(grep -r "$2" "$1" | wc -l)" # counting number of matching lines
                echo "The number of files are ${NUMBER_OF_FILES} and the number of matching  lines are  ${NUMBER_OF_OCCURENCE}"
        else
		echo "$1 not found"
                exit 1
        fi
else 
	echo "invalid argument"
	exit 1
fi
			

