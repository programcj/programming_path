#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

#include "ffmpegex.h"
#include "MPP_Frame.h"

//YUV420截图
void YUV420P_ImagePtr_crop(
			uint8_t *ptrY,
			uint8_t *ptrU,
			uint8_t *ptrV,
			//			int src_w,
			//			int src_h,
			int src_yhor, //Y��ˮƽ
			int src_uhor, //Uˮƽ���(w/2)
			int src_vhor, //Vˮƽ���(w/2)
			uint8_t *dstbuff,
			int dst_format,
			int dst_x, int dst_y, int dst_w, int dst_h)
{
	uint8_t *src_Y = ptrY;
	uint8_t *src_U = ptrU;
	uint8_t *src_V = ptrV;

	uint8_t *dst_Y = dstbuff;
	uint8_t *dst_UV = dst_Y + (dst_w * dst_h);
	uint8_t *dst_U = dst_UV;
	uint8_t *dst_V = dst_UV + (dst_w * dst_h) / 4;

	int column_x, line_y;
	int dst_max_x = dst_x + dst_w;
	int dst_max_y = dst_y + dst_h;

	//X�� Y ��
	for (line_y = dst_y; line_y < dst_max_y; line_y++)
	{
		//һ��һ�и���
		memcpy(dst_Y, src_Y + (line_y * src_yhor) + dst_x, dst_w);
		dst_Y += dst_w;

		if (line_y % 2 == 0) //UV����
		{
			//line_y : 0 2 4 6 8 10 12 14
			//dst_y/2: 0 1 2 3 4  5  6
			if (dst_format == AV_PIX_FMT_YUV420P || dst_format == AV_PIX_FMT_YUVJ420P) //Ϊ�߿��һ��
			{
				memcpy(dst_U, src_U + (line_y / 2) * src_uhor + dst_x / 2, dst_w / 2);
				memcpy(dst_V, src_V + (line_y / 2) * src_vhor + dst_x / 2, dst_w / 2);
				dst_U += dst_w / 2;
				dst_V += dst_w / 2;
			}
			if (dst_format == AV_PIX_FMT_NV12)
			{
				//dst_UV
				//YYYYYYYY
				//YYYYYYYY
				//YYYYYYYY
				//YYYYYYYY
				//UVUVUVUV
				//UVUVUVUV
				for (column_x = 0; column_x < dst_w / 2; column_x++)
				{
					//(line_y / 2)* hor_stride //��һ��
					*dst_UV++ = src_U[(line_y / 2) * src_uhor + dst_x / 2 + column_x];
					*dst_UV++ = src_V[(line_y / 2) * src_uhor + dst_x / 2 + column_x];
				}
			}
		}
	}
}

//NV12截图
void NV12_ImagePtr_crop(uint8_t *src_Y,
			uint8_t *src_UV,
			int src_w,
			int src_h,
			int src_yhor,  //Yˮƽ���
			int src_uvhor, //UVˮƽ���
			uint8_t *dstbuff,
			int dst_format,
			int dst_x, int dst_y, int dst_w, int dst_h)
{
	uint8_t *dst_Y = dstbuff;
	uint8_t *dst_UV = dst_Y + (dst_w * dst_h);
	uint8_t *dst_U = dst_UV;
	uint8_t *dst_V = dst_UV + (dst_w * dst_h) / 4;

	int line_x, line_y;
	int dst_max_x = dst_x + dst_w;
	int dst_max_y = dst_y + dst_h;

	for (line_y = dst_y; line_y < dst_max_y; line_y++)
	{
		memcpy(dst_Y, src_Y + (line_y * src_yhor) + dst_x, dst_w); //Y����
		dst_Y += dst_w;

		if (line_y % 2 == 0) //UV����
		{
			if (dst_format == AV_PIX_FMT_NV12)
			{
				memcpy(dst_UV, src_UV + (line_y / 2) * src_uvhor + dst_x, dst_w);
				dst_UV += dst_w;
			}
			if (dst_format == AV_PIX_FMT_YUV420P || dst_format == AV_PIX_FMT_YUVJ420P)
			{
				for (line_x = dst_x; line_x < dst_max_x; line_x += 2)
				{
					*dst_U++ = src_UV[(line_y / 2) * src_uvhor + line_x];
					*dst_V++ = src_UV[(line_y / 2) * src_uvhor + line_x + 1];
				}
			}
		}
	}
}

