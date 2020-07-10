/*
 * communications.c -- Send commands to a running copy of sdorfehs.
 * Copyright (C) 2000, 2001, 2002, 2003, 2004 Shawn Betts <sabetts@vcn.bc.ca>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

#include "sdorfehs.h"

void
init_control_socket_path(void)
{
	char *config_dir;

	config_dir = get_config_dir();
	rp_glob_screen.control_socket_path = xsprintf("%s/control", config_dir);
	free(config_dir);
}

void
listen_for_commands(void)
{
	struct sockaddr_un sun;

	if ((rp_glob_screen.control_socket_fd = socket(AF_UNIX,
	    SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
		err(1, "socket");

	sun.sun_family = AF_UNIX;
        if (strlcpy(sun.sun_path, rp_glob_screen.control_socket_path,
	    sizeof(sun.sun_path)) >= sizeof(sun.sun_path))
                err(1, "control socket path too long: %s",
		    rp_glob_screen.control_socket_path);

        if (unlink(rp_glob_screen.control_socket_path) == -1 &&
	    errno != ENOENT)
		err(1, "unlink %s",rp_glob_screen.control_socket_path);

	if (bind(rp_glob_screen.control_socket_fd, (struct sockaddr *)&sun,
	    sizeof(sun)) == -1)
		err(1, "bind %s", rp_glob_screen.control_socket_path);

	if (chmod(rp_glob_screen.control_socket_path, 0600) == -1)
		err(1, "chmod %s", rp_glob_screen.control_socket_path);

	if (listen(rp_glob_screen.control_socket_fd, 2) == -1)
		err(1, "listen %s", rp_glob_screen.control_socket_path);

	PRINT_DEBUG(("listening for commands at %s\n",
	    rp_glob_screen.control_socket_path));
}

int
send_command(int interactive, unsigned char *cmd)
{
	struct sockaddr_un sun;
	char *wcmd;
	char ret[1024];
	size_t len;
	int fd;

	len = 1 + strlen(cmd) + 2;
	wcmd = malloc(len);
	if (snprintf(wcmd, len, "%c%s\n", interactive, cmd) != (len - 1))
		errx(1, "snprintf");

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		err(1, "socket");

	sun.sun_family = AF_UNIX;
        if (strlcpy(sun.sun_path, rp_glob_screen.control_socket_path,
	    sizeof(sun.sun_path)) >= sizeof(sun.sun_path))
                err(1, "control socket path too long: %s",
		    rp_glob_screen.control_socket_path);

	if (connect(fd, (struct sockaddr *)&sun, sizeof(sun)) == -1)
		err(1, "failed to connect to control socket at %s",
		    rp_glob_screen.control_socket_path);

	if (write(fd, wcmd, len) != len)
		err(1, "short write to control socket");

	free(wcmd);

	len = read(fd, &ret, sizeof(ret) - 1);
	if (len > 2) {
		ret[len - 1] = '\0';
		fprintf(stderr, "%s\n", &ret[1]);
	}

	return ret[0];
}

void
receive_command(void)
{
	cmdret *cmd_ret;
	char cmd[1024] = { 0 }, c;
	char *result, *rcmd;
	int cl, len = 0, interactive = 0;

	PRINT_DEBUG(("have connection waiting on command socket\n"));

	if ((cl = accept(rp_glob_screen.control_socket_fd, NULL, NULL)) == -1) {
		warn("accept");
		return;
	}

	while (len <= sizeof(cmd)) {
		if (len == sizeof(cmd)) {
			warn("%s: bogus command length", __func__);
			close(cl);
			return;
		}

		if (read(cl, &c, 1) == 1) {
			if (c == '\n') {
				cmd[len++] = '\0';
				break;
			}
			cmd[len++] = c;
		} else if (errno != EAGAIN) {
			PRINT_DEBUG(("bad read result on control socket: %s\n",
			    strerror(errno)));
			break;
		}
	}

	interactive = cmd[0];
	rcmd = cmd + 1;

	PRINT_DEBUG(("read %d byte(s) on command socket: %s\n", len, rcmd));

	cmd_ret = command(interactive, rcmd);

	/* notify the client of any text that was returned by the command */
	len = 2;
	if (cmd_ret->output) {
		result = xsprintf("%c%s\n", cmd_ret->success ? 1 : 0,
		    cmd_ret->output);
		len = 1 + strlen(cmd_ret->output) + 1;
	} else if (cmd_ret->success)
		result = xsprintf("%c\n", 1);
	else
		result = xsprintf("%c\n", 0);

	cmdret_free(cmd_ret);

	PRINT_DEBUG(("writing back %d to command client: %s", len, result + 1));

	write(cl, result, len);
	close(cl);
}
