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
 
   $Id: child.c 373 2005-12-16 10:01:19Z alec $
*/

#include "server.h"

/* just quit */
static void finish(int sig) { _exit(0); }

/* close and shutdown */
int ifup = 0;
vpn_conf_t *my_ptr = NULL;
vpn_crypt_t cry;
static void sock_finish(int fd);

/* check VPN name and set non-blocking socket */
static int check_vpn(int fd, vpn_conf_t *my);

/* handle client */
void client(int fd)
{
    fd_set fds;
    vpn_conf_t my;
    int r, ppp = 0, ctrl = 0;
    char temp[VPN_PACKET];

    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP,  SIG_IGN);
    signal(SIGINT,  finish);
    signal(SIGTERM, finish);
    signal(SIGKILL, finish);
    signal(SIGSEGV, finish);

    if(check_vpn(fd, &my) == -1) {
        sock_finish(fd);
        _exit(1);
    } else
        my_ptr = &my;

    if((ctrl = create_ctrl(my.name)) == -1) {
        sock_finish(fd);
        _exit(1);
    }

    if(vpn_cryptinit(&cry, my.algo, my.srckey, my.dstkey) == -1) {
        close(ctrl);
        sock_finish(fd);
        vpn_cryptfinish(&cry);
        _exit(1);
    }

    if((ppp = run_pppd(my.ppplocal, my.pppremote, 0)) == -1) {
        xmsg(0, VPN_INFO, "unable to start pppd\n");
        close(ctrl);
        sock_finish(fd);
        vpn_cryptfinish(&cry);
        _exit(1);
    }

    for(;;) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        FD_SET(ppp, &fds);
        FD_SET(ctrl, &fds);

        if(select(ppp+1, &fds, 0, 0, 0) >= 1) {
            memset(temp, 0, sizeof(temp));
            if(FD_ISSET(fd, &fds)) {
                if((r = server_recv(fd, &cry, temp, sizeof(temp))) >= 1)
                    write(ppp, temp, r);
                else
                    break; /* invalid packet or peer disconnected */
            } else
            if(FD_ISSET(ppp, &fds)) {
                r = read(ppp, temp, sizeof(temp));
                if(r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
                    continue;
                else
                if(r <= 0) {
                    xmsg(0, VPN_DEBUG|VPN_INFO, "lost pppd connection\n");
                    break;
                }

                server_send(fd, &cry, temp, r);
            } else
            if(FD_ISSET(ctrl, &fds)) {
                r = recv(ctrl, temp, sizeof(temp), 0);
                if(*temp == '!') {
                    destroy_ctrl(my.name);
                    xmsg(0, VPN_DEBUG|VPN_INFO, "killed by user request\n");
                    break;
                }
            }

            if(!ifup) {
                if(!run_cmd(my.cmdup, my.ppplocal)) ifup = 1;
            }
        } else
            break;
    }

    if(ppp) close(ppp);
    if(ctrl) close(ctrl);
    vpn_cryptfinish(&cry);
    sock_finish(fd);
}

static void sock_finish(int fd)
{
    if(my_ptr) {
        if(ifup) run_cmd(my_ptr->cmddown, NULL);
        destroy_ctrl(my_ptr->name);
    }

    shutdown(fd, 2);
    close(fd);
    _exit(0);
}

static int check_vpn(int fd, vpn_conf_t *my)
{
    fd_set fds;
    struct timeval tv;
    socklen_t len;
    struct sockaddr_in s;
    char temp[128];
 
    /* set O_NONBLOCK, KEEPALIVE and TCP_NODELAY */
    sockattr(fd);

    /* get client's IP address */
    len = sizeof(s);
    memset(&s, 0, len);
    if(getpeername(fd, (struct sockaddr *)&s, &len) == -1)
        return xmsg(-1, VPN_DEBUG|VPN_INFO, "getpeername: %s\n", errstr);

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if(select(fd+1, &fds, 0, 0, &tv) <= 0)
        return xmsg(-1, VPN_DEBUG|VPN_INFO,
                "warning: client %s connected but didn't asked for a VPN\n",
                inet_ntoa(s.sin_addr));

    memset(temp, 0, sizeof(temp));
    if(recv(fd, temp, sizeof(temp), 0) == -1)
        return xmsg(-1, VPN_DEBUG|VPN_INFO,
                "warning: client %s timed-out\n", inet_ntoa(s.sin_addr));

    if(!vpn_getconf(my, fixstr(temp)))
        return xmsg(-1, VPN_DEBUG|VPN_INFO,
                "client %s requested unknown VPN %s\n",
                inet_ntoa(s.sin_addr), temp);
    else
        xmsg(0, VPN_DEBUG|VPN_INFO,
                "client %s requested VPN %s\n",
                inet_ntoa(s.sin_addr), temp);

    return 0;
}