//NV12截图
static void yuv_drm_image_crop(uint8_t *src, int src_w, int src_h,
			int hor_stride,
			int ver_stride,
			uint8_t *dstbuff,
			int dst_format, int dst_x, int dst_y, int dst_w, int dst_h)
{
	uint8_t *src_Y = src;
	uint8_t *src_UV = src + (hor_stride * ver_stride);

	uint8_t *dst_Y = dstbuff;
	uint8_t *dst_UV = dst_Y + (dst_w * dst_h);
	uint8_t *dst_U = dst_UV;
	uint8_t *dst_V = dst_UV + (dst_w * dst_h) / 4;

	int line_x, line_y;
	int dst_max_x = dst_x + dst_w;
	int dst_max_y = dst_y + dst_h;

	for (line_y = dst_y; line_y < dst_max_y; line_y++)
	{
		memcpy(dst_Y, src_Y + (line_y * hor_stride) + dst_x, dst_w); //Y分量
		dst_Y += dst_w;

		if (line_y % 2 == 0) //UV分量
		{
			if (dst_format == AV_PIX_FMT_NV12)
			{
				memcpy(dst_UV, src_UV + (line_y / 2) * hor_stride + dst_x, dst_w);
				dst_UV += dst_w;
			}
			if (dst_format == AV_PIX_FMT_YUV420P || dst_format == AV_PIX_FMT_YUVJ420P)
			{
				for (line_x = dst_x; line_x < dst_max_x; line_x += 2)
				{
					*dst_U++ = src_UV[(line_y / 2) * hor_stride + line_x];
					*dst_V++ = src_UV[(line_y / 2) * hor_stride + line_x + 1];
				}
			}
		}
	}
}

//@avframe: AVFrame
//@dst_format: AV_PIX_FMT_YUV420P/AV_PIX_FMT_NV12
uint8_t* ffmpeg_crop(AVFrame *avframe, int dst_format, int x, int y, int w, int h, int *plen)
{
	int imglen = av_image_get_buffer_size((enum AVPixelFormat) dst_format, w, h, 1);
	uint8_t *img = NULL;

#ifdef CONFIG_HAVE_RKMPP
	if (avframe->format == AV_PIX_FMT_DRM_PRIME)
	{
		MppFrame mppframe = AVFrame2MppFrame(avframe);
		MppBuffer buff = mpp_frame_get_buffer(mppframe);
		img = (uint8_t*) av_malloc(imglen);
		yuv_drm_image_crop((uint8_t*) mpp_buffer_get_ptr(buff),
					avframe->width,
					avframe->height,
					mpp_frame_get_hor_stride(mppframe),
					mpp_frame_get_ver_stride(mppframe),
					img,
					dst_format, x, y, w, h);
		return img;
	}
#endif
	if (plen)
		*plen = imglen;

	if (dst_format == AV_PIX_FMT_NV12 ||
				dst_format == AV_PIX_FMT_YUV420P ||
				dst_format == AV_PIX_FMT_YUVJ420P)
	{
		if (avframe->format == AV_PIX_FMT_YUV420P || avframe->format == AV_PIX_FMT_YUVJ420P)
		{
			img = (uint8_t*) av_malloc(imglen);
			YUV420P_ImagePtr_crop(avframe->data[0],
						avframe->data[1],
						avframe->data[2],
						//					avframe->width,
						//					avframe->height,
						avframe->linesize[0],
						avframe->linesize[1],
						avframe->linesize[2],
						img,
						dst_format,
						x, y, w, h);
			return img;
		}

		if (avframe->format == AV_PIX_FMT_NV12)
		{
			img = (uint8_t*) av_malloc(imglen);
			NV12_ImagePtr_crop(avframe->data[0],
						avframe->data[1],
						avframe->width,
						avframe->height,
						avframe->linesize[0],
						avframe->linesize[1],
						img,
						dst_format,
						x, y, w, h);
			return img;
		}
	}
	return img;
}

AVFrame* ffmpeg_imgbuff_fill_avframe(uint8_t *data, int format, int w, int h)
{
	AVFrame *frame = av_frame_alloc();
	frame->format = format;
	frame->width = w;
	frame->height = h;
	av_image_fill_arrays(frame->data, frame->linesize, data,
				(enum AVPixelFormat) frame->format, frame->width, frame->height, 1);
	return frame;
}

#if 0
int ffmpeg_SwsScale(AVFrame *srcFrame, AVFrame *dscFrame)
{
	struct SwsContext *m_pSwsContext;
	int ret = 0;

	enum AVPixelFormat srcformat = (enum AVPixelFormat) srcFrame->format;

	if (srcformat == AV_PIX_FMT_YUVJ420P)
		srcformat = AV_PIX_FMT_YUV420P;

	m_pSwsContext = sws_getContext(srcFrame->width, srcFrame->height,
				srcformat, dscFrame->width,
				dscFrame->height, (enum AVPixelFormat) dscFrame->format,
				SWS_BICUBIC, NULL, NULL,
				NULL);
	ret = sws_scale(m_pSwsContext, (const uint8_t**) srcFrame->data, srcFrame->linesize, 0,
				srcFrame->height, dscFrame->data, dscFrame->linesize);
	sws_freeContext(m_pSwsContext);
	return ret;
}
#endif

