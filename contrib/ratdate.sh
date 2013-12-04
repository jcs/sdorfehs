#!/bin/sh
#
# Copyright (C) 2013 Rob Paisley
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.
# 
# Description:	This script displays the date with a calendar indicating today
# 		via ratpoison's echo command.
# Suggest rebinding :time as follows: bind a exec exec ratdate.sh
t=`date +%e`
cal=`cal | sed -e '1s/.*//' -e 's/[^ [:alnum:]]//g' -e "s/\ $t\ /\<$t\>/"`
exec ratpoison -c "echo `date +'%r - %A%n   %D - %B'` $cal"
