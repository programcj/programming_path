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

//H264定义的类型 values for nal_unit_type
enum
{
	NALU_TYPE_SLICE = 1,
	NALU_TYPE_DPA = 2,
	NALU_TYPE_DPB = 3,
	NALU_TYPE_DPC = 4,
	NALU_TYPE_IDR = 5,
	NALU_TYPE_SEI = 6,
	NALU_TYPE_SPS = 7,
	NALU_TYPE_PPS = 8,
	NALU_TYPE_AUD = 9,
	NALU_TYPE_EOSEQ = 10,
	NALU_TYPE_EOSTREAM = 11,
	NALU_TYPE_FILL = 12
};

typedef struct t_h264_nalu_header
{
	//小端模式哦(反的)
	unsigned char nal_unit_type :5;
	unsigned char nal_reference_idc :2;
	unsigned char forbidden_bit :1;
} H264_NALU_HEADER;

#include "rtmp.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// nal_head_pos : NAL头的位置
// nal_len : NAL长度,从头位置开始
int H264_NALFindPos(uint8_t *data, int size, int *pnal_head_pos, int *pnal_len);

//发送SPS PPS
int RTMP_SendVideoSpsPps(RTMP *rtmp,
			uint32_t timestamp,
			unsigned char *sps, int sps_len,
			unsigned char *pps, int pps_len);

//发送H264数据
int RTMP_H264SendPacket(RTMP *rtmp,
			unsigned char *data, unsigned int size,
			int bIsKeyFrame,
			unsigned int nTimeStamp);

#ifdef __cplusplus
}
#endif

#endif /* RTMP_H264_H_ */
