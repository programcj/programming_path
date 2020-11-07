/*
 * main.c
 *
 *  Created on: 2020年6月24日
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
#include <unistd.h>
#include "log.h"
#include <getopt.h>

#include "rtmp.h"
#include "sps_decode.h"
#include "LibOnvif/SOAP_Onvif.h"

#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavcodec/avcodec.h"
#include "libavutil/pixdesc.h"
#include "libavutil/time.h"

struct stream_in
{
	//AVDictionary *opts;
	AVFormatContext *fmt_ctx;
	AVPacket pkt;
	int video_index;
	int video_frame_count;
};

void stream_in_close(struct stream_in *in)
{
	if (in->fmt_ctx)
		avformat_close_input(&in->fmt_ctx);
}

int stream_in_open(struct stream_in *in, const char *url)
{
	int ret;
	AVDictionary *opts = NULL;
	av_dict_set(&opts, "rtsp_transport", "tcp", 0); //设置tcp or udp，默认一般优先tcp再尝试udp
	av_dict_set(&opts, "stimeout", "3000000", 0);   //设置超时3秒

//	av_dict_set(&opts, "probsize", "4096", 0);
//	av_dict_set(&opts, "fflags", "nobuffer", 0);
//  in->fmt_ctx=avformat_alloc_context();
//  in->fmt_ctx->probesize = 20000000;
//  in->fmt_ctx->max_analyze_duration = 2000;

	ret = avformat_open_input(&in->fmt_ctx, url, 0, &opts);
	if (ret)
		goto _err;

	if (opts)
		av_dict_free(&opts);
	//in->opts = opts;

	ret = avformat_find_stream_info(in->fmt_ctx, 0);

	if (ret < 0)
		goto _err;

	ret = av_find_best_stream(in->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (ret < 0)
		goto _err;
	in->video_index = ret;
	//ret = av_find_best_stream(in->fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	//for (int i = 0; i < in->fmt_ctx->nb_streams; i++)
	//{
	//	AVFormatContext *ofmt_ctx;
	//	AVStream *in_stream = in->fmt_ctx->streams[i];
	//	AVStream *out_stream = NULL;
	//
	//	if (in->fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
	//	{
	//		videoindex = i;
	//		out_stream = avformat_new_stream(ofmt_ctx_v, in_stream->codec->codec);
	//		ofmt_ctx = ofmt_ctx_v;
	//	}
	//	else if (in->fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
	//	{
	//		audioindex = i;
	//		out_stream = avformat_new_stream(ofmt_ctx_a, in_stream->codec->codec);
	//		ofmt_ctx = ofmt_ctx_a;
	//	}
	//	else
	//	{
	//		break;
	//	}
	//}
	return 0;

	_err:
	if (opts)
		av_dict_free(&opts);
	if (in->fmt_ctx)
		avformat_close_input(&in->fmt_ctx);

	return ret;
}

struct stream_out
{
	AVOutputFormat *ofmt;
	AVFormatContext *ofmt_ctx;
};

int stream_out_open(struct stream_out *out, const char *url)
{
	int ret;
	ret = avformat_alloc_output_context2(&out->ofmt_ctx, NULL, "flv", url); //RTMP
	if (out->ofmt_ctx)
		out->ofmt = out->ofmt_ctx->oformat;
	return ret;
}

void stream_out_close(struct stream_out *out)
{
	if (out->ofmt_ctx && out->ofmt && !(out->ofmt->flags & AVFMT_NOFILE))
		avio_close(out->ofmt_ctx->pb);
	if (out->ofmt_ctx)
		avformat_free_context(out->ofmt_ctx);
	out->ofmt = NULL;
	out->ofmt_ctx = NULL;
}

int stream_in2out_build(struct stream_in *in, struct stream_out *out)
{
	int ret;
	int i;

	for (i = 0; i < in->fmt_ctx->nb_streams; i++)
	{
		//Create output AVStream according to input AVStream
		if (in->fmt_ctx->streams[i]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
			continue;

		AVStream *in_stream = in->fmt_ctx->streams[i];
		AVCodec *avcodec = avcodec_find_decoder(in_stream->codecpar->codec_id);
		AVStream *out_stream = avformat_new_stream(out->ofmt_ctx, avcodec);
		if (!out_stream)
		{
			log_w("Stream", "Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto _err;
		}

		AVCodecContext *codec_ctx = avcodec_alloc_context3(avcodec);
		ret = avcodec_parameters_to_context(codec_ctx, in_stream->codecpar);
		if (ret < 0)
		{
			log_w("Stream",
						"Failed to copy context from input to output stream codec context\n");
			goto _err;
		}

		codec_ctx->codec_tag = 0;
		if (out->ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		ret = avcodec_parameters_from_context(out_stream->codecpar, codec_ctx);
	}

	if (!(out->ofmt_ctx->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&out->ofmt_ctx->pb, out->ofmt_ctx->url, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			log_w("Stream", "Could not open output URL '%s'\n", out->ofmt_ctx->url);
			goto _err;
		}
	}
	//Write file header
	ret = avformat_write_header(out->ofmt_ctx, NULL);
	if (ret < 0)
	{
		log_w("Stream", "Error occurred when opening output URL\n");
		goto _err;
	}

	return 0;

	_err:
	return -1;
}

uint64_t os_time_ms()
{
	struct timespec time =
				{ 0, 0 };
	clock_gettime(CLOCK_MONOTONIC, &time);

	return (uint64_t) time.tv_sec * 1000 + (uint64_t) time.tv_nsec / 1000000;
}

void stream_in2out(struct stream_in *in, struct stream_out *out)
{
	int ret;
	AVPacket inpkt;
	av_init_packet(&inpkt);

	int64_t start_time = av_gettime();
	int64_t _dts = 0;
	//PTS：显示时间戳，对应的数据格式为 AVFrame（解码后的帧），通过 AVCodecContext->time_base 获取。
	//DTS：解码时间戳，对应的数据格式为AVPacket（解码前的包），通过AVStream->time_base获取。
	while (1)
	{
		AVStream *in_stream, *out_stream;
		ret = av_read_frame(in->fmt_ctx, &in->pkt);
		if (in->pkt.stream_index != in->video_index)
			continue;
		in->video_frame_count++;
		{
			in_stream = in->fmt_ctx->streams[in->pkt.stream_index];
			out_stream = out->ofmt_ctx->streams[in->pkt.stream_index];
			if (in->pkt.stream_index == in->video_index)
			{
				AVRational time_base = in_stream->time_base;
				AVRational time_base_q =
							{ 1, AV_TIME_BASE };
				int64_t pts_time = av_rescale_q(in->pkt.dts, time_base, time_base_q);
				int64_t now_time = av_gettime() - start_time;
				if (pts_time > now_time)
					av_usleep(pts_time - now_time);
			}

			//转换PTS/DTS（Convert PTS/DTS）
			in->pkt.pts = av_rescale_q_rnd(in->pkt.pts, in_stream->time_base, out_stream->time_base,
						(enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			in->pkt.dts = av_rescale_q_rnd(in->pkt.dts, in_stream->time_base, out_stream->time_base,
						(enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			in->pkt.duration = av_rescale_q(in->pkt.duration, in_stream->time_base, out_stream->time_base);
			in->pkt.pos = -1;

			if (_dts == 0)
				_dts = in->pkt.dts;

			if (in->pkt.dts < _dts)
			{
				printf("pkt.dts< dts:  %lu < %lu\n", in->pkt.dts, _dts);
				continue;
			}
			_dts = in->pkt.dts;
#if 0
			if (in->pkt.pts == AV_NOPTS_VALUE) //没有显示时间(比如未解码的 H.264)
			{
				//AVRational time_base：时基。通过该值可以把PTS，DTS转化为真正的时间。
				AVRational time_base1 =
				in->fmt_ctx->streams[in->video_index]->time_base;

				//计算两帧之间的时间
				/*
				 r_frame_rate 基流帧速率  （不是太懂）
				 av_q2d 转化为double类型
				 */
				int64_t calc_duration = (double) AV_TIME_BASE
				/ av_q2d(
							in->fmt_ctx->streams[in->video_index]->r_frame_rate);
				//配置参数
				in->pkt.pts =
				(double) (in->video_frame_count * calc_duration)
				/ (double) (av_q2d(time_base1) * AV_TIME_BASE);
				in->pkt.dts = in->pkt.pts;
				in->pkt.duration = (double) calc_duration
				/ (double) (av_q2d(time_base1) * AV_TIME_BASE);
			}

			//延时
			if (in->pkt.stream_index == in->video_index)
			{
				AVRational time_base =
				in->fmt_ctx->streams[in->video_index]->time_base;
				AVRational time_base_q =
				{	1, AV_TIME_BASE};
				//计算视频播放时间
				int64_t pts_time = av_rescale_q(in->pkt.dts, time_base,
							time_base_q);
				//计算实际视频的播放时间
				int64_t now_time = av_gettime() - start_time;

				AVRational avr = in->fmt_ctx->streams[in->video_index]->time_base;

				if (pts_time > now_time)
				{
					//睡眠一段时间（目的是让当前视频记录的播放时间与实际时间同步）
					av_usleep((unsigned int) (pts_time - now_time));
				}
			}

			in_stream = in->fmt_ctx->streams[in->pkt.stream_index];
			out_stream = out->ofmt_ctx->streams[in->pkt.stream_index];

			//计算延时后，重新指定时间戳
			in->pkt.pts = av_rescale_q_rnd(in->pkt.pts, in_stream->time_base,
						out_stream->time_base,
						(enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			in->pkt.dts = av_rescale_q_rnd(in->pkt.dts, in_stream->time_base,
						out_stream->time_base,
						(enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			in->pkt.duration = (int) av_rescale_q(in->pkt.duration,
						in_stream->time_base, out_stream->time_base);
#endif
		}
		log_d("Stream", "%lu next.. \n", os_time_ms());
		ret = av_interleaved_write_frame(out->ofmt_ctx, &in->pkt);
		if (ret < 0)
		{
			fprintf(stderr, "发送数据包出错, %s\n", av_err2str(ret));
			break;
		}
		av_packet_unref(&in->pkt);
	}
	av_write_trailer(out->ofmt_ctx);
}

void startPush(const char *uri_in, const char *uri_out)
{
	struct stream_in in;
	struct stream_out out;
	int ret;

	memset(&in, 0, sizeof(in));
	memset(&out, 0, sizeof(out));

	ret = stream_in_open(&in, uri_in);
	if (ret < 0)
	{
		log_e("Stream", "not open input url:%s, ret=%d\n", uri_in, ret);
		goto _quit;
	}

	av_dump_format(in.fmt_ctx, 0, in.fmt_ctx->url, 0);
	ret = stream_out_open(&out, uri_out);
	if (ret < 0)
	{
		log_e("Stream", "not open out url:%s\n", uri_out);
		goto _quit;
	}

	ret = stream_in2out_build(&in, &out);
	if (ret < 0)
	{
		log_e("Stream", "not open out url:%s\n", uri_out);
		goto _quit;
	}

	log_i("Stream", "start push ...\n");

	av_dump_format(out.ofmt_ctx, 0, out.ofmt_ctx->url, 1);
	stream_in2out(&in, &out);

	_quit:

	stream_in_close(&in);
	stream_out_close(&out);
}

#include <sys/socket.h>
#include <arpa/inet.h>

//return 0 success; !0 error;
int ToolIPV4MaskAddressStringCheck(const char *value)
{
	struct in_addr addr;
	int ret;
	memset(&addr, 0, sizeof(addr));
	ret = inet_pton(AF_INET, value, &addr);
	if (ret <= 0)
		ret = -1;
	//printf("ret=%d %s, addr:%X\n", ret, value, htonl(addr.s_addr));
	if (ret == 1)
	{
		addr.s_addr = htonl(addr.s_addr);
		in_addr_t b = ~addr.s_addr + 1;
		if ((b & (b - 1)) == 0)
			return 0;
		return -1;
	}
	return ret;
}

int ToolIP4AndMaskCheck(const char *ipstr, const char *maskstr)
{
	int ret;
	//struct in_addr addr_ip;
	//struct in_addr addr_mask;
	union ipv4
	{
		uint8_t v[4];
		in_addr_t s_addr;
	};

	union ipv4 addr_ip;
	union ipv4 addr_mask;

	ret = inet_pton(AF_INET, ipstr, &addr_ip);
	if (ret != 1)
		return -1;

	ret = inet_pton(AF_INET, maskstr, &addr_mask);
	if (ret != 1)
		return -1;

	if (addr_ip.v[0] == 127 || addr_ip.v[0] == 0 || addr_ip.v[0] == 255)
		return -1;

	ret = ToolIPV4MaskAddressStringCheck(maskstr);
	if (ret == -1)
		return -1;

	printf("%d.%d.%d.%d %d.%d.%d.%d\n", addr_ip.v[0], addr_ip.v[1], addr_ip.v[2], addr_ip.v[3],
				addr_mask.v[0], addr_mask.v[1], addr_mask.v[2], addr_mask.v[3]
				);
	uint32_t _mask = htonl(addr_mask.s_addr);
	uint32_t _ip = htonl(addr_ip.s_addr);
	uint32_t _ip_number = _ip & ~(_ip & _mask);  //去子网网段的ip数字
	uint32_t _mask_number_max = ~_mask;  //子网最大数量

	if (_ip_number == 0 || _ip_number >= _mask_number_max)
		return -1;

	return 0;
}

void Onvif_Tets(const char *ip, const char *user, const char *pass)
{
	struct OnvifDeviceInfo onvifInfo;
	int ret;
	memset(&onvifInfo, 0, sizeof(onvifInfo));
	ret = OnvifDeviceInfo_Get(ip, user, pass, &onvifInfo);
	if (ret != 200)
	{
		printf("error.....\n");
		return;
	}
	printf("Manufacturer:[%s]\n", onvifInfo.devInfo.Manufacturer);
	printf("mediaUrl:%s\n", onvifInfo.mediaUrl);
	for (int i = 0; i < 3; i++)
	{
		if (strlen(onvifInfo.uri[i]) == 0)
			break;
		printf("url:[%s]\n", onvifInfo.uri[i]);
	}
	printf("---------------------------------------------\n");
}

int main(int argc, char **argv)
{
	int ch;
	char uri_in[300];
	char uri_out[300];
	int ret;
	char hname[100];
//	gethostname(hname, sizeof(hname));
//	printf("host:[%s]\n", hname);
//	Onvif_Tets("192.168.0.9", "admin", "Dc123456"); //hk
//	Onvif_Tets("192.168.0.150", "admin", "admin"); //dahua
//	Onvif_Tets("192.168.0.212", "admin", "xbwl8888"); //hk
//	Onvif_Tets("192.168.0.197", "admin", "admin123456"); //dahua
//	Onvif_Tets("192.168.0.67", "admin", "admin"); //dahua
//	Onvif_Tets("192.168.0.88", "admin", "123456"); //
	//第一个整数不能为127（环回地址），不能为0和255
	printf("%d\n", ToolIP4AndMaskCheck("192.168.1.2", "255.255.255.0"));

	memset(uri_in, 0, sizeof(uri_in));
	memset(uri_out, 0, sizeof(uri_out));

	while ((ch = getopt(argc, argv, "i:o:t:h")) != -1)
	{
		switch (ch)
		{
			case 'i':
				strcpy(uri_in, optarg);
			break;
			case 'o':
				strcpy(uri_out, optarg);
			break;
			case 'h':
				printf("-i input file url\n");
				printf("-o out file url\n");
				return 0;
			break;
			case 'v':
				printf("%s %s\n", __DATE__, __TIME__);
			break;
			default:
				break;
		}
	}

	//rtsp://admin@admin:192.168.0.150:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif
	char *url_in =
				"rtsp://admin:admin@192.168.0.150:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif";
	//url_in = "rtsp://admin:admin@192.168.0.88:554/profile1";
	//url_in = "rtsp://192.168.100.123:8880/hg_base.h264";

	char *url_out = "rtmp://127.0.0.1:1985/live/test";

	//url_out = "rtmp://oms.lxvision.com:2935/test/test1";
	int librtmp_test(const char *url_in, const char *url_out);
	librtmp_test(url_in, url_out);
	//startPush(url_in, url_out);
	return 0;
}

static void databuff_out_hex(uint8_t *data, int size, FILE *stream, int maxlen)
{
	uint8_t *ptr = data;
	uint8_t *ptr_end = ptr + size;
	int v;
	int i = 0;
	int prvpos = 0;
	int hexlen = 0;
	if (maxlen > 0 && maxlen < size)
	{
		ptr_end = ptr + maxlen;
	}
	while (ptr < ptr_end)
	{
		fprintf(stream, "%02X ", *ptr);
		hexlen = (i + 1) % 16;

		if (hexlen == 0 || ptr + 1 >= ptr_end)
		{

			while (hexlen != 0 && hexlen++ < 16)
			{
				fprintf(stream, "   ");
			}
			fprintf(stream, "|");
			//fprintf(stream, "|[%2d,%2d]",  prvpos, i);
			while (prvpos <= i)
			{
				v = ptr[prvpos - i]; //
				if (v == '\n' || v == '\r')
					v = '.';
				fputc(v, stream);
				prvpos++;
			}
			fprintf(stream, "|\n");
			prvpos = i + 1;
		}
		i++;
		ptr++;
	}
}

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

typedef enum _NALTYPE NalType;

typedef struct t_h264_nalu_header
{
	//小端模式哦(反的)
	unsigned char nal_unit_type :5;
	unsigned char nal_reference_idc :2;
	unsigned char forbidden_bit :1;
} H264_NALU_HEADER;

int NALFindPos(uint8_t *data, int size, int *pnal_head, int *pnal_len)
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
		if (memcmp(data + pos, "\x00\x00\x01", 3) == 0)
		{
			nal_head_flag = 1;
			nal_split_pos = pos;
			nal_head_pos = pos + 3;
			break;
		}
	}
	if (!nal_head_flag)
		return -1;
	for (pos = nal_head_pos; pos < size - 3; pos++)
	{
		if (pos < size - 4 && memcmp(data + pos, "\x00\x00\x00\x01", 4) == 0)
		{
			break;
		}
		if (memcmp(data + pos, "\x00\x00\x01", 3) == 0)
		{
			break;
		}
	}

	nal_len = pos - nal_head_pos;

	*pnal_head = nal_head_pos;
	*pnal_len = nal_len;
	return nal_split_pos;
}

