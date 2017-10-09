#!/bin/bash

#source ./test_helpers.sh

rm testlog.txt

num_tests=9
tests_passed=0
tests_failed=0
i=1
while [ $i -le $num_tests ]; do
	echo *Running test number $i
	echo *Running test number $i >> testlog.txt
	./run_test.sh $i >>testlog.txt 2>&1
	retVal=$?
	if [ $retVal -ne 0 ]; then
		echo *Test number $i failed
		echo *Test number $i failed >> testlog.txt
		let tests_failed++
	else
		echo *Test number $i passed
		echo *Test number $i passed >> testlog.txt
		let tests_passed++
	fi
	let i++
done
echo Of $num_tests tests, $tests_passed passed and $tests_failed failed.
echo Of $num_tests tests, $tests_passed passed and $tests_failed failed. >> testlog.txt
echo Results are in testlog.txt
