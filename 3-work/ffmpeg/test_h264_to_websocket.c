#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <syslog.h>

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libavutil/avutil.h"
#include "libavutil/pixdesc.h"
#include "libavutil/time.h"

#include "libwebsockets.h"

#include "list.h"
#include "lqueue.h"

void websockets_send_h264(void *data, int size);

//nal类型
enum nal_unit_type_e {
	NAL_UNKNOWN = 0,
	NAL_SLICE = 1,
	NAL_SLICE_DPA = 2,
	NAL_SLICE_DPB = 3,
	NAL_SLICE_DPC = 4,
	NAL_SLICE_IDR = 5, /* ref_idc != 0 */
	NAL_SEI = 6, /* ref_idc == 0 */
	NAL_SPS = 7,
	NAL_PPS = 8
/* ref_idc == 0 for 6,9,10,11,12 */
};

//帧类型
enum Frametype_e {
	FRAME_I = 15, FRAME_P = 16, FRAME_B = 17
};

int media_uri_push(const char *uri, const char *tourl) {
	AVFormatContext *ifmt_ctx = NULL;
	//AVFormatContext *ofmt_ctx = NULL;
	AVOutputFormat *ofmt = NULL;

	AVPacket pkt;
	const char *in_filename, *out_filename;
	int ret, i;
	int videoindex = -1;
	int frame_index = 0;
	int64_t start_time = 0;

	in_filename = uri;
	out_filename = tourl;

	printf("%s->%s\n", uri, tourl);
	//注1：av_register_all()这个方法在FFMPEG 4.0以后将不再建议使用，而且是非必需的，因此直接注释掉即可
	//av_register_all();
	av_log_set_level(AV_LOG_DEBUG);
	//Network
	avformat_network_init();

	AVDictionary *opts = NULL;
	av_dict_set(&opts, "buffer_size", "1024000", 0);
	av_dict_set(&opts, "max_delay", "500000", 0);
	av_dict_set(&opts, "rtsp_transport", "tcp", 0); //设置tcp or udp，默认一般优先tcp再尝试udp
	av_dict_set(&opts, "stimeout", "3000000", 0); //设置超时3秒

	//Input
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, &opts)) < 0) {
		printf("Could not open input file.");
		goto end;
	}
	printf("open success\n");
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		printf("Failed to retrieve input stream information");
		goto end;
	}

	videoindex = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1,
	NULL, 0);
	if (videoindex < 0) {
		fprintf(stderr, "err not find media type video\n");
		goto end;
	}

	av_dump_format(ifmt_ctx, 0, ifmt_ctx->filename, 0);
	{
		AVStream *st = ifmt_ctx->streams[videoindex];
		if (st) {
			const char *pix_fmt_name =
					st->codecpar->format == AV_PIX_FMT_NONE ?
							"none" :
							av_get_pix_fmt_name(
									(enum AVPixelFormat) st->codecpar->format);
			const char *avcodocname = avcodec_get_name(st->codecpar->codec_id);
			const char *profilestring = avcodec_profile_name(
					st->codecpar->codec_id, st->codecpar->profile);
			//char * codec_fourcc =  av_fourcc_make_string(videostream->codecpar->codec_tag);

			printf("编码方式 : %s\n    Codec Profile : %s\n", avcodocname,
					profilestring);
			printf("显示编码格式: %s \n", pix_fmt_name);
			printf("像素尺寸:%dx%d \n", st->codecpar->width, st->codecpar->height);
			printf("最低帧率:%f fps 平均帧率:%f fps\n", av_q2d(st->r_frame_rate),
					av_q2d(st->avg_frame_rate));
			printf("视频流比特率 :%fkbps\n", st->codecpar->bit_rate / 1000.0);
			printf("码流:");
		}
	}
	printf("start read [%s] videoindex=%d\n", uri, videoindex);

	start_time = av_gettime();
	while (1) {
		AVStream *in_stream, *out_stream;
		//Get an AVPacket
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret == AVERROR_EOF) {
			//mp4 file
			int result = av_seek_frame(ifmt_ctx, -1, 0 * AV_TIME_BASE,
			AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);
//			printf(" >>>>>>>>>>>>>>>>>>>>> file eof, seek ret=%d start:%d\n",
//					result, ifmt_ctx->start_time);
			continue;
		}
		if (ret < 0)
			break;

