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
 
   $Id: server.c 352 2005-12-12 16:53:19Z alec $
*/

#include "server.h"

int main(int argc, char **argv)
{
    int c, optind = 0;
    const char *opts = "u:k:p:b:f:dltvh";
    const struct option optl[] = {
        { "user",    1, 0, 'u' },
        { "bind",    1, 0, 'b' },
        { "port",    1, 0, 'p' },
        { "kill",    1, 0, 'k' },
        { "file",    1, 0, 'f' },
        { "debug",   0, 0, 'd' },
        { "list",    0, 0, 'l' },
        { "test",    0, 0, 't' },
        { "version", 0, 0, 'v' },
        { "help",    0, 0, 'h' },
        { 0, 0, 0, 0 },
    };

    uint16_t test_only = 0;
    uint16_t port = htons(VPN_PORT);
    struct in_addr bind_addr = { INADDR_ANY };

    void help(void) {
        fprintf(stdout, "use: %s [options]\n"
                "OPTIONS\n"
                " --user,    -u USER     alternate user [default %s]\n"
                " --file,    -f FILE     alternate config file [default %s]\n"
                " --bind,    -b ADDR     bind address [default ALL]\n"
                " --port,    -p PORT     alternate TCP port [default %d]\n"
                " --kill,    -k NAME     kill VPN `NAME'\n"
                " --list,    -l          list of active VPNs\n"
                " --test,    -t          test configuration file\n"
                " --debug,   -d          run in debug mode (no daemon)\n"
                " --version, -v          show version\n"
                " --help,    -h          this help\n", 
                *argv, VPN_USERNAME, VPN_CONF_FILE, VPN_PORT);
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
            case 'b':
                if(!inet_aton(optarg, &bind_addr)) {
                    xmsg(0, VPN_TERM, "invalid address: %s\n", optarg);
                    return 1;
                }
                break;
            case 'p':
                port = htons(atoi(optarg));
                break;
            case 'k':
                return kill_ctrl(optarg) == -1 ? 1 : 0;
            case 'l':
                list_ctrl(0);
                return 0;
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

    /* open syslog */
    openlog("linvpnd", LOG_PID, LOG_DAEMON);

    /* change user */
    setperm(VPN_USER);

    /* main loop */
    return run_server(port, bind_addr) == -1 ? 1 : 0;
}
