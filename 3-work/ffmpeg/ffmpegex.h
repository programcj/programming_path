﻿#ifndef __FFMPEG_EX_H__
#define __FFMPEG_EX_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/time.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"

	int ffmpeg_AVFrame2ImageJpg(AVFrame *frame,
		void(*funcall)(void *context, void *jdata, int jsize), void *context);

	int ffmpeg_YUVJ420PAVFrameToJpgData(AVFrame *frame,
		void(*funcall)(void *context, void *jdata, int jsize), void *context);

	int ffmpeg_AVFrame2ImageJpg(AVFrame *frame,
		void(*funcall)(void *context, void *jdata, int jsize), void *context);

	AVFrame *ffmpeg_imgbuff_fill_avframe(uint8_t *data, int format, int w, int h);

	int ffmpeg_SwsScale2(AVFrame *srcFrame, AVFrame *dscFrame);

	int ffmpeg_SwsScale(AVFrame *srcFrame, AVFrame *dscFrame);

	AVFrame *ffmpeg_av_frame_alloc(int format, int w, int h);

	int ffmpeg_YUVJ420PAVFrameToJpgAVPacket(AVFrame *frame, AVPacket *avpkt);
	
	int ffmpeg_AVFrame2JpgPacket(AVFrame *frame, AVPacket *avpkt);

	uint8_t *ffmpeg_crop(AVFrame *avframe, int dst_format, int x, int y, int w, int h, int *plen);

	void rect_align(int *x, int *y, int *w, int *h, int align, int bkWidth, int bkHeight);

	int ffmpeg_avframe2BGR24Buff(AVFrame *avframe, uint8_t *buff);

	int ffmpeg_imagefile_read(const char *filepath, AVFrame *avframe);


	void ffmpeg_yuvbuff_draw_box(int width, int height, int format, uint8_t *image,
				int x, int y, int w, int h,
				int R, int G, int B);


	//接收数据流
	struct ffmpegIn
	{
		AVFormatContext *fmt_ctx;
		AVStream *vstream;
		int video_stream_index;
		int video_codec_id;
		char video_codec_name[32];
		int width;
		int height;
		int fps;
	};

	//解码
	struct ffmpegDec
	{
		AVCodec *pCodec;
		AVCodecContext *pCodecCtx; //解码器上下文
		int isrkmpp;
		int def_unused;
	};

	//编码
	struct ffmpegEnc
	{
		AVFormatContext *format_ctx;
		AVRational src_time_base;
	};

	int ffmpegInOpen(struct ffmpegIn *in, const char *url);
	int ffmpegInRead(struct ffmpegIn *in, AVPacket *avpkt);
	void ffmpegInClose(struct ffmpegIn *in);

	int ffmpegDecOpen(struct ffmpegDec *dec, struct ffmpegIn *in);
	void ffmpegDecClose(struct ffmpegDec *dec);
	int ffmpegDecIn(struct ffmpegDec *fdec, AVPacket *avpkt, AVFrame *avframe);

	int ffmpegEncOpen(struct ffmpegEnc *fenc, enum AVCodecID in_video_code_id, AVCodecParameters *codecpar, const char *out_url);
	int ffmpegEncOpen2(struct ffmpegEnc *fenc, AVStream *in_stream, const char *out_url);
	void ffmpegEncClose(struct ffmpegEnc *fenc);
/***
	pkt->pts = pts; //重建pts
	pkt->dts = pts;
	pts += pkt->duration;
	av_packet_rescale_ts(pkt, fenc->src_time_base, fenc->format_ctx->streams[0]->time_base);
***/
#ifdef __cplusplus
}
#endif

#endif
