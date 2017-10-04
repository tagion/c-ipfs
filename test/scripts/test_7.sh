#!/bin/bash

####
# Attempt to add and retrieve binary file from running daemon
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
	rm hello.bin;
	rm hello2.bin;
}

function body {
	create_binary_file;
	
	#start the daemon
	eval "../../main/ipfs --config /tmp/ipfs_1 daemon &"
	daemon_id=$!
	sleep 5
	
	# add file
	eval "$IPFS" add hello.bin
	check_failure "add hello.bin" $?
	sleep 5

	# retrieve file
	eval "$IPFS" cat QmX4zpwaE7CSgZZsULgoB3gXYC6hh7RN19bEfWxw7sL8Xx > hello2.bin
	check_failure "cat" $?
	
	# file size should be 256
	actualsize=$(wc -c < hello2.bin)
	if [ $actualsize -ne 256 ]; then
		echo '*** Failure *** file size incorrect'
	fi
	
	kill -9 $daemon_id
}
