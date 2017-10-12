#!/bin/bash

####
# Attempt to connect to a go swarm
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
	echo cleanup complete
}

function body {
	#start the daemon
	eval "../../main/ipfs --config /tmp/ipfs_1 daemon &"
	daemon_id=$!
	sleep 5
	
	eval "$IPFS" swarm connect /ip4/107.170.104.234/tcp/4001/ipfs/QmRvKrhY2k55uQmGBq4da5XkfGEQvbhiJ4dCLKiG9qMGNF
	
	sleep 10

	kill -9 $daemon_id
}
