/*
 * Tool_ffmpeg.h
 *
 *  Created on: 2019年6月25日
 *      Author: cc
 *
 *                .-~~~~~~~~~-._       _.-~~~~~~~~~-.
 *            __.'              ~.   .~              `.__
 *          .'//                  \./                  \\`.
 *        .'//                     |                     \\`.
 *      .'// .-~"""""""~~~~-._     |     _,-~~~~"""""""~-. \\`.
 *    .'//.-"                 `-.  |  .-'                 "-.\\`.
 *  .'//______.============-..   \ | /   ..-============.______\\`.
 *.'______________________________\|/______________________________`.
 *.'_________________________________________________________________`.
 * 
 * 请注意编码格式
 */

#ifndef APP_TOOL_FFMPEG_H_
#define APP_TOOL_FFMPEG_H_

#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
#include "libavutil/pixfmt.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct FFmpegMJPEGDecoder
{
	AVCodec *pCodec;
	AVCodecContext *pCodecCtx;
	AVFrame *j420pFrame;

	struct
	{
		uint8_t *data;
		int size;
	} userJpgData;
};

int FFmpegMJPEGDecoder_Init(struct FFmpegMJPEGDecoder *fdec);

int FFmpegMJPEGDecoder_DecoderJPGData(struct FFmpegMJPEGDecoder *ffDecoder,
		void *jdata, int jsize, AVFrame *j420pFrame);

void FFmpegMJPEGDecoder_Destory(struct FFmpegMJPEGDecoder *fdec);

//ffmpeg 解码jpg图片
// AVFrame *j420pFrame= av_frame_alloc();
// if( ffmpeg_DecodeJpgDataToAVFrame(jdata, jsize, j420pFrame) ==0) )
// {  ...decode ok }
// av_frame_free(&j420pFrame);
int ffmpeg_DecodeJpgDataToAVFrame(void *jdata, int jsize, AVFrame *j420pFrame);

//
//AVFrame *nv12Frame = av_frame_alloc();
//int nv12len = av_image_get_buffer_size(AV_PIX_FMT_NV12,
//						j420pFrame->width,
//						j420pFrame->height, 1);
//
//uint8_t *nv12data = av_malloc(nv12len);
//
//nv12Frame->format = AV_PIX_FMT_NV12;
//nv12Frame->width = j420pFrame->width;
//nv12Frame->height = j420pFrame->height;
//
//av_image_fill_arrays(nv12Frame->data, nv12Frame->linesize,
//		nv12data, AV_PIX_FMT_NV12, nv12Frame->width,
//		nv12Frame->height, 1);
int ffmpeg_SwsScale(AVFrame *srcFrame, AVFrame *dscFrame);

int ffmpeg_YUVJ420PAVFrameToJpgData(AVFrame *frame,
		void (*funcall)(void *context, void *jdata, int jsize), void *context);

AVFrame *ffmpeg_av_frame_alloc(int format, int w, int h);

int ffmpeg_AVFrameSaveJpgFile(AVFrame *srcframe, const char *filename);

//AVPacket pkt;
//av_init_packet(&avpkt);
//pkt.data = NULL;
//pkt.size = 0;
//av_packet_unref(&avpkt);
int ffmpeg_YUVJ420PAVFrameToJpgAVPacket(AVFrame *frame, AVPacket *avpkt);

#ifdef __cplusplus
}
#endif

#endif /* APP_TOOL_FFMPEG_H_ */
