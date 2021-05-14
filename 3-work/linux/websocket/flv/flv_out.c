/*
 * test.c
 *
 *  Created on: 2021年3月1日
 *      Author: cc
 *  输出FLV数据,用于 ws://../xx.flv 输出，支持 H264/H265
 *
 typedef struct {
 UB[2] Reserved;
 UB[1] Filter;
 UB[5] TagType;
 UI24 DataSize;
 UI24 Timestamp;
 UI8 TimestampExtended;
 UI24 StreamID;
 IF TagType == 8
 AudioTagHeader Header;
 IF TagType == 9
 VideoTagHeader Header;
 IF TagType == 8
 AUDIODATA Data;
 IF TagType == 9
 VIDEODATA Data;
 IF TagType == 18
 SCRIPTDATA Data;
 }   FLVTAG;

 * 请注意编码格式
 */
#include "amf.h"
#include "rtmp_h264.h"
#include "sps_decode.h"
#include "flv_out.h"

#if defined(WIN32) || defined(_WIN32)
static int H264_NALFindPos(uint8_t* data, int size, int* pnal_head, int* pnal_len)
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
		if (pos < size - 3 && memcmp(data + pos, "\x00\x00\x01", 3) == 0)
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

	if (pnal_head)
		*pnal_head = nal_head_pos;
	if (pnal_len)
		*pnal_len = nal_len;
	return nal_split_pos;
}
#else
#include <netinet/in.h>
#endif

static char *put_byte(char *output, uint8_t nVal)
{
	output[0] = nVal;
	return output + 1;
}

static char *put_be16(char *output, uint16_t nVal)
{
	output[1] = nVal & 0xff;
	output[0] = nVal >> 8;
	return output + 2;
}

static char *put_be24(char *output, uint32_t nVal)
{
	output[2] = nVal & 0xff;
	output[1] = nVal >> 8;
	output[0] = nVal >> 16;
	return output + 3;
}

static char *put_be32(char *output, uint32_t nVal)
{
	output[3] = nVal & 0xff;
	output[2] = nVal >> 8;
	output[1] = nVal >> 16;
	output[0] = nVal >> 24;
	return output + 4;
}

static char *put_be64(char *output, uint64_t nVal)
{
	output = put_be32(output, nVal >> 32);
	output = put_be32(output, nVal);
	return output;
}

static char *put_amf_string(char *c, const char *str)
{
	uint16_t len = strlen(str);
	c = put_be16(c, len);
	memcpy(c, str, len);
	return c + len;
}

static char *put_amf_double(char *c, double d)
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

void *memcpya(void *dstdata, const void *srcdata, int size)
{
	char*ptr =(char*) memcpy(dstdata, srcdata, size);
	ptr += size;
	return ptr;
}

typedef struct FlvOut
{
	uint32_t PreviousTagSize;
	int (*_write)(struct FlvOut *out, void *data, int size, void *usr);
	void *usr;
	struct media_data mdata;
	int frame_key_exists;
} FLV;

int flv_out_data(FLV *flv, void *data, int size, void *usr)
{
	if (flv->_write)
		return flv->_write(flv, data, size, usr);
	return 0;
}

//@tag_type: 8=音频; 9=视频; 18(0x12)=脚本数据
int flv_out_tag(FLV *flv, uint8_t tag_type, int timestamp, uint8_t *tag_data, int tag_size)
{
	uint32_t previousTagSize;
	uint8_t tag_header_data[11]; //FLVTAG
	int i = 0;

	memset(tag_header_data, 0, sizeof(tag_header_data));
	tag_header_data[i++] = tag_type; //8 = 音频	9 = 视频	18(0x12) = 脚本数据
	tag_header_data[i++] = (tag_size & 0x00FF0000) >> 16; //DataSize
	tag_header_data[i++] = (tag_size & 0x0000FF00) >> 8; //
	tag_header_data[i++] = (tag_size & 0xFF); //
	tag_header_data[i++] = (timestamp & 0x00FF0000) >> 16;  //Timestamp
	tag_header_data[i++] = (timestamp & 0x00FF00) >> 8;
	tag_header_data[i++] = (timestamp & 0xFF);
	tag_header_data[i++] = 0; //TimestampExtended
	tag_header_data[i++] = 0; //StreamID
	tag_header_data[i++] = 0;
	tag_header_data[i++] = 0;

	previousTagSize = ntohl(flv->PreviousTagSize);
	flv_out_data(flv, &previousTagSize, 4, flv->usr);
	flv_out_data(flv, tag_header_data, 11, flv->usr);
	flv_out_data(flv, tag_data, tag_size, flv->usr);
	flv->PreviousTagSize = tag_size + 11;
	return 0;
}

