#!/bin/sh

# usage: ./autogen.sh [-f]
# option "-f" means forcefully create symlinks for missing files
#                   (by default: copies are made only if necessary)

if [ x"$1" = x-f ]
  then shift ; am_opt='--force'
  else         am_opt='--copy'
fi

aclocal && autoheader && automake -a $am_opt && autoconf

