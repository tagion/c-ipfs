#!/bin/bash

####
# Attempt to retrieve large binary file from running daemon
#
####

source ./test_helpers.sh

IPFS="../../main/ipfs --config /tmp/ipfs_1"

function pre {
	rm -Rf /tmp/ipfs_1
	eval "$IPFS" init;
	check_failure_with_exit "pre" $?
	cp ../config.test1.wo_journal /tmp/ipfs_1/config
}

function post {
	rm -Rf /tmp/ipfs_1;
	rm hello.bin;
	rm hello2.bin;
}

function body {
	retVal=0
	create_binary_file 300000;
	eval "$IPFS" add hello.bin
	check_failure_with_exit "add hello.bin" $?
	
	#start the daemon
	eval "../../main/ipfs --config /tmp/ipfs_1 daemon &"
	daemon_id=$!
	sleep 5
	
	eval "$IPFS" cat QmQY3qveNvosAgRhcgVVgkPPLZv4fpWuxhL3pfihzgKtTf > hello2.bin
	check_failure "cat" $?
	
	# file size should be 300000
	actualsize=$(wc -c < hello2.bin)
	if [ $actualsize -ne 300000 ]; then
		echo '*** Failure *** file size incorrect'
		let retVal=1
	fi
	
	kill -9 $daemon_id
	exit $retVal
}
