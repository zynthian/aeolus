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

/*  Implements OSC control
    Listens for OSC on UDP port (provided as parameter to class constructor)
    Supports OSC messages:
        /quit   - Exit application
        /exit   - Exit application
        /save   - Save global configuration
        /retune - Retune pipes
            float : Tuning frequence
            int : Temperament index
        /store_preset - Store current state as preset
            int : Bank index
            int : Preset index
        /recall_preset - Recalls a preset
            int : Bank index
            int : Preset index
        /inc_preset - Recall next preset in current bank
        /dec_preset - Recall previous preset in current bank
        /store_midi_config - Store MIDI configuration
            int : MIDI config preset (0..7)
            16 * int : 16-bit word for config of MIDI channel
*/

#ifndef __OSC_H
#define __OSC_H

#define MAX_OSC_CLIENTS 5 // Max quantity of clients that may register for tallies

#include <clthreads.h>
#include "messages.h"
#include "tinyosc.h"
#include <arpa/inet.h> // provides inet_pton

class Osc : public A_thread
{
public:
    Osc(int port, const char *notify_uri);
    virtual ~Osc(void) {}

    void terminate(void) { put_event(EV_EXIT, 1); }

private:
    virtual void thr_main(void);
    void process_osc(tosc_message *osc_msg);
    void sendOscFloat(const char *path, float value);
    void sendOscInt(const char *path, int value);
    void proc_mesg(ITC_mesg *M);
    int udp_port;
    uint16_t midi_config[16];
    int osc_fd;
    bool notify = false;
    struct sockaddr_in notify_sockaddr;
    char notify_path[256] = {'\0'};

    char osc_buffer[1024]; // Used to send OSC messages
};

#endif
