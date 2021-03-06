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
#include <unistd.h>

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
	return ret;
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
#if 1
			int fd = open(filename, O_RDWR | O_CREAT,
			S_IRWXU | S_IRWXG | S_IRWXO);
			if (fd > 0)
			{
				write(fd, pkt.data, pkt.size);
				fsync(fd);
				close(fd);
			}
#else
			FILE *fp = fopen(filename, "wb");
			if (fp)
			{
				fwrite(pkt.data, 1, pkt.size, fp);
				fclose(fp);
				ret = 0;
			}
#endif
		}

	}

	if (frame)
		av_frame_free(&frame);

	__quit: av_packet_unref(&pkt);
	avcodec_close(pCodecCtx);
	avcodec_free_context(&pCodecCtx);
	return ret;
}

/*
AVPacket avpkt;
av_init_packet(&avpkt);
avpkt.data = NULL;
avpkt.size = 0;
av_packet_unref(&avpkt);
*/
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

//align=1
int ffmpeg_NV12ToYuv420p(const unsigned char* image_src,
		unsigned char* image_dst, int image_width, int image_height)
{
	const unsigned char* pNV = image_src + image_width * image_height;
	unsigned char* pU = image_dst + image_width * image_height;
	unsigned char* pV = image_dst + image_width * image_height
			+ ((image_width * image_height) >> 2);

	memcpy(image_dst, image_src, image_width * image_height * 3 / 2);

//NV12
//Y Y Y Y
//Y Y Y Y
//Y Y Y Y
//Y Y Y Y
//U V U V
//U V U V
	for (int i = 0; i < (image_width * image_height) / 2; i++)
	{
		if ((i % 2) == 0)
			*pU++ = *(pNV + i);
		else
			*pV++ = *(pNV + i);
	}
	return 0;
}

//注意 align=1 这样yuv420pframe->linesize[1]=w/2 复制uv时就不用关心linesize
void ffmpeg_Yuv420pFrameToNV12Data(AVFrame *yuv420pframe, uint8_t *dscNv12Data)
{
	unsigned char *srcY = yuv420pframe->data[0];
	unsigned char *srcU = yuv420pframe->data[1];
	unsigned char *srcV = yuv420pframe->data[2];
	unsigned char *dstNV = dscNv12Data
			+ yuv420pframe->width * yuv420pframe->height;
	int y_h = 0;
	int u_h = 0;
	unsigned char *dstY = dscNv12Data;

	for (y_h = 0; y_h < yuv420pframe->height; y_h++)
	{
		memcpy(dstY, srcY, yuv420pframe->width);
		srcY += yuv420pframe->linesize[0];
		dstY += yuv420pframe->width;
	}

	int u_w = 0;
	for (u_h = 0; u_h < yuv420pframe->height / 2; u_h++)
	{
		for (u_w = 0; u_w < yuv420pframe->width / 2; u_w++)
		{
			(*dstNV++) = srcU[u_w];
			(*dstNV++) = srcV[u_w];
		}
		srcU+=yuv420pframe->linesize[1];
		srcV+=yuv420pframe->linesize[2];
	}
}

//align=1
//int ffmpeg_NV12ToYUV420(uint8_t *data, int w, int h, void *dsc, int align)
//{
//	uint8_t *ptr_u = 0;
//	uint8_t *ptr_v = 0;
//	uint8_t *nvptr = data + w * h;
//#define FFALIGN(x, a) (((x)+(a)-1)&~((a)-1))
//
//	int linesize = FFALIGN(w, align);
//	//av_image_get_buffer_size(AV_PIX_FMT_YUV420P,w,h,32);
//	//av_image_fill_arrays()
//	memcpy(dsc, data, w * h);
//
//	ptr_u = dsc + w * h;
//	ptr_v = dsc + w * h + linesize * (h / 2);
//
//	for (int y = 0; y < h / 2; y++)
//	{
//		for (int x = 0; x < w / 2; x++)
//		{
//			//pFrame->data[1][y * pFrame->linesize[1] + x] = pImagePacket->pBuf[y * iWidthNv12 + x * 2];
//			//pFrame->data[2][y * pFrame->linesize[2] + x] = pImagePacket->pBuf[y * iWidthNv12 + x * 2 + 1];
//
//			ptr_u[y * w / 2 + x] = nvptr[y * w + x * 2];
//			ptr_v[y * w / 2 + x] = nvptr[y * w + x * 2 + 1];
//		}
//	}
//	return 0;
//}

int ffmpeg_AVFrame_NV12ToYUV420P(AVFrame *nv12frame, AVFrame *yuv420pframe)
{
	uint8_t *ptry, *ptru, *ptrv;
	uint8_t *nvptr = nv12frame->data[1];

	int width = nv12frame->width;
	int height = nv12frame->height;
	int dsch = yuv420pframe->height;
	int dscw = yuv420pframe->width;

	memcpy(yuv420pframe->data[0], nv12frame->data[0],
			nv12frame->width * nv12frame->height);

	ptru = yuv420pframe->data[1]; //u
	ptrv = yuv420pframe->data[2]; //v

	uint8_t *ptru_tmp, *ptrv_tmp;

	for (int nvy = 0; nvy < dsch / 2 /*height / 2*/; nvy++)
	{
		ptru_tmp = ptru;
		ptrv_tmp = ptrv;

		for (int nvx = 0; nvx < dscw; nvx++)
		{
			if (nvx % 2 == 0)
			{
				*ptru++ = nvptr[nvy * width + nvx]; //u
			}
			else
			{
				*ptrv++ = nvptr[nvy * width + nvx]; //v
			}
		}

		ptru = ptru_tmp;
		ptrv = ptrv_tmp;

		//
		ptru += yuv420pframe->linesize[1];
		ptrv += yuv420pframe->linesize[2];
	}
	return 0;
}