#if 0
		printf("Send %8d , pkt.size=%d, pts:%d pkt.stream_index=%d\n",
				frame_index, pkt.size, pkt.pts, pkt.stream_index);
		{
			for (int i = 0; i < 10; i++) {
				printf("%02X ", pkt.data[i]);
			}
			//00 00 00 01 XX
			if ((0x0 == pkt.data[0] && 0x0 == pkt.data[1] && 0x0 == pkt.data[2]
							&& 0x01 == pkt.data[3] && (0x07 == (pkt.data[4] & 0x1f)))) {
				printf("<I frame>");
			}
			printf("\n");
		}
#endif
		if (pkt.stream_index == videoindex) {

			frame_index++;
			//usleep(1000 * 40); //25fps 1000/25=40ms
			//需要将pkt发送到websocket
			websockets_send_h264(pkt.data, pkt.size);
		}

		//注4：av_free_packet()可被av_free_packet()替换
		//av_free_packet(&pkt);
		av_packet_unref(&pkt);
	}

	//Write file trailer
	end: avformat_close_input(&ifmt_ctx);
	/* close output */
	if (ret < 0 && ret != AVERROR_EOF) {
		printf("Error occurred.\n");
		return -1;
	}
	return 0;
}

struct targs {
	char uri[300];
};

static void *ThreadPush(void *args) {
	struct targs *a = (struct targs *) args;
	media_uri_push(a->uri, "");
	return NULL;
}

LIST_HEAD(list_ws_session);
pthread_mutex_t list_ws_session_lock = PTHREAD_MUTEX_INITIALIZER;

struct ws_session {
	struct list_head list;
	struct lws *wsi;
	int id;
	char path[100];
	struct lqueue queue;
};

struct h264video {
	struct lqueue_node lnode;
	uint8_t *data;
	int pos;
	int size;
};

void websockets_send_h264(void *data, int size) {
	struct h264video *v = NULL;
	struct list_head *pos, *n;
	struct ws_session *session = NULL;

	//printf("size=%d\n", size);
	pthread_mutex_lock(&list_ws_session_lock);
	list_for_each_entry(session, &list_ws_session,list)
	{
		v = (struct h264video*) malloc(sizeof(struct h264video));
		v->data = malloc(size + LWS_PRE);
		memcpy(v->data + LWS_PRE, data, size);
		v->size = size;

		lqueue_append_tail(&session->queue, &v->lnode);
		if (lqueue_len(&session->queue) > 8) {

			v = lqueue_pop(&session->queue);
			if (v) {
				printf("pop...\n");
				free(v->data);
				free(v);
			}
		}
		//size=15971
	}
	pthread_mutex_unlock(&list_ws_session_lock);
}