FLVHandle flv_out_build(int (*_write)(struct FlvOut *out, void *data, int size, void *usr))
{
	FLVHandle handle = (FLV *) calloc(sizeof(FLV), 1);
	if (handle)
		handle->_write = _write;
	return handle;
}
void flv_out_free(FLVHandle h)
{
	if (h == NULL)
		return;
	if (h->mdata.sps)
		free(h->mdata.sps);
	if (h->mdata.pps)
		free(h->mdata.pps);
	if (h->mdata.vps)
		free(h->mdata.vps);
	free(h);
}

void flv_out_set_usr(FLVHandle flv, void *usr)
{
	flv->usr = usr;
}

void flv_out_start(FLVHandle flv)
{
	uint8_t flv_head_data[9];
	uint8_t *ptr = flv_head_data;
	memset(flv_head_data, 0, sizeof(flv_head_data));
	ptr = memcpya(ptr, "FLV", 3);
	*ptr++ = 0x01; //version
	*ptr++ = 0x01; //0x01 video; 0x04 audio; 0x05 video and audio;
	uint32_t data_offset = ntohl(9);
	ptr = memcpya(ptr, &data_offset, 4);
	//preTagSize
	flv_out_data(flv, flv_head_data, 9, flv->usr);
}

//script tag //Metadata Tag
int flv_SendMetaData(FLV *flv, int width, int height, int framerate_fps, const char *vcodename)
{
	char body[1024];
	char *p = body;

//		p = put_byte(p, AMF_STRING);
//		p = put_amf_string(p, "@setDataFrame");

	p = put_byte(p, AMF_STRING); //AMF1 type=2
	p = put_amf_string(p, "onMetaData"); //string size=10; string onMetaData

	p = put_byte(p, AMF_ECMA_ARRAY);
	p = put_be32(p, 8);  //表示下面的内容有8个(存储为flv视频后使用flv视频分析工具[FlvAnalyzer.exe]需要） https://blog.jianchihu.net/flvanalyzer.html

	p = put_amf_string(p, "duration"); //文件总时长，单位秒
	p = put_amf_double(p, 0);

	p = put_amf_string(p, "width");
	p = put_amf_double(p, width);

	p = put_amf_string(p, "height");
	p = put_amf_double(p, height);

	p = put_amf_string(p, "videodatarate"); //视频码率
	p = put_amf_double(p, 0);

	p = put_amf_string(p, "framerate");
	p = put_amf_double(p, 25);

	p = put_amf_string(p, "videocodecid"); //视频解码器ID
	if (strcasecmp("h264", vcodename) == 0)
		p = put_amf_double(p, 7); //FLV_CODECID_H264=7 ; H265 12
	if (strcasecmp("h265", vcodename) == 0)
		p = put_amf_double(p, 12);

	p = put_amf_string(p, "title");
	p = put_byte(p, AMF_STRING);
	p = put_amf_string(p, "Media Server");

	p = put_amf_string(p, "encoder");
	p = put_byte(p, AMF_STRING);
	p = put_amf_string(p, "Lavf56.40.101");

//		p = put_amf_string(p, "filesize");//
//		p = put_amf_double(p, 0); //

	p = put_amf_string(p, ""); // end: 00 00 09
	p = put_byte(p, AMF_OBJECT_END);

	uint32_t tag_size = p - body;

	return flv_out_tag(flv, 0x12, 0, body, tag_size);
}