int ffmpeg_SwsScale(AVFrame *srcFrame, AVFrame *dscFrame)
{
	struct SwsContext *m_pSwsContext;
	int ret = 0;
	int srcStride[AV_NUM_DATA_POINTERS];
	uint8_t *srcSlice[AV_NUM_DATA_POINTERS];
	enum AVPixelFormat srcformat = (enum AVPixelFormat) srcFrame->format;

	memset(srcStride, 0, sizeof(srcStride));

	if (srcformat == AV_PIX_FMT_YUVJ420P)
		srcformat = AV_PIX_FMT_YUV420P;

	if (srcformat == AV_PIX_FMT_DRM_PRIME)
	{
		MppFrame mppframe = AVFrame2MppFrame(srcFrame);
		MppBuffer mppbuff = mpp_frame_get_buffer(mppframe);
		uint8_t *drmptr = (uint8_t*) mpp_buffer_get_ptr(mppbuff);
		int hor = mpp_frame_get_hor_stride(mppframe);
		int ver = mpp_frame_get_ver_stride(mppframe);
		//mpp数据
		srcStride[0] = mpp_frame_get_hor_stride(mppframe);
		srcStride[1] = mpp_frame_get_hor_stride(mppframe);
		srcSlice[0] = drmptr;
		srcSlice[1] = drmptr + hor * ver;
		srcformat = AV_PIX_FMT_NV12;
	}
	else
	{
		srcSlice[0] = srcFrame->data[0];
		srcSlice[1] = srcFrame->data[1];
		srcSlice[2] = srcFrame->data[2];

		srcStride[0] = srcFrame->linesize[0];
		srcStride[1] = srcFrame->linesize[1];
		srcStride[2] = srcFrame->linesize[2];
	}

	m_pSwsContext = sws_getContext(
				srcFrame->width,
				srcFrame->height,
				srcformat,
				dscFrame->width,
				dscFrame->height,
				(enum AVPixelFormat) dscFrame->format,
				SWS_FAST_BILINEAR, //SWS_BICUBIC
				NULL, NULL, NULL);

	ret = sws_scale(m_pSwsContext,
				(const uint8_t**) srcSlice,
				srcStride, 0,
				srcFrame->height,
				dscFrame->data,
				dscFrame->linesize);
	sws_freeContext(m_pSwsContext);
	return ret;
}

int ffmpeg_avframe2BGR24Buff(AVFrame *avframe, uint8_t *buff)
{
	enum AVPixelFormat dst_format = AV_PIX_FMT_BGR24;
	int ret;
	AVFrame *dstframe = ffmpeg_imgbuff_fill_avframe(buff, dst_format, avframe->width, avframe->height);
	ret = ffmpeg_SwsScale(avframe, dstframe);
	av_frame_free(&dstframe);
	return ret;
}

AVFrame* ffmpeg_av_frame_alloc(int format, int w, int h)
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

	//
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		fprintf(stderr, "[ffmpeg-avcodec] Could not open codec ! ffmpeg_YUVJ420PAVFrameToJpgData\n");
		avcodec_free_context(&pCodecCtx);
		return -1;
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

	__quit:
	av_packet_unref(&pkt);
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
int ffmpeg_AVFrame2JpgPacket(AVFrame *frame, AVPacket *avpkt)
{
	AVFrame *avframe420p = NULL;
	int ret = 0;

	if (frame->format != AV_PIX_FMT_YUV420P && frame->format != AV_PIX_FMT_YUVJ420P)
	{
		avframe420p = ffmpeg_av_frame_alloc(AV_PIX_FMT_YUV420P, frame->width, frame->height);
		if (avframe420p == NULL)
			return -1;
		ret = ffmpeg_SwsScale(frame, avframe420p);
	}
	ret = ffmpeg_YUVJ420PAVFrameToJpgAVPacket(avframe420p ? avframe420p : frame, avpkt);
	if (avframe420p)
		av_frame_free(&avframe420p);
	return 0;
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
	AVCodec *pCodec = NULL;
	AVCodecContext *pCodecCtx = NULL;
	int ret = 0;

	if (frame->format != AV_PIX_FMT_YUV420P && frame->format != AV_PIX_FMT_YUVJ420P)
	{
		fprintf(stderr, "AVFrame2Jpg format err:%d\r\n", frame->format);
		return -1;
	}

	pCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
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

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		fprintf(stderr, "[ffmpeg-avcodec] Could not open codec ! ffmpeg_YUVJ420PAVFrameToJpgAVPacket\n");
		avcodec_free_context(&pCodecCtx);
		return -1;
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

	__quit:
	avcodec_close(pCodecCtx);
	avcodec_free_context(&pCodecCtx);
	return ret;
}

int ffmpeg_AVFrame2ImageJpg(AVFrame *avframe, void (*funcall)(void *context, void *jdata, int jsize), void *context)
{
	AVFrame *avframe420p = NULL;
	int ret = 0;

	if (avframe->format != AV_PIX_FMT_YUV420P && avframe->format != AV_PIX_FMT_YUVJ420P)
	{
		avframe420p = ffmpeg_av_frame_alloc(AV_PIX_FMT_YUV420P, avframe->width, avframe->height);
		if (avframe420p == NULL)
			return -1;
		ret = ffmpeg_SwsScale(avframe, avframe420p);
	}
	ret = ffmpeg_YUVJ420PAVFrameToJpgData(avframe420p ? avframe420p : avframe, funcall, context);
	if (avframe420p)
		av_frame_free(&avframe420p);
	return ret;
}

