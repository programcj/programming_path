#include "RTMPPush.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ffmpegex.h"
#include "rtmp_video.h"

#if defined(_MSC_VER)
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

extern "C"
{
#define CONFIG_HAVE_RKMPP 0

	enum URLType {
		URL_TYPE_UNKNOW,
		URL_TYPE_RTSP,
		URL_TYPE_HTTP,
		URL_TYPE_MP4
	};

	extern int H264_NALFindPos(uint8_t* data, int size, int* pnal_head_pos, int* pnal_len);

	struct ffmpegIn
	{
		AVFormatContext *fmt_ctx;
		///
		AVStream *vstream;
		int video_stream_index;
		int video_codec_id;
		char video_codec_name[32];
		int width;
		int height;
		int fps;
		int filetype;

		AVBSFContext* h264bsfc;
	};

	//解码
	struct ffmpegDec
	{
		AVCodec *pCodec;
		AVCodecContext *pCodecCtx; //解码器上下文
		int isrkmpp;
		int def_unused;
	};

	int ffmpegInOpen(struct ffmpegIn *in, const char *url)
	{
		int ret;
		AVCodec *dec;
		AVDictionary *opts = NULL;

		if (strncasecmp("rtsp://", url, strlen("rtsp://")) == 0)
		{
			av_dict_set(&opts, "rtsp_transport", "tcp", 0); //设置tcp or udp，默认一般优先tcp再尝试udp
			av_dict_set(&opts, "buffer_size", "1044000", 0);
			av_dict_set(&opts, "max_delay", "500000", 0);
			av_dict_set(&opts, "stimeout", "20000000", 0); //设置超时20秒
														   //in->opts = opts;
			in->filetype = URL_TYPE_RTSP;
		}
		else {
			//非rtsp地址
			if (strstr(url, "://") == NULL) 
			{
				const char *exptr=strrchr(url, '.');
				if (exptr) 
				{
					if (0 == strcasecmp(exptr, ".mp4"))
					{
						//MP4文件
						in->filetype = URL_TYPE_MP4;
					}
				}
			}
		}

		if ((ret = avformat_open_input(&in->fmt_ctx, url, NULL, opts ? &opts : NULL)) < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Cannot open input file:%s\n", url);
			goto _errret;
		}
		if (opts)
		{
			av_dict_free(&opts);
			opts = NULL;
		}

		if ((ret = avformat_find_stream_info(in->fmt_ctx, NULL)) < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
			goto _errret;
		}

		/* select the video stream */
		ret = av_find_best_stream(in->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
		if (ret < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
			goto _errret;
		}
		in->video_stream_index = ret;

		AVStream *vstream = in->fmt_ctx->streams[in->video_stream_index];
		if (!vstream || !vstream->codecpar)
		{
			printf("pCodecPar not found ! \n");
			ret = -1;
			goto _errret;
		}
		in->vstream = vstream;
		in->video_codec_id = (int)vstream->codecpar->codec_id;

		strncpy(in->video_codec_name, avcodec_get_name(vstream->codecpar->codec_id),
				32 - 1);
		if (strcasecmp("hevc", in->video_codec_name) == 0)
			strcpy(in->video_codec_name, "H265");
		if (strcasecmp("h264", in->video_codec_name) == 0)
			strcpy(in->video_codec_name, "H264");

		av_dump_format(in->fmt_ctx, in->video_stream_index, url, 0);

		in->width = vstream->codecpar->width;
		in->height = vstream->codecpar->height;

		//	fps  表示平均帧率，总帧数除以总时长（以s为单位）。
		//	tbr  表示帧率，该参数倾向于一个基准，往往tbr跟fps相同。
		//	tbn  表示视频流 timebase（时间基准），比如ts流的timebase 为90000，flv格式视频流timebase为1000
		//	tbc  表示视频流codec timebase ，对于264码流该参数通过解析sps间接获取（通过sps获取帧率）。
		in->fps = av_q2d(vstream->avg_frame_rate); //fps
		if (in->fps == 0)
			in->fps = av_q2d(vstream->r_frame_rate); //tbr
		if (in->fps < 0)
			in->fps = 25;

		if (in->filetype == URL_TYPE_MP4 && vstream->codecpar->codec_id == AV_CODEC_ID_H264)
		{
			const struct AVBitStreamFilter* bsfptr = av_bsf_get_by_name("h264_mp4toannexb");

			ret = av_bsf_alloc(bsfptr, &in->h264bsfc);
			avcodec_parameters_copy(in->h264bsfc->par_in, vstream->codecpar);
			ret = av_bsf_init(in->h264bsfc);
		}
			//in->h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");

		printf("media_stream  %dx%d,%dfps (%.2f)\n", in->width, in->height, in->fps,
			   av_q2d(vstream->avg_frame_rate));
		return 0;

	_errret:

		if (opts)
			av_dict_free(&opts);

		if (in)
		{
			if (in->fmt_ctx)
				avformat_close_input(&in->fmt_ctx);
			in->fmt_ctx = NULL;
		}

		if (ret == AVERROR_HTTP_UNAUTHORIZED)
			ret = 401;
		return ret;
	}

	int ffmpegInRead(struct ffmpegIn *in, AVPacket *pavpkt)
	{
		return av_read_frame(in->fmt_ctx, pavpkt);
	}

	static void ffmpegInClose(struct ffmpegIn *in)
	{
		if (in == NULL)
			return;
		if (in->h264bsfc)
			av_bsf_free(&in->h264bsfc);

		if (in->fmt_ctx)
			avformat_close_input(&in->fmt_ctx);
	}

	static int ffmpegDecOpen(struct ffmpegDec *dec, struct ffmpegIn *in)
	{
		//查找解码器
		int ret;
		AVStream *vstream = NULL;

		dec->isrkmpp = 0;
		vstream = in->fmt_ctx->streams[in->video_stream_index];
		if (vstream->codecpar->codec_id == AV_CODEC_ID_H264)
		{
			printf("open h264_rkmpp \n");
			dec->pCodec = avcodec_find_decoder_by_name("h264_rkmpp");
			dec->isrkmpp = 1;
		}
		else if (vstream->codecpar->codec_id == AV_CODEC_ID_HEVC)
		{
			printf("open hevc_rkmpp(h265) \n");
			dec->pCodec = avcodec_find_decoder_by_name("hevc_rkmpp");
			dec->isrkmpp = 1;
		}

		if (dec->pCodec == NULL)
		{
			dec->pCodec = avcodec_find_decoder(vstream->codecpar->codec_id);
		}

		dec->pCodecCtx = avcodec_alloc_context3(dec->pCodec); //AVCodecContext *pCodecCtx;
		avcodec_parameters_to_context(dec->pCodecCtx, vstream->codecpar);

		if (avcodec_open2(dec->pCodecCtx, dec->pCodec, NULL) < 0)
		{
			avcodec_free_context(&dec->pCodecCtx);
			return -1;
		}

#if CONFIG_HAVE_RKMPP
		dec->def_unused = 0;
		if (dec->isrkmpp && dec->pCodecCtx)
		{ //RK3399 RKMpp解码器缓存设置为30
			RKMPPDecodeContext *rk_context = dec->pCodecCtx->priv_data;
			RKMPPDecoder *decoder = (RKMPPDecoder *)rk_context->decoder_ref->data;
			ret = mpp_buffer_group_limit_config(decoder->frame_group, 0, 30);
			dec->def_unused = mpp_buffer_group_unused(decoder->frame_group);

			//DISABLE err : mpp 接收错误帧处理,增加这个可以不需要修改ffmpeg
			{
				void *param = NULL;
				unsigned int disable_error = 1;
				param = &disable_error;
				ret = decoder->mpi->control(decoder->ctx, MPP_DEC_SET_DISABLE_ERROR, param);
				if (ret)
					LOGE("ffmpeg", "[RKMPP] Failed to set disable err %d\n", ret);
			}
		}
#endif
		return 0;
	}

	static void ffmpegDecClose(struct ffmpegDec *dec)
	{
		//wait channel ref is 0
		if (dec->pCodecCtx)
			avcodec_flush_buffers(dec->pCodecCtx);

			//close,如果为rk3399的硬件编解码,则需要等待MPP_Buff释放完成后再关闭?是否需要这样不知道
#if CONFIG_HAVE_RKMPP
		if (dec->isrkmpp && dec->pCodecCtx)
		{
			RKMPPDecodeContext *rk_context = dec->pCodecCtx->priv_data;
			RKMPPDecoder *decoder = (RKMPPDecoder *)rk_context->decoder_ref->data;
			size_t unused = 0;
			while (dec->def_unused != (unused = mpp_buffer_group_unused(decoder->frame_group)))
			{
				logInfo("Video", "mpp_buffer_group_unused:%d, def:%d\n", unused, dec->def_unused);
				sleep(1);
			}
		}
#endif
		if (dec->pCodecCtx)
		{
			avcodec_close(dec->pCodecCtx);
			avcodec_free_context(&dec->pCodecCtx);
		}
	}
}

RTMPPush::RTMPPush(const char *urlin, const char *urlout)
{
	strcpy(this->urlin, urlin);
	strcpy(this->urlout, urlout);
	_loop = false;
}

RTMPPush::~RTMPPush()
{
	_loop = false;
}

/******************************************
* ffmpeg -i F:\mp4\20200521-2-wanchaowu.mp4 -vcodec h264 -acodec aac -f flv rtmp://192.168.0.252:1935/live/test1
* ffmpeg -i F:\mp4\dt.mp4 -vcodec h264 -f flv rtmp://192.168.0.252:1935/live/test1
*
* ffmpeg -rtsp_transport tcp -i rtsp://192.168.0.30:8880/hg_base.h264 -vcodec copy -f flv rtmp://192.168.0.252:1935/live/test1
* ffmpeg -rtsp_transport tcp -i rtsp://admin:admin@192.168.0.88:554/profile1 -vcodec copy -f flv rtmp://192.168.0.252:1935/live/test1
*******************************************/
bool RTMPPush::start()
{
	int ret;
	struct ffmpegIn fin;
	memset(&fin, 0, sizeof(fin));

	_loop = true;

	ret = ffmpegInOpen(&fin, urlin);
	if (ret)
	{
		sprintf(stat, "输入流打开失败:%s", urlin);
		return false;
	}
	AVPacket avpkt;
	av_init_packet(&avpkt);

	AVStream *st = fin.fmt_ctx->streams[fin.video_stream_index];
	int64_t pts_old = 0;
	int64_t pts = 0;

	int64_t pts2_old = 0;
	int64_t pts2 = 0;
	uint64_t pktindex = 0;
	uint64_t ostimes[5];

	RTMPOut *out = NULL;

	sprintf(stat, "输入流打开成功 %dx%d [%s]", fin.width, fin.height, fin.video_codec_name);

	while (_loop)
	{
		ret = ffmpegInRead(&fin, &avpkt);
		if (ret)
		{
			if (ret == AVERROR_EOF) 
			{
				printf("文件读取完成\r\n");
			}
			printf("read packet err[%s],ret=%d\r\n", urlin, ret);
			break;
		}

		//非视频帧
		if (avpkt.stream_index != fin.video_stream_index)
		{
			av_packet_unref(&avpkt);
			continue;
		}

		if (strncasecmp("rtsp", urlin, 4) != 0) {
			if (fin.h264bsfc)
			{
				ret = av_bsf_send_packet(fin.h264bsfc, &avpkt);
				if (ret < 0) {
					printf("bsf send err: ret=%d\r\n", ret);
					av_packet_unref(&avpkt);
					break;
				}
				av_packet_unref(&avpkt);
				ret = av_bsf_receive_packet(fin.h264bsfc, &avpkt);
				if (ret < 0) {
					printf(" bsg_receive_packet is error! \r\n");
					av_packet_unref(&avpkt);
					break;
				}
			}		
		}

		//rtmp 推流
		if (strncasecmp("rtsp", urlin, 4) != 0)
		{
			pts += 40;
		}
		else
		{
			AVRational dst_time_base = {1, 1000};
			if (avpkt.pts < 0)
				avpkt.pts = 0;
			//将以 "时钟基c" 表示的 数值a 转换成以 "时钟基b" 来表示。
			pts = av_rescale_q_rnd(avpkt.pts, st->time_base, dst_time_base, AV_ROUND_NEAR_INF);
		}

		if (out == NULL)
		{
			out = (RTMPOut *)calloc(sizeof(RTMPOut), 1);

			printf("推流地址打开:[%s]\r\n", urlout);
			ret = RTMPStreamOut_open(out, urlout);
			if (ret == -1)
			{
				printf("推流地址打开失败:[%s]\r\n", urlout);

				sprintf(stat, "推流地址打开失败[%s]", urlout);
				free(out);
				out = NULL;
			}
			sprintf(stat, "推流中,%dx%d,%s", fin.width, fin.height, fin.video_codec_name);
		}
		
		if (out)
		{
			ostimes[0] = GetTickCount64();
			if (fin.video_codec_id == AV_CODEC_ID_H264)
				ret = RTMPStreamOut_Send(out, avpkt.data, avpkt.size, pts - pts_old);
			if (fin.video_codec_id == AV_CODEC_ID_H265)
				ret = RTMPStreamOut_SendH265(out, avpkt.data, avpkt.size, pts - pts_old);
			ostimes[1] = GetTickCount64();

			if (ret == -1)
			{
				printf("推流错误\r\n");
				RTMPStreamOut_close(out);
				free(out);
				out = NULL;
			}
		}

		pts_old = pts;
		
		if (strncasecmp("rtsp", urlin, 4) != 0)
		{
			pts2 = GetTickCount64(); //

			uint64_t Duration = pts2 - pts2_old;
			//25fps=1000/25=40 //第一帧不需要延时
			const char* msgptr = "";
			if (Duration > 40) {
				int nal_head_pos = 0;
				int nal_len = 0;
				int nal_split_pos = H264_NALFindPos(avpkt.data, avpkt.size, &nal_head_pos, &nal_len);
				if (nal_split_pos != -1)
				{
					uint8_t type = avpkt.data[nal_head_pos];
					printf("[%llu]>>Duration=%llu, type=%d, size=%d\r\n", pktindex++, Duration, type & 0x1F, avpkt.size);
				}
				msgptr = " frame time>40";
			}
							
			if (Duration > 40)
			{
				Duration = 0;
			}
			else
			{
				Duration = 40 - Duration;
			}

			//printf("[%llu]>>Duration=%llu ms, size=%d, %s\r\n", pktindex++, Duration, avpkt.size, msgptr);
			Sleep(Duration);

			pts2_old = GetTickCount64();
		}
		av_packet_unref(&avpkt);
	}

	ffmpegInClose(&fin);
	if (out)
	{
		RTMPStreamOut_close(out);
		free(out);
		out = NULL;
	}
	printf("停止推流[%s]=>[%s]\r\n", urlin, urlout);
	return false;
}

void RTMPPush::stop()
{
	_loop = false;
}

const char* RTMPPush::getURLOut()
{
	return urlout;
}

const char* RTMPPush::getStatStr()
{
	return stat;
}
