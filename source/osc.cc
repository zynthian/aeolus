// ----------------------------------------------------------------------------
//
//  Copyright (C) 2003-2023 Fons Adriaensen <fons@linuxaudio.org>
//                2023 Brian Walton <brian@riban.co.uk>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------

#include "osc.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>

void Osc::thr_main () 
{
    char buffer[2048];
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    fcntl(fd, F_SETFL, O_NONBLOCK);
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(udp_port);
    sin.sin_addr.s_addr = INADDR_ANY;
    bind(fd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in));
    printf("Listening for OSC on port %d\n", udp_port);

    while (1)
    {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(fd, &readSet);
        if (select(fd + 1, &readSet, NULL, NULL, NULL) > 0)
        {
            struct sockaddr sa;
            socklen_t sa_len = sizeof(struct sockaddr_in);
            int len = 0;
            while ((len = (int) recvfrom(fd, buffer, sizeof(buffer), 0, &sa, &sa_len)) > 0) {
                if (tosc_isBundle(buffer)) {
                    tosc_bundle bundle;
                    tosc_parseBundle(&bundle, buffer, len);
                    tosc_message osc;
                    while (tosc_getNextMessage(&bundle, &osc)) {
                        process_osc(&osc);
                    }
                } else {
                    tosc_message osc;
                    tosc_parseMessage(&osc, buffer, len);
                    process_osc(&osc);
                }
            }
        }
    }
}

void Osc::process_osc(tosc_message* osc_msg)
{
    if (strcmp(tosc_getAddress(osc_msg), "/exit") == 0)
        send_event (EV_EXIT, 1);
    else if (strcmp(tosc_getAddress(osc_msg), "/quit") == 0)
        send_event (EV_EXIT, 1);
    else if (strcmp(tosc_getAddress(osc_msg), "/save") == 0)
    	send_event (TO_MODEL, new ITC_mesg (MT_IFC_SAVE));
    else if (strcmp(tosc_getAddress(osc_msg), "/retune") == 0 && strcmp(tosc_getFormat(osc_msg), "fi") == 0)
        send_event (TO_MODEL, new M_ifc_retune (tosc_getNextFloat(osc_msg), tosc_getNextInt32(osc_msg)));
    else if (strcmp(tosc_getAddress(osc_msg), "/recall_preset") == 0 && strcmp(tosc_getFormat(osc_msg), "ii") == 0)
        send_event (TO_MODEL, new M_ifc_preset (MT_IFC_PRRCL, tosc_getNextInt32(osc_msg), tosc_getNextInt32(osc_msg), 0, 0));
    else if (strcmp(tosc_getAddress(osc_msg), "/store_preset") == 0 && strcmp(tosc_getFormat(osc_msg), "ii") == 0)
        send_event (TO_MODEL, new M_ifc_preset (MT_IFC_PRSTO, tosc_getNextInt32(osc_msg), tosc_getNextInt32(osc_msg), 0, 0));
    else if (strcmp(tosc_getAddress(osc_msg), "/inc_preset") == 0)
        send_event (TO_MODEL, new ITC_mesg (MT_IFC_PRINC));
    else if (strcmp(tosc_getAddress(osc_msg), "/dec_preset") == 0)
        send_event (TO_MODEL, new ITC_mesg (MT_IFC_PRDEC));
    else if (strcmp(tosc_getAddress(osc_msg), "/store_midi_config") == 0 && strcmp(tosc_getFormat(osc_msg), "iiiiiiiiiiiiiiiii") == 0)
    {
        int preset = tosc_getNextInt32(osc_msg);
        for (int i = 0; i < 16; ++i)
            midi_config[i] = tosc_getNextInt32(osc_msg);
        send_event (TO_MODEL, new M_ifc_chconf (MT_IFC_MCSET, preset, midi_config));
    }
    //else
    //    tosc_printMessage(osc_msg);
}