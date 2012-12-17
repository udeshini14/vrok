/*
  Vrok - smokin' audio
  (C) 2012 Madura A. released under GPL 2.0. All following copyrights
  hold. This notice must be retained.

  See LICENSE for details.
*/

#ifndef EFFECT_H
#define EFFECT_H

#include "vplayer.h"

class VPEffectPlugin
{
public:
    virtual int init(VPlayer *v) = 0;
    virtual void process(float *buffer) = 0;
    virtual int finit() = 0;
};
#endif // EFFECT_H
