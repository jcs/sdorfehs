#!/usr/bin/perl
# Maintainer: Trent Buck <[EMAIL PROTECTED]>
# License: Public Domain

# Changelog:
#  2003-11-16 cantsin	rpshowall.sh
#  2003-11-19 various	Misc. fixes to rpshowall
#  2003-11-20 twb	expose.pl
#  2003-11-20 cantsin	minor changes
#  2003-11-22 twb	new algorithm, broke script
#  2004-05-21 bkhl	posted on wiki
#  2004-06-22 twb	full rewrite, changelog started
#  2004-06-24 twb	release candidate
#  2004-07-07 twb	toggle logic for fselect (line 43)
#  2005-07-05 ivan	updated for the new fdump format (works with 1.4.0-beta4)

# Description:
#  Duplicates OS-X 10.3's Expose' functionality for ratpoison.
#  You should apply the TWBPatch or reverse the logic at line 43.

#-- tweaking -------------------------------------------------------------------

require "assert.pl";  #used for validation.
use strict;
my $N_LIMIT = 2;
my $LIMIT = 0.01; #smaller number --> slower, but more accurate
my $RATPOISON = 'ratpoison';

#-- main -----------------------------------------------------------------------

#&th_ratio (1400, 1050, 23);
#&th_split (\&split, 5, (0, 0, 1400, 1050));

# my $id = &xid2num(8388638);
# print "'$id'\n";
# exit 0;

my @xids = &rp('windows %i'); chomp @xids;
my $n = @xids; #@ in $ context evals to length of @.
my $frames_old = &rp('fdump'); chomp $frames_old;
my @r; # Sub-optimal way of grabbing screen x,y,w,h.

#fixed to use sdump
($_, $r[0], $r[1], $r[2], $r[3]) = split(/ /,&rp ('sdump'));

my $ret = join(", ", &rp_split(\@xids, \&split, $n, \@r));

&rp("frestore $ret");

$ret = &rpi('fselect');

&rp('only');

#-- subroutines ----------------------------------------------------------------

#-- rp-specific --------------------------------------------

sub rp  { return `$RATPOISON    -c "@_"`; }
sub rpi { return `$RATPOISON -i -c "@_"`; }

sub rpb {
    my @accum = ();
    for (@_)
    {
        push @accum, "-c \"$_\"";
    }
    return `$RATPOISON @accum`;
}

sub rp_split
{
    # Prints partitions in :fdump format.
    my ($xids, $alg, $n, $r) = @_;
    my @ret = ();
    my $num = 0;
    my $i;

    foreach $i (&$alg($n, @$r))
    {
        my ($x, $y, $w, $h) = @$i;
        my $xid = pop(@$xids);
        push @ret, "(frame :number $num :x $x :y $y :width $w :height $h :window $xid)";
        $num++;
    }
    return @ret;
}


# sub frame2xid {
#     my ($fnum) = @_;
#     my ($frame, $num, $xid);
#     foreach $frame ( split(/,/, &rp('sfdump')) )
#     {
# 	($num, $_, $_, $_, $_, $xid) = split(/ /, $frame);
# 	if ($num eq $fnum)
# 	{
# 	    chomp $xid;
# 	    return $xid;
# 	}
#     }
#     return -1;			# canthappen
# }

# sub xid2num {
#     my ($xid) = @_;
#     my @windows = &rp('windows %i %n');
#     my ($window, $id, $num);
#     foreach $window ( @windows )
#     {
# 	($id, $num) = split(/ /, $window);
# 	if ($id eq $xid)
# 	{
# 	    chomp $num;
# 	    return $num;
# 	}
#     }
#     return -1;			# canthappen
# }


#-- generic ------------------------------------------------


sub isint { #from http://google.com/groups?selm=BECK.95Oct20135611%40visi5.qtp.ufl.edu
    my $x = shift;
    return 0 if ($x eq "");
    my $sign ='^\s* [-+]? \s*';
    my $int ='\d+ \s* $ ';
    return ($x =~ /$sign $int/x) ? 1 : 0;
}


sub isnum { #from http://google.com/groups?selm=90ra0g%24u0m%241%40nnrp1.deja.com
    my $x = shift;
    return ($x eq $x+0) ? 1:0;
}

sub ratio {
    my ($t, $n) = @_;
    my $ret = 1;
    my $i;
    &assert(&isnum($t) and &isint($n) and $n >= 0);

    for $i ( 2 .. $n )
    {
        if ( &isint($n / $i) # $j must also be an integer.
             and abs(($i**2 / $n) - $t) < abs(($ret**2 / $n) - $t) )
        {
            $ret = $i;
        }
    }
    #Third return value is `error', used by split().
    return ( $ret, $n / $ret, abs(($ret**2 / $n) - $t));
}

