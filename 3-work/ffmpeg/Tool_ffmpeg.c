/*
 * Tool_ffmpeg.c
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
 * 请注意编码格式utf8
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/prctl.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Tool_ffmpeg.h"

int FFmpegMJPEGDecoder_Init(struct FFmpegMJPEGDecoder *fdec)
{
	int ret = 0;
	memset(fdec, 0, sizeof(struct FFmpegMJPEGDecoder));
	fdec->pCodec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
	fdec->pCodecCtx = avcodec_alloc_context3(fdec->pCodec); //解码器初始化
	if (!fdec->pCodecCtx)
		return -1;

	ret = avcodec_open2(fdec->pCodecCtx, fdec->pCodec, NULL);  //打开解码器
	if (ret < 0)
	{
		av_log(fdec->pCodecCtx, AV_LOG_ERROR, "Can't open decoder\n");
		return -1;
	}
	fdec->j420pFrame = av_frame_alloc();
	if (!fdec->j420pFrame)
	{
		av_log(NULL, AV_LOG_ERROR, "Can't allocate frame\n");
		ret = AVERROR(ENOMEM);

		return ret;
	}
	return ret;
}

int FFmpegMJPEGDecoder_DecoderJPGData(struct FFmpegMJPEGDecoder *ffDecoder,
		void *jdata, int jsize, AVFrame *j420pFrame)
{
	int ret = 0;
	AVPacket pkt;

	av_init_packet(&pkt);
	pkt.data = jdata;
	pkt.size = jsize;

	avcodec_flush_buffers(ffDecoder->pCodecCtx);
	ret = avcodec_send_packet(ffDecoder->pCodecCtx, &pkt);
	if (ret >= 0)
		ret = avcodec_receive_frame(ffDecoder->pCodecCtx, j420pFrame);
	av_packet_unref(&pkt);
	return ret;
}

void FFmpegMJPEGDecoder_Destory(struct FFmpegMJPEGDecoder *fdec)
{
	avcodec_close(fdec->pCodecCtx);
	avcodec_free_context(&fdec->pCodecCtx);
	av_frame_free(&fdec->j420pFrame);
}

//ffmpeg 解码jpg图片
// AVFrame *j420pFrame= av_frame_alloc();
// if( ffmpeg_DecodeJpgDataToAVFrame(jdata, jsize, j420pFrame) ==0) )
// {  ...decode ok }
// av_frame_free(&j420pFrame);
int ffmpeg_DecodeJpgDataToAVFrame(void *jdata, int jsize, AVFrame *j420pFrame)
{
	AVCodec *pCodec;
	AVCodecContext *pCodecCtx;
	int ret = 0;
	AVPacket pkt;

	pCodec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
	if (!pCodec)
	{
		fprintf(stderr, "[ffmpeg-avcode] decoder not mjpeg\n");
		return -1;
	}

	pCodecCtx = avcodec_alloc_context3(pCodec); //解码器初始化
	if (!pCodecCtx)
	{
		fprintf(stderr, "[ffmpeg-avcode] mjpeg not alloc context\n");
		return -1;
	}

	AVDictionary *opts = NULL;

	ret = avcodec_open2(pCodecCtx, pCodec, &opts);  //打开解码器
	if (ret != 0)
	{
		fprintf(stderr, "[ffmpeg-avcode] mjpeg not open context\n");
		avcodec_free_context(&pCodecCtx);
		return -1;
	}

	av_init_packet(&pkt);
	pkt.data = jdata;
	pkt.size = jsize;

	//avcodec_flush_buffers(pCodecCtx);
	ret = avcodec_send_packet(pCodecCtx, &pkt);
	if (ret >= 0)
		ret = avcodec_receive_frame(pCodecCtx, j420pFrame);
	av_packet_unref(&pkt);

	avcodec_close(pCodecCtx);
	avcodec_free_context(&pCodecCtx);
	return 0;
}

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
int ffmpeg_SwsScale(AVFrame *srcFrame, AVFrame *dscFrame)
{
	struct SwsContext *m_pSwsContext;
	int ret = 0;
	m_pSwsContext = sws_getContext(srcFrame->width, srcFrame->height,
			(enum AVPixelFormat) srcFrame->format, dscFrame->width,
			dscFrame->height, dscFrame->format,
			SWS_BICUBIC, NULL, NULL,
			NULL);
	ret = sws_scale(m_pSwsContext, srcFrame->data, srcFrame->linesize, 0,
			srcFrame->height, dscFrame->data, dscFrame->linesize);
	sws_freeContext(m_pSwsContext);
	return ret;
}

int ffmpeg_YUVJ420PAVFrameToJpgData(AVFrame *frame,
		void (*funcall)(void *context, void *jdata, int jsize), void *context)
{
	AVCodec *pCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
	AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
	int ret = 0;

	if (!pCodecCtx)
	{
		fprintf(stderr, "[ffmpeg-avcodec] encode mjpeg not open context\n");
		return -1;
	}

	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	//cropframe->format;
	pCodecCtx->width = frame->width;
	pCodecCtx->height = frame->height;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;

	// 设置pCodecCtx的编码器为pCodec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		fprintf(stderr, "[ffmpeg-avcodec] Could not open codec ! \n");
		avcodec_free_context(&pCodecCtx);
		exit(1);
	}
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	ret = avcodec_send_frame(pCodecCtx, frame);
	if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
	{
		fprintf(stderr,
				"[ffmpeg-send-frame] error sending a frame for Encodec.\n");
		goto __quit;
		return -1;
	}

	while (1)
	{
		if (avcodec_receive_packet(pCodecCtx, &pkt) == AVERROR(EAGAIN))
		{
			break;
		}

		funcall(context, pkt.data, pkt.size);
	}

	__quit: av_packet_unref(&pkt);
	avcodec_close(pCodecCtx);
	avcodec_free_context(&pCodecCtx);
	return ret;
}

AVFrame *ffmpeg_av_frame_alloc(int format, int w, int h)
{
	AVFrame *newframe = av_frame_alloc();

	newframe->format = format; //pFrame->format;
	newframe->width = w;
	newframe->height = h;

	if (av_frame_get_buffer(newframe, 1) < 0)
	{
		av_frame_free(&newframe);
		return NULL;
	}

	if (av_frame_make_writable(newframe) < 0)
	{
		av_frame_free(&newframe);
		return NULL;
	}
	return newframe;
}

int ffmpeg_AVFrameSaveJpgFile(AVFrame *srcframe, const char *filename)
{
	AVCodec *pCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
	AVCodecContext *pCodecCtx = NULL;
	int ret = 0;

	AVFrame *frame = NULL;

	if (srcframe->format != AV_PIX_FMT_YUVJ420P
			|| srcframe->format != AV_PIX_FMT_YUV420P)
	{
		frame = ffmpeg_av_frame_alloc(AV_PIX_FMT_YUVJ420P, srcframe->width,
				srcframe->height);

		if (!frame)
			return -1;

		ffmpeg_SwsScale(srcframe, frame);
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx)
	{
		fprintf(stderr, "[ffmpeg-avcodec] encode mjpeg not open context\n");
		return -1;
	}

	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	//cropframe->format;
	pCodecCtx->width = frame->width;
	pCodecCtx->height = frame->height;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;

	// 设置pCodecCtx的编码器为pCodec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		fprintf(stderr, "[ffmpeg-avcodec] Could not open codec ! \n");
		avcodec_free_context(&pCodecCtx);
		exit(1);
	}

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	ret = avcodec_send_frame(pCodecCtx, frame ? frame : srcframe);

	if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
	{
		fprintf(stderr,
				"[ffmpeg-send-frame] error sending a frame for Encodec.\n");
		goto __quit;
		return -1;
	}

	while (1)
	{
		if (avcodec_receive_packet(pCodecCtx, &pkt) == AVERROR(EAGAIN))
		{
			break;
		}

		{
			int fd = open(filename, O_RDWR | O_CREAT);
			if (fd > 0)
			{
				write(fd, pkt.data, pkt.size);
				fsync(fd);
				close(fd);
			}
//			FILE *fp = fopen(filename, "wb");
//			if (fp)
//			{
//				fwrite(pkt.data, 1, pkt.size, fp);
//				fclose(fp);
//				ret = 0;
//			}
		}
	}

	if (frame)
		av_frame_free(&frame);

	__quit: av_packet_unref(&pkt);
	avcodec_close(pCodecCtx);
	avcodec_free_context(&pCodecCtx);
	return ret;
}

int ffmpeg_YUVJ420PAVFrameToJpgAVPacket(AVFrame *frame, AVPacket *avpkt)
{
	AVCodec *pCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
	AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
	int ret = 0;

	if (!pCodecCtx)
	{
		fprintf(stderr, "[ffmpeg-avcodec] encode mjpeg not open context\n");
		return -1;
	}

	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
	//cropframe->format;
	pCodecCtx->width = frame->width;
	pCodecCtx->height = frame->height;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;

	// 设置pCodecCtx的编码器为pCodec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		fprintf(stderr, "[ffmpeg-avcodec] Could not open codec ! \n");
		avcodec_free_context(&pCodecCtx);
		exit(1);
	}

	ret = avcodec_send_frame(pCodecCtx, frame);
	if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
	{
		fprintf(stderr,
				"[ffmpeg-send-frame] error sending a frame for Encodec.\n");
		goto __quit;
		return -1;
	}

	while (1)
	{
		if (avcodec_receive_packet(pCodecCtx, avpkt) == AVERROR(EAGAIN))
			break;
		break;
	}

	__quit: avcodec_close(pCodecCtx);
	avcodec_free_context(&pCodecCtx);
	return ret;
}