static int _webSockServiceCallback(struct lws *wsi,
		enum lws_callback_reasons reason, void *user, void *in, size_t len) {
	struct ws_session *session = (struct ws_session *) user;

	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT: /* per vhost */
		printf("LWS_CALLBACK_PROTOCOL_INIT \"version\":\"%s\","
				" \"hostname\":\"%s\" \n", lws_get_library_version(),
				lws_canonical_hostname(lws_get_context(wsi)));
		break;
	case LWS_CALLBACK_PROTOCOL_DESTROY:
		break;
	case LWS_CALLBACK_WSI_CREATE: {
		printf("create:%s\n", in);
	}
		break;
	case LWS_CALLBACK_ESTABLISHED: {

		printf("new:fd=%d, %s\n", lws_get_socket_fd(wsi), in);
		char name[100], rip[50];
		lws_get_peer_addresses(wsi, lws_get_socket_fd(wsi), name, sizeof(name),
				rip, sizeof(rip));
		printf("name=%s, rip=%s\n", name, rip);

		char buff[100];
		memset(buff, 0, sizeof(buff));
		lws_hdr_copy(wsi, buff, sizeof(buff), WSI_TOKEN_HTTP_USER_AGENT);
		printf("WSI_TOKEN_HTTP_USER_AGENT:%s\n", buff);

		memset(buff, 0, sizeof(buff));
		lws_hdr_copy(wsi, buff, sizeof(buff), WSI_TOKEN_GET_URI);
		printf("WSI_TOKEN_GET_URI:%s\n", buff);

		memset(session, 0, sizeof(struct ws_session));
		session->wsi = wsi;
		session->id=lws_get_socket_fd(wsi);
		lqueue_init(&session->queue);

		pthread_mutex_lock(&list_ws_session_lock);
		list_add(&session->list, &list_ws_session);
		pthread_mutex_unlock(&list_ws_session_lock);
	}
		break;
	case LWS_CALLBACK_SERVER_WRITEABLE: {
		int ret;

		struct h264video *v = (struct h264video*) lqueue_dequeue(
				&session->queue);
		if (v) {
			ret = lws_write(wsi, v->data + LWS_PRE, v->size, LWS_WRITE_BINARY);
			//printf("write :%d v->size=%d, queue=%d\n", ret, v->size,
			//		lqueue_len(&session->queue));
			if (ret != v->size) {
				printf("lws_write err \n");
			}
			free(v->data);
			free(v);
		}
		break;
	}
	case LWS_CALLBACK_RECEIVE:
		printf("recv\n");
		break;
	case LWS_CALLBACK_CLOSED:
		printf("close:\n");
		break;
	case LWS_CALLBACK_WSI_DESTROY:
		printf("destroy:\n");
		if (!session)
			break;
		pthread_mutex_lock(&list_ws_session_lock);
		list_del(&session->list);
		pthread_mutex_unlock(&list_ws_session_lock);
		break;
	}
	return 0;
}

static struct lws_protocols protocols[] = {  ///
		{ .name = "h264move", //////////
				.callback = _webSockServiceCallback, ///////////////
				.per_session_data_size = sizeof(struct ws_session), ///////////////
				.rx_buffer_size = 1024 * 1024, //////////////////
				},/////////////
				{ NULL, NULL, 0, 0 } /* end */
		};

static void *_ThreadServer(void *args) {
	setlogmask(LOG_UPTO(LOG_DEBUG));
	openlog("lwsts", LOG_PID | LOG_PERROR, LOG_DAEMON);

	lws_set_log_level(7, lwsl_emit_syslog);

	//struct lws_client_connect_info

	struct lws_context_creation_info info;
	//struct lws_protocols protocol;
	struct lws_context *context = NULL;
	memset(&info, 0, sizeof info);

	info.port = 8082;
	info.max_http_header_pool = 16;
	info.iface = NULL;
	info.protocols = protocols;
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	info.extensions = NULL; 	//lws_get_internal_extensions();
	info.gid = -1;
	info.uid = -1;
	info.options = 0;

	info.options |= LWS_SERVER_OPTION_VALIDATE_UTF8;

	context = lws_create_context(&info);

	int ret = 0;
	struct list_head *pos, *n;
	struct ws_session *session = NULL;
	while (ret >= 0) {
		pthread_mutex_lock(&list_ws_session_lock);
		list_for_each_entry(session, &list_ws_session,list)
		{
			if (lqueue_len(&session->queue) > 0) {
				lws_callback_on_writable(session->wsi);
			}
		}
		pthread_mutex_unlock(&list_ws_session_lock);
		ret = lws_service(context, 200);
	}
	lws_context_destroy(context);

	exit(1);
	return NULL;
}

int main(int argc, char* argv[]) {
	char uri[300] = "";
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

	if (strlen(uri) == 0) {
		char *url="rtsp://admin:admin@192.168.0.1:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif";
		url="rtsp://admin:admin123@192.168.0.1:554/Streaming/Channels/101?transportmode=unicast&profile=Profile_1";
		url="rtsp://admin:admin@192.168.0.1:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif";
		strcpy(uri,url);
	}

	if (strlen(tourl) == 0)
		strcpy(tourl, "rtmp://127.0.0.1:1936/live/1");

	struct targs arg;
	strcpy(arg.uri, uri);

	pthread_t pt;
	pthread_create(&pt, NULL, ThreadPush, &arg);

	_ThreadServer(NULL);
	return 0;
}