int ffmpeg_DecodeDataToAVFrame(int codec_id, const void *jdata, int jsize, AVFrame *avframe)
{
	AVCodec *pCodec;
	AVCodecContext *pCodecCtx;
	int ret = 0;
	AVPacket pkt;

	if (jsize == 0 || jdata == NULL)
		return -1;

	pCodec = avcodec_find_decoder((enum AVCodecID) codec_id);
	if (!pCodec)
	{
		fprintf(stderr, "[ffmpeg-avcode] decoder not mjpeg\n");
		return -1;
	}

	pCodecCtx = avcodec_alloc_context3(pCodec); //��������ʼ��
	if (!pCodecCtx)
	{
		fprintf(stderr, "[ffmpeg-avcode] mjpeg not alloc context\n");
		return -1;
	}

	AVDictionary *opts = NULL;

	ret = avcodec_open2(pCodecCtx, pCodec, &opts);  //�򿪽�����
	if (ret != 0)
	{
		fprintf(stderr, "[ffmpeg-avcode] mjpeg not open context\n");
		avcodec_free_context(&pCodecCtx);
		return -1;
	}

	av_init_packet(&pkt);
	pkt.data = (uint8_t*) jdata;
	pkt.size = jsize;

	//avcodec_flush_buffers(pCodecCtx);
	ret = avcodec_send_packet(pCodecCtx, &pkt);
	if (ret >= 0)
		ret = avcodec_receive_frame(pCodecCtx, avframe);
	av_packet_unref(&pkt);

	avcodec_close(pCodecCtx);
	avcodec_free_context(&pCodecCtx);
	return ret;
}

//判断JPG if (memcmp(jpgptr, "\xFF\xD8\xFF", 3) == 0)
int ffmpeg_DecodeJpgDataToAVFrame(const void *jdata, int jsize, AVFrame *avframe)
{
	return ffmpeg_DecodeDataToAVFrame(AV_CODEC_ID_MJPEG, jdata, jsize, avframe);
}

//判断PNG if (memcmp(jpgptr, "\x89\x50\x4E\x47", 4) == 0)
int ffmpeg_DecodePngDataToAVFrame(const void *pngdata, int jsize, AVFrame *avframe)
{
	return ffmpeg_DecodeDataToAVFrame(AV_CODEC_ID_PNG, pngdata, jsize, avframe);
}

AVFrame* ffmpeg_DecodeImageDataToAVFrame(const void *imageptr, int jsize)
{
	int ret = 0;
	if (memcmp(imageptr, "\xFF\xD8\xFF", 3) == 0)
	{
		AVFrame *avframe = av_frame_alloc();
		ret = ffmpeg_DecodeDataToAVFrame(AV_CODEC_ID_MJPEG, imageptr, jsize, avframe);
		if (ret != 0)
		{
			av_frame_free(&avframe);
			avframe = NULL;
		}
		return avframe;
	}
	if (memcmp(imageptr, "\x89\x50\x4E\x47", 4) == 0)
	{
		AVFrame *avframe = av_frame_alloc();
		ret = ffmpeg_DecodeDataToAVFrame(AV_CODEC_ID_PNG, imageptr, jsize, avframe);
		if (ret != 0)
		{
			av_frame_free(&avframe);
			avframe = NULL;
		}
		return avframe;
	}
	return NULL;
}

//将avframe转移到buff中,类型为fmt
int ffmpeg_avframe2buffptr(AVFrame *avframe, void *desbuff, enum AVPixelFormat fmt)
{
	int ret = 0;
	AVFrame *desframe = ffmpeg_imgbuff_fill_avframe((uint8_t*) desbuff, fmt, avframe->width, avframe->height);
	ret = ffmpeg_SwsScale(avframe, desframe);
	av_frame_free(&desframe);
	return ret;
}

uint8_t *ffmpeg_avframe2buff(AVFrame *avframe, enum AVPixelFormat fmt, int *pbufflen)
{
	int desbufflen = av_image_get_buffer_size(fmt, avframe->width, avframe->height, 1);
	uint8_t *desbuff = (uint8_t*) av_malloc(desbufflen + 1);
	ffmpeg_avframe2buffptr(avframe, desbuff, fmt);
	if (pbufflen)
		*pbufflen = desbufflen;
	return desbuff;
}

uint8_t* ffmpeg_avframe_tonv12buff(AVFrame *avframe)
{
	return ffmpeg_avframe2buff(avframe, AV_PIX_FMT_NV12, NULL);
}

void rect_align(int *x, int *y, int *w, int *h, int align, int bkWidth, int bkHeight)
{
	*x = *x / align * align;
	*y = *y / align * align;
	*w = *w / align * align;
	*h = *h / align * align;

	if (*x < 0)
		*x = 0;
	if (*y < 0)
		*y = 0;

	if (*w < 0)
		*w = 2;
	if (*h < 0)
		*h = 2;

	if (*x + *w > bkWidth)
	{
		*w = bkWidth - *x;
		*w = *w / 16 * 16;
	}

	if (*y + *h > bkHeight)
	{
		*h = bkHeight - *y;
		*h = *h / 16 * 16;
	}
}

/**
 * @avframe : AVFrame *avframe = av_frame_alloc();   av_frame_free(&avframe);
 */
