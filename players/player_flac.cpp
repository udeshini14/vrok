#include "player_flac.h"

#define SHORTTOFL (1.0f/__SHRT_MAX__)

static void metadata_callback(const FLAC__StreamDecoder *decoder,
                              const FLAC__StreamMetadata *metadata,
                              void *client_data)
{
    FLACPlayer *me = (FLACPlayer*) client_data;
    bool changed=false;
    if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        if (me->prev_track_samples != metadata->data.stream_info.sample_rate)
            me->track_samples =  metadata->data.stream_info.sample_rate;
        else
            changed=true;

        if (me->prev_track_channels != metadata->data.stream_info.channels)
            me->track_channels =  metadata->data.stream_info.channels;
        else
            changed=true;

        if (changed){
            // restart out engine
        }
    }
}

static void error_callback(const FLAC__StreamDecoder *decoder,
                           FLAC__StreamDecoderErrorStatus status,
                           void *client_data)
{

    DBG("Got error callback: "<< FLAC__StreamDecoderErrorStatusString[status]);
}

static FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder,
                                                     const FLAC__Frame *frame,
                                                     const FLAC__int32 * const buffer[],
                                                     void *client_data)
{
    VPlayer *self = (VPlayer*) client_data;
    FLACPlayer *selfp = (FLACPlayer*) client_data;

    // fill buffer if not full
    // return ok

    // if full
    // write buffer1
    // write buffer2
    // if blocksize<buffersize
    //  fill
    //  return ok
    // else
    //  write full buffers to buffer1 buffer2
    //  write remaining to buffer
    //  return ok

    if (selfp->buffer_write+frame->header.blocksize*self->track_channels + 1 < VPlayer::BUFFER_FRAMES*2*self->track_channels){
        size_t i=0,j=0;
        DBG("s1");
        while (i<frame->header.blocksize) {
            for (unsigned ch=0;ch<self->track_channels;ch++){
                selfp->buffer[selfp->buffer_write+j]=SHORTTOFL*buffer[ch][i];
                j++;
            }
            i++;
        }
        selfp->buffer_write+=j;
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    } else if (selfp->buffer_write+frame->header.blocksize*self->track_channels + 1 == VPlayer::BUFFER_FRAMES*2*self->track_channels) {
        size_t i=0,j=0;
        DBG("s2");

        while (i<frame->header.blocksize) {
            for (unsigned ch=0;ch<self->track_channels;ch++){
                selfp->buffer[selfp->buffer_write+j]=SHORTTOFL*buffer[ch][i];
                j++;
            }
            i++;
        }

        self->mutexes[0]->lock();
        j=0;
        DBG("wb1");
        // write buffer1
        while(j<VPlayer::BUFFER_FRAMES){
            self->buffer1[j] = selfp->buffer[j];
            j++;
        }
        self->mutexes[1]->unlock();

        self->mutexes[2]->lock();
        j=0;
        DBG("wb2");
        // write buffer2
        while(j<VPlayer::BUFFER_FRAMES*self->track_channels){
            self->buffer2[j] = selfp->buffer[VPlayer::BUFFER_FRAMES*self->track_channels+j];
            j++;
        }
        self->mutexes[3]->unlock();

        selfp->buffer_write=0;
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    } else {


        size_t i=0,j=selfp->buffer_write;
        while (j<VPlayer::BUFFER_FRAMES*2*self->track_channels) {
            for (unsigned ch=0;ch<self->track_channels;ch++){
                selfp->buffer[j]=SHORTTOFL*buffer[ch][i];
                j++;
            }
            i++;
        }

        self->mutexes[0]->lock();
        j=0;
        // write buffer1
        while(j<VPlayer::BUFFER_FRAMES*self->track_channels){
            self->buffer1[j] = selfp->buffer[j];
            j++;
        }
        self->mutexes[1]->unlock();
 DBG("s3");
        self->mutexes[2]->lock();
        j=0;
        // write buffer2
        while(j<VPlayer::BUFFER_FRAMES*self->track_channels){
            self->buffer2[j] = selfp->buffer[VPlayer::BUFFER_FRAMES*self->track_channels+j];
            j++;
        }
        self->mutexes[3]->unlock();

        while (frame->header.blocksize-i > VPlayer::BUFFER_FRAMES*2 ){
            self->mutexes[0]->lock();
            j=0;
            // write buffer1
            while(j<VPlayer::BUFFER_FRAMES*self->track_channels){
                for (unsigned ch=0;ch<self->track_channels;ch++){
                    self->buffer1[j]=SHORTTOFL*buffer[ch][i];
                    j++;
                }
                i++;
            }
            self->mutexes[1]->unlock();

            self->mutexes[2]->lock();
            j=0;
            // write buffer2
            while(j<VPlayer::BUFFER_FRAMES*self->track_channels){
                for (unsigned ch=0;ch<self->track_channels;ch++){
                    selfp->buffer2[j]=SHORTTOFL*buffer[ch][i];
                    j++;
                }
                i++;
            }
            self->mutexes[3]->unlock();
        }

        j=0;
        while (i<frame->header.blocksize) {
            for (unsigned ch=0;ch<self->track_channels;ch++){
                selfp->buffer[j]=SHORTTOFL*buffer[ch][i];
                j++;
            }
            i++;
        }
        selfp->buffer_write=j;
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

}

int FLACPlayer::open(char *url)
{
    DBG("dd");
    FLAC__StreamDecoder *decoder = NULL;
    FLAC__StreamDecoderInitStatus init_status;
    if((decoder = FLAC__stream_decoder_new()) == NULL) {
        DBG("FLACPlayer:open: decoder create fail");
        return -1;
    }
    init_status = FLAC__stream_decoder_init_file(decoder, url, write_callback, metadata_callback, error_callback, (void *) this);
    if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK){
        DBG("FLACPlayer:open: decoder init fail");
    } else {
        DBG("sd");
        buffer_write=0;
        work = true;
        ((VPlayer *) this)->mutexes[0]->unlock();
        ((VPlayer *) this)->mutexes[2]->unlock();
        FLAC__stream_decoder_process_until_end_of_stream(decoder);

    }

}

int FLACPlayer::play()
{
    buffer_write=0;
    work = true;
    ((VPlayer *) this)->mutexes[0]->unlock();
    ((VPlayer *) this)->mutexes[2]->unlock();
    DBG("dff");
}

void FLACPlayer::pause()
{
    ((VPlayer *) this)->mutexes[0]->lock();
    ((VPlayer *) this)->mutexes[1]->lock();
    ((VPlayer *) this)->mutexes[2]->lock();
    ((VPlayer *) this)->mutexes[3]->lock();
}
void FLACPlayer::stop()
{

}
int FLACPlayer::setVolume(unsigned vol)
{
    return 0;
}
unsigned long FLACPlayer::getLength()
{
    return 0;
}
void FLACPlayer::setPosition(unsigned long t)
{

}
unsigned long FLACPlayer::getPosition()
{
    return 0;
}