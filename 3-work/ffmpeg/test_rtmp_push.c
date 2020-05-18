#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"

int media_uri_push(const char *uri, const char *tourl) {
	AVOutputFormat *ofmt = NULL;
	//Input AVFormatContext and Output AVFormatContext
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	const char *in_filename, *out_filename;
	int ret, i;
	int videoindex = -1;
	int frame_index = 0;
	int64_t start_time = 0;

	in_filename = uri;
	out_filename = tourl;

	printf("%s->%s\n", uri, tourl);
	//	in_filename = "rtsp://admin:123456@192.168.0.188:554/profile1";
//	in_filename = "/opt/nfs/hg_base.h264";
//	in_filename = "test.h264";
//	out_filename = "rtmp://127.0.0.1:1936/live/1";	//输出 URL（Output URL）[RTMP]
	//out_filename ="rtmp://192.168.0.59:1936/live/1";

	//注1：av_register_all()这个方法在FFMPEG 4.0以后将不再建议使用，而且是非必需的，因此直接注释掉即可
	//av_register_all();

	//Network
	avformat_network_init();

	AVDictionary *opts = NULL;
	av_dict_set(&opts, "rtsp_transport", "tcp", 0); //设置tcp or udp，默认一般优先tcp再尝试udp
	av_dict_set(&opts, "stimeout", "3000000", 0); //设置超时3秒

	//Input
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, &opts)) < 0) {
		printf("Could not open input file.");
		goto end;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		printf("Failed to retrieve input stream information");
		goto end;
	}

	//	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
	//		if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
	//			videoindex = i;
	//			break;
	//		}
	//	}
	videoindex = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1,
	NULL, 0);
	if (videoindex < 0) {
		fprintf(stderr, "err not find media type video\n");
		goto end;
	}
	//av_dump_format(ifmt_ctx, 0, in_filename, 0);

	//Output

	avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", out_filename); //RTMP
	//avformat_alloc_output_context2(&ofmt_ctx, NULL, "mpegts", out_filename);//UDP

	if (!ofmt_ctx) {
		printf("Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt = ofmt_ctx->oformat;
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		AVStream *in_stream = ifmt_ctx->streams[i];

		//注2：因为codecpar没有codec这个成员变量，所以不能简单地将codec替换成codecpar，可以通过avcodec_find_decoder()方法获取
		//AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
		AVCodec *avcodec = avcodec_find_decoder(in_stream->codecpar->codec_id);
		AVStream *out_stream = avformat_new_stream(ofmt_ctx, avcodec);
		if (!out_stream) {
			printf("Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}
		//Copy the settings of AVCodecContext
		//注3：avcodec_copy_context()方法已被avcodec_parameters_to_context()和avcodec_parameters_from_context()所替代，
		//ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
		AVCodecContext *codec_ctx = avcodec_alloc_context3(avcodec);
		ret = avcodec_parameters_to_context(codec_ctx, in_stream->codecpar);
		if (ret < 0) {
			printf(
					"Failed to copy context from input to output stream codec context\n");
			goto end;
		}

		codec_ctx->codec_tag = 0;
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		ret = avcodec_parameters_from_context(out_stream->codecpar, codec_ctx);
	}
	//Dump Format------------------
	//av_dump_format(ofmt_ctx, 0, out_filename, 1);

	//Open output URL
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf("Could not open output URL '%s'", out_filename);
			goto end;
		}
	}
	//Write file header
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		printf("Error occurred when opening output URL\n");
		goto end;
	}

	start_time = av_gettime();
	while (1) {
		AVStream *in_stream, *out_stream;
		//Get an AVPacket
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
			break;
		//FIX：No PTS (Example: Raw H.264)
		//Simple Write PTS
		if (pkt.pts == AV_NOPTS_VALUE) {
			//Write PTS
			AVRational time_base1 = ifmt_ctx->streams[videoindex]->time_base;
			//Duration between 2 frames (us)
			int64_t calc_duration = (double) AV_TIME_BASE
					/ av_q2d(ifmt_ctx->streams[videoindex]->r_frame_rate);
			//Parameters
			pkt.pts = (double) (frame_index * calc_duration)
					/ (double) (av_q2d(time_base1) * AV_TIME_BASE);
			pkt.dts = pkt.pts;
			pkt.duration = (double) calc_duration
					/ (double) (av_q2d(time_base1) * AV_TIME_BASE);
		}
		//Important:Delay
		if (pkt.stream_index == videoindex) {
			AVRational time_base = ifmt_ctx->streams[videoindex]->time_base;
			AVRational time_base_q = { 1, AV_TIME_BASE };
			int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
			int64_t now_time = av_gettime() - start_time;
			if (pts_time > now_time)
				av_usleep(pts_time - now_time);

		}

		in_stream = ifmt_ctx->streams[pkt.stream_index];
		out_stream = ofmt_ctx->streams[pkt.stream_index];
		/* copy packet */
		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base,
				out_stream->time_base,
				(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base,
				out_stream->time_base,
				(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base,
				out_stream->time_base);
		pkt.pos = -1;
		//Print to Screen
		if (pkt.stream_index == videoindex) {
			//printf("Send %8d video frames to output URL\n", frame_index);
			frame_index++;
		}
		//ret = av_write_frame(ofmt_ctx, &pkt);
		//printf("size=%d \n", pkt.size);
		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);

		if (ret < 0) {
			fprintf(stderr, "Error muxing packet, %s\n", av_err2str(ret));
			//break;
		}
		//注4：av_free_packet()可被av_free_packet()替换
		//av_free_packet(&pkt);
		av_packet_unref(&pkt);
	}
	//Write file trailer
	av_write_trailer(ofmt_ctx);
	end: avformat_close_input(&ifmt_ctx);
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF) {
		printf("Error occurred.\n");
		return -1;
	}
	return 0;
}

struct thread_arg {
	int id;
	char url_in[200];
	char url_out[200];
};

static void *thread_push(void *arg) {
	pthread_detach(pthread_self());
	struct thread_arg *targ = (struct thread_arg *) arg;
	char name[16];
	sprintf(name, "Push:%d", targ->id);
	prctl(PR_SET_NAME, name);

	media_uri_push(targ->url_in, targ->url_out);
	return NULL;
}

int main(int argc, char* argv[]) {
	char uri[300] = "test.h264";
	char tourl[300];
	int thread_count;
	int ch;

	while ((ch = getopt(argc, argv, "i:o:ht:")) != -1) {
		switch (ch) {
		case 'i':
			strcpy(uri, optarg);
			break;
		case 'o':
			strcpy(tourl, optarg);
			break;
		case 't':
			thread_count = atoi(optarg);
			break;
		case 'h':
			printf("-i input file url\n");
			printf("-o out file url\n");
			printf("-t thread count\n");

			return 0;
			break;
		default:
			break;
		}
	}

	if (strlen(uri) == 0)
		strcpy(uri, "rtsp://admin:123456@192.168.0.188:554/profile1");

	if (strlen(tourl) == 0)
		strcpy(tourl, "rtmp://127.0.0.1:1936/live/1");

	int i;
	pthread_t pt;
	for (i = 0; i < thread_count; i++) {
		struct thread_arg *arg = (struct thread_arg*) calloc(
				sizeof(struct thread_arg), 1);

		strcpy(arg->url_in, uri);
		sprintf(arg->url_out, "%s/%d", tourl, i);

		arg->id = i;
		pthread_create(&pt, NULL, thread_push, arg);
	}
	pause();
}

