#!/usr/bin/perl

# clickframe.pl is a utility to switch frames by clicking the
# mouse. You must apply the patch below to xbindkeys for this script
# to work. And add an entry like this to your .xbindkeysrc file:
#
# # bind C-mouse1 to ratpoison click focus hack
# "perl /home/sabetts/src/ratpoison/contrib/clickframe.pl &"
# control + b:1
#
# Requires xbindkeys-1.6.4 with the patch at the end of this file
# applied. This file can be fed directly to patch.

# Make sure the env vars are there
$ENV{XBINDKEYS_BUTTONLOC} || die '$XBINDKEYS_BUTTONLOC not bound';
$ENV{RATPOISON} || die '$RATPOISON not bound';

# Parse the required environment variables
$ratpoison_bin = $ENV{RATPOISON};
($x_loc,$y_loc) = split(/,/, $ENV{XBINDKEYS_BUTTONLOC});

# Rip the frameset from ratpoison
$frameset = `$ratpoison_bin -c fdump`;
@framelist = split(/,/,$frameset);

# Check each frame to see if the mouse was clicked in that frame.
# FIXME: it goes through all the frames even if it found one.
foreach $frame (@framelist) {
    ($num,$left,$top,$width,$height,$win,$access) = split(/ /,$frame);
    if ($x_loc > $left && $x_loc < $left + $width && $y_loc > $top && $y_loc < $top + $height) {
        # Tell ratpoison to switch to the frame
	print "User clicked in frame $num\n";
	system ("$ratpoison_bin -c \"fselect $num\"");
    }
}

__END__

--- xbindkeys.c~	2003-04-06 08:43:27.000000000 -0700
+++ xbindkeys.c	2003-09-24 11:46:20.000000000 -0700
@@ -143,7 +143,15 @@



+void
+add_button_env (int x, int y)
+{
+  char *env;

+  env = malloc (256 * sizeof (char));
+  snprintf (env, 255, "XBINDKEYS_BUTTONLOC=%d,%d", x, y);
+  putenv (env);
+}


 static void
@@ -240,6 +248,8 @@
 			       | Button1Mask | Button2Mask | Button3Mask
 			       | Button4Mask | Button5Mask);

+	  add_button_env (e.xbutton.x, e.xbutton.y);
+
 	  for (i = 0; i < nb_keys; i++)
 	    {
 	      if (keys[i].type == BUTTON && keys[i].event_type == PRESS)
@@ -266,6 +276,8 @@
 			       | Button1Mask | Button2Mask | Button3Mask
 			       | Button4Mask | Button5Mask);

+	  add_button_env (e.xbutton.x, e.xbutton.y);
+
 	  for (i = 0; i < nb_keys; i++)
 	    {
 	      if (keys[i].type == BUTTON && keys[i].event_type == RELEASE)

