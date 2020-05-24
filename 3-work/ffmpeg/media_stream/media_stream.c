/*
 * rtsp_video_stream.c
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include "libavdevice/avdevice.h"

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libavutil/avutil.h"
#include "libavutil/pixdesc.h"
#include "libavutil/time.h"
#include "libavutil/imgutils.h"

#include "libswscale/swscale.h"

#include "media_stream.h"

int fps_calc_step(struct fps_calc *fps)
{
	uint64_t delta;
	fps->count++;
	if (fps->count == 1)
		fps->time_start = av_gettime();

	delta = (av_gettime() - fps->time_start) / 1000;
	if (delta >= 1000) //计算出处理时的fps, ms
	{
		//fps->fps = fps->count / (delta * 0.001f); //实时网络传输帧率
		fps->fps = fps->count * 1000.0 / delta; //(fps->count*1.0) / delta * 1000;
		fps->count = 0;
		return 1;
	}
	return 0;
}

void AVFrame_yuv420_to_buff(AVFrame *avframe, unsigned char *buff)
{
	memcpy(buff, avframe->data[0],
		   avframe->width * avframe->height); //Y
	void *_uv = buff + (avframe->width * avframe->height);

	memcpy(_uv, avframe->data[1],
		   (avframe->width / 2) * (avframe->height / 2)); //U

	_uv = _uv += (avframe->width / 2) * (avframe->height / 2);

	memcpy(_uv,
		   avframe->data[2],
		   (avframe->width / 2) * (avframe->height / 2)); //V
}

void AVFrame_yuv420_to_nv12_buff(AVFrame *avframe, unsigned char *buff)
{
	unsigned char *srcY = avframe->data[0];
	unsigned char *srcU = avframe->data[1];
	unsigned char *srcV = avframe->data[2];
	unsigned char *dstNV = buff + avframe->width * avframe->height;
	int y_h = 0;
	int u_h = 0;
	unsigned char *dstY = buff;

	for (y_h = 0; y_h < avframe->height; y_h++) //Y
	{
		memcpy(dstY, srcY, avframe->width);
		srcY += avframe->linesize[0];
		dstY += avframe->width;
	}

	int u_w = 0;
	for (u_h = 0; u_h < avframe->height / 2; u_h++) //UV
	{
		for (u_w = 0; u_w < avframe->width / 2; u_w++)
		{
			(*dstNV++) = srcU[u_w];
			(*dstNV++) = srcV[u_w];
		}
		srcU += avframe->linesize[1];
		srcV += avframe->linesize[2];
	}
}

void AVFrame_nv12_to_buff(AVFrame *avframe, unsigned char *buff)
{
#if 0
	memcpy(buff, avframe->data[0],
		   avframe->width * avframe->height); //Y

	memcpy(buff + (avframe->width * avframe->height),
	    avframe->data[1],
	    avframe->width * (avframe->height / 2)); //UV
#else
	unsigned char *srcY = avframe->data[0];

	int y_h = 0;
	unsigned char *dstY = buff;

	for (y_h = 0; y_h < avframe->height; y_h++) //Y
	{
		memcpy(dstY, srcY, avframe->width);
		srcY += avframe->linesize[0];
		dstY += avframe->width;
	}

	int u_h = 0;
	int u_w = 0;
	unsigned char *srcUV = avframe->data[1];
	unsigned char *dstUV = dstY;

	for (u_h = 0; u_h < avframe->height / 2; u_h++) //UV
	{
		memcpy(dstUV, srcUV, avframe->width / 2);
		dstUV += avframe->width / 2;
	}
#endif
}

void AVFrame_nv12_to_yuv420_buff(AVFrame *avframe, unsigned char *buff)
{
#if 0

#else
	unsigned char *pU = buff + avframe->width * avframe->height;
	unsigned char *pV = pU + ((avframe->width * avframe->height) >> 2);

	memcpy(buff, avframe->data[0],
		   avframe->width * avframe->height); //Y

	//---NV12---
	//Y Y Y Y
	//Y Y Y Y
	//Y Y Y Y
	//Y Y Y Y
	//U V U V
	//U V U V
	for (int i = 0; i < (avframe->width * avframe->height) / 2; i++)
	{
		if ((i % 2) == 0)
			*pU++ = *(avframe->data[1] + i); //U
		else
			*pV++ = *(avframe->data[1] + i); //V
	}
#endif
}

int media_stream_loop(struct media_stream *mstream)
{
	AVFormatContext *ifmt_ctx = NULL;
	AVOutputFormat *ofmt = NULL;
	AVCodecContext *avdecoder_ctx = NULL;
	AVCodec *avdecoder = NULL;

	int in_codec_id = 0;

	AVPacket pkt;
	const char *in_filename;
	int ret, i;
	int avmedia_type_video = -1;

	in_filename = mstream->rtsp_url;

	printf("%s\n", in_filename);
	//注1：av_register_all()这个方法在FFMPEG 4.0以后将不再建议使用，而且是非必需的，因此直接注释掉即可
	//av_register_all();
	av_log_set_level(AV_LOG_INFO);
	//Network
	avdevice_register_all();
	avformat_network_init();

	AVDictionary *opts = NULL;

	if (strncasecmp(in_filename, "rtsp://", strlen("rtsp://")) == 0)
	{
		av_dict_set(&opts, "buffer_size", "1024000", 0);
		av_dict_set(&opts, "max_delay", "500000", 0);
		av_dict_set(&opts, "rtsp_transport", "tcp", 0); //设置tcp or udp，默认一般优先tcp再尝试udp
		av_dict_set(&opts, "stimeout", "3000000", 0);	//设置超时3秒
	}

	AVInputFormat *input_fmt = NULL; //输入视频流格式

	input_fmt = av_find_input_format("video4linux2");
	printf("input_fmt<%d>\n", input_fmt);

	ifmt_ctx = avformat_alloc_context();

	//Input
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, input_fmt, &opts)) < 0)
	{
		printf("Could not open input file.");
		goto end;
	}
	printf("open success\n");
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0)
	{
		printf("Failed to retrieve input stream information");
		goto end;
	}

	avmedia_type_video = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1,
											 -1, NULL, 0);
	if (avmedia_type_video < 0)
	{
		fprintf(stderr, "err not find media type video\n");
		goto end;
	}

	av_dump_format(ifmt_ctx, 0, ifmt_ctx->url, 0);
	{
		AVStream *st = ifmt_ctx->streams[avmedia_type_video];
		if (st)
		{
			const char *pix_fmt_name =
				st->codecpar->format == AV_PIX_FMT_NONE ? "none" : av_get_pix_fmt_name((enum AVPixelFormat)st->codecpar->format);
			const char *avcodocname = avcodec_get_name(st->codecpar->codec_id);
			const char *profilestring = avcodec_profile_name(
				st->codecpar->codec_id, st->codecpar->profile);
			//char * codec_fourcc =  av_fourcc_make_string(videostream->codecpar->codec_tag);

			printf("编码方式:%s\n Codec Profile:%s\n", avcodocname, profilestring);
			printf("显示编码格式: %s \n", pix_fmt_name);
			printf("像素尺寸:%dx%d \n", st->codecpar->width, st->codecpar->height);
			printf("最低帧率:%f fps 平均帧率:%f fps\n", av_q2d(st->r_frame_rate),
				   av_q2d(st->avg_frame_rate));
			printf("视频流比特率 :%fkbps\n", st->codecpar->bit_rate / 1000.0);
			//const AVCodecDescriptor *codecDes=avcodec_descriptor_get(st->codecpar->codec_id);

			in_codec_id = st->codecpar->codec_id;

			if (st->codecpar->codec_id == AV_CODEC_ID_H264)
			{

				printf("open h264_rkmpp \n");
				//avdecoder = avcodec_find_decoder_by_name("h264_rkmpp");

				avdecoder = avcodec_find_decoder(st->codecpar->codec_id);
			}
			else if (st->codecpar->codec_id == AV_CODEC_ID_H265)
			{
				printf("open h265 \n");
				avdecoder = avcodec_find_decoder(st->codecpar->codec_id);
			}
			else
			{
				avdecoder = avcodec_find_decoder(st->codecpar->codec_id);
				//printf("open hevc_rkmpp \n");
				//avdecoder = avcodec_find_decoder_by_name("hevc_rkmpp");
			}

			avdecoder_ctx = avcodec_alloc_context3(avdecoder);
			avcodec_parameters_to_context(avdecoder_ctx, st->codecpar);

			if (avcodec_open2(avdecoder_ctx, avdecoder, NULL) < 0)
			{
				avcodec_free_context(&avdecoder_ctx);
				avdecoder_ctx = NULL;
				printf("not open decoder");
				goto end;
			}

			mstream->minfo.status = 1;
			mstream->minfo.video_h = st->codecpar->height;
			mstream->minfo.video_w = st->codecpar->width;
			strcpy(mstream->minfo.video_code_name, avcodocname);
			mstream->minfo.fps = st->codecpar->bit_rate / 1000.0;

			if (mstream->bk_status_change)
			{
				mstream->bk_status_change(mstream);
			}
		}
	}

	printf("start read [%s] avmedia_type_video=%d\n", in_filename,
		   avmedia_type_video);

	AVFrame *avframe = av_frame_alloc();

	uint64_t frame_video_count = 0;

	struct fps_calc _fps1, _fps2;
	uint64_t time_read_start = 0, time_read_end = 0, time_rest = 0;

	uint8_t *_yuvdata = NULL;
	int _yuvdata_len = 0;
	int _yuv_format = 0;

	memset(&_fps1, 0, sizeof(_fps1));
	memset(&_fps2, 0, sizeof(_fps2));

	unsigned int _out_not = 0;

	while (mstream->loop)
	{
		//AVStream *in_stream, *out_stream;
		time_read_start = av_gettime();
		//Get an AVPacket
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret == AVERROR_EOF)
		{
			//mp4 file
			int result = av_seek_frame(ifmt_ctx, -1, 0 * AV_TIME_BASE,
									   AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);
			printf("---------------  file eof, seek ret=%d start:%lu ---------------\n", result,
				   ifmt_ctx->start_time);
			continue;
		}
		if (ret < 0)
			break;
			/**
		pts: （int64_t）显示时间，结合AVStream->time_base转换成时间戳
		dts: （int64_t）解码时间，结合AVStream->time_base转换成时间戳
		size: （int）data的大小
		stream_index: （int）packet在stream的index位置
		flags: （int）标示，结合AV_PKT_FLAG使用，其中最低为1表示该数据是一个关键帧。
		#define AV_PKT_FLAG_KEY    0x0001 //关键帧
		#define AV_PKT_FLAG_CORRUPT 0x0002 //损坏的数据
		#define AV_PKT_FLAG_DISCARD  0x0004 /丢弃的数据
		side_data_elems: （int）边缘数据元数个数
		duration: （int64_t）数据的时长，以所属媒体流的时间基准为单位，未知则值为默认值0
		pos: （int64_t ）数据在流媒体中的位置，未知则值为默认值-1
		 */
