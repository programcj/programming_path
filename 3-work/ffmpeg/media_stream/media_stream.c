/*
 * rtsp_video_stream.c
 *
 *  Created on: 2020年5月23日
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#define __USE_GNU
#include <sched.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "libavdevice/avdevice.h"

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libavutil/avutil.h"
#include "libavutil/pixdesc.h"
#include "libavutil/time.h"
#include "libavutil/imgutils.h"

#include "libswscale/swscale.h"

#include "media_stream.h"

static char get_media_type_char(enum AVMediaType type)
{
	switch (type)
	{
		case AVMEDIA_TYPE_VIDEO:
			return 'V';
		case AVMEDIA_TYPE_AUDIO:
			return 'A';
		case AVMEDIA_TYPE_DATA:
			return 'D';
		case AVMEDIA_TYPE_SUBTITLE:
			return 'S';
		case AVMEDIA_TYPE_ATTACHMENT:
			return 'T';
		default:
			return '?';
	}
}

static const AVCodec *next_codec_for_id(enum AVCodecID id, const AVCodec *prev,
		int encoder)
{
	while ((prev = av_codec_next(prev)))
	{
		if (prev->id == id
				&& (encoder ?
						av_codec_is_encoder(prev) : av_codec_is_decoder(prev)))
			return prev;
	}
	return NULL;
}

static int compare_codec_desc(const void *a, const void *b)
{
	const AVCodecDescriptor * const *da = a;
	const AVCodecDescriptor * const *db = b;

	return (*da)->type != (*db)->type ?
			FFDIFFSIGN((*da)->type, (*db)->type) :
			strcmp((*da)->name, (*db)->name);
}

static unsigned get_codecs_sorted(const AVCodecDescriptor ***rcodecs)
{
	const AVCodecDescriptor *desc = NULL;
	const AVCodecDescriptor **codecs;
	unsigned nb_codecs = 0, i = 0;

	while ((desc = avcodec_descriptor_next(desc)))
		nb_codecs++;
	if (!(codecs = av_calloc(nb_codecs, sizeof(*codecs))))
	{
		av_log(NULL, AV_LOG_ERROR, "Out of memory\n");
		exit(1);
	}
	desc = NULL;
	while ((desc = avcodec_descriptor_next(desc)))
		codecs[i++] = desc;
	qsort(codecs, nb_codecs, sizeof(*codecs), compare_codec_desc);
	*rcodecs = codecs;
	return nb_codecs;
}

static void print_codecs(int encoder)
{
	const AVCodecDescriptor **codecs;
	unsigned i, nb_codecs = get_codecs_sorted(&codecs);

	printf("%s:\n"
			" V..... = Video\n"
			" A..... = Audio\n"
			" S..... = Subtitle\n"
			" .F.... = Frame-level multithreading\n"
			" ..S... = Slice-level multithreading\n"
			" ...X.. = Codec is experimental\n"
			" ....B. = Supports draw_horiz_band\n"
			" .....D = Supports direct rendering method 1\n"
			" ------\n", encoder ? "Encoders" : "Decoders");
	for (i = 0; i < nb_codecs; i++)
	{
		const AVCodecDescriptor *desc = codecs[i];
		const AVCodec *codec = NULL;

		while ((codec = next_codec_for_id(desc->id, codec, encoder)))
		{
			printf(" %c", get_media_type_char(desc->type));
			printf(
					(codec->capabilities & AV_CODEC_CAP_FRAME_THREADS) ?
							"F" : ".");
			printf(
					(codec->capabilities & AV_CODEC_CAP_SLICE_THREADS) ?
							"S" : ".");
			printf(
					(codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL) ?
							"X" : ".");
			printf(
					(codec->capabilities & AV_CODEC_CAP_DRAW_HORIZ_BAND) ?
							"B" : ".");
			printf((codec->capabilities & AV_CODEC_CAP_DR1) ? "D" : ".");

			printf(" %-20s %s", codec->name,
					codec->long_name ? codec->long_name : "");
			if (strcmp(codec->name, desc->name))
				printf(" (codec %s)", desc->name);

			printf("\n");
		}
	}
	av_free(codecs);
}

void media_help()
{
	printf("================ Decoders ======================\n");
	print_codecs(0);
	printf("================ Encoders ======================\n");
	print_codecs(1);
}

int fps_calc_step(struct fps_calc *fps)
{
	uint64_t delta;
	fps->count++;
	if (fps->count == 1)
		fps->time_start = av_gettime();

	delta = (av_gettime() - fps->time_start) / 1000;
	if (delta >= 1000) //计算出处理时的fps, ms
	{
		//fps->fps = fps->count / (delta * 0.001f); //实时网络传输帧率
		fps->fps = fps->count * 1000.0 / delta; //(fps->count*1.0) / delta * 1000;
		fps->count = 0;
		return 1;
	}
	return 0;
}

void AVFrame_yuv420_to_buff(AVFrame *avframe, unsigned char *buff)
{
	uint8_t *srcY = avframe->data[0];
	uint8_t *dstY = buff;
	uint8_t *srcU = avframe->data[1];
	uint8_t *dstU = NULL;
	uint8_t *srcV = avframe->data[2];
	uint8_t *dstV = NULL;

	int y_h = 0, u_h;

	for (y_h = 0; y_h < avframe->height; y_h++) //Y
	{
		memcpy(dstY, srcY, avframe->width);
		srcY += avframe->linesize[0];
		dstY += avframe->width;
	}

	dstU = dstY;
	dstV = dstU + ((avframe->height * avframe->width) >> 2);

	for (u_h = 0; u_h < avframe->height / 2; u_h++) //UV
	{
		memcpy(dstU, srcU, avframe->width / 2);
		memcpy(dstV, srcV, avframe->width / 2);
		dstU += avframe->width / 2;
		dstV += avframe->width / 2;
		srcU += avframe->linesize[1];
		srcV += avframe->linesize[2];
	}
}

void AVFrame_yuv420_to_nv12_buff(AVFrame *avframe, unsigned char *buff)
{
	unsigned char *srcY = avframe->data[0];
	unsigned char *srcU = avframe->data[1];
	unsigned char *srcV = avframe->data[2];
	unsigned char *dstNV = buff + avframe->width * avframe->height;
	int y_h = 0;
	int u_h = 0;
	unsigned char *dstY = buff;

	for (y_h = 0; y_h < avframe->height; y_h++) //Y
	{
		memcpy(dstY, srcY, avframe->width);
		srcY += avframe->linesize[0];
		dstY += avframe->width;
	}

	int u_w = 0;
	for (u_h = 0; u_h < avframe->height / 2; u_h++) //UV
	{
		for (u_w = 0; u_w < avframe->width / 2; u_w++)
		{
			(*dstNV++) = srcU[u_w];
			(*dstNV++) = srcV[u_w];
		}
		srcU += avframe->linesize[1];
		srcV += avframe->linesize[2];
	}
}

//---NV12---
//Y Y Y Y
//Y Y Y Y
//Y Y Y Y
//Y Y Y Y
//U V U V
//U V U V
void AVFrame_drm_prime_to_nv12_buff(AVFrame *avframe, unsigned char *buff)
{
#if 1
	unsigned char *srcY = avframe->data[0];
	int y_h = 0;
	unsigned char *dstY = buff;

	for (y_h = 0; y_h < avframe->height; y_h++) //Y
	{
		memcpy(dstY, srcY, avframe->width);
		srcY += avframe->linesize[0];
		dstY += avframe->width;
	}

	int u_h = 0;
	int u_w = 0;
	unsigned char *srcUV = avframe->data[1];
	unsigned char *dstUV = dstY;

	for (u_h = 0; u_h < avframe->height / 2; u_h++) //UV: h264 OK
	{
		memcpy(dstUV, srcUV, avframe->width); //1920*1080
		dstUV += avframe->width;
		srcUV += avframe->linesize[1] * 2;
	}
#else
	unsigned char *srcY = avframe->data[0];
	int y_h = 0;
	unsigned char *dstY = buff;

	for (y_h = 0; y_h < avframe->height; y_h++) //Y
	{
		memcpy(dstY, srcY, avframe->width);
		srcY += avframe->linesize[0];
		dstY += avframe->width;
	}
#endif
}

void AVFrame_drm_prime_to_yuv420_buff(AVFrame *avframe, unsigned char *buff)
{
	unsigned char *srcY = avframe->data[0];
	int y_h = 0;
	unsigned char *dstY = buff;

	for (y_h = 0; y_h < avframe->height; y_h++) //Y
	{
		memcpy(dstY, srcY, avframe->width);
		srcY += avframe->linesize[0];
		dstY += avframe->width;
	}

	int u_h = 0;
	int u_w = 0;
	unsigned char *srcUV = avframe->data[1];
	unsigned char *dstU = dstY;
	unsigned char *dstV = dstU + (avframe->width * avframe->height) / 4;

	for (u_h = 0; u_h < avframe->height / 2; u_h++) //UV: h264 OK
	{
		for (u_w = 0; u_w < avframe->width; u_w += 2)
		{
			*dstU++ = srcUV[u_w];
			*dstV++ = srcUV[u_w + 1];
		}
		srcUV += avframe->linesize[1] * 2;
	}
}

void AVFrame_nv12_to_yuv420_buff(AVFrame *avframe, unsigned char *buff)
{
	unsigned char *pU = buff + avframe->width * avframe->height;
	unsigned char *pV = pU + ((avframe->width * avframe->height) >> 2);
	unsigned char *srcY = avframe->data[0];
	int y_h = 0;
	unsigned char *dstY = buff;

	for (y_h = 0; y_h < avframe->height; y_h++) //Y
	{
		memcpy(dstY, srcY, avframe->width);
		srcY += avframe->linesize[0];
		dstY += avframe->width;
	}

	//---NV12---
	//Y Y Y Y
	//Y Y Y Y
	//Y Y Y Y
	//Y Y Y Y
	//U V U V
	//U V U V
	for (int i = 0; i < (avframe->width * avframe->height) / 2; i++)
	{
		if ((i % 2) == 0)
			*pU++ = *(avframe->data[1] + i); //U
		else
			*pV++ = *(avframe->data[1] + i); //V
	}
}

typedef struct Frame
{
	AVFrame *frame;
} Frame;

#define FRAME_QUEUE_SIZE 20

typedef struct FrameQueue
{
	Frame queue[FRAME_QUEUE_SIZE];

	int rindex; // 头指针，若队列不空，指向队列头元素
	int windex; // 尾指针，若队列不空，指向队列尾元素的下一个位置

	int rindex_shown;

	int keep_last;

	int size; //当前大小
	int max_size; //最大大小
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int abort;
} FrameQueue;

static int frame_queue_init(FrameQueue *f, int max_size, int keep_last)
{
	int i;
	int ret;
	memset(f, 0, sizeof(FrameQueue));

	pthread_condattr_t sattr;

	ret = pthread_mutex_init(&f->mutex, NULL);
	ret = pthread_condattr_init(&sattr);
	ret = pthread_condattr_setclock(&sattr, CLOCK_MONOTONIC);
	ret = pthread_cond_init(&f->cond, &sattr);
	pthread_condattr_destroy(&sattr);

	f->keep_last = !!keep_last;

	f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
	for (i = 0; i < f->max_size; i++)
		if (!(f->queue[i].frame = av_frame_alloc()))
			return AVERROR(ENOMEM);
	return ret;
}

static void frame_queue_destory(FrameQueue *f)
{
	int i;
	for (i = 0; i < f->max_size; i++)
	{
		Frame *vp = &f->queue[i];
		av_frame_unref(vp->frame);
		av_frame_free(&vp->frame);
	}
	pthread_mutex_destroy(&f->mutex);
	pthread_cond_destroy(&f->cond);
}

//向队列尾部申请一个可写的帧空间，若队列已满无空间可写，则等待
static Frame *frame_queue_peek_writable(FrameQueue *f)
{
	/* wait until we have space to put a new frame */
	pthread_mutex_lock(&f->mutex);
	while (f->size >= f->max_size && !f->abort)
	{
		pthread_cond_wait(&f->cond, &f->mutex);
	}
	pthread_mutex_unlock(&f->mutex);
	if (f->abort)
		return NULL;

	return &f->queue[f->windex];
}

