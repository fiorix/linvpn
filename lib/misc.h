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
 
   $Id: misc.h 352 2005-12-12 16:53:19Z alec $
*/

#ifndef _MISC_H
#define _MISC_H

/* print error messages and always returns retval */
typedef enum {
    VPN_INFO  = 0x10,
    VPN_TERM  = 0x20,
    VPN_DEBUG = 0x40,
} vpn_msg_t;
extern int xmsg(int retval, vpn_msg_t type, const char *fmt, ...);

/* check ip address */
extern int checkip(const char *addr);

/* resolv hostname */
extern int resolv(char *dst, size_t dstlen, const char *addr);

/* remove \r or \n from strings */
extern char *fixstr(char *string);

/* set linvpn socket options */
extern void sockattr(int fd);

/* convert hex string to ASCII string */
extern int hstrtoa(char *dst, size_t dstlen, const char *hexstr);

/* get interface name of `ipaddr' */
extern int getifname(char *dst, size_t dstlen, const char *ipaddr);

/* set euid */
typedef enum {
    VPN_USER,
    VPN_ROOT,
} vpn_perm_t;
extern void setperm(vpn_perm_t perm);

#endif /* misc.h */
