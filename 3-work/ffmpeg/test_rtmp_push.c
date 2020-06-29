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
	av_dict_set(&opts, "probsize", "4096", 0);

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

int main(int argc, char **argv)
{
	int ch;
	char uri_in[300];
	char uri_out[300];

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

	char *url_in =
				"rtsp://admin:admin@192.168.0.150:554/cam/realmonitor?channel=1&subtype=1&unicast=true&proto=Onvif";
	char *url_out = "rtmp://192.168.0.189:1985/hls/aaaaa";

	startPush(url_in, url_out);
	return 0;
}

