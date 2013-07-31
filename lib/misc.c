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
 
   $Id: misc.c 406 2005-12-23 09:38:32Z alec $
*/

#include "linvpn.h"
#include <pwd.h>
#include <sys/ioctl.h>

/* print messages on terminal, as VPN_INFO or VPN_DEBUG
 * and syslog as VPN_INFO */
int xmsg(int retval, vpn_msg_t type, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if(type & VPN_TERM) vfprintf(stderr, fmt, ap);
    
    if(type & VPN_INFO && !getenv("DEBUG")) 
        vsyslog(LOG_INFO, fmt, ap);

    if(type & VPN_DEBUG && getenv("DEBUG"))
        vfprintf(stderr, fmt, ap);

    va_end(ap);

    return retval;
}

/* validate IP addresses */
int checkip(const char *addr)
{
    int r;
    struct in_addr in;

    r = inet_aton(addr, &in);
    if(!r)
        return xmsg(-1, VPN_INFO|VPN_TERM, "invalid ip address: %s\n", addr);

    return 0;
}

/* DNS resolver */
int resolv(char *dst, size_t dstlen, const char *addr)
{
    struct hostent *he = gethostbyname(addr);

    if(!he)
        return xmsg(-1, VPN_INFO|VPN_TERM, "cannot resolv %s: %s\n",
                addr, hstrerror(h_errno));

    if(dst) {
        struct in_addr *in = (struct in_addr *) he->h_addr;

        memset(dst, 0, dstlen);
        strncpy(dst, inet_ntoa(*in), dstlen);
    }

    return 0;
}

/* change '\r' or '\n' to '\0' */
char *fixstr(char *string)
{
    char *p;

    for(p = string; *p != '\0'; p++)
        if(*p == '\r' || *p == '\n') {
            *p = '\0';
            break;
        }

    return string;
}

/* set O_NONBLOCK, KEEPALIVE and TCP_NODELAY */
void sockattr(int fd)
{
    /* set non-blocking, KEEPALIVE and TCP_NODELAY */
    int flags, opt = 1;

    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
    setsockopt(fd, SOL_SOCKET, SO_OOBINLINE, &opt, sizeof(opt));
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

    /* set snd and rcv buffers */
    opt = VPN_BIGPACKET;
    if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) == -1)
        xmsg(0, VPN_DEBUG, "cannot set SO_SNDBUF=%d: %s\n", opt, errstr);

    opt = VPN_BIGPACKET;
    if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) == -1)
        xmsg(0, VPN_DEBUG, "cannot set SO_RCVBUF=%d: %s\n", opt, errstr);
}

/* convert hex string to ASCII */
int hstrtoa(char *dst, size_t dstlen, const char *hexstr)
{
    int i = 0, keylen = strlen(hexstr) - 2;
    char *p = dst, *s = (char *) hexstr + 2, real;

    /* check `hexstr' size */
    if(keylen < 4) 
        return xmsg(-1, VPN_DEBUG, "invalid key size: %d\n", keylen);

    if(keylen % 2)
        return xmsg(-1, VPN_DEBUG, "key is not a multiple of 2!\n");
    else
    if((keylen / 2) >= dstlen)
        return xmsg(-1, VPN_DEBUG, "key is bigger than `dst'!\n");
    
    memset(dst, 0, dstlen);

    while(*s != '\0') {
        /* if(!isxdigit(*s)) return -1;
         * it will always be an hex digit because of the parser */

        if(isdigit(*s))
            real = *s - '0';
        else
            real = toupper(*s) - 'A' + 10;

        if(!i)
            *p = real << 4;
        else
            *p++ |= real;

        i = !i;
        s++;
    }

    return 0;
}

int getifname(char *dst, size_t dstlen, const char *ipaddr)
{
    int i, fd;
    struct ifreq ifr;
    struct in_addr ip;
    struct sockaddr_in *in;

    if(!inet_aton(ipaddr, &ip)) return -1;
    else
    if((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        return -1;

    for(i = 0; i < VPN_CONF_MAX_ENTRIES; i++) {
        memset(&ifr, 0, sizeof(ifr));
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "ppp%d", i);
        if(ioctl(fd, SIOCGIFADDR, &ifr) == -1)
            continue;

        in = (struct sockaddr_in *) &ifr.ifr_addr;
        if(in->sin_addr.s_addr != ip.s_addr)
            continue;
        else {
            if(dst) strncpy(dst, ifr.ifr_name, dstlen);
            close(fd);
            return 0;
        }
    }

    return -1;
}

void setperm(vpn_perm_t perm)
{
    static uid_t euid;
    static int initialized = 0;

    if(!initialized) {
        struct passwd *pw;
        char *user = getenv("VPN_USERNAME");
        if(!user) user = VPN_USERNAME;

        if((pw = getpwnam(user)) == NULL) {
            xmsg(0, VPN_TERM|VPN_INFO, 
                "unable to get information of user %s, using root\n", user);
            euid = 0;
        } else
            euid = pw->pw_uid;

        initialized = 1;
    }

    switch(perm) {
        case VPN_USER:
            xmsg(0, VPN_DEBUG, "setperm to euid %d\n", euid);
            seteuid(euid);
            break;
        case VPN_ROOT:
            xmsg(0, VPN_DEBUG, "setperm to root\n");
            seteuid(0);
            break;
    }
}