/**
 * 解码SPS,获取视频图像宽、高和帧率信息
 *
 * @param buf SPS数据内容
 * @param nLen SPS数据的长度
 * @param width 图像宽度
 * @param height 图像高度
 * @成功则返回true , 失败则返回false
 */
/**
 * 发送RTMP数据包
 *
 * @param nPacketType 数据类型
 * @param data 存储数据内容
 * @param size 数据大小
 * @param nTimestamp 当前包的时间戳
 *
 * @成功则返回 1 , 失败则返回一个小于0的数
 */
//定义包头长度，RTMP_MAX_HEADER_SIZE=18
#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)

int SendPacket(RTMP *rtmp, unsigned int nPacketType, unsigned char *data, unsigned int size, unsigned int nTimestamp)
{
	RTMPPacket* packet;
	/*分配包内存和初始化,len为包体长度*/
	packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + size);
	memset(packet, 0, RTMP_HEAD_SIZE);
	/*包体内存*/
	packet->m_body = (char *) packet + RTMP_HEAD_SIZE;
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
		nRet = RTMP_SendPacket(rtmp, packet, TRUE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
	}
	/*释放内存*/
	free(packet);
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
			unsigned char *pps, int pps_len,
			unsigned char *sps, int sps_len)
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

	/*调用发送接口*/
	int nRet = RTMP_SendPacket(rtmp, packet, TRUE);
	free(packet);    //释放内存
	return nRet;
}

