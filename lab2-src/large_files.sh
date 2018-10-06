#!/bin/bash
first=1
small=$( ls -l | awk {'print $5'} | sort -nr | head -"$1" | tail --lines=1 )
ls -l | while read -r line
do
	curr=$( echo $line | awk {'print $5'} )
	if [ "$curr" -ge "$small" ]; then
		echo $(echo $line | awk {'print $9'}) $(echo $line | awk {'print $5'})
	fi
done

