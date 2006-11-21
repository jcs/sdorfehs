#! /bin/sh
# ratpoison helper script to start up an menu
# change this line to globally set options
# (like -fg, -bg or -font)
options="-popup"

if [ "$#" -ne "1" ] ; then
	echo "Syntax: /etc/X11/ratpoison/ratpoisonmenu <menufile>"
	exit 1
fi
file="$1"
parent="`echo "$file" | sed -e 's/\.[^.]*\.menu$/.menu/'`"
if [ -f "$HOME/.ratpoison_menu/$file" ] ; then
	dir="$HOME/.ratpoison_menu"
elif [ -f "/etc/X11/ratpoison/menu/$file" ] ; then
	# To allow a global override of single files...
	# (additionally to the easy changing of what
	# update-menus generates...
	dir="/etc/X11/ratpoison/menu"
else
	dir="/var/lib/ratpoison/menu"
fi
if [ -f /etc/X11/ratpoison/ratpoisonmenu.options ] ; then
	# for those that do not like changing this file directly...
	. /etc/X11/ratpoison/ratpoisonmenu.options
fi
if [ -f "$HOME/.ratpoison_menu/options" ] ; then
	# parse file, so you can set $options
	# like options="$options -fg blue -bg black"
	# You can even exec in there, if you do not like 9menu
	. "$HOME/.ratpoison_menu/options"
fi
if ! which 9menu >/dev/null ; then
	if which ratmenu >/dev/null && [ -x "/etc/X11/ratmenu/$file" ] ; then
		ratpoison -c "echo 9menu not installed, using ratmenu instead"
		exec "/etc/X11/ratmenu/$file"
	else
		exec ratpoison -c "echo 9menu not installed"
	fi
fi
if ! [ -f "$dir/$file" ] ; then
  if [ "$file" = "debian.menu" ] ; then
    exec ratpoison -c "echo no menu definition found (package 'menu' missing?)"
  else
    exec ratpoison -c "echo no definition for $file found!"
  fi
fi
if [ "$file" = "debian.menu" ] ; then
	exec 9menu $options -file "$dir/$file" '(cancel):exec'
else
	exec 9menu $options -file "$dir/$file" ..:"$0 \"$parent\""
fi
