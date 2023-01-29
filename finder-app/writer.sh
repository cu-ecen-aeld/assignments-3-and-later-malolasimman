

#!/bin/sh
#File name    : writer.sh
#Author name  : Malola Simman Srinivasan Kannan

writefile=$1
writestr=$2
path=$(dirname ${writefile})	
if [ $# -eq 2 ] # checking whether two arguments present
then
	if [ -d "$path" ] # cheching wether path exists
	then
        	echo "${writestr}" > "${writefile}"
	else
        	`mkdir  -p  "$path" `
        	echo "${writestr}" > "${writefile}"
	fi
else
	echo "invalid"
	exit 1 
fi

