#!/bin/sh

# rpshowall
# ratpoison script to show all open windows 
# 2003 Florian Cramer <cantsin@zedat.fu-berlin.de>

if [ -z $RATPOISON ]; then
    RATPOISON=ratpoison
fi
# Save current frameset

framecount=`$RATPOISON -c windows | wc -l | sed -e "s/[ ]*//g"`
curframe=`$RATPOISON -c windows | grep "^.\*" | sed -e "s/^\([0-9]*\)\*.*/\1/"`
$RATPOISON -c "setenv tmp `$RATPOISON -c 'fdump'`"


# Create split view of all open windows

$RATPOISON -c only
i=2; 
while [ $i -le $framecount ]; do 
	if [ $i -le `echo $framecount/2 | bc` ] ; then
		$RATPOISON -c hsplit
	else
		$RATPOISON -c vsplit
	fi
	$RATPOISON -c focus
	$RATPOISON -c focus
	i=$[$i+1];
done


# Restore frameset

$RATPOISON -c "select $curframe"
echo -n "Restore window layout... "
read i
$RATPOISON -c "frestore `$RATPOISON -c 'getenv tmp'`"