struct media_data
{
	char *pps;
	char *sps;
	int pps_len;
	int sps_len;
};

int SendH264Packet(RTMP *rtmp, struct media_data *mdata,
			unsigned char *data, unsigned int size,
			int bIsKeyFrame,
			unsigned int nTimeStamp)
{
	if (data == NULL && size < 11)
	{
		return -1;
	}

	RTMPPacket *packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + size + 9);
	memset(packet, 0, RTMP_HEAD_SIZE);
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

	if (bIsKeyFrame)
	{
		RTMP_SendVideoSpsPps(rtmp, nTimeStamp,
					mdata->pps, mdata->pps_len,
					mdata->sps, mdata->sps_len);
	}
	int ret = -1;

	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nInfoField2 = rtmp->m_stream_id;
	packet->m_nChannel = 0x04;
	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet->m_nTimeStamp = nTimeStamp & 0xffffff;

	if (RTMP_IsConnected(rtmp))
		ret = RTMP_SendPacket(rtmp, packet, TRUE);
	//ret = SendPacket(rtmp, RTMP_PACKET_TYPE_VIDEO, body, 9 + size, nTimeStamp);
	free(packet);
	return ret;
}

void h264_sps_parse(uint8_t *sps, int len)
{
	int width = 0, height = 0, fps = 0;
	h264_decode_sps(sps, len, &width, &height, &fps);
	printf("width: %d height:%d fps:%d\n", width, height, fps);
}

