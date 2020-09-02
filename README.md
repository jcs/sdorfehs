## sdorfehs
<img src="https://jcs.org/images/sdorfehs-300.jpg" align="right">

#### (pronounced "starfish")

sdorfehs is a tiling window manager descended from
[ratpoison](https://www.nongnu.org/ratpoison/)
(which itself is modeled after
[GNU Screen](https://www.gnu.org/software/screen/)).

sdorfehs divides the screen into one or more frames, each only displaying
one window at a time but can cycle through all available windows (those
which are not being shown in another frame).

Like Screen, sdorfehs primarily uses prefixed/modal key bindings for most
actions.
sdorfehs's command mode is entered with a configurable keystroke
(`Control+a` by default) which then allows a number of bindings accessible
with just a single keystroke or any other combination.
For example, to cycle through available windows in a frame, press
`Control+a` then `n`.

### License

sdorfehs retains ratpoison's GPL2 license.

### Compiling

Run `make` to compile, and `make install` to install to `/usr/local` by
default.

## Features

sdorfehs retains most of ratpoison's features while adding some more modern
touches.

![Screenshot](https://jcs.org/images/sdorfehs-20190826.png)

### EWMH

sdorfehs strives to be a bit more
[EWMH](https://specifications.freedesktop.org/wm-spec/wm-spec-latest.html)
compliant, supporting Atoms such as `_NET_ACTIVE_WINDOW` for compositors
to dim unfocused windows, and `_NET_WM_STATE_FULLSCREEN` to allow full-screen
programs like VLC and Firefox to take over the entire screen when requested
(though still allowing the standard key bindings to switch windows or kill
the program).

### Virtual Screens

sdorfehs adds virtual screens which each have their own frame configuration
and set of windows.
Windows can be moved between virtual screens with `vmove`.
By default, virtual screens can be selected (via the `vselect` command)
with `Control+a, F1` through `Control+a, F12`.

### Bar

sdorfehs has a bar window which is shown temporarily to display status
messages or the output of certain commands.
By default, this bar is made sticky with the `barsticky` setting which
forces the bar to be permanently affixed to the top or bottom (configurable
with the `bargravity` setting) of every virtual screen displaying the
currently-focused window's title on the left side.

When the bar is sticky, it also enables a mechanism to display arbitrary
text on the right side, similar to bar programs for other window managers like
i3bar, dzen2, etc.
sdorfehs creates a
[named pipe](https://en.wikipedia.org/wiki/Named_pipe)
at `~/.config/sdorfehs/bar` and any text input into the pipe shows up on
the bar, making it easy to integrate with standard utilities which can just
echo into the pipe.
For an extremely simple example, a shell script can just echo the output of
`date` into the pipe once a second.

    while true; do
      date > ~/.config/sdorfehs/bar
      sleep 1
    done

Bar input supports some markup commands from dzen2 in the format
`^command(details)` which affect the text following the command until the
command is reset with `^command()`.
Currently supported commands:

- `^ca(btn,cmd)`: execute `cmd` when mouse button `btn` is clicked on this
area of text.
Closing the area of clickable text can be done with `^ca()`.

- `^fg(color)`: color the text following until the next `^fg()` command.
A line of text such as `hello ^fg(green)world^fg()!` will color `hello` with
the default foreground color (`set fgcolor`), then `world` in green, and the
exclamation point with the default color.
Colors can be specified as their common name (`blue`) or as a hex code
(`#0000ff`).

- `^fn(font)`: change the font of the following text.
Fonts must be specified in Xft format like `set font` such as
`^fn(noto emoji:size=13)`.
Resetting to the default font can be done with `^fn()`.

### Gaps

sdorfehs enables a configurable gap (with `set gap`) between frames by
default to look a bit nicer on larger displays.

### Secure Remote Control

sdorfehs's `-c` command line option uses a more secure IPC mechanism
than Ratpoison for sending commands to a running sdorfehs process,
such as a script controlling sdorfehs and restore a particular
layout.

Ratpoison's IPC only requires that a process create a new X11 window
and set an Atom on it, which the parent Ratpoison process reads and
executes the value of that Atom.
An unprivileged application that only has an X11 connection (say a
sandboxed Firefox child process) could set the `RP_COMMAND_REQUEST`
Atom property on its existing window with a value of `0exec
something` to cause Ratpoison to read it and execute that shell
command as the user id running Ratpoison.

sdorfehs's IPC mechanism switches to a Unix socket in the
`~/.config/sdorfehs` directory which ensures the requesting process
has the ability to make socket connections and can write to that
path.

## Tips

- Enable the `ignoreresizehints` setting (`set ignoreresizehints 1`) to force
windows to conform to the frame size, eliminating any gap around windows like
terminals that supply resize hints to request sizing in a multiple of their
font size.

- Enable xterm's `allowSendEvents` setting to allow sdorfehs to send a fake
`Control+a` key when pressing `Control+a, a` (which runs its `meta` command).
xterm disables this by default as a security measure to prevent other programs
from sending it arbitrary key input.

- Since sdorfehs does not color active and inactive window borders differently
by default (though this can be done by setting `bwcolor` and `fwcolor`), a
compositor like
[Compton](https://github.com/chjj/compton)
can be used with its `inactive-dim` setting to dim inactive windows.
Its `use-ewmh-active-win` setting should also be enabled since sdorfehs focuses
its own input windows periodically, and it supports EWMH hints to inform
Compton which window should actually be considered focused.

- While most default key bindings require the prefix command key (`Control+a`
by default), keys can be bound without them in the `top` keymap.
For example, `definekey top F1 vselect 0` will switch to the first virtual
screen when `F1` is pressed.

  A related tip: if you have bound a key to an action and then need to send
that key to a window, you can do it with the `meta` command accessible via the
interactive input menu (`Control+a, :meta F1`), or with
`sdorfehs -c "meta F1"`.