int ffmpeg_imagefile_read(const char *filepath, AVFrame *avframe)
{
	int ret;
	AVCodecContext *pCodecCtx = NULL;
	AVFrame *pFrame = NULL;
	AVCodec *pCodec = NULL;
	AVFormatContext *pFormatCtx = NULL;

	int video_index = 0;

	ret = avformat_open_input(&pFormatCtx, filepath, NULL, NULL);
	if (ret < 0)
	{
		printf("avformat open file err\n");
		return -1;
	}
	ret = avformat_find_stream_info(pFormatCtx, NULL);
	if (ret < 0)
	{
		printf("avformat_find_stream_info err\n");
		return -1;
	}

	ret = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	video_index = ret;

	pCodec = avcodec_find_decoder(pFormatCtx->streams[video_index]->codecpar->codec_id);

	pCodecCtx = avcodec_alloc_context3(pCodec); //AVCodecContext *pCodecCtx;
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[video_index]->codecpar);
	ret = avcodec_open2(pCodecCtx, pCodec, NULL);

	AVPacket packet;
	av_init_packet(&packet);

	//AVFrame *avframe = av_frame_alloc();
	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		avcodec_send_packet(pCodecCtx, &packet);
		ret = avcodec_receive_frame(pCodecCtx, avframe); //内部会av_frame_unref

		av_packet_unref(&packet);
	}
	//printf("wh: %dx%d\n", avframe->width, avframe->height);
//	av_frame_unref(avframe);
//	av_frame_free(&avframe);
	avformat_close_input(&pFormatCtx);
	avcodec_close(pCodecCtx);  //close,如果为rk3399的硬件编解码,则需要等待MPP_Buff释放完成后再关闭?是否需要这样不知道
	avcodec_free_context(&pCodecCtx);
	return 0;
}

void rgb2yuv(int R, int G, int B, uint8_t *Y, uint8_t *U, uint8_t *V)
{
	*Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
	*U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
	*V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;

	//	*Y = 0.299 * R + 0.587 * G + 0.114 * B;
	//	*U = -0.1687 * R - 0.3313 * G + 0.5 * B + 128;
	//	*V = 0.5 * R - 0.4187 * G - 0.0813 * B + 128;
}

void yuv2rgb(uint8_t Y, uint8_t U, uint8_t V, int *R, int *G, int *B)
{
	//	B = 1.164383 * (Y - 16) + 2.017232 * (U - 128);
	//	G = 1.164383 * (Y - 16) - 0.391762 * (U - 128) - 0.812968 * (V - 128);
	//	R = 1.164383 * (Y - 16) + 1.596027 * (V - 128);
	int r, g, b;
	r = (int) ((Y & 0xff) + 1.4075 * ((V & 0xff) - 128));
	g = (int) ((Y & 0xff) - 0.3455 * ((U & 0xff) - 128) - 0.7169 * ((V & 0xff) - 128));
	b = (int) ((Y & 0xff) + 1.779 * ((U & 0xff) - 128));
	r = (r < 0 ? 0 : r > 255 ? 255 : r);
	g = (g < 0 ? 0 : g > 255 ? 255 : g);
	b = (b < 0 ? 0 : b > 255 ? 255 : b);
	*R = r;
	*G = g;
	*B = b;
}

void ffmpeg_yuvbuff_draw_box(int width, int height, int format, uint8_t *image,
			int x, int y, int w, int h,
			int R, int G, int B)
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

	//x = x / 2 * 2;
	//y = y / 2 * 2;
	//w = w / 2 * 2;
	//h = h / 2 * 2;

	int maxx = x + w;
	int maxy = y + h;
	int px; //横线
	int py; //竖线

	uint8_t _YColor = R;
	uint8_t _UColor = B;
	uint8_t _VColor = G;

	rgb2yuv(R, G, B, &_YColor, &_UColor, &_VColor);

	if (maxx == width)
		maxx--;
	if (maxy == height)
		maxy--;

	//通过Y的起点计算UV点: (x=5, y=6)  (Y= W*y+x, U_420P=V420P= (y/2)*(width/2)+x/2
	//在Y上绘线
	for (px = x; px < maxx; px++) //X线横
	{
		dstY[width * y + px] = _YColor; //上横线
		dstY[width * maxy + px] = _YColor; //下横线
	}
	for (py = y; py <= maxy; py++) //Y画竖线
	{
		dstY[width * py + x] = _YColor; //左竖线
		dstY[width * py + maxx] = _YColor; //右竖线
	}

	if (AV_PIX_FMT_NV12 == format)
	{
		//UV
		for (px = x; px < maxx; px += 2) //画横线
		{

			dstUV[width * (y / 2) + px] = _UColor; //上横线UV
			dstUV[width * (y / 2) + px + 1] = _VColor;

			dstUV[width * (maxy / 2) + px] = _UColor; //下横线UV
			dstUV[width * (maxy / 2) + px + 1] = _VColor;
		}

		for (py = y; py <= maxy; py += 2) //画竖线
		{
			//左竖线
			dstUV[width * (py / 2) + x] = _UColor;
			dstUV[width * (py / 2) + x + 1] = _VColor;

			//右竖线
			dstUV[width * (py / 2) + maxx] = _UColor;
			dstUV[width * (py / 2) + maxx + 1] = _VColor;
		}
	}
	if (AV_PIX_FMT_YUV420P == format)
	{
		//UV
		for (px = x; px < maxx; px += 2) //画横线
		{
			dstU[width / 2 * (y / 2) + px / 2] = _UColor; //上横线UV
			dstV[width / 2 * (y / 2) + px / 2] = _VColor;

			dstU[width / 2 * (maxy / 2) + px / 2] = _UColor; //下横线UV
			dstV[width / 2 * (maxy / 2) + px / 2] = _VColor;
		}

		for (py = y; py <= maxy; py += 2) //画竖线
		{

			dstU[width / 2 * (py / 2) + x / 2] = _UColor; //左竖线
			dstV[width / 2 * (py / 2) + x / 2] = _VColor;

			dstU[width / 2 * (py / 2) + maxx / 2] = _UColor; //右竖线
			dstV[width / 2 * (py / 2) + maxx / 2] = _VColor;
		}
	}