/**
 AVFrame *j420pFrame=ffmpeg_av_frame_alloc(AV_PIX_FMT_YUVJ420P, w, h);
 ffmpeg_NV12ToYUV420PFrame(data, w, h, j420pFrame);
 av_frame_free(&j420pFrame);
 */
int ffmpeg_NV12ToYUV420PFrame(uint8_t *data, int w, int h, AVFrame *j420pFrame) {
	AVFrame *frameNV12 = av_frame_alloc();
	frameNV12->format = AV_PIX_FMT_NV12;
	frameNV12->width = w;
	frameNV12->height = h;

	av_image_fill_arrays(frameNV12->data, frameNV12->linesize, data,
			(enum AVPixelFormat) frameNV12->format, frameNV12->width,
			frameNV12->height, 1);
	ffmpeg_AVFrame_NV12ToYUV420P(frameNV12, j420pFrame);
	av_frame_free(&frameNV12);
	return 0;
}

void ffmpeg_yuvbuff_draw_box(int width, int height, int format, uint8_t *image,
						  int size, int x, int y, int w, int h, int R, int G, int B)
{
	/**
	 * NV12
	 *
	 //Y Y Y Y
	 //Y Y Y Y
	 //Y Y Y Y
	 //Y Y Y Y
	 //U V U V
	 //U V U V
	 */
	uint8_t *dstY = image;
	uint8_t *dstUV = image + (width * height);
	uint8_t *dstU = dstUV;
	uint8_t *dstV = dstU + (width * height) / 4;

	int maxx = x + w;
	int maxy = y + h;
	int px; //横线
	int py; //竖线

	uint8_t _YColor = R;
	uint8_t _UColor = B;
	uint8_t _VColor = G;

	//通过Y的起点计算UV点: (x=5, y=6)  (Y= W*y+x, U_420P=V420P= (y/2)*(width/2)+x/2

	for (px = x; px < maxx; px += 2) //画横线
	{
		//上横线
		dstY[width * y + px] = _YColor;
		dstY[width * y + px + 1] = _YColor;
		dstY[width * y + width + px] = _YColor;
		dstY[width * y + width + px + 1] = _YColor;
		//下横线
		dstY[width * maxy + px] = _YColor;
		dstY[width * maxy + px + 1] = _YColor;
		dstY[width * (maxy + 1) + px] = _YColor;
		dstY[width * (maxy + 1) + px + 1] = _YColor;

		if (AV_PIX_FMT_NV12 == format)
		{
			//上横线UV
			dstUV[width * (y / 2) + px] = _UColor;
			dstUV[width * (y / 2) + px + 1] = _VColor;
			//下横线UV
			dstUV[width * (maxy / 2) + px] = _UColor;
			dstUV[width * (maxy / 2) + px + 1] = _VColor;
		}

		if (AV_PIX_FMT_YUV420P == format)
		{
			//上横线UV
			dstU[width/2 * (y / 2) + px/2] = _UColor;
			dstV[width/2 * (y / 2) + px/2] = _VColor;

			//下横线UV
			dstU[width/2 * (maxy / 2) + px/2] = _UColor;
			dstV[width/2 * (maxy / 2) + px/2] = _VColor;
		}
	}

	for (py = y; py <= maxy; py += 2) //画竖线
	{
		//左竖线
		dstY[width * py + x] = _YColor;
		dstY[width * py + x + 1] = _YColor;
		dstY[width * py + x + width] = _YColor;
		dstY[width * py + x + width + 1] = _YColor;

		//右竖线
		dstY[width * py + maxx] = _YColor;
		dstY[width * py + maxx + 1] = _YColor;
		dstY[width * (py + 1) + maxx] = _YColor;
		dstY[width * (py + 1) + maxx + 1] = _YColor;

		if (AV_PIX_FMT_NV12 == format)
		{
			//左竖线
			dstUV[width * (py / 2) + x] = _UColor;
			dstUV[width * (py / 2) + x + 1] = _VColor;

			//右竖线
			dstUV[width * (py / 2) + maxx] = _UColor;
			dstUV[width * (py / 2) + maxx + 1] = _VColor;
		}
		if (AV_PIX_FMT_YUV420P == format)
		{ //左竖线
			dstU[width/2 * (py / 2) + x/2] = _UColor;
			dstV[width/2 * (py / 2) + x/2] = _VColor;

			//右竖线
			dstU[width/2 * (py / 2) + maxx/2] = _UColor;
			dstV[width/2 * (py / 2) + maxx/2] = _VColor;
		}
	}
}


