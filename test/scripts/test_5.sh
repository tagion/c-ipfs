#!/bin/bash

####
# Attempt to start 2 deamons and have an api client of server A ask for a file from server B
####

source ./test_helpers.sh

IPFS1="../../main/ipfs --config /tmp/ipfs_1"
IPFS2="../../main/ipfs --config /tmp/ipfs_2"

function pre {
	post
	eval "$IPFS1" init;
	check_failure_with_exit "pre" $?
	cp ../config.test1.wo_journal /tmp/ipfs_1/config
	
	eval "$IPFS2" init;
	check_failure_with_exit "pre ipfs2" $?
	cp ../config.test2.wo_journal /tmp/ipfs_2/config
}

function post {
	rm -Rf /tmp/ipfs_1;
	rm -Rf /tmp/ipfs_2;
	rm hello.txt;
}

function body {
	create_hello_world;
	eval "$IPFS1" add hello.txt
	check_failure_with_exit "add hello.txt" $?
	
	#start the daemons
	eval "../../main/ipfs --config /tmp/ipfs_1 daemon &"
	daemon_id_1=$!
	eval "../../main/ipfs --config /tmp/ipfs_2 daemon &"
	daemon_id_2=$!
	sleep 5
	
	#A client of server 2 wants the file at server 1
	eval "$IPFS2" cat QmYAXgX8ARiriupMQsbGXtKdDyGzWry1YV3sycKw1qqmgH
	check_failure "cat" $?
	
	kill -9 $daemon_id_1
	kill -9 $daemon_id_2
}


