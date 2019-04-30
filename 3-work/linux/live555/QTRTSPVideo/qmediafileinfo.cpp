#include "qmediafileinfo.h"
#include <QDebug>

#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
#endif

extern "C" {

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/pixdesc.h"
}


char* BytesToSize( double Bytes )
{
    float tb = 1099511627776;
    float gb = 1073741824;
    float mb = 1048576;
    float kb = 1024;
    char returnSize[256];
    if( Bytes >= tb )
        sprintf(returnSize, "%.2f TB", (float)Bytes/tb);
    else if( Bytes >= gb && Bytes < tb )
        sprintf(returnSize, "%.2f GB", (float)Bytes/gb);
    else if( Bytes >= mb && Bytes < gb )
        sprintf(returnSize, "%.2f MB", (float)Bytes/mb);
    else if( Bytes >= kb && Bytes < mb )
        sprintf(returnSize, "%.2f KB", (float)Bytes/kb);
    else if ( Bytes < kb)
        sprintf(returnSize, "%.2f Bytes", Bytes);
    else
        sprintf(returnSize, "%.2f Bytes", Bytes);
    static char ret[256];
    strcpy(ret, returnSize);
    return ret;
}


int QMediaFileInfo::getHeight() const
{
    return fHeight;
}


int QMediaFileInfo::getWidth() const
{
    return fWidth;
}


void QMediaFileInfo::debugInfo(const QString &filePath)
{

}

int QMediaFileInfo::getVideoBitRae() const
{
    return fViewBitRae;
}

void QMediaFileInfo::addVideoInfo(int v, const QString &info)
{
    QString key=QString::number(v);
    videoInfoMap[key]=info;
}

QString QMediaFileInfo::getVideoInfo(int v)
{
    QString key=QString::number(v);
    return videoInfoMap[key];
}

void QMediaFileInfo::Init()
{
    qDebug()<<"ffmpeg init";
    av_register_all();
}

