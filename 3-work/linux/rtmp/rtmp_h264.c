/*
 * rtmp_h264.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "rtmp_h264.h"
#include "sps_decode.h"

int H264_NALFindPos(uint8_t *data, int size, int *pnal_head, int *pnal_len)
{
	int pos = 0;
	int nal_head_flag = 0;
	int nal_split_pos = 0;
	int nal_head_pos = 0;
	int nal_len = 0;

	for (pos = 0; pos < size - 3; pos++)
	{
		if (pos < size - 4 && memcmp(data + pos, "\x00\x00\x00\x01", 4) == 0)
		{
			nal_head_flag = 1;
			nal_split_pos = pos;
			nal_head_pos = pos + 4;
			break;
		}
		if (pos < size - 3 &&memcmp(data + pos, "\x00\x00\x01", 3) == 0)
		{
			nal_head_flag = 1;
			nal_split_pos = pos;
			nal_head_pos = pos + 3;
			break;
		}
	}
	if (!nal_head_flag)
		return -1;

	for (pos = nal_head_pos; pos < size; pos++)
	{
		if (pos < size - 4 && memcmp(data + pos, "\x00\x00\x00\x01", 4) == 0)
			break;
		if (pos < size - 3 && memcmp(data + pos, "\x00\x00\x01", 3) == 0)
			break;
	}

	nal_len = pos - nal_head_pos;

	*pnal_head = nal_head_pos;
	*pnal_len = nal_len;
	return nal_split_pos;
}

#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)

char * put_byte(char *output, uint8_t nVal)
{
	output[0] = nVal;
	return output + 1;
}

char * put_be16(char *output, uint16_t nVal)
{
	output[1] = nVal & 0xff;
	output[0] = nVal >> 8;
	return output + 2;
}

char * put_be24(char *output, uint32_t nVal)
{
	output[2] = nVal & 0xff;
	output[1] = nVal >> 8;
	output[0] = nVal >> 16;
	return output + 3;
}

char * put_be32(char *output, uint32_t nVal)
{
	output[3] = nVal & 0xff;
	output[2] = nVal >> 8;
	output[1] = nVal >> 16;
	output[0] = nVal >> 24;
	return output + 4;
}

char *put_be64(char *output, uint64_t nVal)
{
	output = put_be32(output, nVal >> 32);
	output = put_be32(output, nVal);
	return output;
}

char *put_amf_string(char *c, const char *str)
{
	uint16_t len = strlen(str);
	c = put_be16(c, len);
	memcpy(c, str, len);
	return c + len;
}

char *put_amf_double(char *c, double d)
{
	*c++ = AMF_NUMBER; /* type: Number */
	{
		unsigned char *ci, *co;
		ci = (unsigned char *) &d;
		co = (unsigned char *) c;
		co[0] = ci[7];
		co[1] = ci[6];
		co[2] = ci[5];
		co[3] = ci[4];
		co[4] = ci[3];
		co[5] = ci[2];
		co[6] = ci[1];
		co[7] = ci[0];
	}
	return c + 8;
}

int SendPacket(RTMP *rtmp,
			unsigned int nPacketType,
			unsigned char *data, unsigned int size,
			unsigned int nTimestamp)
{
	RTMPPacket* packet;
	packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + size); /*分配包内存和初始化,len为包体长度*/
	memset(packet, 0, RTMP_HEAD_SIZE);
	packet->m_body = (char *) packet + RTMP_HEAD_SIZE;/*包体内存*/
	packet->m_nBodySize = size;
	memcpy(packet->m_body, data, size);

	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = nPacketType; /*此处为类型有两种一种是音频,一种是视频*/
	packet->m_nInfoField2 = rtmp->m_stream_id;
	packet->m_nChannel = 0x04;
	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	if (RTMP_PACKET_TYPE_AUDIO == nPacketType && size != 4)
	{
		packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	}
	packet->m_nTimeStamp = nTimestamp;
	/*发送*/
	int nRet = -1;
	if (RTMP_IsConnected(rtmp))
	{
		nRet = RTMP_SendPacket(rtmp, packet, FALSE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
	}
	/*释放内存*/
	free(packet);
	return nRet;
}

//调整输出块大小的函数
int RTMP_ChangeChunkSize(RTMP *r, int outChunkSize)
{
	RTMPPacket packet;
	char pbuf[RTMP_MAX_HEADER_SIZE + 4];

	packet.m_nBytesRead = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

	packet.m_packetType = RTMP_PACKET_TYPE_CHUNK_SIZE;
	packet.m_nChannel = 0x04;
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 0;
	packet.m_hasAbsTimestamp = 0;
	packet.m_nBodySize = 4;
	r->m_outChunkSize = outChunkSize;

	r->m_outChunkSize = htonl(r->m_outChunkSize);

	memcpy(packet.m_body, &r->m_outChunkSize, 4);

	r->m_outChunkSize = ntohl(r->m_outChunkSize);
	return RTMP_SendPacket(r, &packet, TRUE);
}

