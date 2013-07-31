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
 
   $Id: proto.c 406 2005-12-23 09:38:32Z alec $
*/

#include "linvpn.h"

typedef struct {
    uint16_t checksum;
    uint16_t pad;
} vpn_hdr_t;

/* encrypt data */
static int xencrypt(gcry_cipher_hd_t hd, int blklen, const char *iv,
        void *dst, size_t dstlen, const void *src, size_t srclen);

/* decrypt data */
static int xdecrypt(gcry_cipher_hd_t hd, int blklen, const char *iv,
        void *dst, size_t dstlen, const void *src, size_t srclen);

/* safe way to send and recv data */
static int safe_send(int fd, const char *buf, size_t buflen);
static int safe_recv(int fd, char *buf, size_t buflen);

/* iv generator */
const uint16_t initial_iv_random = 1980;
static void gen_iv(char *dst, size_t dstlen, uint16_t random);

/* send encrypted packet */
int vpn_send(int fd, vpn_crypt_t *cry, vpn_proto_t type,
        const void *buf, size_t buflen)
{
    vpn_hdr_t hdr;
    gcry_cipher_hd_t hd = NULL;
    char bigpack[VPN_BIGPACKET], encpack[VPN_BIGPACKET], xiv[256], *s;
    int pad = ~((buflen % cry->blklen) - cry->blklen)+1;
    int re, rs, rc, rx;

    switch(type) {
        case VPN_CLIENT:
            hd = cry->hsrc;
            break;
        case VPN_SERVER:
            hd = cry->hdst;
    }

    /* fill header */
    hdr.pad = pad == cry->blklen ? 0 : pad;
    hdr.checksum = buflen + hdr.pad;

    /* check buffer size */
    if(sizeof(bigpack) < hdr.checksum)
        return xmsg(-1, VPN_DEBUG|VPN_INFO, 
                "packet too big to send: %d data bytes + %d header bytes\n", 
                buflen, sizeof(hdr));

    /* copy data to `bigpack' */
    memset(bigpack, 0, sizeof(bigpack));
    memcpy(&bigpack, buf, buflen);

    if(!cry->rndsend) cry->rndsend = initial_iv_random;
    memset(xiv, 0, sizeof(xiv));
    gen_iv(xiv, cry->blklen, cry->rndsend);
    /* xmsg(0, VPN_DEBUG, "send using iv: %s\n", xiv); */

    re = xencrypt(hd, cry->blklen, xiv,
            encpack, sizeof(encpack), bigpack, hdr.checksum);
    if(re == -1 || re != hdr.checksum)
        return xmsg(-1, VPN_DEBUG|VPN_INFO,
                "vpn_send: cannot encrypt packet (%d != %d)\n",
                re, hdr.checksum);

    /* send header */
    rs = send(fd, &hdr, sizeof(hdr), 0);
    if(rs == -1)
        return rs;
    else
    if(!rs)
        return xmsg(0, VPN_DEBUG|VPN_INFO, 
                "vpn_send: lost connection, peer disconnected\n");
    else
    if(rs != sizeof(hdr))
        return xmsg(-1, VPN_DEBUG|VPN_INFO,
                "vpn_send: partial send of header not allowed\n");

    xmsg(0, VPN_DEBUG, "sent %d bytes header, checksum=%d and pad=%d\n", 
            rs, hdr.checksum, hdr.pad);

    /* send data */
    s = encpack;
    rc = re;
    rx = 0;
    do {
        rs = safe_send(fd, s, rc);
        if(rs == -1)
            return rs;
        else
        if(!rs)
            return xmsg(0, VPN_DEBUG|VPN_INFO, 
                    "vpn_send: lost connection, peer disconnected\n");

        xmsg(0, VPN_DEBUG, "sent %d bytes of packet...\n", rs);

        s  += rs;
        rx += rs;
        rc -= rs;
    } while(rx < re);

    cry->rndsend = buflen;
    return buflen;
}

static int xencrypt(gcry_cipher_hd_t hd, int blklen, const char *iv,
        void *dst, size_t dstlen, const void *src, size_t srclen)
{
    int i;
    gcry_error_t err = 0;
    char *s = (char *) src, *d = dst;

    if(dstlen < srclen)
        return xmsg(-1, VPN_DEBUG,
                "not enough space to put encrypted data!\n");

    for(i = 0; i < srclen; i+=blklen, s+=blklen, d+=blklen) {
        err = gcry_cipher_setiv(hd, iv, blklen);
        if(err) {
            xmsg(0, VPN_DEBUG, "encrypt iv mismatch: %s\n", gpg_strerror(err));
            break;
        }

        err = gcry_cipher_encrypt(hd, d, blklen, s, blklen);
        if(err) {
            xmsg(0, VPN_DEBUG, "encrypt failed: %s\n", gpg_strerror(err));
            break;
        }
    }

    return err ? -1 : i;
}