#if 0
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
			dstU[width / 2 * (y / 2) + px / 2] = _UColor;
			dstV[width / 2 * (y / 2) + px / 2] = _VColor;

			//下横线UV
			dstU[width / 2 * (maxy / 2) + px / 2] = _UColor;
			dstV[width / 2 * (maxy / 2) + px / 2] = _VColor;
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
			dstU[width / 2 * (py / 2) + x / 2] = _UColor;
			dstV[width / 2 * (py / 2) + x / 2] = _VColor;

			//右竖线
			dstU[width / 2 * (py / 2) + maxx / 2] = _UColor;
			dstV[width / 2 * (py / 2) + maxx / 2] = _VColor;
		}
	}
#endif
}

//////////////////////////////////////////////////////////
#if 0
int ffmpegInOpen(struct ffmpegIn *in, const char *url)
{
	int ret;
	AVCodec *dec;
	AVDictionary *opts = NULL;
	AVStream *vstream = NULL;
	if (strncasecmp("rtsp://", url, strlen("rtsp://")) == 0)
	{
		av_dict_set(&opts, "rtsp_transport", "tcp", 0); //设置tcp or udp，默认一般优先tcp再尝试udp
		av_dict_set(&opts, "buffer_size", "1044000", 0);

		av_dict_set(&opts, "max_delay", "500000", 0);
		//av_dict_set(&opts, "stimeout", "3000000", 0); //3S
		av_dict_set(&opts, "stimeout", "20000000", 0); //设置超时
	}

	if ((ret = avformat_open_input(&in->fmt_ctx, url, NULL, opts ? &opts : NULL)) < 0)
	{
		printf("Cannot open input file:%s\n", url);
		goto _errret;
	}
	if (opts)
	{
		av_dict_free(&opts);
		opts = NULL;
	}

	if ((ret = avformat_find_stream_info(in->fmt_ctx, NULL)) < 0)
	{
		printf("Cannot find stream information\n");
		goto _errret;
	}

	/* select the video stream */
	ret = av_find_best_stream(in->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
	if (ret < 0)
	{
		printf("Cannot find a video stream in the input file\n");
		goto _errret;
	}
	in->video_stream_index = ret;

	vstream = in->fmt_ctx->streams[in->video_stream_index];
	if (!vstream || !vstream->codecpar)
	{
		printf("pCodecPar not found ! \n");
		ret = -1;
		goto _errret;
	}
	in->vstream = vstream;
	in->video_codec_id = vstream->codecpar->codec_id;

	strncpy(in->video_codec_name, avcodec_get_name(vstream->codecpar->codec_id),
				32 - 1);
	if (strcasecmp("hevc", in->video_codec_name) == 0)
		strcpy(in->video_codec_name, "H265");
	if (strcasecmp("h264", in->video_codec_name) == 0)
		strcpy(in->video_codec_name, "H264");

	av_dump_format(in->fmt_ctx, in->video_stream_index, url, 0);

	in->width = vstream->codecpar->width;
	in->height = vstream->codecpar->height;

	//	fps  表示平均帧率，总帧数除以总时长（以s为单位）。
	//	tbr  表示帧率，该参数倾向于一个基准，往往tbr跟fps相同。
	//	tbn  表示视频流 timebase（时间基准），比如ts流的timebase 为90000，flv格式视频流timebase为1000
	//	tbc  表示视频流codec timebase ，对于264码流该参数通过解析sps间接获取（通过sps获取帧率）。
	in->fps = (int) av_q2d(vstream->avg_frame_rate); //fps
	if (in->fps == 0)
		in->fps = (int) av_q2d(vstream->r_frame_rate); //tbr
	if (in->fps < 0)
		in->fps = 25;

	printf("%dx%d,%dfps (%.2f)\n", in->width, in->height, in->fps,
				av_q2d(vstream->avg_frame_rate));
	return 0;

	_errret:

	if (opts)
		av_dict_free(&opts);

	if (in)
	{
		if (in->fmt_ctx)
			avformat_close_input(&in->fmt_ctx);
		in->fmt_ctx = NULL;
	}

	if (ret == AVERROR_HTTP_UNAUTHORIZED)
		ret = 401;
	return ret;
}

//	AVPacket avpkt;
//	av_init_packet(&avpkt);
int ffmpegInRead(struct ffmpegIn *in, AVPacket *avpkt)
{
	return av_read_frame(in->fmt_ctx, avpkt);
}

void ffmpegInClose(struct ffmpegIn *in)
{
	if (in == NULL)
		return;
	if (in->fmt_ctx)
		avformat_close_input(&in->fmt_ctx);
}

typedef struct
{
	MppCtx ctx;
	MppApi *mpi;
	MppBufferGroup frame_group;

	char first_packet;
	char eos_reached;

	AVBufferRef *frames_ref;
	AVBufferRef *device_ref;
} RKMPPDecoder;

typedef struct
{
	AVClass *av_class;
	AVBufferRef *decoder_ref;
} RKMPPDecodeContext;

int ffmpegDecOpen(struct ffmpegDec *dec, struct ffmpegIn *in)
{
	//查找解码器
	int ret;
	AVStream *vstream = NULL;
	vstream = in->fmt_ctx->streams[in->video_stream_index];

	dec->isrkmpp = 0;

#if 1
	if (vstream->codecpar->codec_id == AV_CODEC_ID_H264)
	{
		printf("open h264_rkmpp \n");
		dec->pCodec = avcodec_find_decoder_by_name("h264_rkmpp");
		dec->isrkmpp = 1;
	}
	else if (vstream->codecpar->codec_id == AV_CODEC_ID_HEVC)
	{
		printf("open hevc_rkmpp(h265) \n");
		dec->pCodec = avcodec_find_decoder_by_name("hevc_rkmpp");
		dec->isrkmpp = 1;
	}
#else
	if (vstream->codecpar->codec_id == AV_CODEC_ID_H264)
	{
		//V....D h264_qsv             H.264 / AVC / MPEG - 4 AVC / MPEG - 4 part 10 (Intel Quick Sync Video acceleration) (codec h264)
		//V.....h264_cuvid           Nvidia CUVID H264 decoder(codec h264)
		//h264_qsv h264_cuvid
		//dec->pCodec = avcodec_find_decoder_by_name("h264_qsv");
		//dec->pCodec = avcodec_find_decoder_by_name("h264_cuvid");
	}
	else if (vstream->codecpar->codec_id == AV_CODEC_ID_HEVC)
	{
		//VFS..D hevc                 HEVC(High Efficiency Video Coding)
		//V....D hevc_qsv             HEVC(Intel Quick Sync Video acceleration) (codec hevc)
		//V.....hevc_cuvid           Nvidia CUVID HEVC decoder(codec hevc)
		//dec->pCodec = avcodec_find_decoder_by_name("hevc_qsv");
		//dec->pCodec = avcodec_find_decoder_by_name("hevc_cuvid");
	}
#endif
	if (dec->pCodec == NULL)
	{
		dec->pCodec = avcodec_find_decoder(vstream->codecpar->codec_id);
		dec->isrkmpp = 0;
	}

	if (dec->pCodec == NULL)
	{
		fprintf(stderr, "decode id err:%d\n", vstream->codecpar->codec_id);
		return -1;
	}

	dec->pCodecCtx = avcodec_alloc_context3(dec->pCodec); //AVCodecContext *pCodecCtx;
	avcodec_parameters_to_context(dec->pCodecCtx, vstream->codecpar);

	if (avcodec_open2(dec->pCodecCtx, dec->pCodec, NULL) < 0)
	{
		avcodec_free_context(&dec->pCodecCtx);
		return -1;
	}

	dec->def_unused = 0;
	if (dec->isrkmpp)
	{
		RKMPPDecodeContext *rk_context = (RKMPPDecodeContext*) dec->pCodecCtx->priv_data;
		if (rk_context)
		{
			RKMPPDecoder *decoder = (RKMPPDecoder*) rk_context->decoder_ref->data;
			ret = mpp_buffer_group_limit_config(decoder->frame_group, 0, 30);
			dec->def_unused = mpp_buffer_group_unused(decoder->frame_group);

			//DISABLE err : mpp 接收错误帧处理,增加这个可以不需要修改ffmpeg
			{
				void *param = NULL;
				unsigned int disable_error = 1;
				param = &disable_error;
				ret = decoder->mpi->control(decoder->ctx, MPP_DEC_SET_DISABLE_ERROR, param);
				if (ret)
					fprintf(stderr, "[RKMPP] Failed to set disable err %d\n", ret);
			}
			printf("[rkmpp] set group %d frame, disable error\n", dec->def_unused);
		}
	}
	return 0;
}

void ffmpegDecClose(struct ffmpegDec *dec)
{
	//wait channel ref is 0
	if (dec->pCodecCtx)
		avcodec_flush_buffers(dec->pCodecCtx);

	//close,如果为rk3399的硬件编解码,则需要等待MPP_Buff释放完成后再关闭?是否需要这样不知道
	if (dec->isrkmpp && dec->pCodecCtx)
	{
		RKMPPDecodeContext *rk_context = (RKMPPDecodeContext*) dec->pCodecCtx->priv_data;
		RKMPPDecoder *decoder = (RKMPPDecoder*) rk_context->decoder_ref->data;
		size_t unused = 0;
		while (dec->def_unused
					!= (unused = mpp_buffer_group_unused(decoder->frame_group)))
		{
			fprintf(stderr, "mpp_buffer_group_unused:%lu, def:%d\n", unused, dec->def_unused);
			sleep(1);
		}
	}

	if (dec->pCodecCtx)
	{
		avcodec_close(dec->pCodecCtx);
		avcodec_free_context(&dec->pCodecCtx);
	}
}

int ffmpegDecPacket(struct ffmpegDec *fdec, AVPacket *avpkt)
{
	int ret;
	//解码成yuv: 3840x2160 在这里会死循环
	ret = avcodec_send_packet(fdec->pCodecCtx, avpkt);
	if (ret != 0)
	{
		//AVERROR_INVALIDDATA
		char errstr[64] =
					{ 0 };
		fprintf(stderr, "avcodec_send_packet error : 0x%x %s\n", ret, av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, ret));
		av_packet_unref(avpkt);
		avcodec_flush_buffers(fdec->pCodecCtx);
		return ret;
	}
	return ret;
}

