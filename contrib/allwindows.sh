#!/bin/bash
# Copyright (C) 2003 Rupert Levene
# Author: Rupert Levene <r.levene@lancaster.ac.uk>

# List all groups and the windows in each group.

# we want to cope with spaces in group names
IFS='
'

#initialize our list
list=''

# Allow external scripts to tell it where ratpoison is
if [ -z $RATPOISON ]; then
    RATPOISON=ratpoison
fi

GROUPLIST=$($RATPOISON -c groups)
SED_GET_NUM='s/^\([0-9]*\).*/\1/'

FIRSTGROUPNUM=$(echo "$GROUPLIST"|head -1|sed -e "$SED_GET_NUM")
LASTGROUP=$(echo "$GROUPLIST"|tail -1)
CURRENTGROUPNUM=$(echo "$GROUPLIST"|grep '^[0-9]*\*'|sed -e "$SED_GET_NUM")

$RATPOISON -c "gselect $FIRSTGROUPNUM"

for i in $GROUPLIST; do
    list=$(printf '%s%s\n%s' "$list" "$i" "$($RATPOISON -c windows|sed -e 's/^/ /')");
    if [ "$i" != "$LASTGROUP" ]; then
        list="${list}
"
    fi
    $RATPOISON -c gnext
done;

$RATPOISON -c "echo $list"
$RATPOISON -c "gselect $CURRENTGROUPNUM"