//获取待显示的第一帧
static Frame *frame_queue_peek_readable(FrameQueue *f)
{
	/* wait until we have a readable a new frame */
	pthread_mutex_lock(&f->mutex);
	while (f->size - f->rindex_shown <= 0 && !f->abort)
	{
		pthread_cond_wait(&f->cond, &f->mutex);
	}
	pthread_mutex_unlock(&f->mutex);
	if (f->abort)
		return NULL;

	return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static Frame *frame_queue_peek(FrameQueue *f)
{
	return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static Frame *frame_queue_peek_next(FrameQueue *f)
{
	return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

static Frame *frame_queue_peek_last(FrameQueue *f)
{
	return &f->queue[f->rindex];
}

/* return the number of undisplayed frames in the queue */
static int frame_queue_nb_remaining(FrameQueue *f)
{
	return f->size - f->rindex_shown;
}

//入队
static void frame_queue_push(FrameQueue *f)
{
	if (++f->windex == f->max_size)
		f->windex = 0;
	pthread_mutex_lock(&f->mutex);
	f->size++;
	pthread_cond_signal(&f->cond);
	pthread_mutex_unlock(&f->mutex);
}

//出队
static void frame_queue_next(FrameQueue *f)
{
	if (f->keep_last && !f->rindex_shown)
	{
		f->rindex_shown = 1;
		return;
	}
	if (++f->rindex == f->max_size)
		f->rindex = 0;

	//解除本AVFrame对AVFrame中所有缓存区的引用, 并复位AVFrame中的各成员.
	av_frame_unref(f->queue[f->rindex].frame);
	pthread_mutex_lock(&f->mutex);
	f->size--;
	pthread_cond_signal(&f->cond);
	pthread_mutex_unlock(&f->mutex);
}

int media_stream_test()
{
#if 0
	FrameQueue queue;
	Frame *frame;
	int i = 0;

	frame_queue_init(&queue, 20, 0);

	for (i = 0; i < 20; i++)
	{
		frame = frame_queue_peek_writable(&queue);

		printf("push frame:%d <%p>\n", i, frame);
		frame_queue_push(&queue);
	}

	printf("=======================\n");
	for (i = 0; i < 20; i++)
	{
		frame = frame_queue_peek_readable(&queue);
		printf("next frame:%d <%p>\n", i, frame);
		frame_queue_next(&queue);
	}
	exit(0);
#endif
	return 0;
}

//int media_stream_read_queue_loop(struct media_stream *mstream)
//{
//	FrameQueue *queue_video = (FrameQueue *) mstream->queue_video;
//
//	while ((queue_video = (FrameQueue *) mstream->queue_video) == NULL)
//	{
//		usleep(1000 * 100);
//	}
//	struct fps_calc _fps2;
//
//	uint8_t *_yuv_out_data = NULL;
//	int _yuvoutdata_len = 0;
//	int _yuvout_format = 0;
//
//	memset(&_fps2, 0, sizeof(_fps2));
//
//	const AVPixFmtDescriptor *des = NULL;
//	Frame *vp, *lastvp;
//
//	while (1)
//	{
//		if (frame_queue_nb_remaining(queue_video) > 0)
//		{
//			lastvp = frame_queue_peek_last(queue_video);
//			vp = frame_queue_peek(queue_video);
//
//			frame_queue_next
//		}
//
//		if (frame)
//		{
//			des = av_pix_fmt_desc_get(frame->frame->format);
//			frame_queue_next(queue_video);
//
//			avframe = frame->frame;
//			printf("frame.....>> %s, %dx%d \n", des->name, frame->frame->width,
//					frame->frame->height);
//
//			_yuvout_format = avframe->format;
//
//			switch (avframe->format)
//			{
//				case AV_PIX_FMT_DRM_PRIME:
//				{
//					_yuvout_format = AV_PIX_FMT_NV12;
//					//_yuvout_format = AV_PIX_FMT_YUV420P;
//				}
//				break;
//				case AV_PIX_FMT_YUYV422:
//					_yuvout_format = AV_PIX_FMT_YUV420P;
//				break;
//			}
//
//			//				int _imagelen = av_image_get_buffer_size(_yuvout_format,
//			//						avframe->width, avframe->height, 1);
//
//			int _imagelen = avframe->linesize[0] * avframe->height
//					+ (avframe->linesize[1] * 2) * (avframe->height / 2)
//					+ (avframe->linesize[2] * (avframe->height / 2));
//
//			if (_imagelen != _yuvoutdata_len || _yuv_out_data == NULL)
//			{
//				if (_yuv_out_data)
//				{
//					av_free(_yuv_out_data);
//					_yuv_out_data = NULL;
//				}
//				_yuvoutdata_len = _imagelen;
//				_yuv_out_data = (uint8_t *) av_malloc(_imagelen + 1024);
//
//				printf("malloc yuv frame:[%s] size:%d\n",
//						des ? des->name : "[unknow]", _yuvoutdata_len);
//			}
//
//			///////
//			if (_yuv_out_data)			//达到16帧fps
//			{
//				_out_not++;
//				if (_out_not % 2 == 0)
//				{
//					break;
//				}
//
//				if (avframe->format == AV_PIX_FMT_DRM_PRIME)
//				{
//					uint64_t _tmc[2];
//					_tmc[0] = av_gettime();
//
//					//						if (_yuvout_format == AV_PIX_FMT_NV12)
//					//							AVFrame_drm_prime_to_nv12_buff(avframe,
//					//									_yuv_out_data);
//					//						else if (_yuvout_format == AV_PIX_FMT_YUV420P)
//					//							AVFrame_drm_prime_to_yuv420_buff(avframe,
//					//									_yuv_out_data);
//
//					//40ms
//					memcpy(_yuv_out_data, avframe->data[0], _imagelen);
//					_tmc[1] = av_gettime();
//
//					printf("%p %p %d, ", avframe->data[0], avframe->data[1],
//							avframe->data[1] - avframe->data[0]);
//					printf("%d %d %d, >>", avframe->linesize[0],
//							avframe->linesize[1], avframe->linesize[2]);
//					printf("%lu\n", (_tmc[1] - _tmc[0]) / 1000);
//				}
//
//				if (avframe->format == AV_PIX_FMT_YUV420P
//						|| avframe->format == AV_PIX_FMT_YUVJ420P)
//				{
//					AVFrame_yuv420_to_buff(avframe, _yuv_out_data);
//					_yuvout_format = AV_PIX_FMT_YUV420P;
//				}
//
//				if (avframe->format == AV_PIX_FMT_YUYV422)
//				{
//					{
//						AVFrame *srcFrame = avframe;
//						AVFrame *dscFrame = av_frame_alloc();
//						dscFrame->format = AV_PIX_FMT_YUV420P; //AV_PIX_FMT_NV12;
//						dscFrame->width = srcFrame->width;
//						dscFrame->height = srcFrame->height;
//						av_image_fill_arrays(dscFrame->data, dscFrame->linesize,
//								_yuv_out_data, dscFrame->format,
//								dscFrame->width, dscFrame->height, 1);
//
//						struct SwsContext *m_pSwsContext;
//						int ret = 0;
//						m_pSwsContext = sws_getContext(srcFrame->width,
//								srcFrame->height,
//								(enum AVPixelFormat) srcFrame->format,
//								dscFrame->width, dscFrame->height,
//								dscFrame->format,
//								SWS_BICUBIC, NULL, NULL,
//								NULL);
//						ret = sws_scale(m_pSwsContext, srcFrame->data,
//								srcFrame->linesize, 0, srcFrame->height,
//								dscFrame->data, dscFrame->linesize);
//						sws_freeContext(m_pSwsContext);
//
//						AVFrame_yuv420_to_buff(dscFrame, _yuv_out_data);
//
//						av_frame_free(&dscFrame); //nv12 to yuv420p
//					}
//				}
//
//				fps_calc_step(&_fps2);
//
//				if (mstream->bk_media_stream_frame_yuv)
//					mstream->bk_media_stream_frame_yuv(mstream,
//							frame_video_count, avframe->width, avframe->height,
//							_yuvout_format, _yuv_out_data, _yuvoutdata_len);
//			}
//		}
//		else
//		{
//			usleep(1000);
//		}
//	}
//	return 0;
//}

void media_yuvdata_realloc(struct media_yuvdata *media_data, int format, int w,
		int h)
{
	int _imagelen = av_image_get_buffer_size(format, w, h, 1);

	if (_imagelen != media_data->size || media_data->yuvdata == NULL)
	{
		if (media_data->yuvdata)
		{
			av_free(media_data->yuvdata);
			media_data->yuvdata = NULL;
		}
		media_data->size = _imagelen;
		media_data->yuvdata = (uint8_t *) av_malloc(_imagelen + 1024);
		if (media_data->yuvdata)
			memset(media_data->yuvdata, 0, _imagelen);
	}

	media_data->width = w;
	media_data->height = h;
}

static void *_thread_queue_read(void *args)
{
	struct media_stream *mstream = (struct media_stream *) args;
	FrameQueue *queue_video = (FrameQueue *) mstream->queue_video;
	AVFrame *avframe = NULL;
	Frame *vp;
	const AVPixFmtDescriptor *des = NULL;

	struct media_yuvdata media_data;
	memset(&media_data, 0, sizeof(media_data));

	uint64_t _tmc[2];
	struct fps_calc _fps2;

	memset(&_fps2, 0, sizeof(_fps2));

	prctl(PR_SET_NAME, "Stream#R");
	pthread_detach(pthread_self());

	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(6, &mask);
	if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
	{
		perror("pthread_setaffinity_np");
	}

	while (mstream->loop)
	{
//		if(frame_queue_nb_remaining(queue_video)==0)
//		{
//		}
		vp = frame_queue_peek_readable(queue_video);

		if (vp)
		{
			avframe = vp->frame;

			des = av_pix_fmt_desc_get(avframe->format);

			printf("[%s] %dx%d, linesize:%d %d %d, [-]:%d qsize:%d\n",
					des->name, avframe->width, avframe->height,
					avframe->linesize[0], avframe->linesize[1],
					avframe->linesize[2],

					avframe->data[1] - avframe->data[0], queue_video->size);

//						int _imagelen = avframe->linesize[0] * avframe->height
//								+ (avframe->linesize[1] * 2) * (avframe->height / 2)
//								+ (avframe->linesize[2] * (avframe->height / 2));

			switch (avframe->format)
			{
				case AV_PIX_FMT_DRM_PRIME:
				{
					// AV_PIX_FMT_YUV420P;
					media_yuvdata_realloc(&media_data, AV_PIX_FMT_NV12,
							avframe->width, avframe->height);

					_tmc[0] = av_gettime();

//						AVFrame *pFrameOK=av_frame_alloc();
//						pFrameOK->width = avframe->width;
//						pFrameOK->height = avframe->height;
//						pFrameOK->format = AV_PIX_FMT_NV12;
					//av_hwframe_transfer_data(pFrameOK, avframe, 0);

					if (media_data.format == AV_PIX_FMT_NV12)
						AVFrame_drm_prime_to_nv12_buff(avframe,
								media_data.yuvdata);
					else if (media_data.format == AV_PIX_FMT_YUV420P)
						AVFrame_drm_prime_to_yuv420_buff(avframe,
								media_data.yuvdata);

					//40ms
					_tmc[1] = av_gettime();

					printf("format:%d, rtime:%d \n", media_data.format,
							(_tmc[1] - _tmc[0]) / 1000);
				}
				break;
				case AV_PIX_FMT_YUV420P:
				case AV_PIX_FMT_YUVJ420P:
					media_yuvdata_realloc(&media_data, AV_PIX_FMT_YUV420P,
							avframe->width, avframe->height);
					AVFrame_yuv420_to_buff(avframe, media_data.yuvdata);
				break;
			}

			media_data.id=avframe->pkt_pos;

			frame_queue_next(queue_video);

			fps_calc_step(&_fps2);

			if (mstream->bk_media_stream_frame_yuv)
				mstream->bk_media_stream_frame_yuv(mstream, media_data.id,
						media_data.width, media_data.height, media_data.format,
						media_data.yuvdata, media_data.size);
		}
	}

	if (media_data.yuvdata)
		av_free(media_data.yuvdata);
	return NULL;
}

int media_stream_loop(struct media_stream *mstream)
{
	AVFormatContext *ifmt_ctx = NULL;
	AVOutputFormat *ofmt = NULL;
	AVCodecContext *avdecoder_ctx = NULL;
	AVCodec *avdecoder = NULL;
	FrameQueue *queue_video = NULL;

	int in_codec_id = 0;

	AVPacket pkt;
	const char *in_filename;
	int ret, i;
	int avmedia_type_video = -1;

	in_filename = mstream->rtsp_url;

	{
		const AVOutputFormat *fmt = NULL;
		void *ifmt_opaque = 0;
		while ((fmt = av_muxer_iterate(&ifmt_opaque)))
		{
			if (!fmt->priv_class)
				continue;

			if (AV_IS_INPUT_DEVICE(
					fmt->priv_class->category) || AV_IS_OUTPUT_DEVICE(fmt->priv_class->category))
			{
				printf("out device:%s,%s\n", fmt->name, fmt->long_name);
			}
		}
		const AVInputFormat *ifmt = NULL;
		ifmt_opaque = NULL;
		while ((ifmt = av_demuxer_iterate(&ifmt_opaque)))
		{
			if (!ifmt->priv_class)
				continue;
			if (AV_IS_INPUT_DEVICE(
					ifmt->priv_class->category) || AV_IS_OUTPUT_DEVICE(ifmt->priv_class->category))
			{
				printf("input device:%s,%s\n", ifmt->name, ifmt->long_name);
			}
		}
	}
	printf("%s\n", in_filename);
	//注1：av_register_all()这个方法在FFMPEG 4.0以后将不再建议使用，而且是非必需的，因此直接注释掉即可
	//av_register_all();
	av_log_set_level(AV_LOG_INFO);
	//Network
#if HAVE_AV_DEVICE
	avdevice_register_all();
#endif
	avformat_network_init();

	AVDictionary *opts = NULL;

	if (strncasecmp(in_filename, "rtsp://", strlen("rtsp://")) == 0)
	{
		av_dict_set(&opts, "buffer_size", "1024000", 0); //设置缓存大小，1080p可将值调大
		av_dict_set(&opts, "max_delay", "500000", 0);
		av_dict_set(&opts, "rtsp_transport", "tcp", 0); //设置tcp or udp，默认一般优先tcp再尝试udp
		av_dict_set(&opts, "stimeout", "3000000", 0);	//设置超时3秒
	}

	AVInputFormat *input_fmt = NULL; //输入视频流格式

	//input_fmt = av_find_input_format("video4linux2");
	printf("input_fmt<%d>\n", input_fmt);

	ifmt_ctx = avformat_alloc_context();

	//Input
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, input_fmt, &opts))
			< 0)
	{
		printf("Could not open input file.");
		goto end;
	}
	printf("open success\n");
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0)
	{
		printf("Failed to retrieve input stream information");
		goto end;
	}

	avmedia_type_video = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1,
			-1, NULL, 0);
	if (avmedia_type_video < 0)
	{
		fprintf(stderr, "err not find media type video\n");
		goto end;
	}

	av_dump_format(ifmt_ctx, 0, ifmt_ctx->url, 0);
	{
		AVStream *st = ifmt_ctx->streams[avmedia_type_video];
		if (st)
		{
			const char *pix_fmt_name =
					st->codecpar->format == AV_PIX_FMT_NONE ?
							"none" :
							av_get_pix_fmt_name(
									(enum AVPixelFormat) st->codecpar->format);
			const char *avcodocname = avcodec_get_name(st->codecpar->codec_id);
			const char *profilestring = avcodec_profile_name(
					st->codecpar->codec_id, st->codecpar->profile);
			//char * codec_fourcc =  av_fourcc_make_string(videostream->codecpar->codec_tag);

			printf("编码方式:%s\n Codec Profile:%s\n", avcodocname, profilestring);
			printf("显示编码格式: %s \n", pix_fmt_name);
			printf("像素尺寸:%dx%d \n", st->codecpar->width, st->codecpar->height);
			printf("最低帧率:%f fps 平均帧率:%f fps\n", av_q2d(st->r_frame_rate),
					av_q2d(st->avg_frame_rate));
			printf("视频流比特率 :%fkbps\n", st->codecpar->bit_rate / 1000.0);
			//const AVCodecDescriptor *codecDes=avcodec_descriptor_get(st->codecpar->codec_id);

			in_codec_id = st->codecpar->codec_id;

			if (st->codecpar->codec_id == AV_CODEC_ID_H264)
			{

				printf("open decoder: h264_rkmpp \n");
				avdecoder = avcodec_find_decoder_by_name("h264_rkmpp");
			}
			else if (st->codecpar->codec_id == AV_CODEC_ID_HEVC)
			{
				printf("open decoder: hevc_rkmpp \n");
				//avdecoder = avcodec_find_decoder(st->codecpar->codec_id);
				avdecoder = avcodec_find_decoder_by_name("hevc_rkmpp");
			}
			else
			{
				printf("open decoder:%d \n", st->codecpar->codec_id);
				avdecoder = avcodec_find_decoder(st->codecpar->codec_id);
			}

			if (avdecoder == NULL)
				avdecoder = avcodec_find_decoder(st->codecpar->codec_id);

			avdecoder_ctx = avcodec_alloc_context3(avdecoder);
			avcodec_parameters_to_context(avdecoder_ctx, st->codecpar);

			if (avcodec_open2(avdecoder_ctx, avdecoder, NULL) < 0)
			{
				avcodec_free_context(&avdecoder_ctx);
				avdecoder_ctx = NULL;
				printf("not open decoder");
				goto end;
			}

			mstream->minfo.status = 1;
			mstream->minfo.video_h = st->codecpar->height;
			mstream->minfo.video_w = st->codecpar->width;
			strcpy(mstream->minfo.video_code_name, avcodocname);
			mstream->minfo.fps = st->codecpar->bit_rate / 1000.0;

			if (mstream->bk_status_change)
			{
				mstream->bk_status_change(mstream);
			}
		}
	}

	printf("start read [%s] avmedia_type_video=%d\n", in_filename,
			avmedia_type_video);
	sleep(1);

	uint64_t frame_video_count = 0;

	struct fps_calc _fps1, _fps2;
	uint64_t time_read_start = 0, time_read_end = 0, time_rest = 0;

	struct media_yuvdata media_data;
	memset(&media_data, 0, sizeof(media_data));

	memset(&_fps1, 0, sizeof(_fps1));
	memset(&_fps2, 0, sizeof(_fps2));

	unsigned int _out_not = 0;

	queue_video = (FrameQueue*) malloc(sizeof(FrameQueue));

	AVFrame *avframe = av_frame_alloc();

	frame_queue_init(queue_video, 20, 1);

	mstream->queue_video = queue_video;

	pthread_t pt;
	pthread_create(&pt, NULL, _thread_queue_read, mstream);

	while (mstream->loop)
	{
		//AVStream *in_stream, *out_stream;
		time_read_start = av_gettime();
		//Get an AVPacket
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret == AVERROR_EOF)
		{
			//mp4 file
			int result = av_seek_frame(ifmt_ctx, -1, 0 * AV_TIME_BASE,
			AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);
			printf(
					"---------------  file eof, seek ret=%d start:%lu ---------------\n",
					result, ifmt_ctx->start_time);
			continue;
		}
		if (ret < 0)
			break;
		/**
		 pts: （int64_t）显示时间，结合AVStream->time_base转换成时间戳
		 dts: （int64_t）解码时间，结合AVStream->time_base转换成时间戳
		 size: （int）data的大小
		 stream_index: （int）packet在stream的index位置
		 flags: （int）标示，结合AV_PKT_FLAG使用，其中最低为1表示该数据是一个关键帧。
		 #define AV_PKT_FLAG_KEY    0x0001 //关键帧
		 #define AV_PKT_FLAG_CORRUPT 0x0002 //损坏的数据
		 #define AV_PKT_FLAG_DISCARD  0x0004 /丢弃的数据
		 side_data_elems: （int）边缘数据元数个数
		 duration: （int64_t）数据的时长，以所属媒体流的时间基准为单位，未知则值为默认值0
		 pos: （int64_t ）数据在流媒体中的位置，未知则值为默认值-1
		 */
