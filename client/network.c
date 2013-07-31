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
 
   $Id: network.c 373 2005-12-16 10:01:19Z alec $
*/

#include "client.h"

vpn_conf_t *my_ptr = NULL;
static int ifup = 0;
static pid_t pppd_pid = 0;
static vpn_crypt_t cry;

/* exit */
static void mysig(int sig)
{
    xmsg(0, VPN_DEBUG|VPN_INFO, "killed by signal %d\n", sig);
    
    /* kill pppd */
    if(pppd_pid) kill(pppd_pid, SIGTERM);

    /* run command */
    if(my_ptr && ifup)
        run_cmd(my_ptr->cmddown, NULL);

    vpn_cryptfinish(&cry);

    exit(0);
}

/* save pidfile */
void save_pidfile(const char *suffix)
{
    FILE *fp;
    char pidfile[1024];
    
    memset(pidfile, 0, sizeof(pidfile));
    snprintf(pidfile, sizeof(pidfile), "/var/run/linvpn-%s.pid", suffix);

    setperm(VPN_ROOT);
    if((fp = fopen(pidfile, "w")) == NULL) {
        xmsg(0, VPN_DEBUG|VPN_INFO, 
                "unable to create pidfile %s: %s\n", pidfile, errstr);
    } else {
        fprintf(fp, "%d", getpid());
        fclose(fp);
    }

    setperm(VPN_USER);
}

int run_client(uint16_t port, vpn_conf_t *my)
{
    fd_set fds;
    int r, fd = 0, ppp = 0;
    struct in_addr out;
    struct sockaddr_in s;
    char temp[VPN_PACKET];
    
    if(vpn_cryptinit(&cry, my->algo, my->srckey, my->dstkey) == -1)
        return -1;

    signal(SIGHUP,  SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        xmsg(0, VPN_DEBUG|VPN_INFO, "socket: %s\n", errstr);
        vpn_cryptfinish(&cry);
        exit(1);
    }

    if(!inet_aton(my->dstaddr, &out)) {
        xmsg(0, VPN_DEBUG|VPN_INFO, "invalid IP address: %s\n", my->dstaddr);
        vpn_cryptfinish(&cry);
        exit(1);
    }

    /* connect */
    s.sin_family = AF_INET;
    s.sin_port   = port;
    s.sin_addr   = out;
    if(connect(fd, (struct sockaddr *)&s, sizeof(s)) == -1) {
        close(fd);
        vpn_cryptfinish(&cry);
        return xmsg(-1, VPN_DEBUG|VPN_INFO, 
                "connect (%s): %s\n", my->name, errstr);
    }

    /* set O_NONBLOCK, KEEPALIVE and TCP_NODELAY */
    sockattr(fd);
    
    /* send VPN name */
    memset(temp, 0, sizeof(temp));
    snprintf(temp, sizeof(temp), "%s\n", my->name);
    send(fd, temp, strlen(temp), 0);

    /* xmsg(0, VPN_TERM, "connected to %s, running pppd\n", my->name); */

    /* run pppd */
    if((ppp = run_pppd(my->ppplocal, my->pppremote, &pppd_pid)) == -1) {
        close(fd);
        vpn_cryptfinish(&cry);
        return xmsg(-1, VPN_DEBUG|VPN_INFO, "unable to start pppd\n");
    } else
        xmsg(0, VPN_INFO, "connected to %s\n", my->name);

    /* save pidfile */
    save_pidfile(my->name);
    
    /* set signals */
    signal(SIGINT,  mysig);
    signal(SIGTERM, mysig);
    signal(SIGKILL, mysig);
    signal(SIGSEGV, mysig);

    for(;;) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        FD_SET(ppp, &fds);

        if(select(ppp+1, &fds, 0, 0, 0) >= 1) {
            memset(temp, 0, sizeof(temp));
            if(FD_ISSET(fd, &fds)) {
                if((r = client_recv(fd, &cry, temp, sizeof(temp))) >= 1)
                    write(ppp, temp, r);
                else {
                    /* received invalid packet or peer disconnected 
                     * kill pppd */
                    kill(pppd_pid, SIGTERM);
                    break;
                }
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

                client_send(fd, &cry, temp, r);
            }
        } else
            break;

        /* run `up' command */
        if(!ifup) {
            if(!run_cmd(my->cmdup, my->ppplocal)) {
                my_ptr = my;
                ifup = 1;
            }
        }
    }

    if(fd) close(fd);
    if(ppp) close(ppp);
    vpn_cryptfinish(&cry);

    /* run downcmd */
    if(ifup) {
        run_cmd(my->cmddown, NULL);
        ifup = 0;
    }

    return 0;
}
