#!/bin/bash

#DO NOT REMOVE THE FOLLOWING TWO LINES
git add $0 >> .local.git.out
git commit -a -m "Lab 2 commit" >> .local.git.out
git push >> .local.git.out || echo


#Your code here
INPUT=$(cat $1)
LEN=${#INPUT}
if [ "$LEN" -lt 6 -o "$LEN" -gt 32 ]; then
	echo "Error: Password length invalid."
else
	SCORE=$LEN
	if egrep -q [#$/+%@] "$1" ; then
		let SCORE=SCORE+5
	fi
	if egrep -q [0-9] "$1" ; then
		let SCORE=SCORE+5
	fi
	if egrep -q [A-Za-z] "$1" ; then
		let SCORE=SCORE+5
	fi
	if egrep -q "([0-9a-zA-Z])\1"+ "$1" ; then
		let SCORE=SCORE-10
	fi
	if egrep -q [a-z][a-z][a-z] "$1" ; then
		let SCORE=SCORE-3
	fi
	if egrep -q [A-Z][A-Z][A-Z] "$1" ; then
		let SCORE=SCORE-3
	fi
	if egrep -q [0-9][0-9][0-9] "$1" ; then
		let SCORE=SCORE-3
	fi
	echo "Password score: $SCORE"
fi
