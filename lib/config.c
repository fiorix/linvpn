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
 
   $Id: config.c 352 2005-12-12 16:53:19Z alec $
*/

#include "linvpn.h"

/* configuration file */
static vpn_conf_t main_cfg[VPN_CONF_MAX_ENTRIES];
static size_t     main_cfg_nmemb = 0;
static int        main_cfg_sem = 0;

/* lex & yacc parser */
extern int xvpn_readconf(FILE *fp, vpn_conf_t *cfg, size_t cfg_nmemb);

/* check duplicated entries */
static char *check_de(vpn_conf_t *cfg, size_t cfg_nmemb);

/* read configuration */
int vpn_readconf(vpn_conf_t *cfg, size_t cfg_nmemb)
{
    int r;
    FILE *fp;
    char *temp;
    
    if((temp = getenv("VPN_CONF_FILE")) == NULL)
        temp = VPN_CONF_FILE;

    setperm(VPN_ROOT);
    
    if(!(fp = fopen(temp, "r")))
        return xmsg(-1, VPN_TERM|VPN_INFO, 
                "open(%s): %s\n", VPN_CONF_FILE, errstr);

    /* parse configuration file */
    memset(cfg, 0, sizeof(vpn_conf_t)*cfg_nmemb);
    r = xvpn_readconf(fp, cfg, cfg_nmemb);
    fclose(fp);

    {
        int i;
        vpn_conf_t *p = cfg;

        /* check PPP addresses and ALGO for ALL entries */
        for(i = 0; i < r; i++, p++) {
            if(!p->algo) p->algo = GCRY_CIPHER_3DES;

            if(!*p->ppplocal || !*p->pppremote)
                return xmsg(-1, VPN_TERM|VPN_INFO,
                        "error: entry [%s] missing PPP addresses\n", p->name);
        }
    }

    if(r == -1)
        return r;
    else
    if((temp = check_de(cfg, r)) != NULL)
        return xmsg(-1, VPN_TERM|VPN_INFO, "error: duplicated entry [%s], "
                "check configuration file\n", temp);
    
    setperm(VPN_USER);
    return r;
}

/* check configuration */
int vpn_checkconf(int print, vpn_conf_t **ret, size_t *ret_nmemb)
{
    int r, i;
    vpn_conf_t *p = main_cfg;

    while(main_cfg_sem) sleep(1);

    main_cfg_sem = 1;
    r = vpn_readconf(main_cfg, vsizeof(main_cfg));

    if(r == -1)
        return -1;
    else
    if(!r)
        return xmsg(-1, VPN_TERM|VPN_INFO, "error: found no entries in %s\n",
                VPN_CONF_FILE);
    else
        main_cfg_nmemb = r;

    if(print) {
        xmsg(0, VPN_TERM, "configuration OK, found %d entr%s\n",
                r, r == 1 ? "y" : "ies");

        for(i = 0; i < r; i++, p++)
            xmsg(0, VPN_TERM, "-> %s [%s]\n", 
                    p->name, gcry_cipher_algo_name(p->algo));
    }

    /* copy */
    if(ret && ret_nmemb) {
        *ret = main_cfg;
        *ret_nmemb = r;
    }

    main_cfg_sem = 0;
    return 0;
}

/* get VPN entry based on its `name' */
int vpn_getconf(vpn_conf_t *dst, const char *name)
{
    int i, found = 0;
    vpn_conf_t *p = main_cfg;

    while(main_cfg_sem) sleep(1);

    main_cfg_sem = 1;

    for(i = 0; i < main_cfg_nmemb; i++, p++)
        if(!strncmp(p->name, name, sizeof(p->name))) {
            found = 1;
            memcpy(dst, p, sizeof(vpn_conf_t));
        }

    main_cfg_sem = 0;
    return found;
}

static char *check_de(vpn_conf_t *cfg, size_t cfg_nmemb)
{
    int i, j;
    vpn_conf_t *ci, *cj;

    for(i = 0, ci = cfg; i < cfg_nmemb; i++, ci++) {
        for(j = 0, cj = cfg; j < cfg_nmemb; j++, cj++) {
            if(j == i) continue;

            if(!strncmp(cj->name, ci->name, sizeof(cj->name)))
                return cj->name;
        }
    }

    return NULL;
}
