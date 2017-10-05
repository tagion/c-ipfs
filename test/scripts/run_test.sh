#!/bin/bash

TEST_FILE=test_$1.sh

test_success=0

source ./$TEST_FILE

function do_it() {
	pre
	body
	post
	return $?
}

do_it
exit $test_success