int nTimeStamp = 0;

// error while decoding MB 29 44, bytestream -70
int h264buff_send(RTMP *rtmp, struct media_data *mdata, int isiframe, uint8_t *data, int size)
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

	do
	{
		nal_split_pos = NALFindPos(ptr, len, &nal_head_pos, &nal_len);
		if (nal_split_pos == -1)
			break;
		uint8_t type = ptr[nal_head_pos];
		H264_NALU_HEADER header;

		header.forbidden_bit = (type & 0x80) >> 7;
		header.nal_reference_idc = (type & 0x60) >> 5;
		header.nal_unit_type = (type & 0x1F);

		printf(">%d,NAL:(%02X) %d, %d, %d: len=%d\n", nal_head_pos,
					ptr[nal_head_pos], header.forbidden_bit, header.nal_reference_idc, header.nal_unit_type, nal_len);

		if (NALU_TYPE_SPS == header.nal_unit_type)
		{
			printf("SPS\n");
			// 解码SPS,获取视频图像宽、高信息
			databuff_out_hex(ptr + nal_head_pos, nal_len, stdout, nal_len);

			h264_sps_parse(ptr + nal_head_pos, nal_len);
			if (mdata->sps)
				free(mdata->sps);

			mdata->sps = (char*) malloc(nal_len);
			mdata->sps_len = nal_len;
			memcpy(mdata->sps, ptr + nal_head_pos, mdata->sps_len);
		}
		if (NALU_TYPE_PPS == header.nal_unit_type)
		{
			printf("PPS\n");
			if (mdata->pps)
				free(mdata->pps);
			mdata->pps = (char*) malloc(nal_len);
			mdata->pps_len = nal_len;
			memcpy(mdata->pps, ptr + nal_head_pos, mdata->pps_len);
			databuff_out_hex(mdata->pps, mdata->pps_len, stdout, mdata->pps_len);
		}
		if (NALU_TYPE_IDR == header.nal_unit_type)
		{
			printf("IDR\n");
			//I帧发送
			ret = SendH264Packet(rtmp, mdata, ptr + nal_head_pos, nal_len, 1, nTimeStamp);

			printf("sen ret=%d\n", ret);
			if (ret == -1)
				return ret;
		}
		if (NALU_TYPE_SLICE == header.nal_unit_type)
		{
			printf("SLICE\n");
			ret = SendH264Packet(rtmp, mdata, ptr + nal_head_pos, nal_len, 0, nTimeStamp);
			//nTimeStamp += 1000 / 25;
			printf("sen ret=%d\n", ret);
			if (ret == -1)
				return ret;
		}

		ptr += nal_head_pos + nal_len;
		len -= nal_len + (nal_head_pos - nal_split_pos);
	} while (1);

	nTimeStamp += 1000 / 25;
	//exit(1);
	return ret;
}

