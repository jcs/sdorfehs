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
#include <stdlib.h>

#include "sdorfehs.h"

#define BUFSZ 1024

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
	    SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)) == -1)
		err(1, "socket");

	if (strlen(rp_glob_screen.control_socket_path) >= sizeof(sun.sun_path))
		err(1, "control socket path too long: %s",
		    rp_glob_screen.control_socket_path);

	strncpy(sun.sun_path, rp_glob_screen.control_socket_path,
	    sizeof(sun.sun_path)-1);
	sun.sun_path[sizeof(sun.sun_path) - 1] = '\0';
	sun.sun_family = AF_UNIX;

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

static ssize_t
recv_unix(int fd, char **callerbuf)
{
	int firstloop;
	char *message;
	ssize_t len, count;

	int flags = 0x0;
	int retries = 0;

#ifdef SENDCMD_DEBUG
	pid_t pid = getpid();
	char *dpfx = xsprintf("recv_unix_%d", pid);
#endif
	WARNX_DEBUG("%s: enter\n", dpfx);

	message = xmalloc(BUFSZ);
	memset(message, 0, BUFSZ);

	len = 0;
	firstloop = 1;

	while ((count = recv(fd, message + len, BUFSZ, flags))) {
		if (count == -1) {
			switch (errno) {
			/*
			 * message is complete
			 */
			case ECONNRESET: /* sender finished and closed */
			case EAGAIN:     /* no more left to read */
				WARNX_DEBUG("%s: done: e%d\n", dpfx, errno);
				break;
			/*
			 * transient conditions for retrying
			 */
			case ECONNREFUSED:
			case ENOMEM:
				if (retries++ >= 5) {
					warn("recv_unix: retries exhausted");
					len = -1;
					break;
				}
				usleep(200);
				/* fallthrough */
			case EINTR:
				warn("recv_unix: trying again");
				continue;
			/*
			 * usage error or untenable situation:
			 * EBADF, EFAULT, EINVAL, ENOTCONN, ENOTSOCK
			 */
			default:
				warn("unanticipated receive error");
				len = -1;
			}
			break;
		}
		if (firstloop) {
			WARNX_DEBUG("%s: first recv: %zd\n", dpfx, count);
			/*
			 * after blocking for the first buffer, we can
			 * keep reading until it blocks again, which
			 * should exhaust the message.
			 */
			flags += MSG_DONTWAIT;
		}
		len += count;
		message = xrealloc(message, len + BUFSZ);
		memset(message + len, 0, BUFSZ);
		WARNX_DEBUG("%s: looping after count %zd\n", dpfx, count);
		firstloop = 0;
	}
#ifdef SENDCMD_DEBUG
	free(dpfx);
#endif
	*callerbuf = message;
	return len;
}

static ssize_t
send_unix(int fd, char *buf, ssize_t sz)
{
	ssize_t ret, off = 0;

	WARNX_DEBUG("entered send_unix with sz %zd\n", sz);

	while (sz > 0) {
		if (((ret = write(fd, buf + off, sz)) != sz) && ret == -1) {
			if (errno == EINTR)
				continue;
			warn("send_unix: bad write");
			break;
		}
		sz -= ret;
		off += ret;
	}

	WARNX_DEBUG("leaving send_unix, off %zd, errno %d\n", off, errno);
	return off;
}

int
send_command(int interactive, char *cmd)
{
	struct sockaddr_un sun;
	char *wcmd, *response;
	char success = 0;
	ssize_t len;
	int fd;
	FILE *outf = NULL;

#ifdef SENDCMD_DEBUG
	pid_t pid = getpid();
	char *dpfx = xsprintf("send_command_%d", pid);
#endif
	WARNX_DEBUG("%s: enter\n", dpfx);

	len = 1 + strlen(cmd) + 1;
	wcmd = xmalloc(len);
	*wcmd = (unsigned char)interactive;
	strncpy(wcmd + 1, cmd, len - 1);

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		err(1, "socket");

	if (strlen(rp_glob_screen.control_socket_path) >= sizeof(sun.sun_path))
		err(1, "control socket path too long: %s",
		    rp_glob_screen.control_socket_path);

	strncpy(sun.sun_path, rp_glob_screen.control_socket_path,
	    sizeof(sun.sun_path)-1);
	sun.sun_path[sizeof(sun.sun_path) - 1] = '\0';
	sun.sun_family = AF_UNIX;

	if (connect(fd, (struct sockaddr *)&sun, sizeof(sun)) == -1)
		err(1, "failed to connect to control socket at %s",
		    rp_glob_screen.control_socket_path);

	if (send_unix(fd, wcmd, len) != len)
		err(1, "%s: aborting after bad write", __func__);

	free(wcmd);

	if ((len = recv_unix(fd, &response)) == -1)
		warnx("send_message: aborted reply from receiver");

	/* first byte is exit status */
	success = *response;
	outf = success ? stdout : stderr;

	if (len > 1) {
		/* command had some output */
		if (response[len] != '\0') {
			/* should not be possible, TODO remove */
			warnx("%s\n", "last byte of response not null");
			response[len] = '\0';
		}
		fprintf(outf, "%s", response + 1);
		fflush(outf);
	}
	free(response);

	WARNX_DEBUG("%s: no more bytes\n", dpfx);
#ifdef SENDCMD_DEBUG
	free(dpfx);
#endif

	return success;
}

void
receive_command(void)
{
	cmdret *cmd_ret;
	char *result, *rcmd, *cmd;
	int cl, len = 0, interactive = 0;

	PRINT_DEBUG(("have connection waiting on command socket\n"));

	if ((cl = accept(rp_glob_screen.control_socket_fd, NULL, NULL)) == -1) {
		warn("accept");
		return;
	}
	if ((fcntl(cl, F_SETFD, FD_CLOEXEC)) == -1) {
		warn("cloexec");
		close(cl);
		return;
	}

	if ((len = recv_unix(cl, &cmd)) <= 1) {
		warnx("receive_command: %s\n",
		      (len == -1 ? "encountered error during receive"
		                 : "received command was malformed"));
		goto done;
	}
	if (cmd[len] != '\0') {
		/* should not be possible, TODO remove */
		warnx("%s\n", "last byte of sent command not null");
		cmd[len] = '\0';
	}
	interactive = cmd[0];
	rcmd = cmd + 1;

	PRINT_DEBUG(("read %d byte(s) on command socket: %s\n", len, rcmd));

	cmd_ret = command(interactive, rcmd);

	/* notify the client of any text that was returned by the command */
	len = 2;
	if (cmd_ret->output) {
		result = xsprintf("%c%s", cmd_ret->success ? 1 : 0,
		    cmd_ret->output);
		len = 1 + strlen(cmd_ret->output) + 1;
	} else if (cmd_ret->success)
		result = xsprintf("%c", 1);
	else
		result = xsprintf("%c", 0);

	cmdret_free(cmd_ret);

	PRINT_DEBUG(("writing back %d to command client: %s", len, result + 1));

	if (send_unix(cl, result, len) != len)
		warnx("%s: proceeding after bad write", __func__);

	PRINT_DEBUG(("receive_command: write finished, closing\n"));
done:
	free(cmd);
	close(cl);
}
