#!/bin/sh

# rpshowall
# ratpoison script to show all open windows 
# 2003 Florian Cramer <cantsin@zedat.fu-berlin.de>


# Save current frameset

framecount=`ratpoison -c windows | wc -l | sed -e "s/[ ]*//g"`
curframe=`ratpoison -c windows | grep "^.\*" | sed -e "s/^\([0-9]*\)\*.*/\1/"`
ratpoison -c "setenv tmp `ratpoison -c 'fdump'`"


# Create split view of all open windows

ratpoison -c only
i=2; 
while [ $i -le $framecount ]; do 
	if [ $i -le `echo $framecount/2 | bc` ] ; then
		ratpoison -c hsplit
	else
		ratpoison -c vsplit
	fi
	ratpoison -c focus
	ratpoison -c focus
	i=$[$i+1];
done


# Restore frameset

ratpoison -c "select $curframe"
echo -n "Restore window layout... "
read i
ratpoison -c "frestore `ratpoison -c 'getenv tmp'`"
