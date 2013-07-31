%{
#include "linvpn.h"

#define YYSTYPE char *

extern int   confparse();
extern int   conflex();
extern int   conflineno;
extern FILE *confin;

static vpn_conf_t *xcfg = NULL;
static ssize_t     xcfg_nmemb = 0;
static ssize_t     xcfg_current = 0;
static int         xcfg_err = 0;

void conferror(const char *str)
{
    static int zline = 0;

    if(zline != conflineno) {
        xmsg(0, VPN_TERM|VPN_INFO, 
            "error: %s, check configuration file line %d\n", str, conflineno);
        zline = conflineno;
        xcfg_err++;
    }
}

int confwrap(void)
{
    return 1;
}

int xvpn_readconf(FILE *fp, vpn_conf_t *cfg, ssize_t cfg_nmemb)
{
    xcfg = cfg;
    xcfg_nmemb = cfg_nmemb;
    xcfg_current = 0;

    confin = fp;
    confparse();

    return xcfg_err ? -1 : xcfg_current;
}

%}

%token TOKVPN TOKALGO TOK3DES TOKBLOW TOKSRC TOKDST TOKKEY TOKPPP TOKLOCAL TOKREMOTE TOKUP TOKDOWN TOKEND TOKADDR TOKHEX TOKNAME TOKSTR

%%

commands:
        |
        commands vpn
        ;

vpn:
        TOKVPN TOKNAME options TOKEND
        {
            strncpy(xcfg->name, $2, sizeof(xcfg->name));

            if(++xcfg_current == xcfg_nmemb)
                YYACCEPT;
            else
                xcfg++;
        }
        ;

options:
        |
        options validopt
        ;

validopt: algo | src | dst | ppp | up | down

algo:
        TOKALGO TOK3DES
        {
            xcfg->algo = GCRY_CIPHER_3DES;
        }
        |
        TOKALGO TOKBLOW
        {
            xcfg->algo = GCRY_CIPHER_BLOWFISH;
        }
        ;

src:
        TOKSRC TOKNAME TOKKEY TOKHEX
        {
            if(resolv(xcfg->srcaddr, sizeof(xcfg->srcaddr), $2) == -1) {
                free($2);
                free($4);
                YYFAIL;
            }

            strncpy(xcfg->srckey,  $4, sizeof(xcfg->srckey));
            free($2);
            free($4);
        }
        |
        TOKSRC TOKADDR TOKKEY TOKHEX
        {
            if(checkip($2) == -1) {
                free($2);
                free($4);
                YYFAIL;
            }

            strncpy(xcfg->srcaddr, $2, sizeof(xcfg->srcaddr));
            strncpy(xcfg->srckey,  $4, sizeof(xcfg->srckey));
            free($2);
            free($4);
        }
        |
        TOKSRC TOKKEY TOKHEX
        {
            strncpy(xcfg->srckey,  $3, sizeof(xcfg->srckey));
            free($3);
        }
        ;

dst:
        TOKDST TOKNAME TOKKEY TOKHEX
        {
            if(resolv(xcfg->dstaddr, sizeof(xcfg->dstaddr), $2) == -1) {
                free($2);
                free($4);
                YYFAIL;
            }

            strncpy(xcfg->dstkey,  $4, sizeof(xcfg->dstkey));
            free($2);
            free($4);
        }
        |
        TOKDST TOKADDR TOKKEY TOKHEX
        {
            if(checkip($2) == -1) {
                free($2);
                free($4);
                YYFAIL;
            }

            strncpy(xcfg->dstaddr, $2, sizeof(xcfg->dstaddr));
            strncpy(xcfg->dstkey,  $4, sizeof(xcfg->dstkey));
            free($2);
            free($4);
        }
        |
        TOKDST TOKKEY TOKHEX
        {
            strncpy(xcfg->dstkey,  $3, sizeof(xcfg->dstkey));
            free($3);
        }
        ;

ppp:
        TOKPPP TOKLOCAL TOKADDR TOKREMOTE TOKADDR
        {
            if(checkip($3) == -1 || checkip($5) == -1) {
                free($3);
                free($5);
                YYFAIL;
            }

            strncpy(xcfg->ppplocal,  $3, sizeof(xcfg->ppplocal));
            strncpy(xcfg->pppremote, $5, sizeof(xcfg->pppremote));
            free($3);
            free($5);
        }
        |
        TOKPPP TOKREMOTE TOKADDR TOKLOCAL TOKADDR
        {
            strncpy(xcfg->ppplocal,  $5, sizeof(xcfg->ppplocal));
            strncpy(xcfg->pppremote, $3, sizeof(xcfg->pppremote));
            free($3);
            free($5);
        }
        ;

up:
        TOKUP TOKSTR
        {
            strncpy(xcfg->cmdup, $2, sizeof(xcfg->cmdup));
            free($2);
        }
        ;

down:
        TOKDOWN TOKSTR
        {
            strncpy(xcfg->cmddown, $2, sizeof(xcfg->cmddown));
            free($2);
        }
        ;
