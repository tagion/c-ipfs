#!/bin/bash

####
# Attempt to start a deamon and have an api client do a name resolve
#
####

source ./test_helpers.sh

IPFS="../../main/ipfs --config /tmp/ipfs_1"

function pre {
	post
	eval "$IPFS" init;
	check_failure_with_exit "pre" $?
	cp ../config.test1.wo_journal /tmp/ipfs_1/config
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
	
	eval "$IPFS" name publish QmYAXgX8ARiriupMQsbGXtKdDyGzWry1YV3sycKw1qqmgH
	check_failure_with_exit "name publish" $?
	
	eval "$IPFS" name resolve /ipns/QmZVoAZGFfinB7MQQiDzB84kWaDPQ95GLuXdemJFM2r9b4
	check_failure_with_exit "name resolve" $?
	
	kill -9 $daemon_id
}


