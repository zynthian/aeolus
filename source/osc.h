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


#ifndef __OSC_H
#define __OSC_H

#include <clthreads.h>
#include "messages.h"
#include "tinyosc.h"

class Osc : public A_thread
{
public:

    Osc (int port) : A_thread ("OSC"), udp_port(port) {}
    virtual ~Osc (void) {}

    void terminate (void) {  put_event (EV_EXIT, 1); }

private:

    virtual void thr_main (void);
    void process_osc(tosc_message* osc_msg);
    int udp_port;
    uint16_t midi_config[16];
};


#endif