int flv_SendVideoSpsPps(FLV *flv, int tm,
			unsigned char *sps, int sps_len,
			unsigned char *pps, int pps_len)
{
	uint8_t tag_data[1024];
	int i = 0;
	uint8_t *body = tag_data;
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
	return flv_out_tag(flv, 0x09, tm, tag_data, i);
}

int flv_SendPacketH264(FLV *flv, unsigned char *data, unsigned int size,
			int bIsKeyFrame,
			unsigned int nTimeStamp)
{
	uint8_t *tag_data = (uint8_t*) malloc(size + 9);
	unsigned char *body = tag_data;
	int ret;
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
	i += size;
	ret = flv_out_tag(flv, 0x09, nTimeStamp, tag_data, i);
	free(tag_data);
	return ret;
}

void flv_out_clear(FLVHandle flv)
{
	flv->mdata.status = 0; //需要重新发送sps/pps/等info
}

int flv_out_video_h264(FLVHandle flv, uint8_t *data, int size, int ms)
{
	// 读取SPS帧
	// 读取PPS帧
	// 解码SPS,获取视频图像宽、高信息
	//databuff_out_hex((uint8_t*) data, size, stdout, 16 * 5);
	int nal_head_pos = 0;
	int nal_split_pos = 0;
	int nal_len = 0;
	uint8_t *ptr = data;
	int len = size;
	int ret = 0;
	struct media_data *pmdata = &flv->mdata;

	do
	{
		nal_split_pos = H264_NALFindPos(ptr, len, &nal_head_pos, &nal_len);
		if (nal_split_pos == -1)
			break;
		uint8_t type = ptr[nal_head_pos];
		H264_NALU_HEADER header;

		header.forbidden_bit = (type & 0x80) >> 7;
		header.nal_reference_idc = (type & 0x60) >> 5;
		header.nal_unit_type = (type & 0x1F);

		//printf(">%d,NAL:(%02X) %d, %d, %d: len=%d\n", nal_head_pos,
		//			ptr[nal_head_pos], header.forbidden_bit, header.nal_reference_idc, header.nal_unit_type, nal_len);
		switch (header.nal_unit_type)
		{
			case NALU_TYPE_SPS: //7
			{
				//printf("SPS\n");
				// 解码SPS,获取视频图像宽、高信息
				//log_print_hex(ptr + nal_head_pos, nal_len, stdout, nal_len);
				int width, height, fps;
				h264_decode_sps(ptr + nal_head_pos, nal_len, &width, &height, &fps);
				if (pmdata->width != 0 && pmdata->height != 0 && pmdata->width != width && pmdata->height != height)
				{
					pmdata->status = 0;
					log_d("flvout", "分辨率有变化:%dx%d\n", width, height);
				}
				pmdata->width = width;
				pmdata->height = height;
				pmdata->fps = fps;
				if (pmdata->sps)
					free(pmdata->sps);

				pmdata->sps = (char*) malloc(nal_len);
				pmdata->sps_len = nal_len;
				memcpy(pmdata->sps, ptr + nal_head_pos, pmdata->sps_len);
			}
			break;
			case NALU_TYPE_PPS: //8
			{
				//printf("PPS\n");
				if (pmdata->pps)
					free(pmdata->pps);
				pmdata->pps = (char*) malloc(nal_len);
				pmdata->pps_len = nal_len;
				memcpy(pmdata->pps, ptr + nal_head_pos, pmdata->pps_len);
				//log_print_hex(pmdata->pps, pmdata->pps_len, stdout, pmdata->pps_len);
			}
			break;
			case NALU_TYPE_IDR: //5 I帧
			{
				//printf("IDR\n");
				if (pmdata->status == 0)
				{
					log_d("flvout", "save MetaData: width:%d, height:%d fps:%d\n", pmdata->width,
								pmdata->height,
								pmdata->fps);
					ret = flv_SendMetaData(flv, pmdata->width, pmdata->height, pmdata->fps, "h264");
					log_d("flvout", "SendVideoSpsPps: sps=%d, pps=%d\n", pmdata->sps_len, pmdata->pps_len);
					ret = flv_SendVideoSpsPps(flv, 0,
								pmdata->sps,
								pmdata->sps_len,
								pmdata->pps,
								pmdata->pps_len
								);
					pmdata->status = 1;
				}
				flv->frame_key_exists = 1;
				ret = flv_SendPacketH264(flv, ptr + nal_head_pos, nal_len, 1, pmdata->nTimeStamp);
				pmdata->nTimeStamp += ms;
				if (ret == -1)
					return ret;
			}
			break;
			case NALU_TYPE_SLICE: //1
			{
				//printf("SLICE\n");
				if (flv->frame_key_exists == 0)
					break;

				ret = flv_SendPacketH264(flv, ptr + nal_head_pos, nal_len, 0, pmdata->nTimeStamp);
				pmdata->nTimeStamp += ms;
				if (ret == -1)
					return ret;
			}
			break;
			case NALU_TYPE_SEI: //补充增强信息
			break;
			default:
				break;
		}

		ptr += nal_head_pos + nal_len;
		len -= nal_len + (nal_head_pos - nal_split_pos);
	} while (1);

	//pmdata->nTimeStamp += ms;				// 1000 / 25; //需要知道多少ms
	return ret;
}

