#!/bin/sh
# License: CC BY-ND 3.0 see http://creativecommons.org/licenses/by-nd/3.0
#
# Description:	This script displays the date with a calendar indicating today
#		via ratpoison's echo command.
# Suggest rebinding :time as follows: bind a exec exec ratdate.sh
today=`date +%e`
cal=`cal -h | tail -n +2 | sed -e 's/^Su/\n Su/' -e 's/.*/ & /' -e "s/\ $today\ /\<$today\>/"`
exec ratpoison -c "echo `date +' %r - %A %n    %D - %B'` $cal"
