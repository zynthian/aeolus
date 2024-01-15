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
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

Osc::Osc(int port, const char *notify_uri) : A_thread("OSC"),
                                             udp_port(port)
{
    if (notify_uri)
    {
        char buffer[1024];
        strcpy(buffer, notify_uri);
        char *notify_addr = buffer;
        char *port_str = NULL;
        for (char *p = buffer; *p != '\0'; ++p)
        {
            if (*p == ':')
            {
                *p = '\0';
                port_str = p + 1;
            }
            else if (*p == '/')
            {
                strcpy(notify_path, p);
                *p = '\0';
                break;
            }
        }
        printf("nofify_addr: %s port_str: %s notify_path: %s\n", notify_addr, port_str, notify_path);
        int port_number = 0;
        if (port_str)
            port_number = atoi(port_str);

        memset(notify_sockaddr.sin_zero, '\0', sizeof notify_sockaddr.sin_zero);
        notify_sockaddr.sin_family = AF_INET;
        if (port_number)
            notify_sockaddr.sin_port = htons(port_number);
        else
            notify_sockaddr.sin_port = htons(udp_port + 1);
        notify_sockaddr.sin_addr.s_addr = 0;
        notify = (inet_pton(AF_INET, notify_addr, &notify_sockaddr.sin_addr) == 1);
        if (!notify)
        {
            // Not dot notation so try name lookup
            hostent *record = gethostbyname(notify_addr);
            if (record)
            {
                notify_sockaddr.sin_addr = *((in_addr *)record->h_addr_list[0]);
                notify_sockaddr.sin_family = record->h_addrtype;
                notify = true;
            }
        }
        if (notify)
        {
            char buffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &notify_sockaddr.sin_addr, buffer, sizeof(buffer));
            printf("Sending OSC notifications to %s:%d Path: %s\n", buffer, ntohs(notify_sockaddr.sin_port), notify_path);
        }
    }
}

void Osc::thr_main()
{
    char buffer[2048];
    osc_fd = socket(AF_INET, SOCK_DGRAM, 0);
    fcntl(osc_fd, F_SETFL, O_NONBLOCK);
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(udp_port);
    sin.sin_addr.s_addr = INADDR_ANY;
    bind(osc_fd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in));
    printf("Listening for OSC on port %d\n", udp_port);
    struct timeval timeout;
    timeout.tv_sec = 0;

    while (1)
    {
        int E = get_event_timed();
        switch (E)
        {
        case FM_MODEL:
            proc_mesg(get_message());
            break;
        }

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(osc_fd, &readSet);
        timeout.tv_usec = 10000;
        if (select(osc_fd + 1, &readSet, NULL, NULL, &timeout) > 0)
        {
            struct sockaddr sa;
            socklen_t sa_len = sizeof(struct sockaddr_in);
            int len = 0;
            while ((len = (int)recvfrom(osc_fd, buffer, sizeof(buffer), 0, &sa, &sa_len)) > 0)
            {
                if (tosc_isBundle(buffer))
                {
                    tosc_bundle bundle;
                    tosc_parseBundle(&bundle, buffer, len);
                    tosc_message osc;
                    while (tosc_getNextMessage(&bundle, &osc))
                    {
                        process_osc(&osc);
                    }
                }
                else
                {
                    tosc_message osc;
                    tosc_parseMessage(&osc, buffer, len);
                    process_osc(&osc);
                }
            }
        }
        // usleep(10000);
    }
}

