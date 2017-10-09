#!/bin/bash

#####
# global to keep track of failures
#####
failure_count=0

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
	num_bytes=$1
	if [ $num_bytes -eq 0 ]; then
		num_bytes=255;
	fi
	exec ./generate_file hello.bin $num_bytes
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
	return $RESULT
}

####
# Checks the return code and displays message if return code is not 0
# Param $1 name of function
# Param $2 return code
####
function check_failure_with_exit() {
	FUNC=$1;
	RESULT=$2;
	if [ $RESULT -ne 0 ]; then
		echo "***Failure*** in $FUNC. The return value was $RESULT";
		exit $RESULT
	fi
	return $RESULT
}
