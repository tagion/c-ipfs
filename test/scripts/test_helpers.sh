#!/bin/bash

#####
# Functions to help with test scripts
#####

####
# Create a text file "hello.txt" that contains "Hello, World!"
####
function create_hello_world {
	echo 'Hello, World!' > hello.txt
}

####
# Create a binary file
###
function create_binary_file {
	rm hello.bin
	for byte in `seq 0 256`;
	do
		printf "\\$(printf "%o" 0x$byte)" >> hello.bin
	done
}

####
# Checks the return code and displays message if return code is not 0
# Param $1 name of function
# Param $2 return code
####
function check_failure() {
	FUNC=$1;
	RESULT=$2;
	if [ $RESULT -ne 0 ]; then
		echo "***Failure*** in $FUNC. The return value was $RESULT";
	fi
}

