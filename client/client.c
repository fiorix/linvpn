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
 
   $Id: client.c 355 2005-12-13 10:12:11Z alec $
*/

#include "client.h"

int main(int argc, char **argv)
{
    int c, optind = 0;
    const char *opts = "u:p:c:f:r:dtvh", *vpn_name = NULL;
    const struct option optl[] = {
        { "user",    1, 0, 'u' },
        { "connect", 1, 0, 'c' },
        { "port",    1, 0, 'p' },
        { "file",    1, 0, 'f' },
        { "retry",   1, 0, 'r' },
        { "debug",   0, 0, 'd' },
        { "test",    0, 0, 't' },
        { "version", 0, 0, 'v' },
        { "help",    0, 0, 'h' },
        { 0, 0, 0, 0 },
    };

    vpn_conf_t cfg;
    uint16_t port = htons(VPN_PORT);
    uint16_t test_only = 0, retry = 0;

    void help(void) {
        fprintf(stdout, "use: %s [options]\n"
                "OPTIONS\n"
                " --user,    -u USER     alternate user [default %s]\n"
                " --file,    -f FILE     alternate config file [default %s]\n"
                " --connect, -c NAME     connect to endpoint `NAME'\n"
                " --port,    -p PORT     alternate TCP port [default %d]\n"
                " --retry,   -r SEC      reconnect in SEC seconds [default %d]\n"
                " --test,    -t          test configuration file\n"
                " --debug,   -d          run in debug mode (no daemon)\n"
                " --version, -v          show version\n"
                " --help,    -h          this help\n", 
                *argv, VPN_USERNAME, VPN_CONF_FILE, VPN_PORT, RETRY_CONN);
        exit(1);
    }

    while((c = getopt_long(argc, argv, opts, optl, &optind)) != -1)
        switch(c) {
            case 'u':
                setenv("VPN_USERNAME", optarg, 1);
                break;
            case 'f':
                setenv("VPN_CONF_FILE", optarg, 1);
                break;
            case 'c':
                vpn_name = optarg;
                break;
            case 'p':
                port = htons(atoi(optarg));
                break;
            case 'r':
                retry = atoi(optarg);
                break;
            case 't':
                test_only = 1;
                break;
            case 'd':
                setenv("DEBUG", "1", 1);
                break;
            case 'v':
                fprintf(stdout, "linvpn %s by Alexandre Fiori\n", 
                        LINVPN_VERSION);
                return 0;
            default:
                help();
        }

    if(test_only)
        return vpn_checkconf(1, NULL, NULL) == -1 ? 1 : 0;
    else
    if(!vpn_name)
        help();
    else {
        if(vpn_checkconf(0, 0, 0) == -1)
            return xmsg(1, VPN_TERM, "unable to initialize client\n");
        else
        if(!vpn_getconf(&cfg, vpn_name))
            return xmsg(1, VPN_TERM, 
                    "unknown VPN %s, unable to initialize client\n", vpn_name);
    }

    if(!*cfg.dstaddr)
        return xmsg(1, VPN_TERM,
                "entry [%s] have no dst address, "
                "unable to initialize client\n", vpn_name);

    /* change user */
    setperm(VPN_USER);

    /* become daemon */
    if(!getenv("DEBUG")) {
        if(daemon(0, 0) == -1)
            return xmsg(1, VPN_TERM, "daemon: %s\n", errstr);
    }

    /* open syslog */
    openlog("linvpn", LOG_PID, LOG_DAEMON);

    /* main loop */
    if(!retry) retry = RETRY_CONN;
    for(;;sleep(retry)) run_client(port, &cfg);
    return 0;
}