QMediaFileInfo::QMediaFileInfo(const QString &filePath, QObject *parent) : QObject(parent)
{
    AVCodecContext *origin_ctx = NULL;
    AVFormatContext *fmt_ctx = NULL;
    int result=0;
    QByteArray ba=filePath.toLocal8Bit();
    const char *fileName=ba.constData();

    this->fWidth=this->fHeight=0;

    result = avformat_open_input(&fmt_ctx, fileName, NULL, NULL);
    if (result < 0) {
        printf("Can't open file:%s\n", fileName);
        return ;
    }

    result = avformat_find_stream_info(fmt_ctx, NULL);
    if (result < 0) {
        qDebug()<<"Can't get stream info "<<filePath;
        return ;
    }

    int video_stream = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1,NULL, 0);
    //int audio_stream = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1,NULL, 0);
    // AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");

    if (video_stream < 0) {
        qDebug()<< "Can't find video stream in input file";
    }
    else
    {
        //视频信息:
        //视频总帧数：视频总时长 * 视频帧率;
        //视频总时长：AVFormatContext -> duration;
        int64_t tns, thh, tmm, tss;
        tns  = fmt_ctx->duration / 1000000;
        thh  = tns / 3600;
        tmm  = (tns % 3600) / 60;
        tss  = (tns % 60);
        //1000 bit/s = 1 kbit/s
        //qDebug()<<"总时长 "<<(fmt_ctx->duration * 1.0 / AV_TIME_BASE) * 1000 <<"ms,"
        //       <<"fmt:" <<thh<<":"<<tmm<<":"<<tss<<"总比特率:"<<fmt_ctx->bit_rate / 1000.0<<"kbs";

        QString str;
        double fsize = (fmt_ctx->duration * 1.0 / AV_TIME_BASE * fmt_ctx->bit_rate / 8.0);
        str.sprintf("文件大小 : %s\n",BytesToSize(fsize));
        //qDebug()<<str;
        str.sprintf("协议白名单 : %s \n协义黑名单 : %s\n",fmt_ctx->protocol_whitelist,fmt_ctx->protocol_blacklist);
        //qDebug()<<str;
        str.sprintf("数据包的最大数量 : %d\n",fmt_ctx->max_ts_probe);
        //qDebug()<<str;
        str.sprintf("最大缓冲时间 : %lld\n",fmt_ctx->max_interleave_delta);
        //qDebug()<<str;
        str.sprintf("缓冲帧的最大缓冲 : %u Bytes\n",fmt_ctx->max_picture_buffer);
        //qDebug()<<str;

        AVStream *videostream = NULL;
        videostream = fmt_ctx->streams[video_stream];
        int videoStreamIdx=video_stream;

        str.sprintf("视频流信息(%s):\n",av_get_media_type_string(videostream->codecpar->codec_type));
        //qDebug()<<str;

        str.sprintf("  Stream #%d\n",videoStreamIdx);
        //qDebug()<<str;
        str.sprintf("    总帧数 : %lld\n",videostream->nb_frames);

        addVideoInfo(QMediaFileInfo::SUM_FRAME,QString::number(videostream->nb_frames));

        //qDebug()<<str;
        const char *avcodocname = avcodec_get_name(videostream->codecpar->codec_id);
        const char *profilestring = avcodec_profile_name(videostream->codecpar->codec_id,videostream->codecpar->profile);
        //char * codec_fourcc =  av_fourcc_make_string(videostream->codecpar->codec_tag);

        str.sprintf("    编码方式 : %s\n    Codec Profile : %s\n",avcodocname,profilestring);
        //qDebug()<<str;
        ///如果是C++引用(AVPixelFormat)注意下强转类型
        const char *pix_fmt_name = videostream->codecpar->format == AV_PIX_FMT_NONE ?
                    "none" : av_get_pix_fmt_name((AVPixelFormat)videostream->codecpar->format);

        str.sprintf("    显示编码格式(color space) : %s \n",pix_fmt_name);
        //qDebug()<<str;
        str.sprintf("    宽:%d pixels,高:%d pixels\n",videostream->codecpar->width,videostream->codecpar->height);
        //qDebug()<<str;
        this->fWidth=videostream->codecpar->width;
        this->fHeight=videostream->codecpar->height;

        AVRational display_aspect_ratio;
        av_reduce(&display_aspect_ratio.num, &display_aspect_ratio.den,
                  videostream->codecpar->width * (int64_t)videostream->sample_aspect_ratio.num,
                  videostream->codecpar->height* (int64_t)videostream->sample_aspect_ratio.den,
                  1024 * 1024);
        str.sprintf("    simple_aspect_ratio(SAR) : %d : %d\n     display_aspect_ratio(DAR) : %d : %d \n",
                    videostream->sample_aspect_ratio.num,
                    videostream->sample_aspect_ratio.den,display_aspect_ratio.num,display_aspect_ratio.den);
        //qDebug()<<str;
        str.sprintf("    最低帧率 : %f fps\n    平均帧率 : %f fps\n",av_q2d(videostream->r_frame_rate),av_q2d(videostream->avg_frame_rate));
        //qDebug()<<str;
        str.sprintf("    每个像素点的比特数 : %d bits\n",videostream->codecpar->bits_per_raw_sample);
        //qDebug()<<str;
        str.sprintf("    每个像素点编码比特数 : %d bits\n",videostream->codecpar->bits_per_coded_sample); //YUV三个分量每个分量是8,即24
        //qDebug()<<str;
        str.sprintf("   视频流比特率 :%fkbps\n",videostream->codecpar->bit_rate/1000.0);
        //qDebug()<<str;
        str.sprintf("%fkbps", videostream->codecpar->bit_rate/1000.0);
        addVideoInfo(QMediaFileInfo::VIDEO_BIT_RATE,str);

        this->fViewBitRae= videostream->codecpar->bit_rate/1000;

        str.sprintf("    基准时间 : %d / %d = %f \n",videostream->time_base.num,videostream->time_base.den,av_q2d(videostream->time_base));
        //qDebug()<<str;
        str.sprintf("    视频流时长 : %f ms\n",videostream->duration * av_q2d(videostream->time_base) * 1000);
        //qDebug()<<str;
        str.sprintf("    帧率(tbr) : %f\n",av_q2d(videostream->r_frame_rate));
        //qDebug()<<str;
        str.sprintf("    文件层的时间精度(tbn) : %f\n",1/av_q2d(videostream->time_base));
        //qDebug()<<str;
        str.sprintf("    视频层的时间精度(tbc) : %f\n",1/av_q2d(videostream->codec ->time_base));
        //qDebug()<<str;
        double s = videostream->duration * av_q2d(videostream->time_base);
        int64_t tbits = videostream->codecpar->bit_rate * s;
        double stsize = tbits / 8;
        str.sprintf("    视频流大小(Bytes) : %s \n",BytesToSize(stsize));
        //qDebug()<<str;
    }
    //av_bitstream_filter_close(h264bsfc);
    avformat_close_input(&fmt_ctx);
}

