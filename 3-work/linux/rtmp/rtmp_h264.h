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
#include "h264_stream.h"
#include "h265_stream.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct media_data
{
	int width;
	int height;
	int fps;
	int status;

	int64_t nTimeStamp;

	unsigned char vps[100];
	unsigned char pps[100];
	unsigned char sps[100];
	
	int pps_len;
	int sps_len;
	int vps_len;
} ;

struct RTMPStreamOut
{
	char url[300];
	void* rtmp;
	int isopen;
	int frame_sps_exists;

	struct media_data mdata;

	sps_t sps; //h264
	h265_sps_t sps_h5;
};

typedef struct RTMPStreamOut RTMPOut;

RTMPOut* RTMPStreamOut_calloc();

int RTMPStreamOut_open(RTMPOut *out, const char *url);

void RTMPStreamOut_close(RTMPOut *out);

void RTMPStreamOut_setfps(RTMPOut *out, int fps);

void RTMPStreamOut_SetInfo(RTMPOut* out, int w, int h, int fps);

int RTMPStreamOut_Send(RTMPOut *out, uint8_t *data, int size, int ms);

int RTMPStreamOut_SendH265(RTMPOut *out, uint8_t *data, int size, int ms);

// pos_start : NAL头的位置
// pos_end : NAL的尾部
int NALUSplit(const void* ptr, int size, int* pos_start, int* pos_end);

#ifdef __cplusplus
}
#endif

#endif /* RTMP_H264_H_ */
