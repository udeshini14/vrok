/*
  Vrok - smokin' audio
  (C) 2012 Madura A. released under GPL 2.0. All following copyrights
  hold. This notice must be retained.

  See LICENSE for details.
*/

#ifndef PLAYER_MPEG_H
#define PLAYER_MPEG_H

#include <mpg123.h>

#include "vplayer.h"
#include "decoder.h"

class MPEGDecoder : public VPDecoderPlugin
{
private:
    FILE *fcurrent;
public:
    static VPDecoderPlugin* VPDecoderMPEG_new(VPlayer *v);
    mpg123_handle *mh;
    short *buffer;
    VPlayer *owner;

    MPEGDecoder(VPlayer *v);
    int open(const char *url);
    void reader();
    unsigned long getLength();
    void setPosition(unsigned long t);
    unsigned long getPosition();
    ~MPEGDecoder();
};


#endif // PLAYER_MPEG_H
