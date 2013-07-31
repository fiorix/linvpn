%{
#ifndef YYSTYPE
#define YYSTYPE char *
extern YYSTYPE conflval;
#endif

#include "parse.h"

extern int conflineno;

%}

octet               ([0-9][0-9]?[0-9]?)+
addr                {octet}\.{octet}\.{octet}\.{octet}
hex                 0[xX][0-9A-Fa-f]+
name                [A-Za-z][0-9A-Za-z._-]+
string              \"[0-9A-Za-z /._-]+\"
comment             #.*$
null                [ \t]*
newline             \n

%%
{comment}           /* ignore comments */
{null}              /* ignore space and tab */
vpn                 return TOKVPN;
src                 return TOKSRC;
dst                 return TOKDST;
key                 return TOKKEY;
ppp                 return TOKPPP;
local               return TOKLOCAL;
remote              return TOKREMOTE;
up                  return TOKUP;
down                return TOKDOWN;
algo                return TOKALGO;
3des                return TOK3DES;
blowfish            return TOKBLOW;
\;                  return TOKEND;
{hex}               conflval=strdup(conftext); return TOKHEX;
{addr}              conflval=strdup(conftext); return TOKADDR;
{name}              conflval=strdup(conftext); return TOKNAME;
{string}            conflval=strdup(conftext); return TOKSTR;
{newline}           conflineno++;
