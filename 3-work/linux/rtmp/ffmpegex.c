#include "ffmpegex.h"

//YUV420缓冲截图
void YUV420P_ImagePtr_crop(
	uint8_t *ptrY,
	uint8_t *ptrU,
	uint8_t *ptrV,
	//			int src_w,
	//			int src_h,
	int src_yhor, //Y的水平
	int src_uhor, //U水平宽度(w/2)
	int src_vhor, //V水平宽度(w/2)
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

	//X列 Y 行
	for (line_y = dst_y; line_y < dst_max_y; line_y++)
	{
		//一行一行复制
		memcpy(dst_Y, src_Y + (line_y * src_yhor) + dst_x, dst_w);
		dst_Y += dst_w;

		if (line_y % 2 == 0) //UV分量
		{
			//line_y : 0 2 4 6 8 10 12 14
			//dst_y/2: 0 1 2 3 4  5  6
			if (dst_format == AV_PIX_FMT_YUV420P || dst_format == AV_PIX_FMT_YUVJ420P) //为高宽的一半
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
					//(line_y / 2)* hor_stride //那一行
					*dst_UV++ = src_U[(line_y / 2) * src_uhor + dst_x / 2 + column_x];
					*dst_UV++ = src_V[(line_y / 2) * src_uhor + dst_x / 2 + column_x];
				}
			}
		}
	}
}

void NV12_ImagePtr_crop(uint8_t *src_Y,
						uint8_t *src_UV,
						int src_w,
						int src_h,
						int src_yhor,  //Y水平宽度
						int src_uvhor, //UV水平宽度
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
		memcpy(dst_Y, src_Y + (line_y * src_yhor) + dst_x, dst_w); //Y分量
		dst_Y += dst_w;

		if (line_y % 2 == 0) //UV分量
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

//@avframe: AVFrame
//@dst_format: AV_PIX_FMT_YUV420P/AV_PIX_FMT_NV12
uint8_t *ffmpeg_crop(AVFrame *avframe, int dst_format, int x, int y, int w, int h)
{
	int imglen = av_image_get_buffer_size((enum AVPixelFormat)dst_format, w, h, 1);
	uint8_t *img = NULL;

#ifdef CONFIG_HAVE_RKMPP
	if (avframe->format == AV_PIX_FMT_DRM_PRIME)
	{
		MppFrame mppframe = AVFrame2MppFrame(avframe);
		MppBuffer buff = mpp_frame_get_buffer(mppframe);

		img = av_malloc(imglen);

		yuv_drm_image_crop(mpp_buffer_get_ptr(buff),
						   avframe->width,
						   avframe->height,
						   mpp_frame_get_hor_stride(mppframe),
						   mpp_frame_get_ver_stride(mppframe),
						   img,
						   dst_format, x, y, w, h); //截图
		return img;
	}
#endif

	if (dst_format == AV_PIX_FMT_NV12 ||
		dst_format == AV_PIX_FMT_YUV420P ||
		dst_format == AV_PIX_FMT_YUVJ420P)
	{
		if (avframe->format == AV_PIX_FMT_YUV420P || avframe->format == AV_PIX_FMT_YUVJ420P)
		{
			img = (uint8_t *)av_malloc(imglen);
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
			img = (uint8_t *)av_malloc(imglen);
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

AVFrame *ffmpeg_imgbuff_fill_avframe(uint8_t *data, int format, int w, int h)
{
	AVFrame *frame = av_frame_alloc();
	frame->format = format;
	frame->width = w;
	frame->height = h;
	av_image_fill_arrays(frame->data, frame->linesize, data,
						 (enum AVPixelFormat)frame->format, frame->width, frame->height, 1);
	return frame;
}

AVFrame *ffmpeg_av_frame_alloc(int format, int w, int h);

int ffmpeg_SwsScale(AVFrame *srcFrame, AVFrame *dscFrame)
{
	struct SwsContext *m_pSwsContext;
	int ret = 0;
	m_pSwsContext = sws_getContext(srcFrame->width, srcFrame->height,
								   (enum AVPixelFormat)srcFrame->format, dscFrame->width,
								   dscFrame->height, (enum AVPixelFormat)dscFrame->format,
								   SWS_BICUBIC, NULL, NULL,
								   NULL);
	ret = sws_scale(m_pSwsContext, (const uint8_t **)srcFrame->data, srcFrame->linesize, 0,
					srcFrame->height, dscFrame->data, dscFrame->linesize);
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

int ffmpeg_SwsScale2(AVFrame *srcFrame, AVFrame *dscFrame)
{
	struct SwsContext *m_pSwsContext;
	int ret = 0;
	int srcStride[AV_NUM_DATA_POINTERS];
	uint8_t *srcSlice[AV_NUM_DATA_POINTERS];

	memset(srcStride, 0, sizeof(srcStride));

#if 0
	MppFrame mppframe = AVFrame2MppFrame(srcFrame);
	MppBuffer mppbuff = mpp_frame_get_buffer(mppframe);
	uint8_t* drmptr = mpp_buffer_get_ptr(mppbuff);
	int hor = mpp_frame_get_hor_stride(mppframe);
	int ver = mpp_frame_get_ver_stride(mppframe);
#endif

	enum AVPixelFormat srcFormat = srcFrame->format;

	if (srcFrame->format == AV_PIX_FMT_DRM_PRIME)
	{
#if 0
		MppFrame mppframe = AVFrame2MppFrame(srcFrame);
		MppBuffer mppbuff = mpp_frame_get_buffer(mppframe);
		uint8_t* drmptr = mpp_buffer_get_ptr(mppbuff);
		int hor = mpp_frame_get_hor_stride(mppframe);
		int ver = mpp_frame_get_ver_stride(mppframe);
		//mpp数据
		srcStride[0] = mpp_frame_get_hor_stride(mppframe);
		srcStride[1] = mpp_frame_get_hor_stride(mppframe);
		srcSlice[0] = drmptr;
		srcSlice[1] = drmptr + hor * ver;
		srcFormat = AV_PIX_FMT_NV12;
#endif
	}
	else
	{
		srcSlice[0] = srcFrame->data[0];
		srcSlice[1] = srcFrame->data[1];
		srcSlice[2] = srcFrame->data[2];
	}

	m_pSwsContext = sws_getContext(
		srcFrame->width,
		srcFrame->height,
		srcFormat,
		dscFrame->width,
		dscFrame->height,
		dscFrame->format,
		SWS_FAST_BILINEAR, //SWS_BICUBIC
		NULL, NULL, NULL);

	ret = sws_scale(m_pSwsContext,
					(const uint8_t **)srcSlice,
					srcStride, 0,
					srcFrame->height,
					dscFrame->data,
					dscFrame->linesize);
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

	//
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

__quit:
	av_packet_unref(&pkt);
	avcodec_close(pCodecCtx);
	avcodec_free_context(&pCodecCtx);
	return ret;
}

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
		//log_e("ffmpeg", "AVFrame2Jpg format err:%d\r\n", frame->format);
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