sub hsplit ($@) {
    my ($n, $rx, $ry, $rw, $rh) = @_;
    my @ret = ();
    my $i;
    &assert(isint($n) and $n >= 0);
    &assert(isint($rx) and isint($ry) and isint($rw) and isint($rh));
    &assert($rx>=0 and $ry>=0 and $rw>0 and $rh>0);
    &assert($rw >= $n);

    # remove remainder from rw, add to the last rectangle.
    my $rem = $rw % $n;
    $rw -= $rem;
    &assert(0 == $rw % $n);

    for $i (1 .. $n)
    {
        push @ret, [ ($rx + ($i-1)*$rw/$n,
                      $ry,
                      $rw/$n + ($i==$n?$rem:0),
                      $rh) ];
    }
    return @ret;
}

sub vsplit ($@) {
    my ($n, $rx, $ry, $rw, $rh) = @_;
    my @ret = ();
    my $i;
    &assert(isint($n) and $n >= 0);
    &assert(isint($rx) and isint($ry) and isint($rw) and isint($rh));
    &assert($rx>=0 and $ry>=0 and $rw>0 and $rh>0);
    &assert($rh >= $n);

    # remove remainder from rh, add to the last rectangle.
    my $rem = $rh % $n;
    $rh -= $rem;
    &assert(0 == $rh % $n);

    for $i (1 .. $n)
    {
        push @ret, [ ($rx,
                      $ry + ($i-1)*$rh/$n,
                      $rw,
                      $rh/$n + ($i==$n?$rem:0)) ];
    }
    return @ret;
}

sub boxsplit ($@) {
    my ($n, $rx, $ry, $rw, $rh) = @_;
    my @ret = ();
    my ($rows, $cols) = &ratio($rw/$rh, $n);
    my $i;
    &assert(isint($n) and $n >= 0);
    &assert(isint($rx) and isint($ry) and isint($rw) and isint($rh));
    &assert($rx>=0 and $ry>=0 and $rw>0 and $rh>0);
    &assert(($rh * $rw) >= $n);

    foreach $i (&vsplit($rows, ($rx, $ry, $rw, $rh)))
    {
        @ret = (@ret, &hsplit($cols, @$i));
    }
    return @ret;
}

sub split ($@) {
    my ($n, $rx, $ry, $rw, $rh) = @_;
    my ($rows, $cols, $prox) = &ratio($rw/$rh, $n);

    # if base case, palm off to boxsplit()
    if ($n < $N_LIMIT or $prox < $LIMIT)
    {
        return &boxsplit($n, $rx, $ry, $rw, $rh);
    }
    else
    {
        my @ret = ();
        my $nA = int($n/2); #fixme: int() is bad.  Use POSIX::floor?
        my $x = $nA / ($n - $nA);
        my $i;
        &assert(isint($n) and $n >= 0);
        &assert(isint($rx) and isint($ry) and isint($rw) and isint($rh));
        &assert($rx>=0 and $ry>=0 and $rw>0 and $rh>0);
        &assert(($rh * $rw) >= $n);

        if ($rw > $rh) # Divide the larger dimension
        {
            my $k = int($rw * $x / (1 + $x)); #fixme: bad int().
            @ret = (@ret, &split($nA,      ($rx,    $ry, $k,     $rh)));
            @ret = (@ret, &split($n - $nA, ($rx+$k, $ry, $rw-$k, $rh)));
        }
        else
        {
            my $k = int($rh * $x / (1 + $x)); #fixme: bad int().
            @ret = (@ret, &split($nA,      ($rx, $ry,    $rw, $k    )));
            @ret = (@ret, &split($n - $nA, ($rx, $ry+$k, $rw, $rh-$k)));
        }
        return @ret;
    }
}

#-- test harnesses -------------------------------------------------------------

# Commented out to speed up compilation.

# sub th_ratio
# {
#     my ($num, $denom, $n) = @_;
#     my ($row, $col, $prox) = &ratio ($num / $denom, $n);
#     print "$n = $row * $col, proximity $prox\n";
# }

# sub th_split
# {   # Prints partitions as a SVG.  See inkscape.org for info.
#     my ($alg, $n, @r) = @_;
#     my $i;

#     print "<svg width=\"$r[2]\" height=\"$r[3]\">\n";
#     foreach $i (&$alg($n, @r))
#     {
#       my ($x, $y, $w, $h) = @$i;
#       print "<rect x=\"$x\"\ty=\"$y\"\twidth=\"$w\"\theight=\"$h\"",
#             " style=\"fill:white;stroke:black;\"/>\n";
#     }
#     print "</svg>\n";
# }

