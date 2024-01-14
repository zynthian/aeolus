// ----------------------------------------------------------------------------
//
//  Copyright (C) 2003-2022 Fons Adriaensen <fons@linuxaudio.org>
//                2022-2024 riban <riban@zynthian.org>
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

#ifndef __AUDIO_H
#define __AUDIO_H

#include <stdlib.h>
#include <clthreads.h>
#include <jack/jack.h>
#include "asection.h"
#include "division.h"
#include "lfqueue.h"
#include "reverb.h"
#include "global.h"

class Audio : public A_thread
{
public:
    Audio(const char *jname, Lfq_u32 *qnote, Lfq_u32 *qcomm);
    virtual ~Audio(void);
    void init_jack(const char *server, bool bform, Lfq_u8 *qmidi);
    void start(void);

    const char *appname(void) const { return _appname; }
    uint16_t *midimap(void) const { return (uint16_t *)_midimap; }
    int policy(void) const { return _policy; }
    int abspri(void) const { return _abspri; }
    int relpri(void) const { return _relpri; }

private:
    enum
    {
        VOLUME,
        REVSIZE,
        REVTIME,
        STPOSIT
    };

    void init_audio(void);
    void close_jack(void);
    virtual void thr_main(void);
    void jack_shutdown(void);
    int jack_callback(jack_nframes_t);
    bool proc_jmidi(int);
    void proc_queue(Lfq_u32 *);
    void proc_synth(int);
    void proc_keys(void);
    void proc_stops(void);
    void proc_mesg(void);

    /* _keymap is 16-bit flag for each keyboard key:
            bit 0..13 asserted if key pressed on corresponding manual
            bit 14 asserted if hold pedal pressed
            bit 15 asserted if state has changed
    */
    void key_on(uint8_t chan, uint8_t key)
    {
        printf("keyon(%d, %d) midimap[%d]=0x%04x\n",chan, key, chan, _midimap[chan]);
        if (_midimap[chan] & 0x1000)
        {
            uint16_t m = 1 << (_midimap[chan] & 15);
            _keymap[key] |= m | KMAP_SET;
            if (_hold)
                _keymap[key] |= (m << 7);
        }
    }

    void key_off(uint8_t chan, uint8_t key)
    {
        if (_midimap[chan] & 0x1000)
        {
            uint16_t m = ~(1 << (_midimap[chan] & 15));
            _keymap[key] &= m;
            _keymap[key] |= KMAP_SET;
        }
    }

    void hold_on()
    {
        _hold = true;
        uint16_t i, *p;
        for (i = 0, p = _keymap; i < NNOTES; ++i, ++p)
        {
            if (*p & 0x7F)
                *p |= ((*p & 0x7F) << 7) | KMAP_SET; // Set hold and changed flags
        }
    }

    void hold_off()
    {
        _hold = false;
        uint16_t i, *p;
        for (i = 0, p = _keymap; i < NNOTES; ++i, ++p)
        {
            if (*p & 0x3f80) {
                *p = (*p & 0x7F); // Clear hold flags
                *p |= KMAP_SET; // Set changed flag
            }
        }
    }

    static void jack_static_shutdown(void *);
    static int jack_static_callback(jack_nframes_t, void *);

    const char *_appname;
    uint16_t _midimap[16];
    Lfq_u32 *_qnote;
    Lfq_u32 *_qcomm;
    Lfq_u8 *_qmidi;
    volatile bool _running;
    jack_client_t *_jack_handle;
    jack_port_t *_jack_opport[8];
    jack_port_t *_jack_midipt;
    int _policy;
    int _abspri;
    int _relpri;
    int _jmidi_count;
    int _jmidi_index;
    void *_jmidi_pdata;
    bool _hold = false;
    bool _bform;
    int _nplay;
    unsigned int _fsamp;
    unsigned int _fsize;
    int _nasect;
    int _ndivis;
    Asection *_asectp[NASECT];
    Division *_divisp[NDIVIS];
    Reverb _reverb;
    float *_outbuf[8];
    uint16_t _keymap[NNOTES];
    Fparm _audiopar[4];
    float _revsize;
    float _revtime;

    static const char *_ports_stereo[2];
    static const char *_ports_ambis1[4];
};

#endif
