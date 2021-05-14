#ifndef __FFMPEG_EX_H__
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

	uint8_t *ffmpeg_crop(AVFrame *avframe, int dst_format, int x, int y, int w, int h);

	void rect_align(int *x, int *y, int *w, int *h, int align, int bkWidth, int bkHeight);

	int ffmpeg_avframe2BGR24Buff(AVFrame *avframe, uint8_t *buff);

#ifdef __cplusplus
}
#endif

#endif