int flv_SendVideoSpsPpsVps(FLV *flv,
			uint32_t tm,
			unsigned char *pps, int pps_len,
			unsigned char *sps, int sps_len,
			unsigned char *vps, int vps_len)
{
	uint8_t tag_data[1024];
	int i = 0;
	int ret;
	uint8_t *body = tag_data;
	i = 0;
	body[i++] = 0x10 | 0x0C;

	//          4 | HEVCDecoderConfigurationRecord：（最小长度23字节）
	//            | 01 3  4  5  6  8  9  10             17
	//00 00 00 00 | 01 01 60 00 00 00 b0 00 00 00 00 00 7b f0 00 fc fd f8 f8 00 00 0f 03
	// SPS:  4201 | 01 01 60 00 00 03 00 b0 00 00 03 00 00 03 00 7b a0 03 c0 80 10 e5 8d ae 49 14 bf 37 01 01 01 00 80
	//        42 01 01 01 60 00 00 00 b0 00 00 00 00 00 5d a0 02 80 80 2d 16 36 b9 24 52 fc dc 04 04 04 02 02 02 02
	//       0  1    2 3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
	body[i++] = 0x00;	// AVC sequence header   1byte
	body[i++] = 0x00;	//composition time 3 byte
	body[i++] = 0x00;
	body[i++] = 0x00;

	body[i++] = 0x01;
	/*
	 * unsigned int(2) general_profile_space;
	 * unsigned int(1) general_tier_flag;
	 * unsigned int(5) general_profile_idc;
	 */
	body[i++] = sps[3];
	body[i++] = sps[4]; /* unsigned int(32) general_profile_compatibility_flags; */  //0x60
	body[i++] = sps[5];
	body[i++] = sps[6];
	body[i++] = 00;

	body[i++] = sps[8]; /* unsigned int(48) general_constraint_indicator_flags; */
	body[i++] = sps[9];
	body[i++] = sps[10];
	body[i++] = 0;
	body[i++] = 0;
	body[i++] = 0;
	body[i++] = sps[17]; /* unsigned int(8) general_level_idc; */
	/*
	 * bit(4) reserved = ‘1111’b;
	 * unsigned int(12) min_spatial_segmentation_idc;
	 */
	body[i++] = 0xF0;
	body[i++] = 0;
	/*
	 * bit(6) reserved = ‘111111’b;
	 * unsigned int(2) parallelismType;
	 */
	body[i++] = 0xFC;
	/*
	 * bit(6) reserved = ‘111111’b;
	 * unsigned int(2) chromaFormat;
	 */
	body[i++] = 0xFD;
	/*
	 * bit(5) reserved = ‘11111’b;
	 * unsigned int(3) bitDepthLumaMinus8;
	 */
	body[i++] = 0xF8;
	/*
	 * bit(5) reserved = ‘11111’b;
	 * unsigned int(3) bitDepthChromaMinus8;
	 */
	body[i++] = 0xF8;
	/* bit(16) avgFrameRate; */
	body[i++] = 0x00;
	body[i++] = 0x00;
	/*
	 * bit(2) constantFrameRate;
	 * bit(3) numTemporalLayers;
	 * bit(1) temporalIdNested;
	 * unsigned int(2) lengthSizeMinusOne;
	 */
	body[i++] = 0x0F;
	/* unsigned int(8) numOfArrays; */
	body[i++] = 0x03;

	// VPS
	body[i++] = 0x20;
	body[i++] = (1 >> 8) & 0xff;
	body[i++] = 1 & 0xff;
	body[i++] = (vps_len >> 8) & 0xff;
	body[i++] = (vps_len) & 0xff;
	memcpy(&body[i], vps, vps_len);
	i += vps_len;

	// sps
	body[i++] = 0x21;
	body[i++] = (1 >> 8) & 0xff;
	body[i++] = 1 & 0xff;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	memcpy(&body[i], sps, sps_len);
	i += sps_len;

	// pps
	pps_len++;
	body[i++] = 0x22;
	body[i++] = (1 >> 8) & 0xff;
	body[i++] = 1 & 0xff;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len) & 0xff;
	memcpy(&body[i], pps, pps_len - 1);
	body[i + pps_len - 1] = 0;
	i += pps_len;
	return flv_out_tag(flv, 0x09, tm, tag_data, i);
}

