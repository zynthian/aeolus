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

#ifndef __RANKWAVE_H
#define __RANKWAVE_H

#include "addsynth.h"
#include "rngen.h"

#define PERIOD 64

class Pipewave
{
private:
    Pipewave(void) : _p0(0), _p1(0), _p2(0), _l1(0),
                     _k_s(0), _k_r(0),
                     _m_r(0), _d_r(0), _d_a(0), _d_w(0),
                     _link(0), _sbit(0), _sdel(0),
                     _p_p(0), _y_p(0), _z_p(0), _p_r(0), _y_r(0), _g_r(0), _i_r(0)
    {
    }

    ~Pipewave(void) { delete[] _p0; }

    friend class Rankwave;

    void genwave(Addsynth *D, int n, float fsamp, float fpipe);
    void save(FILE *F);
    void load(FILE *F);
    void play(void);

    static void looplen(float f, float fsamp, int lmax, int *aa, int *bb);
    static void attgain(int n, float p);

    float *_p0;   // attack start
    float *_p1;   // loop start
    float *_p2;   // loop end
    int32_t _l0;  // attack length
    int32_t _l1;  // loop length
    int16_t _k_s; // sample step
    int16_t _k_r; // release lenght
    float _m_r;   // release multiplier
    float _d_r;   // release detune
    float _d_a;   // instability amplitude
    float _d_w;   // instability bandwidth

    Pipewave *_link; // link to next in active chain
    uint32_t _sbit;  // on state bit
    uint32_t _sdel;  // delayed state
    float *_out;     // audio output buffer
    float *_p_p;     // play pointer
    float _y_p;      // play interpolation
    float _z_p;      // play interpolation speed
    float *_p_r;     // release pointer
    float _y_r;      // release interpolation
    float _g_r;      // release gain
    int16_t _i_r;    // release count

    static void initstatic(float fsamp);

    static Rngen _rgen;
    static float *_arg;
    static float *_att;
};

class Rankwave
{
public:
    Rankwave(int n0, int n1);
    ~Rankwave(void);

    void note_on(uint8_t n)
    {
        if ((n < _n0) || (n > _n1))
            return;
        Pipewave *P = _pipes + (n - _n0);
        P->_sbit = _sbit;
        if (!(P->_sdel || P->_p_p || P->_p_r))
        {
            P->_sdel |= _sbit;
            P->_link = _list;
            _list = P;
        }
    }

    void note_off(uint8_t n)
    {
        if ((n < _n0) || (n > _n1))
            return;
        Pipewave *P = _pipes + (n - _n0);
        P->_sdel >>= 4;
        P->_sbit = 0;
    }

    void all_off(void)
    {
        Pipewave *P;
        for (P = _list; P; P = P->_link)
            P->_sbit = 0;
    }

    int n0(void) const { return _n0; }
    int n1(void) const { return _n1; }
    void play(int shift);
    void set_param(float *out, int del, int pan);
    void gen_waves(Addsynth *D, float fsamp, float fbase, float *scale);
    int save(const char *path, Addsynth *D, float fsamp, float fbase, float *scale);
    int load(const char *path, Addsynth *D, float fsamp, float fbase, float *scale);
    bool modif(void) const { return _modif; }

    uint16_t _nmask; // used by division logic

private:
    Rankwave(const Rankwave &);
    Rankwave &operator=(const Rankwave &);

    int _n0;
    int _n1;
    uint32_t _sbit;
    Pipewave *_list;
    Pipewave *_pipes;
    bool _modif;
};

#endif