void Osc::process_osc(tosc_message *osc_msg)
{
    if (strcmp(tosc_getAddress(osc_msg), "/exit") == 0)
        send_event(EV_EXIT, 1);
    else if (strcmp(tosc_getAddress(osc_msg), "/quit") == 0)
        send_event(EV_EXIT, 1);
    else if (strcmp(tosc_getAddress(osc_msg), "/save") == 0)
        send_event(TO_MODEL, new ITC_mesg(MT_IFC_SAVE));
    else if (strcmp(tosc_getAddress(osc_msg), "/retune") == 0 && strcmp(tosc_getFormat(osc_msg), "fi") == 0)
        send_event(TO_MODEL, new M_ifc_retune(tosc_getNextFloat(osc_msg), tosc_getNextInt32(osc_msg)));
    else if (strcmp(tosc_getAddress(osc_msg), "/recall_preset") == 0 && strcmp(tosc_getFormat(osc_msg), "ii") == 0)
        send_event(TO_MODEL, new M_ifc_preset(MT_IFC_PRRCL, tosc_getNextInt32(osc_msg), tosc_getNextInt32(osc_msg), 0, 0));
    else if (strcmp(tosc_getAddress(osc_msg), "/store_preset") == 0 && strcmp(tosc_getFormat(osc_msg), "ii") == 0)
        send_event(TO_MODEL, new M_ifc_preset(MT_IFC_PRSTO, tosc_getNextInt32(osc_msg), tosc_getNextInt32(osc_msg), 0, 0));
    else if (strcmp(tosc_getAddress(osc_msg), "/inc_preset") == 0)
        send_event(TO_MODEL, new ITC_mesg(MT_IFC_PRINC));
    else if (strcmp(tosc_getAddress(osc_msg), "/dec_preset") == 0)
        send_event(TO_MODEL, new ITC_mesg(MT_IFC_PRDEC));
    else if (strcmp(tosc_getAddress(osc_msg), "/store_midi_config") == 0 && strcmp(tosc_getFormat(osc_msg), "iiiiiiiiiiiiiiiii") == 0)
    {
        int preset = tosc_getNextInt32(osc_msg);
        for (int i = 0; i < 16; ++i)
            midi_config[i] = tosc_getNextInt32(osc_msg);
        send_event(TO_MODEL, new M_ifc_chconf(MT_IFC_MCSET, preset, midi_config));
    }
    else if (strcmp(tosc_getAddress(osc_msg), "/stop") == 0 && strcmp(tosc_getFormat(osc_msg), "iii") == 0)
    {
        int div = tosc_getNextInt32(osc_msg);
        int stop = tosc_getNextInt32(osc_msg);
        int val = tosc_getNextInt32(osc_msg);
        if (val)
            send_event(TO_MODEL, new M_ifc_ifelm(MT_IFC_ELSET, div, stop));
        else
            send_event(TO_MODEL, new M_ifc_ifelm(MT_IFC_ELCLR, div, stop));

    }
    else if (strcmp(tosc_getAddress(osc_msg), "/audio_param") == 0 && strcmp(tosc_getFormat(osc_msg), "iif") == 0)
    {
        int asect = tosc_getNextInt32(osc_msg);
        int parid = tosc_getNextInt32(osc_msg);
        float val = tosc_getNextFloat(osc_msg);
        send_event(TO_MODEL, new M_ifc_aupar(FM_OSC, asect, parid, val));

    }
    // else
    //     tosc_printMessage(osc_msg);
}

void Osc::proc_mesg(ITC_mesg *M)
{
    if (M)
        switch (M->type())
        {
        case MT_IFC_RETUNE:
            printf("OSC retune\n");
            break;
        case MT_IFC_READY:
            printf("OSC ready\n");
            if (notify)
            {
                char buffer[1024], str[1024];
                sprintf(str, "%s/ready", notify_path);
                int len = tosc_writeMessage(buffer, sizeof(buffer), str, "");
                sendto(osc_fd, buffer, len, MSG_CONFIRM | MSG_DONTWAIT, (const struct sockaddr *)&notify_sockaddr, sizeof(notify_sockaddr));
            }
            break;
        case MT_IFC_ELSET:
        case MT_IFC_ELCLR:
            {
                M_ifc_ifelm *X = (M_ifc_ifelm *)M;
                printf("OSC received %d for group %d element %d\n", M->type(), X->_group, X->_ifelm);
                if (notify)
                {
                    char buffer[1024], str[1024];
                    sprintf(str, "%s/stop", notify_path);
                    int len = tosc_writeMessage(buffer, sizeof(buffer), str, "");
                    //!@todo Add parameters!!!
                    sendto(osc_fd, buffer, len, MSG_CONFIRM | MSG_DONTWAIT, (const struct sockaddr *)&notify_sockaddr, sizeof(notify_sockaddr));
                }
            }
        }

}
