#!/bin/bash

####
# Attempt to start a deamon and have an api client do a object get
#
####

source ./test_helpers.sh

IPFS="../../main/ipfs --config /tmp/ipfs_1"

function pre {
	post
	eval "$IPFS" init;
	check_failure_with_exit "pre" $?
}

function post {
	rm -Rf /tmp/ipfs_1;
	rm hello.txt;
}

function body {
	create_hello_world;
	eval "$IPFS" add hello.txt
	check_failure_with_exit "add hello.txt" $?
	
	#start the daemon
	eval "../../main/ipfs --config /tmp/ipfs_1 daemon &"
	daemon_id=$!
	sleep 5
	
	eval "$IPFS" cat QmYAXgX8ARiriupMQsbGXtKdDyGzWry1YV3sycKw1qqmgH
	check_failure_with_exit "cat hello.txt" $?
	
	kill -9 $daemon_id
}


