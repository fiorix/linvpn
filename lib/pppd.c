/* linvpn 3.0
   Copyright (C) 1984-2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  
 
   $Id: pppd.c 355 2005-12-13 10:12:11Z alec $
*/

#include "linvpn.h"

int run_pppd(const char *localaddr, const char *remoteaddr, pid_t *pid)
{
    pid_t xpid;
    int fdm, fds, opts;
    char addr[128], pack[128];

    /* open a new pty */
    if(openpty(&fdm, &fds, NULL, NULL, NULL) == -1)
        return xmsg(-1, VPN_DEBUG, "openpty: %s\n", errstr);

    signal(SIGCHLD, SIG_IGN);

    memset(addr, 0, sizeof(addr));
    snprintf(addr, sizeof(addr), "%s:%s", localaddr, remoteaddr);

    memset(pack, 0, sizeof(pack));
    snprintf(pack, sizeof(pack), "%d", VPN_PACKET);

    /* run pppd */
    switch((xpid = fork())) {
        case -1:
            return xmsg(-1, VPN_DEBUG|VPN_INFO, "fork: %s\n", errstr);
        case 0:
            close(fdm);
            setperm(VPN_ROOT);
            /*
            dup2(fds, 0);
            dup2(fds, 1);
            dup2(fds, 2);
            */
            login_tty(fds);
            execl("/usr/sbin/pppd", "pppd", "noauth", "nodetach", "proxyarp",
                    "mru", pack, "mtu", pack, addr, NULL);
            xmsg(0, VPN_DEBUG|VPN_INFO, "exec pppd: %s\n", errstr);
            _exit(1);
    }

    /* set non-blocking descriptor */
    close(fds);
    opts = fcntl(fdm, F_GETFL, 0);
    fcntl(fdm, F_SETFL, opts | O_NONBLOCK);

    if(pid) *pid = xpid;

    return fdm;
}

int run_cmd(const char *cmd, const char *ipaddr)
{
    char device[128],
         mycmd[4096], *p = mycmd, err[sizeof(mycmd)],
         *arr[(sizeof(mycmd)/2)+2], **arrp = arr;

    if(!cmd) return xmsg(-1, VPN_DEBUG, "no command to run!\n");
    else
    if(!*cmd) return xmsg(0, VPN_DEBUG, "command not set\n");

    /* check cmd size */
    if(strlen(cmd) > sizeof(mycmd))
        return xmsg(-1, VPN_DEBUG|VPN_INFO, 
                "command too big(max=%d): %s\n", cmd, sizeof(mycmd));

    /* check interface only when there is an ip address */
    if(ipaddr) {
        memset(device, 0, sizeof(device));
        if(getifname(device, sizeof(device), ipaddr) == -1)
            return -1;
    }

    /* make a copy */
    memset(mycmd, 0, sizeof(mycmd));
    strncpy(mycmd, cmd, sizeof(mycmd));

    /* check quotes */
    if(*p == '\"') p++;
    if(p[strlen(p)-1] == '\"') p[strlen(p)-1] = '\0';

    memset(err, 0, sizeof(err));
    strncpy(err, p, sizeof(err));

    xmsg(0, VPN_DEBUG, "running command: %s\n", p);

    /* parse into array */
    while(*p != '\0') {
        while(*p == ' ' && *p != '\0') *p++ = '\0';
        *arrp++ = p;
        while(*p != ' ' && *p != '\0') p++;
    }

    *arrp++ = ipaddr ? device : NULL;
    *arrp = NULL;

    /* run `cmd' */
    switch(fork()) {
        case -1:
            return xmsg(-1, VPN_DEBUG|VPN_INFO, "fork: %s\n", errstr);
        case 0:
            setperm(VPN_ROOT);
            execvp(*arr, arr);
            xmsg(0, VPN_DEBUG|VPN_INFO, "exec cmd (%s): %s\n", err, errstr);
            _exit(1);
    }

    return 0;
}
