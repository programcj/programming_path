#include "stdio.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavutil/imgutils.h"
#include "libavutil/pixdesc.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"

void file_read(const char* path, void** data, int* dlen)
{
    int fd = 0;
    fd = open(path, O_RDONLY, S_IRUSR);
    if (fd < 0) {
        perror("file not open!");
        return;
    }

    int len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    printf("fd=%d,len=%d\n", fd, len);

    *dlen = len;
    *data = malloc(len);
    read(fd, *data, len);
    close(fd);
}

void avframe_encode_jpg_file(const AVFrame* srcframe)
{
    AVCodec* codec = NULL;
    AVCodecContext* ctx = NULL;
    AVPacket pkt;

    codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (codec == NULL) {
        fprintf(stderr, "not find encode\n");
        return;
    }

    ctx = avcodec_alloc_context3(codec);
    if (ctx == NULL) {
        fprintf(stderr, "avcodec_alloc_context3 err!");
        return;
    }
    //ctx->codec_id = srcframe->format;
    //ctx->codec_type = AVMEDIA_TYPE_VIDEO;

    ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    ctx->width = srcframe->width;
    ctx->height = srcframe->height;

    ctx->time_base.num = 1;
    ctx->time_base.den = 16;
    //ctx->max_b_frames = 1;
    ctx->bit_rate = 240000;

    if (0 > avcodec_open2(ctx, codec, NULL)) {

        perror("avcodec_open2 err!\n");
        return;
    }

    av_init_packet(&pkt);

    //int ret = avformat_write_header(ctx, NULL);

    avcodec_send_frame(ctx, srcframe);
    avcodec_receive_packet(ctx, &pkt);
    {
        FILE* fp = fopen("/tmp/aaa.jpg", "wb");
        if (fp) {
            fwrite(pkt.data, 1, pkt.size, fp);
            fclose(fp);
        }
    }
    av_packet_unref(&pkt);

    avcodec_close(ctx);
    avcodec_free_context(&ctx);
}

void avframe_to_nv12(AVFrame* srcframe)
{

    int ret = 0;
    AVFrame* frame = av_frame_alloc();

    frame->format = AV_PIX_FMT_NV12;
    frame->width = srcframe->width;
    frame->height = srcframe->height;

#if 1

    //W*H*3/2
    int len = avpicture_get_size(frame->format, frame->width, frame->height);
    uint8_t* out_buffer = (uint8_t*)malloc(len); //need free,

    printf("NV12 len:%d\n", len); //可以将out_buffer写入文件，即变成NV12格式的照片了

    av_image_fill_arrays(frame->data, frame->linesize, out_buffer,
        AV_PIX_FMT_NV12, frame->width, frame->height, 1);

    {
        int y;
        AVFrame* pFrame = srcframe;
        unsigned char* cpyAddr = frame->data[0];

        //Y
        for (y = 0; y < pFrame->height; y++) {
            memcpy(cpyAddr, pFrame->data[0] + y * pFrame->linesize[0],
                pFrame->width);
            cpyAddr += pFrame->width;
        }

        unsigned char* DstU = frame->data[1]; //UV

        for (int i = 0; i < pFrame->width * pFrame->height / 4; i++) {
            (*DstU++) = pFrame->data[1][i]; //(*SrcU++);
            (*DstU++) = pFrame->data[2][i]; //(*SrcV++);
        }
    }
#else
    //源格式转换成NV12
    struct SwsContext* m_pSwsContext;
    m_pSwsContext = sws_getContext(srcframe->width, srcframe->height,
        (enum AVPixelFormat)srcframe->format, srcframe->width,
        srcframe->height, AV_PIX_FMT_NV12, SWS_BICUBIC, NULL, NULL,
        NULL);

    frame->format = AV_PIX_FMT_NV12;
    frame->width = srcframe->width;
    frame->height = srcframe->height;
    av_frame_get_buffer(frame, 32);
    ret = av_frame_make_writable(frame);

    //还可以裁剪
    ret = sws_scale(m_pSwsContext, srcframe->data, srcframe->linesize, 0,
        srcframe->height, frame->data, frame->linesize);

    sws_freeContext(m_pSwsContext);
#endif

    printf("decode=%s WH:%dx%d\n",
        av_pix_fmt_desc_get((enum AVPixelFormat)frame->format)->name,
        frame->width,
        frame->height); //解码

    av_frame_free(&frame);
    free(out_buffer);
}

// "-lavcodec",
// "-lavdevice",
// "-lavfilter",
// "-lavformat",
// "-lavutil",
// "-lswresample",
// "-lswscale",
// "-lm",
// "-lz"
int main(int argc, char const* argv[])
{
    uint8_t* data = NULL;
    int len = 0;
    int ret = 0;
    file_read("/home/cc/下载/200744wrnuejnknbzb6j8n.jpg", &data, &len);

    printf("len=%d\n", len);

    AVCodec* pCodec;
    AVCodecContext* pCodecCtx;
    AVFrame* pAvFrame;

    avcodec_register_all();

    pCodec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_open2(pCodecCtx, pCodec, NULL);

    pAvFrame = av_frame_alloc();
    {
        AVPacket picPacket;
        av_init_packet(&picPacket);
        picPacket.data = (uint8_t*)data;
        picPacket.size = len;

        ret = avcodec_send_packet(pCodecCtx, &picPacket);
        ret = avcodec_receive_frame(pCodecCtx, pAvFrame);
        printf("decode=%s WH:%dx%d\n",
            av_pix_fmt_desc_get((enum AVPixelFormat)pAvFrame->format)->name,
            pAvFrame->width,
            pAvFrame->height); //解码
        //
        avframe_to_nv12(pAvFrame);

        avframe_encode_jpg_file(pAvFrame);
    }
    avcodec_free_context(&pCodecCtx);
    return 0;
}