int flv_SendPacketH265(FLV *flv, struct media_data *mdata,
			unsigned char *data, unsigned int size,
			int bIsKeyFrame, unsigned int nTimeStamp)
{
	if (data == NULL && size < 11)
		return -1;

	if (bIsKeyFrame)
		flv_SendVideoSpsPpsVps(flv, nTimeStamp,
					mdata->pps, mdata->pps_len,
					mdata->sps, mdata->sps_len,
					mdata->vps, mdata->vps_len);

	uint8_t *tag_data = (uint8_t*) malloc(size + 9);

	unsigned char *body = tag_data;
	memset(body, 0, size + 9);

	int i = 0;
	if (bIsKeyFrame)
		body[i++] = 0x10 | 0x0C;    // 1:Iframe  7:AVC 这里改为C(12)nginx转发必须的
	else
		body[i++] = 0x20 | 0x0C;    // 2:Pframe  7:AVC
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
	i += size;
	int ret = -1;
	ret = flv_out_tag(flv, 0x09, nTimeStamp, tag_data, i);
	free(tag_data);
	return ret;
}

int flv_out_video_h265(FLVHandle flv, uint8_t *data, int size, int ms)
{
	int nal_head_pos = 0;
	int nal_split_pos = 0;
	int nal_len = 0;
	uint8_t *ptr = data;
	int len = size;
	int ret = 0;
	struct media_data *pmdata = &flv->mdata;
	do
	{
		nal_split_pos = H264_NALFindPos(ptr, len, &nal_head_pos, &nal_len);
		if (nal_split_pos == -1)
			break;
		int nal_unit_type = (ptr[nal_head_pos] & 0x7E) >> 1;
		switch (nal_unit_type)
		{
			case NAL_UNIT_SPS: //7
			{
				int width = 0, height = 0, fps = 0;
				if (pmdata->sps)
					free(pmdata->sps);
				pmdata->sps = (unsigned char*) malloc(nal_len);
				pmdata->sps_len = nal_len;
				memcpy(pmdata->sps, ptr + nal_head_pos, pmdata->sps_len);
				h265_decode_sps(ptr + nal_head_pos, nal_len, &width, &height, &fps);
				if (pmdata->width != 0 && pmdata->height != 0 && pmdata->width != width && pmdata->height != height)
				{
					pmdata->status = 0;
					log_d("flvout", "分辨率有变化:%dx%d\n", width, height);
				}
				pmdata->width = width;
				pmdata->height = height;
				pmdata->fps = fps;
				//printf("SPS: %dx%d, %dfps\n", width, height, fps);
			}
			break;
			case NAL_UNIT_PPS: //8
			{ //printf("PPS\n");
				if (pmdata->pps)
					free(pmdata->pps);
				pmdata->pps = (unsigned char*) malloc(nal_len);
				pmdata->pps_len = nal_len;
				memcpy(pmdata->pps, ptr + nal_head_pos, pmdata->pps_len);
//				log_print_hex(pmdata->pps, pmdata->pps_len, stdout, pmdata->pps_len);
			}
			break;
			case NAL_UNIT_VPS:
				{ //printf("VPS\n");
				if (pmdata->vps)
					free(pmdata->vps);
				pmdata->vps = (unsigned char*) malloc(nal_len);
				pmdata->vps_len = nal_len;
				memcpy(pmdata->vps, ptr + nal_head_pos, pmdata->vps_len);
			}
			break;
			case NAL_UNIT_CODED_SLICE_IDR:
				case NAL_UNIT_CODED_SLICE_TRAIL_R:
				{
				int iskey = (nal_unit_type == NAL_UNIT_CODED_SLICE_IDR) ? 1 : 0;
				if (iskey && pmdata->status == 0)
				{
					ret = flv_SendMetaData(flv, pmdata->width, pmdata->height, pmdata->fps, "h265");
					pmdata->status = 1;
					flv->frame_key_exists = 1;
				}
				if (!flv->frame_key_exists)
					break;
				ret = flv_SendPacketH265(flv, pmdata,
							ptr + nal_head_pos, nal_len,
							iskey, pmdata->nTimeStamp);
				pmdata->nTimeStamp += ms;
				if (ret == -1)
					return ret;
			}
			break;
			case NAL_UNIT_SEI:
				break;
			default:
				//printf("unknow send nalu type [%s]%d...\n", HevcNaluType_ToString(nal_unit_type), nal_unit_type);
			break;
		}
		ptr += nal_head_pos + nal_len;
		len -= nal_len + (nal_head_pos - nal_split_pos);
	} while (1);
	return ret;
}

