/*
  Vrok - smokin' audio
  (C) 2012 Madura A. released under GPL 2.0. All following copyrights
  hold. This notice must be retained.

  See LICENSE for details.
*/

#include <stdlib.h>
#include <cstring>

#include "vrok.h"
#include "ogg.h"

VPDecoderPlugin* OGGDecoder::VPDecoderOGG_new(VPlayer *v)
{
    return (VPDecoderPlugin *)new OGGDecoder(v);
}

OGGDecoder::OGGDecoder(VPlayer *v)
{
    owner = v;
}

int OGGDecoder::open(const char *url)
{

    int ret=ov_fopen(url,&vf);
    if (ret<0){
        DBG("open fail");
        return -1;
    }

    buffer = new float[VPBUFFER_FRAMES*ov_info(&vf,0)->channels];
    half_buffer_size = VPBUFFER_FRAMES*ov_info(&vf,0)->channels*sizeof(float);

    VPBuffer bin;
    bin.srate = ov_info(&vf,0)->rate;
    bin.chans = ov_info(&vf,0)->channels;
    bin.buffer[0] = NULL;
    bin.buffer[1] = NULL;

    owner->setOutBuffers(&bin,&bout);
    for (unsigned i=0;i<VPBUFFER_FRAMES*bout->chans;i++){
        bout->buffer[0][i]=0.0f;
        bout->buffer[1][i]=0.0f;

    }

    return 0;
}

void OGGDecoder::reader()
{
    float **pcm;
    long ret=1;
    int bit;
    size_t j,done=0;

    while (ATOMIC_CAS(&owner->work,true,true) && ret > 0 ){
        j=0;
        ret=1;
        done=0;
        while (done<VPBUFFER_FRAMES && ret > 0){
            ret = ov_read_float( &vf, &pcm, VPBUFFER_FRAMES - done,&bit );
            for (long i=0;i<ret;i++){
                for (size_t ch=0;ch<bout->chans;ch++){
                    buffer[j]=pcm[ch][i];
                    j++;
                }
            }
            done+=ret;
        }

        memcpy(bout->buffer[*bout->cursor], buffer, VPBUFFER_FRAMES*bout->chans*sizeof(float) );
        owner->postProcess(bout->buffer[*bout->cursor]);

        owner->mutex[0].lock();
        VP_SWAP_BUFFERS(bout);
        owner->mutex[1].unlock();
    }
}

unsigned long OGGDecoder::getLength()
{
    return ov_pcm_total(&vf,-1);
}

void OGGDecoder::setPosition(unsigned long t)
{
    if (ov_seekable(&vf))
        ov_pcm_seek(&vf,(ogg_int64_t)t);
}
unsigned long OGGDecoder::getPosition()
{
    return (unsigned long)ov_pcm_tell(&vf);
}
OGGDecoder::~OGGDecoder()
{
    ov_clear(&vf);
    delete[] buffer;
}
