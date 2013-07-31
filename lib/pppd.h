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
 
   $Id: pppd.h 352 2005-12-12 16:53:19Z alec $
*/

#ifndef _PPPD_H
#define _PPPD_H

/* run `pppd' and returns a file descriptor to send and recv data */
extern int run_pppd(const char *localaddr, const char *remoteaddr, pid_t *pid);

/* run `cmd' when device with `ipaddr' is available */
extern int run_cmd(const char *cmd, const char *ipaddr);

#endif /* pppd.h */
