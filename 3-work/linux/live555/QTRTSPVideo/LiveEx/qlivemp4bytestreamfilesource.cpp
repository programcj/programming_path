#include "qlivemp4bytestreamfilesource.h"
#include "InputFile.hh"
#include "GroupsockHelper.hh"
#include <stdio.h>
#include <QDebug>

extern "C" {

#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"

}

struct FFMpegObject {
    AVFormatContext *fmt_ctx;
    int video_stream;
    int audio_stream;
    AVBitStreamFilterContext* h264bsfc;
    int result;
};

QLiveMP4ByteStreamFileSource *QLiveMP4ByteStreamFileSource::createNew(UsageEnvironment &env, const char *fileName, unsigned preferredFrameSize, unsigned playTimePerFrame)
{
    QLiveMP4ByteStreamFileSource* newSource = new QLiveMP4ByteStreamFileSource(
                env, fileName, preferredFrameSize, playTimePerFrame);
    return newSource;
}

QLiveMP4ByteStreamFileSource *QLiveMP4ByteStreamFileSource::createNew(UsageEnvironment &env, const char *fileName, FILE *fid, unsigned preferredFrameSize, unsigned playTimePerFrame)
{
    if (fileName == NULL)
        return NULL;

    QLiveMP4ByteStreamFileSource* newSource = new QLiveMP4ByteStreamFileSource(
                env, fileName, preferredFrameSize, playTimePerFrame);
    return newSource;
}

QLiveMP4ByteStreamFileSource::QLiveMP4ByteStreamFileSource(
        UsageEnvironment &env,
        const char *fileName,
        unsigned preferredFrameSize,
        unsigned playTimePerFrame):
    FramedSource(env)
{
    qDebug()<<"::QLiveMP4ByteStreamFileSource";

    struct FFMpegObject *obj=(struct FFMpegObject *)malloc(sizeof(struct FFMpegObject));
    this->bindData=obj;
    if(!this->bindData)
        return;
    memset(bindData,0, sizeof(struct FFMpegObject));

    obj->result = avformat_open_input(&obj->fmt_ctx, fileName, NULL, NULL);
    if(0>obj->result)
    {
        printf("Can't open file:%s\n", fileName);
        return ;
    }
    obj->result = avformat_find_stream_info(obj->fmt_ctx, NULL);
    if(0>obj->result)// ????
    {
        avformat_close_input(&obj->fmt_ctx);
        return ;
    }
    obj->video_stream = av_find_best_stream(obj->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1,
                                            NULL, 0);
    obj->audio_stream = av_find_best_stream(obj->fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1,
                                            NULL, 0);
    obj->h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
}

QLiveMP4ByteStreamFileSource::~QLiveMP4ByteStreamFileSource()
{
    qDebug()<<"~QLiveMP4ByteStreamFileSource";

    struct FFMpegObject *obj=(struct FFMpegObject *)this->bindData;
    if(!this->bindData)
        return;

    if((obj->h264bsfc))
        av_bitstream_filter_close(obj->h264bsfc);
    if(obj->fmt_ctx)
        avformat_close_input(&obj->fmt_ctx);
    free(this->bindData);
}

unsigned QLiveMP4ByteStreamFileSource::maxFrameSize() const
{
    //return 132596;
    return 1024*1024;
}

void QLiveMP4ByteStreamFileSource::doGetNextFrame()
{
    struct FFMpegObject *obj=(struct FFMpegObject *)this->bindData;
    if(!obj || 0>obj->result){
        handleClosure();
        return;
    }

    {
        AVCodecContext *origin_ctx = NULL;
        AVPacket packet;


        int ret=0;


        av_init_packet(&packet);


        while (1) {
            if (av_read_frame(obj->fmt_ctx, &packet) != 0) { //读取到结尾处
                ret=-1;
                av_packet_unref(&packet);
                qDebug()<<"读取到结尾处";
                break;
            }
            if(packet.stream_index==obj->video_stream)
            {
                origin_ctx = obj->fmt_ctx->streams[packet.stream_index]->codec;

                if (origin_ctx->codec_id == AV_CODEC_ID_H264) {
                    AVPacket new_pkt = packet;


                    ret = av_bitstream_filter_filter(obj->h264bsfc, origin_ctx,
                                                     NULL,
                                                     &new_pkt.data, &new_pkt.size,
                                                     packet.data,
                                                     packet.size,
                                                     packet.flags & AV_PKT_FLAG_KEY);

                    av_packet_unref(&packet);
                    packet.data = new_pkt.data;
                    packet.size = new_pkt.size;
                    break;
                }
            }
            av_packet_unref(&packet); //must free the packet
        } ///

        if(ret==-1){ //读取到结尾处
            av_seek_frame(obj->fmt_ctx, obj->video_stream, 0, AVSEEK_FLAG_ANY);
            nextTask() = envir().taskScheduler()
                    .scheduleDelayedTask(0,
                                         (TaskFunc*) FramedSource::afterGetting, this);
            return;
        }

        if(packet.size>fMaxSize){
            qDebug()<<"packet size > max," <<packet.size<<","<<fMaxSize;

            memcpy(fTo, packet.data, fMaxSize);
            fFrameSize = fMaxSize;
        } else {
            memcpy(fTo, packet.data,packet.size);
            fFrameSize = packet.size;
        }

        av_free(packet.data);
    }

    fNumTruncatedBytes = fFrameSize - fMaxSize;
    gettimeofday(&fPresentationTime, NULL);
    fDurationInMicroseconds = 0;

    nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                                                             (TaskFunc*) FramedSource::afterGetting, this);
}