#if 0
		printf("Send %8d , pkt.size=%d, pts:%d pkt.stream_index=%d\n",
				frame_index, pkt.size, pkt.pts, pkt.stream_index);
		{
			for (int i = 0; i < 10; i++)
			{
				printf("%02X ", pkt.data[i]);
			}
			//00 00 00 01 XX
			if ((0x0 == pkt.data[0] && 0x0 == pkt.data[1] && 0x0 == pkt.data[2]
							&& 0x01 == pkt.data[3] && (0x07 == (pkt.data[4] & 0x1f))))
			{
				printf("<I frame>");
			}
			printf("\n");
		}
#endif
		if (pkt.stream_index != avmedia_type_video)
		{
			av_packet_unref(&pkt);
			continue;
		}

		//需要将pkt发送到websocket
		if (mstream->bk_media_stream_frame_raw)
		{
			mstream->bk_media_stream_frame_raw(mstream, in_codec_id, pkt.data,
											   pkt.size);
		}

		//编码成yuv
		if ((ret = avcodec_send_packet(avdecoder_ctx, &pkt)))
		{
			printf("avcodec_send_packet error : 0x%x \n", ret);
			av_packet_unref(&pkt);
			continue;
		}

		//编码
		while (1) //encode->yuv
		{
			ret = avcodec_receive_frame(avdecoder_ctx, avframe);
			if (ret != 0)
			{
				/*0xfffffff5 is just no data*/
				if (ret != 0xfffffff5)
				{
					printf("avcodec_receive_frame error : 0x%x \n", ret);
				}
				break;
			}

			if (strncmp(in_filename, "rtsp://", strlen("rtsp://")) != 0)
			{
				//25fps= 40ms = 40*1000us
				// time_read_end = av_gettime();
				// time_rest = time_read_end - time_read_start;
				// if (time_rest < 40 * 1000)
				// 	usleep(40 * 1000 - time_rest);
			}

			{
				//计算帧率
				const AVPixFmtDescriptor *des = av_pix_fmt_desc_get(avframe->format);

				frame_video_count++;

				if (fps_calc_step(&_fps1))
				{ //实时网络传输帧率
					mstream->minfo.fps = _fps1.fps;
					printf("netfps:>> %.2f\n", _fps1.fps);
					//					printf("%lu -> %dx%d format:%s (%d)\n", av_gettime(),
					//							avframe->width, avframe->height,
					//							des ? des->name : "[unknow]", avframe->format);
				}

				_yuv_format = avframe->format;
				if (avframe->format == AV_PIX_FMT_DRM_PRIME)
					_yuv_format = AV_PIX_FMT_NV12;
				if (avframe->format == AV_PIX_FMT_YUYV422)
					_yuv_format = AV_PIX_FMT_YUV420P;

				int _imagelen = av_image_get_buffer_size(_yuv_format,
														 avframe->width, avframe->height, 1);

				if (_imagelen != _yuvdata_len || _yuvdata == NULL)
				{
					if (_yuvdata)
					{
						av_free(_yuvdata);
						_yuvdata = NULL;
					}
					_yuvdata_len = _imagelen;
					_yuvdata = (uint8_t *)av_malloc(_imagelen);
					printf("malloc yuv frame:[%s] size:%d\n",
						   des ? des->name : "[unknow]", _yuvdata_len);
				}

				if (_yuvdata) //达到16帧fps
				{
					_out_not++;
					if (_out_not % 2 == 0)
					{
						//break;
					}

					if (avframe->format == AV_PIX_FMT_DRM_PRIME)
					{
						AVFrame_nv12_to_buff(avframe, _yuvdata); //nv12的拷贝? 是否拷贝?
																 //AVFrame_nv12_to_yuv420_buff(avframe, _yuvdata);
					}

					if (avframe->format == AV_PIX_FMT_YUV420P || avframe->format == AV_PIX_FMT_YUVJ420P)
					{
						AVFrame_yuv420_to_buff(avframe, _yuvdata);
						_yuv_format = AV_PIX_FMT_YUV420P;
					}

					if (avframe->format == AV_PIX_FMT_YUYV422)
					{
						{
							AVFrame *srcFrame = avframe;

							AVFrame *dscFrame = av_frame_alloc();
							dscFrame->format = AV_PIX_FMT_YUV420P; //AV_PIX_FMT_NV12;
							dscFrame->width = srcFrame->width;
							dscFrame->height = srcFrame->height;
							av_image_fill_arrays(dscFrame->data, dscFrame->linesize,
												 _yuvdata, dscFrame->format,
												 dscFrame->width, dscFrame->height, 1);

							struct SwsContext *m_pSwsContext;
							int ret = 0;
							m_pSwsContext = sws_getContext(srcFrame->width, srcFrame->height,
														   (enum AVPixelFormat)srcFrame->format, dscFrame->width,
														   dscFrame->height, dscFrame->format,
														   SWS_BICUBIC, NULL, NULL,
														   NULL);
							ret = sws_scale(m_pSwsContext, srcFrame->data, srcFrame->linesize, 0,
											srcFrame->height, dscFrame->data, dscFrame->linesize);
							sws_freeContext(m_pSwsContext);

							AVFrame_yuv420_to_buff(dscFrame, _yuvdata);

							av_frame_free(&dscFrame); //nv12 to yuv420p
						}
					}

					fps_calc_step(&_fps2);

					if (mstream->bk_media_stream_frame_yuv)
						mstream->bk_media_stream_frame_yuv(mstream,
														   avframe->pts,
														   avframe->width,
														   avframe->height,
														   _yuv_format,
														   _yuvdata, _yuvdata_len);
				}
			}
		} //end encode->yuv

		//注4：av_free_packet()可被av_free_packet()替换
		//av_free_packet(&pkt);
		av_packet_unref(&pkt);
	}

	if (_yuvdata)
		av_free(_yuvdata);
	av_frame_free(&avframe);

//Write file trailer
end:

	if (opts)
		av_dict_free(&opts);

	if (ifmt_ctx)
		avformat_close_input(&ifmt_ctx);

	if (avdecoder_ctx)
		avcodec_free_context(&avdecoder_ctx);

	/* close output */
	if (ret < 0 && ret != AVERROR_EOF)
	{
		printf("Error occurred.\n");
		return -1;
	}
	return 0;
}
