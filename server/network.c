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
 
   $Id: network.c 352 2005-12-12 16:53:19Z alec $
*/

#include "server.h"

/* exit */
static void mysig(int sig)
{
    xmsg(0, VPN_DEBUG|VPN_INFO, "killed by signal %d\n", sig);
    list_ctrl(1);
    exit(0);
}

/* config */
static void reload(int sig)
{
    xmsg(0, VPN_DEBUG|VPN_INFO, "received HUP signal - reloading...\n");

    if(vpn_checkconf(0, 0, 0) == -1) {
        list_ctrl(1);
        _exit(1);
    }
}

/* save pidfile */
void save_pidfile(void)
{
    FILE *fp;
    const char *pidfile = "/var/run/linvpnd.pid";
    
    setperm(VPN_ROOT);
    if((fp = fopen(pidfile, "w")) == NULL) {
        xmsg(0, VPN_DEBUG|VPN_INFO, 
                "unable to create pidfile %s: %s\n", pidfile, errstr);
        return;
    } else {
        fprintf(fp, "%d", getpid());
        fclose(fp);
    }

    setperm(VPN_USER);
}

int run_server(uint16_t port, struct in_addr bind_addr)
{
    int fd, fdc;
    struct sockaddr_in s;

    /* check configuration */
    if(vpn_checkconf(0, 0, 0) == -1)
        return xmsg(-1, VPN_TERM, "unable to initialize server\n");

    xmsg(0, VPN_DEBUG, "server port is %d and bind address is %s\n",
            ntohs(port), inet_ntoa(bind_addr));

    /* server socket */
    if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return xmsg(-1, VPN_TERM, "socket: %s\n", errstr);
    else {
        int opt = 1;
        if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
            return xmsg(-1, VPN_TERM, "setsockopt: %s\n", errstr);
    }

    /* bind */
    s.sin_family = AF_INET;
    s.sin_port   = port;
    s.sin_addr   = bind_addr;
    if(bind(fd, (struct sockaddr *)&s, sizeof(s)) == -1)
        return xmsg(-1, VPN_TERM, "bind: %s\n", errstr);

    /* listen */
    if(listen(fd, 2) == -1)
        return xmsg(-1, VPN_TERM, "listen: %s\n", errstr);

    /* when using debug mode we don't become daemon */
    if(!getenv("DEBUG")) {
        if(daemon(0, 0) == -1)
            return xmsg(-1, VPN_TERM, "daemon: %s\n", errstr);
    }

    /* save pidfile */
    save_pidfile();

    /* set signals */
    signal(SIGHUP,  reload);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT,  mysig);
    signal(SIGTERM, mysig);
    signal(SIGKILL, mysig);
    signal(SIGSEGV, mysig);

    for(;;) {
        if((fdc = accept(fd, 0, 0)) == -1)
            return xmsg(0, VPN_DEBUG|VPN_INFO, "accept: %s\n", errstr);

        switch(fork()) {
            case -1:
                return xmsg(-1, VPN_DEBUG|VPN_INFO, "fork: %s\n", errstr);
            
            case 0:
                client(fdc);
                break;
        }
    }

    return 0;
}
