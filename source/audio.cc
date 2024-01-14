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

#include <math.h>
#include <jack/midiport.h>
#include "audio.h"
#include "messages.h"

Audio::Audio(const char *name, Lfq_u32 *qnote, Lfq_u32 *qcomm) : A_thread("Audio"),
                                                                 _appname(name),
                                                                 _qnote(qnote),
                                                                 _qcomm(qcomm),
                                                                 _qmidi(0),
                                                                 _running(false),
                                                                 _jack_handle(0),
                                                                 _abspri(0),
                                                                 _relpri(0),
                                                                 _bform(0),
                                                                 _nplay(0),
                                                                 _fsamp(0),
                                                                 _fsize(0),
                                                                 _nasect(0),
                                                                 _ndivis(0)
{
}

Audio::~Audio(void)
{
    int i;

    if (_jack_handle)
        close_jack();
    for (i = 0; i < _nasect; i++)
        delete _asectp[i];
    for (i = 0; i < _ndivis; i++)
        delete _divisp[i];
    _reverb.fini();
}

void Audio::init_audio(void)
{
    int i;

    _jmidi_pdata = 0;
    _audiopar[VOLUME]._val = 0.32f;
    _audiopar[VOLUME]._min = 0.00f;
    _audiopar[VOLUME]._max = 1.00f;
    _audiopar[REVSIZE]._val = _revsize = 0.075f;
    _audiopar[REVSIZE]._min = 0.025f;
    _audiopar[REVSIZE]._max = 0.150f;
    _audiopar[REVTIME]._val = _revtime = 4.0f;
    _audiopar[REVTIME]._min = 2.0f;
    _audiopar[REVTIME]._max = 7.0f;
    _audiopar[STPOSIT]._val = 0.5f;
    _audiopar[STPOSIT]._min = -1.0f;
    _audiopar[STPOSIT]._max = 1.0f;

    _reverb.init(_fsamp);
    _reverb.set_t60mf(_revtime);
    _reverb.set_t60lo(_revtime * 1.50f, 250.0f);
    _reverb.set_t60hi(_revtime * 0.50f, 3e3f);

    _nasect = NASECT;
    for (i = 0; i < NASECT; i++)
    {
        _asectp[i] = new Asection((float)_fsamp);
        _asectp[i]->set_size(_revsize);
    }
}

void Audio::start(void)
{
    M_audio_info *M;
    int i;

    M = new M_audio_info();
    M->_nasect = _nasect;
    M->_fsamp = _fsamp;
    M->_fsize = _fsize;
    M->_instrpar = _audiopar;
    for (i = 0; i < _nasect; i++)
        M->_asectpar[i] = _asectp[i]->get_apar();
    send_event(TO_MODEL, M);
}

void Audio::thr_main(void)
{
#ifdef __linux__

    while (_running)
    {
        proc_queue(_qnote);
        proc_queue(_qcomm);
        proc_mesg();
    }
    put_event(EV_EXIT);
#endif
}