int RTMP_SendMetaData(RTMP *rtmp, int width, int height, int fps)
{
	RTMPPacket * packet = NULL; //rtmp包结构
	unsigned char *body = NULL;
	int i;
	packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + 1024);
	packet->m_body = (char *) packet + RTMP_HEAD_SIZE;
	body = (unsigned char *) packet->m_body;

	char *p = body;

	p = put_byte(p, AMF_STRING);
	p = put_amf_string(p, "@setDataFrame");

	p = put_byte(p, AMF_STRING);
	p = put_amf_string(p, "onMetaData");

	p = put_byte(p, AMF_OBJECT);
//	p = put_amf_string(p, "copyright");
//p = put_byte(p, AMF_STRING);
//p = put_amf_string(p, "firehood");

	p = put_amf_string(p, "duration");
	p = put_amf_double(p, 0);

	p = put_amf_string(p, "width");
	p = put_amf_double(p, width);

	p = put_amf_string(p, "height");
	p = put_amf_double(p, height);

//	p = put_amf_string(p, "framerate");
//	p = put_amf_double(p, fps);
	p = put_amf_string(p, "videodatarate");
	p = put_amf_double(p, 0);

	p = put_amf_string(p, "videocodecid");
	p = put_amf_double(p, 7); //FLV_CODECID_H264=7

	p = put_amf_string(p, "");
	p = put_byte(p, AMF_OBJECT_END);

	packet->m_packetType = RTMP_PACKET_TYPE_INFO; //0x12
	packet->m_nChannel = 0x04;
	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet->m_nTimeStamp = 0;
	packet->m_nInfoField2 = rtmp->m_stream_id;
	packet->m_nBodySize = p - packet->m_body;

	//printf("m_nBodySize=%d\n", packet->m_nBodySize);
	/*调用发送接口*/
	int nRet = RTMP_SendPacket(rtmp, packet, TRUE);
	free(packet);    //释放内存
	return nRet;
}

/**
 * 发送视频的sps和pps信息
 *
 * @param pps 存储视频的pps信息
 * @param pps_len 视频的pps信息长度
 * @param sps 存储视频的pps信息
 * @param sps_len 视频的sps信息长度
 *
 * @成功则返回 1 , 失败则返回0
 */
int RTMP_SendVideoSpsPps(RTMP *rtmp,
			uint32_t timestamp,
			unsigned char *sps, int sps_len,
			unsigned char *pps, int pps_len)
{
	RTMPPacket * packet = NULL; //rtmp包结构
	unsigned char *body = NULL;
	int i;
	packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + 1024);
	//RTMPPacket_Reset(packet);//重置packet状态
	memset(packet, 0, RTMP_HEAD_SIZE + 1024);
	packet->m_body = (char *) packet + RTMP_HEAD_SIZE;
	body = (unsigned char *) packet->m_body;
	i = 0;
	//
	body[i++] = 0x17;
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = sps[1];
	body[i++] = sps[2];
	body[i++] = sps[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++] = 0xe1;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	memcpy(&body[i], sps, sps_len);
	i += sps_len;

	/*pps*/
	body[i++] = 0x01;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len) & 0xff;
	memcpy(&body[i], pps, pps_len);
	i += pps_len;

	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = i;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = timestamp & 0xffffff;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nInfoField2 = rtmp->m_stream_id;

	//printf("sps bps body size:%d\n", i);
	/*调用发送接口*/
	int nRet = RTMP_SendPacket(rtmp, packet, TRUE);
	free(packet);    //释放内存
	return nRet;
}

int RTMP_H264SendPacket(RTMP *rtmp, unsigned char *data, unsigned int size,
			int bIsKeyFrame,
			unsigned int nTimeStamp)
{
	if (data == NULL && size < 11)
	{
		return -1;
	}

	RTMPPacket *packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + size + 9);
	memset(packet, 0, RTMP_HEAD_SIZE);
	RTMPPacket_Reset(packet);
	packet->m_body = (char *) packet + RTMP_HEAD_SIZE;
	packet->m_nBodySize = size + 9;

	unsigned char *body = packet->m_body;
	memset(body, 0, size + 9);

	int i = 0;
	if (bIsKeyFrame)
		body[i++] = 0x17;    // 1:Iframe  7:AVC
	else
		body[i++] = 0x27;    // 2:Pframe  7:AVC
	body[i++] = 0x01;    // AVC NALU
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	// NALU size
	body[i++] = size >> 24 & 0xff;
	body[i++] = size >> 16 & 0xff;
	body[i++] = size >> 8 & 0xff;
	body[i++] = size & 0xff;

	// NALU data
	memcpy(&body[i], data, size);
	int ret = -1;

	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nInfoField2 = rtmp->m_stream_id;
	packet->m_nChannel = 0x04;
	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet->m_nTimeStamp = nTimeStamp & 0xffffff;

	if (RTMP_IsConnected(rtmp))
		ret = RTMP_SendPacket(rtmp, packet, TRUE);
	free(packet);
	return ret;
}

