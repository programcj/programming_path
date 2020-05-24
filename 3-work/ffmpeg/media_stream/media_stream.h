/*
 * media_video_stream.h
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

#ifndef MEDIA_STREAM_H_
#define MEDIA_STREAM_H_

#ifdef __cplusplus
extern "C"
{
#endif

struct fps_calc
{
	uint64_t time_start;
	uint64_t count;
	float fps;
};

int fps_calc_step(struct fps_calc *fps);

struct media_info
{
	int chID;
	char url[300];

	int status; //状态
	char video_code_name[30];
	int video_w;
	int video_h;
	float fps;
};

struct media_stream
{
	struct media_info minfo;
	int loop;
	char rtsp_url[300];
	void (*bk_status_change)(struct media_stream *mstream);
	void (*bk_media_stream_frame_raw)(struct media_stream *mstream, int format,
			void *data, int size);
	void (*bk_media_stream_frame_yuv)(struct media_stream *mstream, int fid,
			int w, int h, int format, void *data, int size);
};

int media_stream_loop(struct media_stream *mstream);

#ifdef __cplusplus
}
#endif

#endif /* MEDIA_STREAM_H_ */
