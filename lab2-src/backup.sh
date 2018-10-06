#!/bin/bash

filename=".$1_backup"
cat $1 > "$filename"
while [ 1 ]
do
	sleep 1
	if diff $filename $1 ; then
		cat $1 > "$filename"
		echo done
	fi
done
