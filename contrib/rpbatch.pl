#!/usr/bin/perl
#
# Copyright (C) 2003 Shawn Betts
# Execute a sequence of commands read from stdin.

$ratpoison = $ENV{RATPOISON} or die "Where is ratpoison?";

while (<>) {
    chomp;
    push @accum, "-c";
    push @accum, "\"$_\"";
}

system ("$ratpoison @accum");
