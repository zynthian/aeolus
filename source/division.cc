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
#include <string.h>
#include "division.h"

Division::Division(Asection *asect, float fsam) : _asect(asect),
                                                  _nrank(0),
                                                  _dmask(0),
                                                  _trem(0),
                                                  _fsam(fsam),
                                                  _swel(1.0f),
                                                  _gain(0.1f),
                                                  _w(0.0f),
                                                  _c(1.0f),
                                                  _s(0.0f),
                                                  _m(0.0f)
{
    for (int i = 0; i < NRANKS; i++)
        _ranks[i] = 0;
}

Division::~Division(void)
{
}

void Division::process(void)
{
    int i;
    float d, g, t;
    float *p, *q;

    memset(_buff, 0, NCHANN * PERIOD * sizeof(float));
    for (i = 0; i < _nrank; i++)
        _ranks[i]->play(1);

    g = _swel;
    if (_trem)
    {
        _s += _w * _c;
        _c -= _w * _s;
        t = sqrtf(_c * _c + _s * _s);
        _c /= t;
        _s /= t;
        if ((_trem == 2) && (fabsf(_s) < 0.05f))
        {
            _trem = 0;
            _c = 1;
            _s = 0;
        }
        g *= 1.0f + _m * _s;
    }

    t = 1.05f * _gain;
    if (g > t)
        g = t;
    t = 0.95f * _gain;
    if (g < t)
        g = t;

    d = (g - _gain) / PERIOD;
    g = _gain;
    p = _buff;
    q = _asect->get_wptr();

    for (i = 0; i < PERIOD; i++)
    {
        g += d;
        q[0 * PERIOD * MIXLEN] += p[0 * PERIOD] * g;
        q[1 * PERIOD * MIXLEN] += p[1 * PERIOD] * g;
        q[2 * PERIOD * MIXLEN] += p[2 * PERIOD] * g;
        q[3 * PERIOD * MIXLEN] += p[3 * PERIOD] * g;
        p++;
        q++;
    }
    _gain = g;
}

// Set or replace the Rankwave for a Rank.
//
void Division::set_rank(int ind, Rankwave *W, int pan, int del)
{
    Rankwave *C;

    C = _ranks[ind];
    if (C)
    {
        W->_nmask = C->_nmask | KMAP_SET;
        delete C;
    }
    else
        W->_nmask = KMAP_SET;
    _ranks[ind] = W;
    del = (int)(1e-3f * del * _fsam / PERIOD);
    if (del > 31)
        del = 31;
    W->set_param(_buff, del, pan);
    if (_nrank < ++ind)
        _nrank = ind;
}

// Handle key up down events.
//
void Division::update_keys(uint8_t key, uint8_t flags)
{
    for (int rank = 0; rank < _nrank; ++rank)
    {
        Rankwave* W = _ranks[rank];
        if (W->_nmask & 0x7F)
        {
            if (W->_nmask & flags) {
                W->note_on(key + 36);
            } else {
                W->note_off(key + 36);
            }
        }
    }
}

void Division::update_stops(uint16_t *keys)
{
    
    for (int rank = 0; rank < _nrank; ++rank)
    {
        Rankwave* W = _ranks[rank];
        if (W->_nmask & KMAP_SET)
        {
            // Stops changed
            W->_nmask &= 0x7FFF; // Clear changed flag
            uint8_t mask = W->_nmask & 0x7F;
            if (mask) {
                for (uint8_t key = 0; key < 61; ++key)
                {
                    uint8_t flags = keys[key] | (keys[key] >> 6);
                    if (W->_nmask & flags)
                        W->note_on(key + 36);
                    else
                        W->note_off(key + 36);
                }
            } else {
                W->all_off();
            }
        }
    }
}

void Division::set_div_mask(int bit)
{
    int r;
    uint16_t b, d;
    Rankwave *W;

    b = 1 << bit;
    d = 1 << NKEYBD;
    _dmask |= b;
    for (r = 0; r < _nrank; r++)
    {
        W = _ranks[r];
        if (W->_nmask & d)
        {
            W->_nmask |= b;
            W->_nmask |= 128;
            W->_nmask |= KMAP_SET;
        }
    }
}

void Division::clr_div_mask(int bit)
{
    int r;
    uint16_t b, d;
    Rankwave *W;

    b = 1 << bit;
    d = 1 << NKEYBD;
    _dmask &= ~b;
    for (r = 0; r < _nrank; r++)
    {
        W = _ranks[r];
        if (W->_nmask & d)
        {
            W->_nmask &= ~b;
            W->_nmask |= 128;
            W->_nmask |= KMAP_SET;
        }
    }
}

void Division::set_rank_mask(int ind, int bit)
{
    uint16_t b = 1 << bit;
    Rankwave *W = _ranks[ind];
    if (bit == NKEYBD) {
        b |= _dmask;
        W->_nmask |= 128;
    }
    W->_nmask |= b;
    W->_nmask |= KMAP_SET;
}

void Division::clr_rank_mask(int ind, int bit)
{
    uint16_t b = 1 << bit;
    Rankwave *W = _ranks[ind];
    if (bit == NKEYBD) {
        b |= _dmask;
        W->_nmask &= ~128;
    }
    W->_nmask &= ~b;
    W->_nmask |= KMAP_SET;
    if (W->_nmask | (1 << NKEYBD))
        W->_nmask |= 128;
    else
        W->_nmask &= ~128;
}
