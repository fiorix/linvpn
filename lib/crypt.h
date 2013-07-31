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
 
   $Id: crypt.h 352 2005-12-12 16:53:19Z alec $
*/

#ifndef _CRYPT_H
#define _CRYPT_H

#include <gcrypt.h>

/* generic crypt data */
typedef struct {
    uint16_t algo;
    uint16_t rndsend;
    uint16_t rndrecv;
    gcry_cipher_hd_t hsrc;
    gcry_cipher_hd_t hdst;
    uint16_t keylen;
    uint16_t blklen;
} vpn_crypt_t;

/* initialize gcrypt with src and dst keys */
extern int vpn_cryptinit(vpn_crypt_t *dst, uint16_t algo,
        const char *srckey, const char *dstkey);

/* finish gcrypt - close handlers */
extern void vpn_cryptfinish(vpn_crypt_t *cfg);

#endif /* crypt.h */
