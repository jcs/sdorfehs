#!/usr/bin/env perl
#
# Copyright (C) 2003 Shawn Betts
# Execute a sequence of commands read from stdin.

$ratpoison = $ENV{RATPOISON} || 'ratpoison';

while (<>) {
    chomp;
    push @accum, "-c";
    push @accum, "\"$_\"";
}

system ("$ratpoison @accum");
