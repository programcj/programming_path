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

#include "rtmp_h264.h"

void h264_sps_parse(uint8_t *sps, int len)
{
	int width = 0, height = 0, fps = 0;
	h264_decode_sps(sps, len, &width, &height, &fps);
	printf("width: %d height:%d fps:%d\n", width, height, fps);
}

static void log_print_hex(uint8_t *data, int size, FILE *stream, int maxlen)
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

struct stream_in
{
	//AVDictionary *opts;
	AVFormatContext *fmt_ctx;
	AVPacket pkt;
	int video_index;
	int video_frame_count;

	int video_codec_id;
	int width;
	int height;
	int fps;
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

	AVStream *stream_video = in->fmt_ctx->streams[ret];
	//av_dump_format(in->fmt_ctx, ret, url, 0);
	in->width = stream_video->codecpar->width;
	in->height = stream_video->codecpar->height;
	in->video_codec_id = stream_video->codecpar->codec_id;

	const char *avcodocname = avcodec_get_name(stream_video->codecpar->codec_id);
	printf("编码方式:%s\n", avcodocname);
	printf("最低帧率:%f fps 平均帧率:%f fps\n", av_q2d(stream_video->r_frame_rate),
				av_q2d(stream_video->avg_frame_rate));
	printf("视频流比特率 :%fkbps\n", stream_video->codecpar->bit_rate / 1000.0);
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

void h264_video_info(uint8_t *data, int size)
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
		nal_split_pos = H264_NALFindPos(ptr, len, &nal_head_pos, &nal_len);
		if (nal_split_pos == -1)
			break;
		uint8_t type = ptr[nal_head_pos];
		int forbidden_bit = (type & 0x80) >> 7;
		int nal_reference_idc = (type & 0x60) >> 5;
		int nal_unit_type = (type & 0x1F);

		printf(">%d,NAL:(%02X) %d, %d, %d: len=%d\n", nal_head_pos,
					ptr[nal_head_pos], forbidden_bit, nal_reference_idc, nal_unit_type, nal_len);

		if (NALU_TYPE_SPS == nal_unit_type)
		{
			printf("SPS\n");
			// 解码SPS,获取视频图像宽、高信息
			log_print_hex(ptr + nal_head_pos, nal_len, stdout, nal_len);
			h264_sps_parse(ptr + nal_head_pos, nal_len);
		}
		if (NALU_TYPE_PPS == nal_unit_type)
		{
			printf("PPS\n");
		}
		if (NALU_TYPE_IDR == nal_unit_type)
		{
			printf("IDR\n");
		}
		if (NALU_TYPE_SLICE == nal_unit_type)
		{
			printf("SLICE\n");
		}
		ptr += nal_head_pos + nal_len;
		len -= nal_len + (nal_head_pos - nal_split_pos);
	} while (1);
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
		{
			av_packet_unref(&in->pkt);
			continue;
		}
		printf("next... ");
		h264_video_info(in->pkt.data, in->pkt.size);
		in->video_frame_count++;
		{
			in_stream = in->fmt_ctx->streams[in->pkt.stream_index];
			out_stream = out->ofmt_ctx->streams[in->pkt.stream_index];
#if 0
			if (in->pkt.stream_index == in->video_index)
			{
				AVRational time_base = in_stream->time_base;
				AVRational time_base_q =
				{	1, AV_TIME_BASE};
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

			printf("pts: %ld dts: %ld \n", in->pkt.pts, in->pkt.dts);
			if (in->pkt.dts < _dts)
			{
				printf("pkt.dts< dts:  %lu < %lu\n", in->pkt.dts, _dts);
				//continue;
			}
			_dts = in->pkt.dts;
#else
			in->pkt.pts = _dts; //pts反映帧什么时候开始显示
			in->pkt.dts = _dts; //dts反映数据流什么时候开始解码
			in->pkt.duration = av_rescale_q(in->pkt.duration, in_stream->time_base, out_stream->time_base);
			in->pkt.pos = -1;
			_dts = _dts + 1000 / 25;
			printf("pts: %ld dts: %ld duration:%lu\n", in->pkt.pts, in->pkt.dts, in->pkt.duration);
#endif
		}
		ret = av_interleaved_write_frame(out->ofmt_ctx, &in->pkt);
		if (ret < 0)
		{
			fprintf(stderr, "发送数据包出错, %s\n", av_err2str(ret));
			av_packet_unref(&in->pkt);
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
	url_in = "rtsp://192.168.0.12:8880/hg_base.h264";

	char *url_out = "rtmp://127.0.0.1:1985/live/test";

	//url_out = "rtmp://oms.lxvision.com:2935/test/test1";
	int librtmp_test(const char *url_in, const char *url_out);
	//librtmp_test(url_in, url_out);
	startPush(url_in, url_out);
	return 0;
}

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
struct media_data
{
	char *pps;
	char *sps;
	int pps_len;
	int sps_len;
};

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
		nal_split_pos = H264_NALFindPos(ptr, len, &nal_head_pos, &nal_len);
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
			log_print_hex(ptr + nal_head_pos, nal_len, stdout, nal_len);

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
			log_print_hex(mdata->pps, mdata->pps_len, stdout, mdata->pps_len);
		}
		if (NALU_TYPE_IDR == header.nal_unit_type)
		{
			printf("IDR\n");
			//I帧发送
			ret = RTMP_SendVideoSpsPps(rtmp, nTimeStamp, mdata->sps, mdata->sps_len, mdata->pps, mdata->pps_len);
			ret = RTMP_H264SendPacket(rtmp, ptr + nal_head_pos, nal_len, 1, nTimeStamp);
			if (ret == -1)
				return ret;
		}
		if (NALU_TYPE_SLICE == header.nal_unit_type)
		{
			printf("SLICE\n");
			ret = RTMP_H264SendPacket(rtmp, ptr + nal_head_pos, nal_len, 0, nTimeStamp);
			if (ret == -1)
				return ret;
		}

		ptr += nal_head_pos + nal_len;
		len -= nal_len + (nal_head_pos - nal_split_pos);
	} while (1);

	nTimeStamp += 1000 / 25; //+40
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