int vpn_recv(int fd, vpn_crypt_t *cry, vpn_proto_t type, 
        void *buf, size_t buflen)
{
    vpn_hdr_t hdr;
    gcry_cipher_hd_t hd = NULL;
    char bigpack[VPN_BIGPACKET], decpack[VPN_BIGPACKET], xiv[256], *s;
    int rr, rd, rc, rx;

    switch(type) {
        case VPN_CLIENT:
            hd = cry->hsrc;
            break;
        case VPN_SERVER:
            hd = cry->hdst;
    }

    memset(bigpack, 0, sizeof(bigpack));
    memset(decpack, 0, sizeof(decpack));

    /* read packet header */
    memset(&hdr, 0, sizeof(hdr));
    rr = recv(fd, &hdr, sizeof(hdr), 0);
    if(rr == -1)
        return rr;
    else
    if(!rr)
        return xmsg(0, VPN_DEBUG|VPN_INFO, 
                "vpn_recv: lost connection, peer disconnected\n");
    else
    if(rr != sizeof(hdr))
        return xmsg(-1, VPN_DEBUG|VPN_INFO, 
                "vpn_recv: partial recv of header not allowed\n");

    xmsg(0, VPN_DEBUG, 
            "received %d bytes header, checksum=%d and pad=%d\n", 
            rr, hdr.checksum, hdr.pad);

    if(hdr.checksum > sizeof(bigpack))
        return xmsg(-1, VPN_DEBUG|VPN_INFO,
                "packet too big: header checksum is %d\n", hdr.checksum);

    /* read packet data */
    s = bigpack;
    rc = hdr.checksum;
    rx = 0;
    do {
        rr = safe_recv(fd, s, rc);
        if(rr == -1)
            return rr;
        else
        if(!rr)
            return xmsg(0, VPN_DEBUG|VPN_INFO,
                    "vpn_recv: lost connection, peer disconnected\n");

        xmsg(0, VPN_DEBUG, "read %d bytes of packet...\n", rr);

        s  += rr;
        rx += rr;
        rc -= rr;
    } while(rx < hdr.checksum);

    /* generate iv to decrypt */
    if(!cry->rndrecv) cry->rndrecv = initial_iv_random;
    memset(xiv, 0, sizeof(xiv));
    gen_iv(xiv, cry->blklen, cry->rndrecv);
    /* xmsg(0, VPN_DEBUG, "recv using iv: %s\n", xiv); */

    /* decrypt */
    rd = xdecrypt(hd, cry->blklen, xiv,
            decpack, sizeof(decpack), bigpack, hdr.checksum);
    if(rd == -1 || rd != hdr.checksum)
        return xmsg(-1, VPN_DEBUG|VPN_INFO,
                "vpn_recv: cannot decrypt packet (%d != %d)\n",
                rd, hdr.checksum);

    /* copy data to user */
    memcpy(buf, decpack, hdr.checksum - hdr.pad);

    /* set next iv */
    cry->rndrecv = hdr.checksum - hdr.pad;
        
    return hdr.checksum - hdr.pad;
}

static int xdecrypt(gcry_cipher_hd_t hd, int blklen, const char *iv,
        void *dst, size_t dstlen, const void *src, size_t srclen)
{
    int i;
    gcry_error_t err = 0;
    char *s = (char *) src, *d = dst;

    if(dstlen < srclen)
        return xmsg(-1, VPN_DEBUG,
                "not enough space to put decrypted data!\n");

    for(i = 0; i < srclen; i+=blklen, s+=blklen, d+=blklen) {
        err = gcry_cipher_setiv(hd, iv, blklen);
        if(err) {
            xmsg(0, VPN_DEBUG, "decrypt iv mismatch: %s\n", gpg_strerror(err));
            break;
        }

        err = gcry_cipher_decrypt(hd, d, blklen, s, blklen);
        if(err) {
            xmsg(0, VPN_DEBUG, "decrypt failed: %s\n", gpg_strerror(err));
            break;
        }
    }

    return err ? -1 : i;
}

static int safe_send(int fd, const char *buf, size_t buflen)
{
    int r;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    if((r = select(fd+1, 0, &fds, 0, 0)) >= 1)
        return send(fd, buf, buflen, 0);
    
    return r;
}

static int safe_recv(int fd, char *buf, size_t buflen)
{
    int r;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    if((r = select(fd+1, &fds, 0, 0, 0)) >= 1)
        return recv(fd, buf, buflen, 0);
    
    return r;
}

static void gen_iv(char *dst, size_t dstlen, uint16_t random)
{
    int i;
    char x[10], *d = dst;

    snprintf(x, sizeof(x), "%04d", random);

    for(i = 0; i < dstlen; i++, d++)
        *d = x[i % 4];

    //strncpy(dst, "01234567", dstlen);
}
