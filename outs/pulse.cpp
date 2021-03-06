/*
  Vrok - smokin' audio
  (C) 2012 Madura A. released under GPL 2.0. All following copyrights
  hold. This notice must be retained.

  See LICENSE for details.
*/

#include <unistd.h>

#include "vrok.h"
#include "pulse.h"

VPOutPlugin* VPOutPluginPulse::VPOutPluginPulse_new()
{
    return (VPOutPlugin *) new VPOutPluginPulse();
}

void VPOutPluginPulse::worker_run(VPOutPluginPulse *self)
{
    int error;
    unsigned chans=self->bin->chans;

    while (ATOMIC_CAS(&self->work,true,true)){
        if (ATOMIC_CAS(&self->pause_check,true,true)) {
            ATOMIC_CAS(&self->paused,false,true);
            self->m_pause.lock();
            self->m_pause.unlock();
            ATOMIC_CAS(&self->paused,true,false);
            ATOMIC_CAS(&self->pause_check,true,false);
            if (!ATOMIC_CAS(&self->work,false,false)) {
                break;
            }

        }

        self->owner->mutex[1].lock();

        pa_simple_write(self->handle,self->bin->buffer[1-*self->bin->cursor],VPBUFFER_FRAMES*sizeof(float)*chans,&error);

        self->owner->mutex[0].unlock();
    }

}

void __attribute__((optimize("O0"))) VPOutPluginPulse::rewind()
{

    if (m_pause.try_lock()){
        //m_pause.lock();
        ATOMIC_CAS(&pause_check,false,true);

        owner->mutex[0].lock();
        for (unsigned i=0;i<VPBUFFER_FRAMES*bin->chans;i++)
            bin->buffer[*bin->cursor][i]=0.0f;
        owner->mutex[1].unlock();

        while (!ATOMIC_CAS(&paused,false,false)) {}
    }

}

void __attribute__((optimize("O0"))) VPOutPluginPulse::resume()
{
    if (ATOMIC_CAS(&paused,true,true)){
        ATOMIC_CAS(&pause_check,true,false);

        m_pause.try_lock();
        m_pause.unlock();
        while (ATOMIC_CAS(&paused,true,true)) {}
    }
}
void __attribute__((optimize("O0"))) VPOutPluginPulse::pause()
{
    if (!ATOMIC_CAS(&paused,false,false)){

        if (m_pause.try_lock()){
            ATOMIC_CAS(&pause_check,false,true);
            while (!ATOMIC_CAS(&paused,false,false)) {}
        }
        while (!ATOMIC_CAS(&paused,false,false)) {}
    }
}

int VPOutPluginPulse::init(VPlayer *v, VPBuffer *in)
{
    DBG("Pulse:init");
    owner = v;
    bin=in;

    int error;
    pa_sample_spec ss;
    ss.channels = in->chans;
    ss.rate = in->srate;
    ss.format = PA_SAMPLE_FLOAT32LE;
    handle = pa_simple_new(NULL,"Vrok",PA_STREAM_PLAYBACK,NULL,"Music",(const pa_sample_spec *)&ss,NULL,NULL,&error);
    if (!handle){
        DBG("Pulse:init: failed to open pcm");
        return -1;
    }

    work = true;
    paused = false;
    pause_check = false;

    worker = new std::thread((void(*)(void*))worker_run, this);
    DBG("pulse thread made");
    return 0;
}

VPOutPluginPulse::~VPOutPluginPulse()
{
    rewind();
    ATOMIC_CAS(&work,true,false);
    resume();


    if (worker){
        worker->join();
        DBG("out thread joined");
        delete worker;
    }

    pa_simple_free(handle);

}
