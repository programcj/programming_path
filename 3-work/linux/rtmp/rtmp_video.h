/*
 * rtmp_h264.h
 *
 *  Created on: 2020年11月9日
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

#ifndef RTMP_H264_H_
#define RTMP_H264_H_

#include <stdint.h>
#include "H264.h"
#include "rtmp.h"

#define NONE "\e[0m"
#define BLACK "\e[0;30m"
#define RED "\e[0;31m"

#ifdef __cplusplus
extern "C"
{
#endif

	// nal_head_pos : NAL头的位置
	// nal_len : NAL长度,从头位置开始
	extern int H264_NALFindPos(uint8_t *data, int size, int *pnal_head_pos, int *pnal_len);

	struct RTMPStreamOut
	{
		char url[300];
		RTMP *rtmp;
		int isopen;

		struct media_data
		{
			unsigned char *pps;
			unsigned char *sps;
			unsigned char *vps;

			int pps_len;
			int sps_len;
			int vps_len;

			int width;
			int height;
			int fps;
			int status;

			int nTimeStamp;
		} mdata;

		int frame_sps_exists;
	};

	typedef struct RTMPStreamOut RTMPOut;

	int RTMPStreamOut_open(RTMPOut *out, const char *url);

	void RTMPStreamOut_close(RTMPOut *out);

	void RTMPStreamOut_setfps(RTMPOut *out, int fps);

	int RTMPStreamOut_Send(RTMPOut *out, uint8_t *data, int size, int ms);

	int RTMPStreamOut_SendH265(RTMPOut *out, uint8_t *data, int size, int ms);

#ifdef __cplusplus
}
#endif

#endif /* RTMP_H264_H_ */
