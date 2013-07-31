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
 
   $Id: linvpn.h 409 2005-12-26 02:03:04Z alec $
*/

#ifndef _LINVPN_H
#define _LINVPN_H

#define LINVPN_VERSION "3.0"

#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>
#include <ctype.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#if defined(__linux__)
#include <pty.h>
#include <utmp.h>
#include <linux/if.h>
#elif defined(__FreeBSD__)
#include <libutil.h>
#include <sys/sockio.h>
#include <net/if.h>
#elif defined(__OpenBSD__) || defined(__NetBSD__)
#include <util.h>
#include <sys/sockio.h>
#include <net/if.h>
#endif

#include "misc.h"
#include "pppd.h"
#include "crypt.h"
#include "proto.h"
#include "config.h"

/* default TCP port for linvpn client and server */
#define VPN_PORT 1905

/* default packet size */
#define VPN_PACKET 4096

/* default user */
#define VPN_USERNAME "daemon"

/* sizeof() for arrays */
#define vsizeof(arr) sizeof(arr)/sizeof(arr[0])

/* error string */
#define errstr strerror(errno)

#endif /* linvpn.h */
