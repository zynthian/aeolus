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

#ifndef __DIVISION_H
#define __DIVISION_H

#include "asection.h"
#include "rankwave.h"

class Division
{
public:
    Division(Asection *asect, float fsam);
    ~Division(void);

    void set_rank(int ind, Rankwave *W, int pan, int del);
    void set_swell(float stat) { _swel = 0.2 + 0.8 * stat * stat; }
    void set_tfreq(float freq) { _w = 6.283184f * PERIOD * freq / _fsam; }
    void set_tmodd(float modd) { _m = modd; }
    void set_div_mask(int bits);
    void clr_div_mask(int bits);
    void set_rank_mask(int ind, int bits);
    void clr_rank_mask(int ind, int bits);
    void trem_on(void) { _trem = 1; }
    void trem_off(void) { _trem = 2; }
    void set_reverb(float val);

    void process(void);
    void update_keys(uint8_t key, uint8_t flags);
    void update_stops(uint16_t *keys);

private:
    Asection *_asect;
    Rankwave *_ranks[NRANKS];
    int _nrank;
    int _dmask;
    int _trem;
    float _fsam;
    float _swel;
    float _gain;
    float _w;
    float _c;
    float _s;
    float _m;
    float _buff[NCHANN * PERIOD];
};

#endif
