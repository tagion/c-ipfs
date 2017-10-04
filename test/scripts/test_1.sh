#!/bin/bash

####
# Attempt to retrieve binary file from running daemon
#
####

source ./test_helpers.sh

IPFS="../../main/ipfs --config /tmp/ipfs_1"

function pre {
	rm -Rf /tmp/ipfs_1
	eval "$IPFS" init;
	check_failure "pre" $?
	cp ../config.test1.wo_journal /tmp/ipfs_1/config
}

function post {
	rm -Rf /tmp/ipfs_1;
#	rm hello.bin;
}

function body {
	create_binary_file;
	eval "$IPFS" add hello.bin
	check_failure "add hello.bin" $?
	
	#start the daemon
	eval "../../main/ipfs --config /tmp/ipfs_1 daemon &"
	daemon_id=$!
	sleep 5
	
	eval "$IPFS" cat QmSsV5T26CnCk3Yt7gtf6Bgyqwe4UtiaLiPbe9uUyr1nHd > hello2.bin
	check_failure "cat" $?
	
	kill -9 $daemon_id
}
