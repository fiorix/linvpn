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
 
   $Id: config.h 352 2005-12-12 16:53:19Z alec $
*/

#ifndef _CONFIG_H
#define _CONFIG_H

/* generic configuration data */
typedef struct {
    char name[50];
    char srcaddr[256];
    char srckey[128];
    char dstaddr[256];
    char dstkey[128];
    char ppplocal[16];
    char pppremote[16];
    char cmdup[1024];
    char cmddown[1024];

    int  algo;
} vpn_conf_t;

/* configuration file */
#define VPN_CONF_FILE "/etc/linvpn.conf"

/* max configuration entries */
#define VPN_CONF_MAX_ENTRIES 128

/* read configuration */
extern int vpn_readconf(vpn_conf_t *cfg, size_t cfg_nmemb);

/* check configuration and make a copy in `ret' */
extern int vpn_checkconf(int print, vpn_conf_t **ret, size_t *ret_nmemb);

/* get VPN entry based on its `name'
 * NOTE: this function works ONLY after a call to vpn_checkconf() */
extern int vpn_getconf(vpn_conf_t *dst, const char *name);

#endif /* config.h */
