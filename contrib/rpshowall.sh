#!/bin/sh

# rpshowall
# ratpoison script to show all open windows
# Copyright (C) 2003 Florian Cramer <cantsin@zedat.fu-berlin.de>

# Usage :
#
# rpshowall [no arguments]
# if called from a terminal, show all open windows and
# restore previous frame layout upon keystroke
#
# rpshowall [n>1]
# show all open windows for n seconds,
# then restore previous frame layout
#
# rpshowall 0
# show all open windows, do not restore previous frame layout

if [ -z "$RATPOISON" ]; then
    RATPOISON=ratpoison
fi

# Parse input argument

case $# in
    0)  wait=-1
        ;;
    1)
        wait=$1
        ;;
    *)
        printf '%s\n' "Usage: $0 [sleep_seconds]" >&2
        exit 1
        ;;
esac

# Save current frameset

framecount=`$RATPOISON -c windows | wc -l | sed -e "s/[ ]*//g"`
curframe=`$RATPOISON -c windows | grep "^[0-9]*\*" | sed -e "s/^\([0-9]*\).*/\1/"`
curlayout=`$RATPOISON -c fdump`


# Create split view of all open windows

$RATPOISON -c only
i=2
while [ "$i" -le "$framecount" ]; do
        if [ "$i" -le $(($framecount/2)) ] ; then
                $RATPOISON -c vsplit
        else
                $RATPOISON -c hsplit
        fi
        $RATPOISON -c focus
        $RATPOISON -c focus
        i=$(($i+1));
done

# Depending on the argument the script was executed with,
# 1- select a new window and exit opening it fullscreen
# 2- restore frameset after keyboard input
# 3- restore frameset after $wait seconds of delay

$RATPOISON -c "select $curframe"
if [ $wait -eq 0 ]; then
        $RATPOISON -i -c fselect
        $RATPOISON -c only
else
        $RATPOISON -i -c windows
        if [ "$wait" -eq -1 ]; then
                printf '%s' "Hit return to restore window layout. "
                read i
        else
                sleep "$wait"
        fi
        $RATPOISON -c "frestore $curlayout"
fi
