#!/bin/bash

source ./test_helpers.sh

IPFS="../../main/ipfs --config /tmp/ipfs_1"

function check_failure() {
	FUNC=$1;
	RESULT=$2;
	if [ $RESULT -eq 0 ]; then
		echo "";
	else
		echo "Failure in $FUNC. The return value was $RESULT";
	fi
}

function pre {
	eval "$IPFS" init;
	check_failure "pre" $?
}

function post {
	rm -Rf /tmp/ipfs_1;
	rm hello.txt;
}

function body {
	create_hello_world;
	eval "$IPFS" add hello.txt
	eval "$IPFS" cat QmYAXgX8ARiriupMQsbGXtKdDyGzWry1YV3sycKw1qqmgH
}