void media_test_flv()
{
	media_io pio;
	int ret;
	char *url = "rtsp://admin:admin@192.168.0.88:554/profile1";

	url = "rtsp://admin:admin@192.168.0.150:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif";

	//ffmpeg -i rtsp://admin:admin@192.168.0.88:554/profile2 -vcodec copy -f flv  aaa.flv
	FILE *fp = fopen("aa.flv", "wb");
	FLV *flv = (FLV*) calloc(sizeof(FLV), 1);

	flv->PreviousTagSize = 0;
	flv->usr = fp;

	ret = media_stream_open(&pio, url);
	if (ret != 0)
		return;

	log_d("flvtest", "[%s],%dx%d,%dfps\n", url, pio->width, pio->height, pio->fps);
	flv_out_start(flv);
	//flv_SendMetaData(flv, pio->width, pio->height, pio->fps, pio->video_code_name);

	uint64_t start_time = os_time_ms();
	uint64_t curr_time = start_time;
	uint64_t old_recv = os_time_ms();
	int64_t pts = 0;
	while (1)
	{
		void *ptr;
		int size;
		int iskey;

		ret = media_stream_read_video_raw(pio, &ptr, &size, &iskey, &pts);
		if (ret == -1)
			break;
		curr_time = os_time_ms();
		if (size > 0)
		{
			if (fp)
			{
				if (strcasecmp("h264", pio->video_code_name) == 0)
					flv_out_video_h264(flv, ptr, size, curr_time - old_recv);
				if (strcasecmp("h265", pio->video_code_name) == 0)
					flv_out_video_h265(flv, ptr, size, curr_time - old_recv);
			}
		}
		media_stream_readfflush(pio);

		old_recv = curr_time;

		if (os_time_ms() - start_time >= 5 * 1000)
			break;
	}

	media_stream_close(pio);
	flv_out_free(flv);

	if (fp)
		fclose(fp);
}

