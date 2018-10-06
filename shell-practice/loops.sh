#!/bin/bash
#for loop
for f in $( ls /var/ ); do
	echo $f
done

#while loop
COUNT=6
while [ $COUNT -gt 0 ]; do
	echo Count: $COUNT
	let COUNT=COUNT-1
done
