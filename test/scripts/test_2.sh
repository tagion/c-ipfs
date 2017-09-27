#!/bin/bash

source ./test_helpers.sh

IPFS="../../main/ipfs --config /tmp/ipfs_1"

function pre {
	eval "$IPFS" init;
}

function post {
	rm -Rf /tmp/ipfs_1;
	rm hello.txt;
}

function body {
	create_hello_world;
	eval "$IPFS" add hello.txt
	eval "$IPFS" object cat Qm123456
}


