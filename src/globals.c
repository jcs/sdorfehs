#include "ratpoison.h"

int alarm_signalled = 0;
int kill_signalled = 0;
int hup_signalled = 0;
int chld_signalled = 0;
int rat_x;
int rat_y;
int rat_visible = 1;		/* rat is visible by default */

Atom wm_name;
Atom wm_state;
Atom wm_change_state;
Atom wm_protocols;
Atom wm_delete;
Atom wm_take_focus;
Atom wm_colormaps;

Atom rp_command;
Atom rp_command_request;
Atom rp_command_result;

int rp_current_screen;
rp_screen *screens;
int num_screens;
Display *dpy;

rp_group *rp_current_group;
LIST_HEAD (rp_groups);
LIST_HEAD (rp_children);
struct rp_defaults defaults;

int ignore_badwindow = 0;

char **myargv;

struct rp_key prefix_key;

struct modifier_info rp_modifier_info;

/* rudeness levels */
int rp_honour_transient_raise = 1;
int rp_honour_normal_raise = 1;
int rp_honour_transient_map = 1;
int rp_honour_normal_map = 1;

char *rp_error_msg = NULL;
