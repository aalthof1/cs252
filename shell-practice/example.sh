#!/bin/bash
COUNT=10
if /usr/xpg4/bin/egrep -q [0-9] testdoc.txt ; then
	let COUNT=COUNT+1
fi
echo $COUNT