int ffmpegDecRead(struct ffmpegDec *fdec, AVFrame *avframe)
{
	int ret;	//解码, 应该是while循环处理的方式
	ret = avcodec_receive_frame(fdec->pCodecCtx, avframe); //内部会av_frame_unref
	if (ret != 0)
	{
		if (ret != AVERROR(EAGAIN))
			fprintf(stderr, "avcodec_receive_frame error : 0x%x %d\n", ret, ret);
		if (ret == AVERROR_UNKNOWN)
		{
			fprintf(stderr, "接收到错误帧\n");
		}
		if (ret == MPP_ERR_TIMEOUT)
		{
			fprintf(stderr, "MPP_ERR_TIMEOUT");
		}
	}
	//ff->pts * av_q2d(rational)
	return ret;
}

int ffmpegEncOpen(struct ffmpegEnc *fenc, enum AVCodecID in_video_code_id, AVCodecParameters *codecpar, const char *out_url)
{
	int ret;
	//mp4 file
	AVFormatContext *ofmt_ctx = NULL;
	AVOutputFormat *pMp4OFormat = NULL;

	if (avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_url) < 0)
	{
		printf("avformat_alloc_output_context2 err\n");
		goto err;
	}

	pMp4OFormat = ofmt_ctx->oformat;
	if (avio_open(&(ofmt_ctx->pb), out_url, AVIO_FLAG_READ_WRITE) < 0)
	{
		printf("avio_open err\n");
		goto err;
	}
	fenc->format_ctx = ofmt_ctx;
	///for (int i = 0; i < pFormat->nb_streams; i++)
	{
		//AVStream *in_stream = pFormat->streams[i];
		//decode 解码器？
		AVCodec *avcodec = avcodec_find_decoder(in_video_code_id);
		printf("new stream:%s\n", avcodec->name);
		AVStream *out_stream = avformat_new_stream(ofmt_ctx, avcodec);
		if (!out_stream)
		{
			printf("Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto err;
		}

		ret = avcodec_parameters_copy(out_stream->codecpar, codecpar);
		if (ret < 0)
		{
			printf("拷贝视频code失败\n");
			goto err;
		}

#if 0
		AVCodecContext *codec_ctx = avcodec_alloc_context3(avcodec);
		ret = avcodec_parameters_to_context(codec_ctx, in_stream->codecpar);
		if (ret < 0)
		{
			printf(
						"Failed to copy context from input to output stream codec context\n");
			return -1;
		}

		codec_ctx->codec_tag = 0;
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		ret = avcodec_parameters_from_context(out_stream->codecpar, codec_ctx);
#endif
	}

	//Dump Format------------------
	av_dump_format(ofmt_ctx, 0, out_url, 1);

	//Open output URL
	if (!(ofmt_ctx->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&ofmt_ctx->pb, out_url, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			printf("Could not open output URL '%s'", out_url);
			goto err;
		}
		printf("avio open\n");
	}

	if (avformat_write_header(ofmt_ctx, NULL) < 0) //写文件头
	{
		printf("avformat_write_header err\n");
		goto err;
	}
	fenc->format_ctx = ofmt_ctx;
	return 0;

	err:
	if (ofmt_ctx)
	{
		if (!(ofmt_ctx->flags & AVFMT_NOFILE))
		{
			if (ofmt_ctx->pb)
				avio_close(ofmt_ctx->pb);
		}
		avformat_free_context(ofmt_ctx);
	}
	return -1;
}

int ffmpegEncOpen2(struct ffmpegEnc *fenc, AVStream *in_stream, const char *out_url)
{
	fenc->src_time_base = in_stream->time_base;
	return ffmpegEncOpen(fenc, in_stream->codecpar->codec_id, in_stream->codecpar, out_url);
}

void _AVFormatContextClose(AVFormatContext *format_ctx)
{
	if (format_ctx && !(format_ctx->oformat->flags & AVFMT_NOFILE))
	{
		if (format_ctx->pb)
		{
			av_write_trailer(format_ctx); // 写文件尾
			avio_close(format_ctx->pb);
			printf("avio_close\n");
		}
	}
	avformat_free_context(format_ctx);
}

void ffmpegEncClose(struct ffmpegEnc *fenc)
{
	/* close output */
	_AVFormatContextClose(fenc->format_ctx);
	fenc->format_ctx = NULL;
}
#endif
