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
 
   $Id: crypt.c 373 2005-12-16 10:01:19Z alec $
*/

#include "linvpn.h"

typedef enum {
    VPN_CRYPT_SRC = 0x01,
    VPN_CRYPT_DST = 0x02,
} vpn_crypt_key_t;

/* initialize gcrypt handlers and keys */
static int init_gcrypt(vpn_crypt_t *cfg, vpn_crypt_key_t k, const char *key);

/* initialize gcrypt using init_gcrypt() */
int vpn_cryptinit(vpn_crypt_t *dst, uint16_t algo, 
        const char *srckey, const char *dstkey)
{
    memset(dst, 0, sizeof(vpn_crypt_t));

    dst->algo = algo;

    if(!gcry_check_version(GCRYPT_VERSION))
        return xmsg(-1, VPN_DEBUG|VPN_INFO, 
                "error: gcrypt version mismatch\n");

    gcry_control(GCRYCTL_DISABLE_SECMEM, 0);
    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

    if(init_gcrypt(dst, VPN_CRYPT_SRC, srckey) == -1)
        return xmsg(-1, VPN_TERM|VPN_INFO, 
                "unable to initialize srckey: invalid key\n");

    if(init_gcrypt(dst, VPN_CRYPT_DST, dstkey) == -1)
        return xmsg(-1, VPN_TERM|VPN_INFO, 
                "unable to initialize dstkey: invalid key\n");

    xmsg(0, VPN_DEBUG, "crypto keylen=%d, blklen=%d\n",
            dst->keylen, dst->blklen);

    return 0;
}

/* close gcrypt handlers */
void vpn_cryptfinish(vpn_crypt_t *cfg)
{
    if(!cfg) return;
    
    if(cfg->hsrc) {
        gcry_cipher_close(cfg->hsrc);
        cfg->hsrc = NULL;
    }

    if(cfg->hdst) {
        gcry_cipher_close(cfg->hdst);
        cfg->hdst = NULL;
    }
}

/* initialize gcrypt handlers and keys */
static int init_gcrypt(vpn_crypt_t *cfg, vpn_crypt_key_t k, const char *key)
{
    char temp[128];
    gcry_error_t err;
    gcry_cipher_hd_t hd;
    int algo = cfg->algo, keylen, blklen;

    err = gcry_cipher_open(&hd, algo, 
            GCRY_CIPHER_MODE_CBC, GCRY_CIPHER_CBC_MAC);
    if(!hd)
        return xmsg(-1, VPN_DEBUG, "gcry_cipher_open() failed: %s\n",
                gpg_strerror(err));
    
    keylen = gcry_cipher_get_algo_keylen(algo);
    if(!keylen) {
        gcry_cipher_close(hd);
        return xmsg(-1, VPN_DEBUG, "gcry_cipher_get_algo_keylen() failed!\n");
    }

    blklen = gcry_cipher_get_algo_blklen(algo);
    if(!blklen) {
        gcry_cipher_close(hd);
        return xmsg(-1, VPN_DEBUG, "gcry_cipher_get_algo_blklen() failed!\n");
    }

    if(hstrtoa(temp, sizeof(temp), key) == -1) {
        gcry_cipher_close(hd);
        return -1;
    }

    err = gcry_cipher_setkey(hd, temp, keylen);
    if(err) {
        gcry_cipher_close(hd);
        return xmsg(-1, VPN_DEBUG, "gcry_cipher_setkey() failed: %s\n",
                gpg_strerror(err));
    }

    switch(k) {
        case VPN_CRYPT_SRC:
            cfg->hsrc = hd;
            break;
        case VPN_CRYPT_DST:
            cfg->hdst = hd;
            break;
    }

    if(!cfg->keylen) cfg->keylen = keylen;
    else if(cfg->keylen != keylen) {
        gcry_cipher_close(hd);
        return xmsg(-1, VPN_DEBUG, "src and dst keys differs in keylen!\n");
    }

    if(!cfg->blklen) cfg->blklen = blklen;
    else if(cfg->blklen != blklen) {
        gcry_cipher_close(hd);
        return xmsg(-1, VPN_DEBUG, "src and dst keys differs in blklen!\n");
    }

    return 0;
}
