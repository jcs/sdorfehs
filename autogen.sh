#!/bin/sh

autoheader
aclocal
automake -a -c
autoconf