void Audio::init_jack(const char *server, bool bform, Lfq_u8 *qmidi)
{
    int i;
    int opts;
    jack_status_t stat;
    struct sched_param spar;
    const char **p;

    _bform = bform;
    _qmidi = qmidi;

    opts = JackNoStartServer;
    if (server)
        opts |= JackServerName;
    _jack_handle = jack_client_open(_appname, (jack_options_t)opts, &stat, server);
    if (!_jack_handle)
    {
        fprintf(stderr, "Error: can't connect to JACK\n");
        exit(1);
    }
    _appname = jack_get_client_name(_jack_handle);

    jack_set_process_callback(_jack_handle, jack_static_callback, (void *)this);
    jack_on_shutdown(_jack_handle, jack_static_shutdown, (void *)this);

    if (_bform)
    {
        _nplay = 4;
        p = _ports_ambis1;
    }
    else
    {
        _nplay = 2;
        p = _ports_stereo;
    }

    for (i = 0; i < _nplay; i++)
    {
        _jack_opport[i] = jack_port_register(_jack_handle, p[i], JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        if (!_jack_opport[i])
        {
            fprintf(stderr, "Error: can't create the '%s' jack port\n", p[i]);
            exit(1);
        }
    }

    if (_qmidi)
    {
        _jack_midipt = jack_port_register(_jack_handle, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        if (!_jack_midipt)
        {
            fprintf(stderr, "Error: can't create the 'midi_in' jack port\n");
            exit(1);
        }
    }

    _fsamp = jack_get_sample_rate(_jack_handle);
    _fsize = jack_get_buffer_size(_jack_handle);
    init_audio();

    if (jack_activate(_jack_handle))
    {
        fprintf(stderr, "Error: can't activate JACK.");
        exit(1);
    }

    pthread_getschedparam(jack_client_thread_id(_jack_handle), &_policy, &spar);
    _abspri = spar.sched_priority;
    _relpri = spar.sched_priority - sched_get_priority_max(_policy);
}

void Audio::close_jack()
{
    jack_deactivate(_jack_handle);
    for (int i = 0; i < _nplay; i++)
        jack_port_unregister(_jack_handle, _jack_opport[i]);
    jack_client_close(_jack_handle);
}

void Audio::jack_static_shutdown(void *arg)
{
    return ((Audio *)arg)->jack_shutdown();
}

void Audio::jack_shutdown(void)
{
    _running = false;
    send_event(EV_EXIT, 1);
}

int Audio::jack_static_callback(jack_nframes_t nframes, void *arg)
{
    return ((Audio *)arg)->jack_callback(nframes);
}

int Audio::jack_callback(jack_nframes_t nframes)
{
    proc_queue(_qnote);
    proc_queue(_qcomm);
    proc_stops(); //!@todo Should this only be called when stops change?
    for (int i = 0; i < _nplay; i++)
        _outbuf[i] = (float *)(jack_port_get_buffer(_jack_opport[i], nframes));
    _jmidi_pdata = jack_port_get_buffer(_jack_midipt, nframes);
    _jmidi_count = jack_midi_get_event_count(_jmidi_pdata);
    _jmidi_index = 0;
    proc_synth(nframes);
    proc_mesg();
    return 0;
}

bool Audio::proc_jmidi(int tmax)
{
    uint8_t cmd, val1, val2, chan, ctrl_flags;
    jack_midi_event_t E;
    bool keys_dirty = false;

    // Read and process MIDI commands from the JACK port.
    // Events related to keyboard state are dealt with
    // locally. All the rest is sent as raw MIDI to the
    // model thread via qmidi.

    while ((jack_midi_event_get(&E, _jmidi_pdata, _jmidi_index) == 0) && (E.time < (jack_nframes_t)tmax))
    {
        cmd = E.buffer[0];
        val1 = E.buffer[1];
        val2 = E.buffer[2];
        chan = cmd & 0x0F;
        ctrl_flags = (_midimap[chan] >> 12) & 7; // Control enabled if (f & 4)
        
        switch (cmd & 0xF0)
        {
        case 0x80:
        case 0x90:
            // Note on or off.
            if (val2 && (cmd & 0x10))
            {
                // Note on.
                if (val1 < 36)
                {
                    // Keys 0..33 may be used for program change
                    if ((ctrl_flags & 4) && (val1 < 34))
                    {
                        // Preset selection, sent to model thread
                        // if on control-enabled channel.
                        if (_qmidi->write_avail() >= 3)
                        {
                            _qmidi->write(0, cmd);
                            _qmidi->write(1, val1);
                            _qmidi->write(2, val2);
                            _qmidi->write_commit(3);
                        }
                    }
                }
                else if (val1 <= 96)
                {
                    // Keys 36..96 used as keyboard
                    key_on(chan, val1 - 36);
                    keys_dirty = true;
                }
            }
            else
            {
                // Note off.
                if (val1 < 36)
                {
                    // Ignore note off for keys 0..35 (prog change)
                }
                else if (val1 <= 96)
                {
                    // Note off for keys 36..96
                    key_off(chan, val1 - 36);
                    keys_dirty = true;
                }
            }
            break;

        case 0xB0: // Controller
            switch (val1)
            {
            case MIDICTL_HOLD:
                // Hold pedal.
                if (val2 > 63)
                    hold_on();
                else
                    hold_off();
                keys_dirty = true;
                break;
            case MIDICTL_ASOFF:
                // All sound off, accepted on control channels only.
                // Clears all keyboards.
                if (ctrl_flags & 4)
                {
                    hold_off();
                    keys_dirty = true;
                }
                else
                {
                    break;
                }
                // Fall through to all notes off

            case MIDICTL_ANOFF:
                // All notes off, accepted on channels controlling
                // a keyboard. Does not clear held notes.
                if (ctrl_flags & 4)
                {
                    for (int i = 0; i < NNOTES; ++i)
                        key_off(chan, i);
                    keys_dirty = true;
                }
                break;

            case MIDICTL_BANK:
            case MIDICTL_IFELM:
                // Program bank selection or stop control, sent
                // to model thread if on control-enabled channel.
                if (ctrl_flags & 4)
                {
                    if (_qmidi->write_avail() >= 3)
                    {
                        _qmidi->write(0, cmd);
                        _qmidi->write(1, val1);
                        _qmidi->write(2, val2);
                        _qmidi->write_commit(3);
                    }
                }
            case MIDICTL_SWELL:
            case MIDICTL_TFREQ:
            case MIDICTL_TMODD:
                // Swell division commands
                if (ctrl_flags & 2)
                {
                    if (_qmidi->write_avail() >= 3)
                    {
                        _qmidi->write(0, 0xB0 | chan);
                        _qmidi->write(1, val1);
                        _qmidi->write(2, val2);
                        _qmidi->write_commit(3);
                    }
                }
                break;

            case MIDICTL_MAVOL:
            case MIDICTL_RDELY:
            case MIDICTL_RTIME:
            case MIDICTL_RPOSI:
            case MIDICTL_DAZIM:
            case MIDICTL_DWIDT:
            case MIDICTL_DDIRE:
            case MIDICTL_DREFL:
            case MIDICTL_DREVB:
                // Division commands
                if (ctrl_flags & 4)
                {
                    if (_qmidi->write_avail() >= 3)
                    {
                        _qmidi->write(0, 0xB0 | chan);
                        _qmidi->write(1, val1);
                        _qmidi->write(2, val2);
                        _qmidi->write_commit(3);
                    }
                }
                break;
            }
            break;

        case 0xC0:
            // Program change sent to model thread
            // if on control-enabled channel.
            if (ctrl_flags & 4)
            {
                if (_qmidi->write_avail() >= 3)
                {
                    _qmidi->write(0, cmd);
                    _qmidi->write(1, val1);
                    _qmidi->write(2, 0);
                    _qmidi->write_commit(3);
                }
            }
            break;
        }
        _jmidi_index++;
    }
    return keys_dirty;
}

void Audio::proc_queue(Lfq_u32 *Q)
{
    uint32_t event;
    int cmd, val1, val2, val3;
    union
    {
        uint32_t i;
        float f;
    } u;

    // Execute commands from the model thread (qcomm),
    // or from the midi thread (qnote).

    int n = Q->read_avail();
    while (n > 0)
    {
        event = Q->read(0); // event
        cmd = event >> 24;  // event command
        val1 = (event >> 16) & 255;
        val2 = (event >> 8) & 255;
        val3 = event & 255;

        switch (cmd)
        {
        case 0:
            // Single key off.
            key_off(val2, val3);
            Q->read_commit(1);
            break;

        case 1:
            // Single key on.
            key_on(val2, val3);
            Q->read_commit(1);
            break;

        case 4:
            // Clear bits in division mask.
            _divisp[val2]->clr_div_mask(val3);
            Q->read_commit(1);
            proc_stops();
            break;

        case 5:
            // Set bit in division mask.
            _divisp[val2]->set_div_mask(val3);
            Q->read_commit(1);
            proc_stops();
            break;

        case 6:
            // Clear bit in rank mask.
            _divisp[val2]->clr_rank_mask(val1, val3);
            Q->read_commit(1);
            proc_stops();
            break;

        case 7:
            // Set bit in rank mask.
            _divisp[val2]->set_rank_mask(val1, val3);
            Q->read_commit(1);
            proc_stops();
            break;

        case 8:
            // Hold off.
            hold_off();
            Q->read_commit(1);
            break;

        case 9:
            // Hold on.
            hold_on();
            Q->read_commit(1);
            break;

        case 16:
            // Tremulant on/off.
            if (val3)
                _divisp[val2]->trem_on();
            else
                _divisp[val2]->trem_off();
            Q->read_commit(1);
            break;

        case 17:
            // Per-division performance controllers.
            if (n < 2)
                return;
            u.i = Q->read(1);
            Q->read_commit(2);
            switch (val1)
            {
            case 0:
                _divisp[val2]->set_swell(u.f);
                break;
            case 1:
                _divisp[val2]->set_tfreq(u.f);
                break;
            case 2:
                _divisp[val2]->set_tmodd(u.f);
                break;
            }
            break;

        default:
            Q->read_commit(1);
        }
        n = Q->read_avail();
    }
}

void Audio::proc_keys(void)
{
    for (int key = 0; key < NNOTES; ++key)
    {
        uint16_t flags = _keymap[key];
        if (flags & KMAP_SET)
        {
            printf("Key %d has changed\n", key);
            //Key state has changed
            flags &= 0x7FFF; // clear changed flag
            _keymap[key] = flags;
            flags |= (flags >> 7);
            for (int div = 0; div < _ndivis; ++div)
                _divisp[div]->update_keys(key, flags & 0x7f);
        }
    }
}

void Audio::proc_stops(void)
{
    int d;

    for (d = 0; d < _ndivis; d++)
    {
        _divisp[d]->update_stops(_keymap);
    }
}

void Audio::proc_synth(int nframes)
{
    int j, k;
    float W[PERIOD];
    float X[PERIOD];
    float Y[PERIOD];
    float Z[PERIOD];
    float R[PERIOD];
    float *out[8];

    if (fabsf(_revsize - _audiopar[REVSIZE]._val) > 0.001f)
    {
        _revsize = _audiopar[REVSIZE]._val;
        _reverb.set_delay(_revsize);
        for (j = 0; j < _nasect; j++)
            _asectp[j]->set_size(_revsize);
    }
    if (fabsf(_revtime - _audiopar[REVTIME]._val) > 0.1f)
    {
        _revtime = _audiopar[REVTIME]._val;
        _reverb.set_t60mf(_revtime);
        _reverb.set_t60lo(_revtime * 1.50f, 250.0f);
        _reverb.set_t60hi(_revtime * 0.50f, 3e3f);
    }

    for (j = 0; j < _nplay; j++)
        out[j] = _outbuf[j];
    for (k = 0; k < nframes; k += PERIOD)
    {
        if (_jmidi_pdata)
            if (proc_jmidi(k + PERIOD))
                proc_keys();

        memset(W, 0, PERIOD * sizeof(float));
        memset(X, 0, PERIOD * sizeof(float));
        memset(Y, 0, PERIOD * sizeof(float));
        memset(Z, 0, PERIOD * sizeof(float));
        memset(R, 0, PERIOD * sizeof(float));

        for (j = 0; j < _ndivis; j++)
            _divisp[j]->process();
        for (j = 0; j < _nasect; j++)
            _asectp[j]->process(_audiopar[VOLUME]._val, W, X, Y, R);
        _reverb.process(PERIOD, _audiopar[VOLUME]._val, R, W, X, Y, Z);

        if (_bform)
        {
            for (j = 0; j < PERIOD; j++)
            {
                out[0][j] = W[j];
                out[1][j] = 1.41 * X[j];
                out[2][j] = 1.41 * Y[j];
                out[3][j] = 1.41 * Z[j];
            }
        }
        else
        {
            for (j = 0; j < PERIOD; j++)
            {
                out[0][j] = W[j] + _audiopar[STPOSIT]._val * X[j] + Y[j];
                out[1][j] = W[j] + _audiopar[STPOSIT]._val * X[j] - Y[j];
            }
        }
        for (j = 0; j < _nplay; j++)
            out[j] += PERIOD;
    }
}

void Audio::proc_mesg(void)
{
    ITC_mesg *M;

    while (get_event_nowait() != EV_TIME)
    {
        M = get_message();
        if (!M)
            continue;

        switch (M->type())
        {
        case MT_NEW_DIVIS:
        {
            M_new_divis *X = (M_new_divis *)M;
            Division *D = new Division(_asectp[X->_asect], (float)_fsamp);
            D->set_div_mask(X->_keybd);
            D->set_swell(X->_swell);
            D->set_tfreq(X->_tfreq);
            D->set_tmodd(X->_tmodd);
            _divisp[_ndivis] = D;
            _ndivis++;
            break;
        }
        case MT_CALC_RANK:
        case MT_LOAD_RANK:
        {
            M_def_rank *X = (M_def_rank *)M;
            _divisp[X->_divis]->set_rank(X->_rank, X->_rwave, X->_synth->_pan, X->_synth->_del);
            send_event(TO_MODEL, M);
            M = 0;
            break;
        }
        case MT_AUDIO_SYNC:
            send_event(TO_MODEL, M);
            M = 0;
            break;
        }
        if (M)
            M->recover();
    }
}

const char *Audio::_ports_stereo[2] = {"out.L", "out.R"};
const char *Audio::_ports_ambis1[4] = {"out.W", "out.X", "out.Y", "out.Z"};
