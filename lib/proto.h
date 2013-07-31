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
 
   $Id: proto.h 352 2005-12-12 16:53:19Z alec $
*/

#ifndef _PROTO_H
#define _PROTO_H

#define VPN_BIGPACKET VPN_PACKET*2

typedef enum {
    VPN_CLIENT = 0x01,
    VPN_SERVER = 0x02,
} vpn_proto_t;

extern int vpn_send(int fd, vpn_crypt_t *cry, vpn_proto_t type, 
        const void *buf, size_t buflen);
extern int vpn_recv(int fd, vpn_crypt_t *cry, vpn_proto_t type,
        void *buf, size_t buflen);

#define server_send(fd, cry, buf, buflen) \
    vpn_send(fd, cry, VPN_SERVER, buf, buflen)

#define server_recv(fd, cry, buf, buflen) \
    vpn_recv(fd, cry, VPN_CLIENT, buf, buflen)

#define client_send(fd, cry, buf, buflen) \
    vpn_send(fd, cry, VPN_CLIENT, buf, buflen)

#define client_recv(fd, cry, buf, buflen) \
    vpn_recv(fd, cry, VPN_SERVER, buf, buflen)

#endif /* proto.h */