#if 0
		printf("Send %8d , pkt.size=%d, pts:%d pkt.stream_index=%d\n",
				frame_index, pkt.size, pkt.pts, pkt.stream_index);
		{
			for (int i = 0; i < 10; i++)
			{
				printf("%02X ", pkt.data[i]);
			}
			//00 00 00 01 XX
			if ((0x0 == pkt.data[0] && 0x0 == pkt.data[1] && 0x0 == pkt.data[2]
							&& 0x01 == pkt.data[3] && (0x07 == (pkt.data[4] & 0x1f))))
			{
				printf("<I frame>");
			}
			printf("\n");
		}
#endif
		if (pkt.stream_index != avmedia_type_video)
		{
			av_packet_unref(&pkt);
			continue;
		}

		//需要将pkt发送到websocket
		if (mstream->bk_media_stream_frame_raw)
		{
			mstream->bk_media_stream_frame_raw(mstream, in_codec_id, pkt.data,
					pkt.size);
		}

		//编码成yuv
		if ((ret = avcodec_send_packet(avdecoder_ctx, &pkt)))
		{
			printf("avcodec_send_packet error : 0x%x \n", ret);
			av_packet_unref(&pkt);
			continue;
		}

		//编码
		while (1) //encode->yuv
		{
			ret = avcodec_receive_frame(avdecoder_ctx, avframe); //从解码器中获取解码的输出数据
			if (ret != 0)
			{
				/*0xfffffff5 is just no data*/
				if (ret != 0xfffffff5)
				{
					printf("avcodec_receive_frame error : 0x%x \n", ret);
				}
				break;
			}
			frame_video_count++;
			time_read_end = av_gettime();
			time_rest = time_read_end - time_read_start; //一帧处理时间
			printf("encode time: %lu\n", time_rest / 1000);

			if (strncmp(in_filename, "rtsp://", strlen("rtsp://")) != 0)
			{
				//25fps= 40ms = 40*1000us
				//time_read_end = av_gettime();
				//time_rest = time_read_end - time_read_start;
				if (time_rest < 40 * 1000)
					usleep(40 * 1000 - time_rest);
			}

			{
				Frame *vp;

				AVFrame *src_frame = avframe;
				vp = frame_queue_peek_writable(queue_video);
				if (vp)
				{
					printf("queue write \n");
					src_frame->pkt_pos=frame_video_count;
					av_frame_move_ref(vp->frame, src_frame);
					frame_queue_push(queue_video);
				}
				av_frame_unref(avframe);
			}

			if (fps_calc_step(&_fps1))
			{
				mstream->minfo.fps = _fps1.fps;
			}
		} //end encode->yuv

		//注4：av_free_packet()可被av_free_packet()替换
		//av_free_packet(&pkt);
		av_packet_unref(&pkt);
	}

	av_frame_free(&avframe);

//	if (_yuv_out_data)
//		av_free(_yuv_out_data);

	if (queue_video)
		free(queue_video);
//Write file trailer
	end: mstream->queue_video = NULL;

	if (opts)
		av_dict_free(&opts);

	if (ifmt_ctx)
		avformat_close_input(&ifmt_ctx);

	if (avdecoder_ctx)
		avcodec_free_context(&avdecoder_ctx);

	/* close output */
	if (ret < 0 && ret != AVERROR_EOF)
	{
		printf("Error occurred.\n");
		return -1;
	}
	return 0;
}
