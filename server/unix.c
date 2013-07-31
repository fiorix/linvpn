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
 
   $Id: unix.c 352 2005-12-12 16:53:19Z alec $
*/

#include "server.h"

const char *prefix_dir = "/tmp/";
const char *prefix_nam = "vpn.";

static int name_ctrl(struct sockaddr_un *dst, const char *vpn_name)
{
    int len;
    
    if(!dst) return -1;

    len = sizeof(dst->sun_path) - (strlen(prefix_dir) + strlen(prefix_nam));

    if(strlen(vpn_name) > len)
        xmsg(0, VPN_TERM|VPN_INFO,
                "name `%s\' is bigger than %d, may conflict with other\n",
                vpn_name, len);

    memset(dst, 0, sizeof(struct sockaddr_un));
    dst->sun_family = AF_UNIX;
    snprintf(dst->sun_path, sizeof(dst->sun_path), "%s%s%s", 
            prefix_dir, prefix_nam, vpn_name);

    return 0;
}

int create_ctrl(const char *vpn_name)
{
    int fd, opts;
    struct sockaddr_un s;

    name_ctrl(&s, vpn_name);
    
    if((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
        return xmsg(-1, VPN_DEBUG|VPN_INFO,
                "cannot create ctrl socket: %s\n", errstr);

    if(bind(fd, (struct sockaddr *)&s, sizeof(s)) == -1)
        return xmsg(-1, VPN_DEBUG|VPN_INFO,
                "cannot bind ctrl socket(%s): %s\n", s.sun_path, errstr);
    
    /* make non-blocking socket */
    opts = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, opts | O_NONBLOCK);

    return fd;
}

int destroy_ctrl(const char *vpn_name)
{
    struct sockaddr_un s;

    name_ctrl(&s, vpn_name);
    return unlink(s.sun_path);
}

int kill_ctrl(const char *vpn_name)
{
    int fd;
    struct sockaddr_un s;

    name_ctrl(&s, vpn_name);

    if((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
        return xmsg(-1, VPN_TERM,
                "cannot create ctrl socket: %s\n", errstr);

    if(sendto(fd, "!\n", 2, 0, (struct sockaddr *)&s, sizeof(s)) == -1)
        return xmsg(-1, VPN_TERM,
                "cannot send KILL cmd to %s: %s\n", vpn_name, errstr);

    return 0;
}

void list_ctrl(int destroy_all)
{
    int n;
    char *p;
    struct dirent **nl;

    if((n = scandir(prefix_dir, &nl, 0, alphasort)) == -1) {
        xmsg(0, VPN_TERM, "cannot scan %s: %s\n", prefix_dir, errstr);
        return;
    }

    if(destroy_all) chdir(prefix_dir);

    while(n--) {
        p = nl[n]->d_name;
        if(!strncmp(prefix_nam, p, strlen(prefix_nam))) {
            if(strlen(p) > strlen(prefix_nam)) {
                if(destroy_all)
                    unlink(p);
                else {
                    p += strlen(prefix_nam);
                    xmsg(0, VPN_TERM, "-> %s\n", p);
                }
            }
        }

        free(nl[n]);
    }

    free(nl);
}