int librtmp_test(const char *url_in, const char *url_out)
{
	struct stream_in streamin;
	int ret;
	memset(&streamin, 0, sizeof(streamin));
	ret = stream_in_open(&streamin, url_in);
	if (ret < 0)
	{
		log_e("Stream", "not open input url:%s, ret=%d\n", url_in, ret);
		return -1;
	}

	RTMP *rtmp = NULL;
	RTMPPacket *packet = NULL;
	rtmp = RTMP_Alloc();
	if (!rtmp)
	{
		RTMP_LogPrintf("RTMP_Alloc failed\n");
		return -1;
	}

//初始化结构体“RTMP”中的成员变量
	RTMP_Init(rtmp);
	rtmp->Link.timeout = 5;
	if (!RTMP_SetupURL(rtmp, url_out))
	{
		RTMP_Log(0, "SetupURL Err\n");
		RTMP_Free(rtmp);
		return -1;
	}
//发布流的时候必须要使用。如果不使用则代表接收流。
	RTMP_EnableWrite(rtmp);
//建立RTMP连接，创建一个RTMP协议规范中的NetConnection
	if (!RTMP_Connect(rtmp, NULL))
	{
		RTMP_Log(1, "Connect Err\n");
		RTMP_Free(rtmp);
		return -1;
	}
//创建一个RTMP协议规范中的NetStream
	if (!RTMP_ConnectStream(rtmp, 0))
	{
		RTMP_Log(1, "ConnectStream Err\n");
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
		return -1;
	}

	RTMP_LogPrintf("Start to send data ...\n");

	uint32_t start_time = 0;
	uint32_t now_time = 0;
	uint32_t timestamp = 0;
	uint32_t type = 0;

	start_time = RTMP_GetTime();

	int iframe = 0;
	struct media_data mediadata;
	memset(&mediadata, 0, sizeof(mediadata));

	while (1)
	{
		ret = av_read_frame(streamin.fmt_ctx, &streamin.pkt);
		if (ret < 0)
		{
			printf("av_read_frame err\n");
			break;
		}

		if (streamin.pkt.stream_index != streamin.video_index)
		{
			av_packet_unref(&streamin.pkt);
			continue;
		}
		if (streamin.pkt.flags && AV_PKT_FLAG_KEY)
			iframe = 1;

		if (iframe == 0)
		{
			av_packet_unref(&streamin.pkt);
			continue;
		}

		printf("next frame\n");
		ret = h264buff_send(rtmp, &mediadata, streamin.pkt.flags && AV_PKT_FLAG_KEY ? 1 : 0, streamin.pkt.data, streamin.pkt.size);
		av_packet_unref(&streamin.pkt);
		if (ret == -1)
		{
			printf("rtmp push err\n");
			break;
		}
#if 0
		timestamp = RTMP_GetTime();

		packet = (RTMPPacket*) malloc(sizeof(RTMPPacket));
		RTMPPacket_Alloc(packet, streamin.pkt.size); //64k
		RTMPPacket_Reset(packet);
		packet->m_hasAbsTimestamp = 0;
		packet->m_nChannel = 0x04;
		packet->m_nInfoField2 = rtmp->m_stream_id;

		//packet header  大尺寸
		packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
		packet->m_nTimeStamp = timestamp;//时间戳
		packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;//包类型
		packet->m_nBodySize = streamin.pkt.size;//包大小
		memcpy(packet->m_body, streamin.pkt.data, streamin.pkt.size);

		av_packet_unref(&streamin.pkt);

		if (!RTMP_IsConnected(rtmp))
		{	            //确认连接
			RTMP_Log(1, "rtmp is not connect\n");
			break;
		}

		RTMP_Log(0, "send packet %d", packet->m_nBodySize);
		//发送一个RTMP数据RTMPPacket
		ret = RTMP_SendPacket(rtmp, packet, 0);
		if (!ret)
		{
			RTMP_Log(1, "Send Error\n");
			break;
		}

		if (packet != NULL)
		{
			RTMPPacket_Free(packet);
			free(packet);
		}
#endif
	}

	RTMP_LogPrintf("\nSend Data Over!\n");
	if (mediadata.pps)
		free(mediadata.pps);
	if (mediadata.sps)
		free(mediadata.sps);

	if (rtmp != NULL)
	{
		//关闭RTMP连接
		RTMP_Close(rtmp);
		//释放结构体RTMP
		RTMP_Free(rtmp);
		rtmp = NULL;
	}
	if (packet != NULL)
	{
		RTMPPacket_Free(packet);
		free(packet);
		packet = NULL;
	}
	return 0;
}
