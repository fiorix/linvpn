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
 
   $Id: server.h 352 2005-12-12 16:53:19Z alec $
*/

#ifndef _SERVER_H
#define _SERVER_H

#include "linvpn.h"
#include <dirent.h>
#include <sys/un.h>

/* network.c: server's main loop */
extern int run_server(uint16_t port, struct in_addr bind_addr);

/* child.c: place to handle each client */
extern void client(int fd);

/* unix.c: control unix sockets of childs */
extern int  create_ctrl(const char *vpn_name);
extern int  destroy_ctrl(const char *vpn_name);
extern int  kill_ctrl(const char *vpn_name);
extern void list_ctrl(int destroy_all);

#endif /* server.h */
