/*
 * WebVideo.c
 *
 *  Created on: 2019年9月17日
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
 * 提供http api服务:
 *
 * 1> 视频服务
 * http://192.168.0.189:8082/video/chid=<通道ID>
 * 2> 区域信息获取
 * http://192.168.0.189:8082/rect/chid=<通道ID>
 * 3> 最新人脸信息
 * http://192.168.0.189:8082/face/info
 * 4> 人脸照片
 * http://192.168.0.189:8082/image/child/<通道ID>/<照片名称, 1234.jpg>
 *
 * 请注意编码格式
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#include "AIInputSource.h"
#include "list.h"
#include "Tool_ffmpeg.h"
#include "lqueue.h"

struct FaceImageCache
{
	int chID;
	void *jdata;
	int jsize;
	int dataSize; //总大小
	uint64_t jid; //jpg id
	FACE_DETECT_PIC_DATA_S picData;
};

struct FaceAttribCache
{
	int front; //start item
	int rear; //next item
	struct FaceImageCache picAttrib[10]; //缓存最新10个缓存
};

static struct FaceAttribCache _FaceAttribCache;
static pthread_mutex_t _mutexFaceAttrib = PTHREAD_MUTEX_INITIALIZER;
static volatile uint64_t JPGUUID = 0;

void FaceAttribCache_Add(int chID, const FACE_DETECT_PIC_DATA_S *data,
		void *jdata, int jsize)
{
	printf("cache: %lld-%d.jpg \n", data->timestamp,
			data->faceDetectResult.indexForOneFrame);

	pthread_mutex_lock(&_mutexFaceAttrib);
	if ((_FaceAttribCache.rear + 1) % 10 == _FaceAttribCache.front) //队列满
		_FaceAttribCache.front = (_FaceAttribCache.front + 1) % 10; //出队

	_FaceAttribCache.picAttrib[_FaceAttribCache.rear].chID = chID;
	memcpy(&_FaceAttribCache.picAttrib[_FaceAttribCache.rear].picData, data,
			sizeof(FACE_DETECT_PIC_DATA_S));

	if (jsize > _FaceAttribCache.picAttrib[_FaceAttribCache.rear].dataSize)
	{
		if (_FaceAttribCache.picAttrib[_FaceAttribCache.rear].jdata)
			free(_FaceAttribCache.picAttrib[_FaceAttribCache.rear].jdata);
		_FaceAttribCache.picAttrib[_FaceAttribCache.rear].jdata = malloc(jsize);
		_FaceAttribCache.picAttrib[_FaceAttribCache.rear].dataSize = jsize;
	}
	memcpy(_FaceAttribCache.picAttrib[_FaceAttribCache.rear].jdata, jdata,
			jsize);
	_FaceAttribCache.picAttrib[_FaceAttribCache.rear].jsize = jsize;

	_FaceAttribCache.picAttrib[_FaceAttribCache.rear].jid = JPGUUID++;

	_FaceAttribCache.rear = (_FaceAttribCache.rear + 1) % 10;
	pthread_mutex_unlock(&_mutexFaceAttrib);
}

void FaceAttribCache_Foreach(
		void (*_func)(struct FaceImageCache *imgCache, void *context),
		void *context)
{
	int length = 0;
	//倒序
	pthread_mutex_lock(&_mutexFaceAttrib);
	length = (_FaceAttribCache.rear - _FaceAttribCache.front + 10) % 10;
	for (int i = length; i > 0; i--)
	{
		int index = (_FaceAttribCache.front + i - 1) % 10;

		_func(&_FaceAttribCache.picAttrib[index], context);
	}
	pthread_mutex_unlock(&_mutexFaceAttrib);
}

struct CCache
{
	struct lqueue_node node;
	long long id;
	void *data;
	int size;
	int sumsize;
};

struct CCache *CCache_Calloc()
{
	struct CCache *cache = (struct CCache*) calloc(1, sizeof(struct CCache));
	return cache;
}

void CCache_free(struct CCache *cache)
{
	if (cache->data)
		free(cache->data);
	free(cache);
}

void CCache_Set(struct CCache *cache, void *data, int size)
{
	if (size > cache->sumsize)
	{
		if (cache->data)
			free(cache->data);
		cache->data = malloc(size);
		cache->sumsize = size;
	}
	memcpy(cache->data, data, size);
	cache->size = size;
}

static inline int socket_nonblock(int fd, int flag)
{
	int opt = fcntl(fd, F_GETFL, 0);
	if (opt == -1)
	{
		return -1;
	}
	if (flag)
		opt |= O_NONBLOCK;
	else
		opt &= ~O_NONBLOCK;

	if (fcntl(fd, F_SETFL, opt) == -1)
	{
		return -1;
	}
	return 0;
}

enum web_mode
{
	web_mode_none, //未定义
	web_mode_video_jpeg_stream, //视频流
	web_mode_rect, //区域
};

static struct list_head clist;
static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;

struct WebRectInfo
{ //web区域
	struct WRect
	{
		int id;
		int x;
		int y;
		int w;
		int h;
	} allRects[30];
	int w;
	int h;
	int rectCount; //区域数量
};

struct WebClient
{
	struct list_head list;
	int epfd;
	struct sockaddr_in addr;
	int fd;
	uint8_t buff[1024];
	int buff_rsize;
	char url[100];
	int work_mode; //工作模式: 输出流模式
	uint64_t frame_count; //
	uint64_t first_time; //开始时间
	uint64_t last_time;
	int fps;

	struct jpg_stream_cache
	{
		pthread_mutex_t mutex;
		void *data;
		int dsize;
		int chID;
		int changeflag;
	} stream_cache;

	struct WebRectInfo webRectInfo;
};

void WebClient_free(struct WebClient *webClient)
{
	pthread_mutex_destroy(&webClient->stream_cache.mutex);
	if (webClient->stream_cache.data)
		free(webClient->stream_cache.data);
	free(webClient);
}

struct WebClient *WebClient_calloc()
{
	struct WebClient *webClient = (struct WebClient*) calloc(1,
			sizeof(struct WebClient));
	if (webClient)
	{
		pthread_mutex_init(&webClient->stream_cache.mutex, NULL);
	}
	return webClient;
}

int WebClients_Count()
{
	int size = 0;
	struct list_head *p = NULL;

	pthread_mutex_lock(&_mutex);
	list_for_each(p, &clist)
		size++;
	pthread_mutex_unlock(&_mutex);
	return size;
}

void WebClients_Add(struct WebClient *client)
{
	pthread_mutex_lock(&_mutex);
	list_add_tail(&client->list, &clist);
	pthread_mutex_unlock(&_mutex);
}

static const char *bad_request_response = "HTTP/1.0 400 Bad Request\r\n"
		"Content-type: text/html\r\n\r\n"
		"<html>\n"
		"<body>\n"
		"<h1>Bad Request</h1>\n"
		"<p>The server did not understand your request.</p>\n"
		"</body>\n"
		"</html>\n";

static ssize_t write_nonblock(int fd, const void *buf, size_t size)
{
	ssize_t nwrite = -1;
	struct timeval tm;
	fd_set fds;

	tm.tv_sec = 1; /* Timeout in seconds */
	tm.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	if (select(fd + 1, NULL, &fds, NULL, &tm) > 0)
	{
		if (FD_ISSET(fd, &fds))
		{
			if ((nwrite = write(fd, buf, size)) < 0)
			{
				if (errno != EWOULDBLOCK)
					return -1;
			}
		}
	}

	return nwrite;
}

int WebClientRecv(struct WebClient *webClient)
{
	int rlen;

	if (webClient->buff_rsize >= 1024 - 1)
		return -1;

	rlen = recv(webClient->fd, webClient->buff,
			1024 - webClient->buff_rsize - 1, 0);
	if (rlen == 0)
		return -1;

	if (rlen == -1)
	{
		if (errno == EAGAIN || EINTR == errno)
			return 0;
		return -1;
	}
	webClient->buff_rsize += rlen;
	return 0;
}

static const char http_boundary_header[] =
		"HTTP/1.0 200 OK\r\n"
				"Server: LXVideo\r\n"
				"Connection: close\r\n"
				"Max-Age: 0\r\n"
				"Expires: 0\r\n"
				"Cache-Control: no-cache, private\r\n"
				"Pragma: no-cache\r\n"
				"Content-Type: multipart/x-mixed-replace;boundary=--BoundaryString\r\n\r\n";

struct AIInputSource *AIInputSource_GetItem(int index);

int WebClientSetRectInfo(int chid, int bkw, int bkh, int rectCount,
		int rectIndex, int id, int x, int y, int w, int h)
{
	struct WebClient *client = NULL;
	struct list_head *p = NULL;
	int ret;

	pthread_mutex_lock(&_mutex);
	list_for_each(p, &clist)
	{
		client = list_entry(p, struct WebClient, list);
		if (client->stream_cache.chID == chid)
		{
			client->webRectInfo.w = bkw;
			client->webRectInfo.h = bkh;
			client->webRectInfo.rectCount = rectCount;
			client->webRectInfo.allRects[rectIndex].id = id;
			client->webRectInfo.allRects[rectIndex].x = x;
			client->webRectInfo.allRects[rectIndex].y = y;
			client->webRectInfo.allRects[rectIndex].w = w;
			client->webRectInfo.allRects[rectIndex].h = h;
		}
	}
	pthread_mutex_unlock(&_mutex);
	return 0;
}

int WebClientAppendFrame(int chid, struct AIInputPicItem *aiInputPicItem)
{
	int size = sizeof(struct AIInputPicItem) + aiInputPicItem->iDatalen;
	struct WebClient *client = NULL;
	struct list_head *p = NULL;
	int ret;

	uint64_t osclockms[2];
	osclockms[0] = ToolOSClockMonotonicMs();

	pthread_mutex_lock(&_mutex);
	list_for_each(p, &clist)
	{
		client = list_entry(p, struct WebClient, list);
		printf("fd=%d, mode=%d, chid=%d\n", client->fd, client->work_mode,
				client->stream_cache.chID);
		if (client->work_mode == web_mode_video_jpeg_stream)
		{
			if (client->stream_cache.chID == chid)
			{
				ret = pthread_mutex_trylock(&client->stream_cache.mutex);
				if (ret == 0)
				{
					printf("add img fd=%d\n", client->fd);
					if (client->stream_cache.dsize < size)
					{
						if (client->stream_cache.data)
							free(client->stream_cache.data);
						client->stream_cache.data = malloc(size);
					}
					if (client->stream_cache.data)
					{
						memcpy(client->stream_cache.data, aiInputPicItem, size);
						client->stream_cache.changeflag = 1;
					}
					pthread_mutex_unlock(&client->stream_cache.mutex);
				}
			}
		}
	}
	pthread_mutex_unlock(&_mutex);
	osclockms[1] = ToolOSClockMonotonicMs();
	printf(" ---- append frame ms : %ld ms\n", osclockms[1] - osclockms[0]);
	return 0;
}

int WebClientWriteStreamVideo(struct WebClient *webClient)
{
	const char jpeghead[] = "--BoundaryString\r\n"
			"Content-type: image/jpeg\r\n"
			"Content-Length:                  ";
	int ret = 0;

	struct AIInputSource *aiInputSoruce = AIInputSource_GetItem(0);

	if (aiInputSoruce)
	{
		struct AIInputPicItem *picItem = NULL;
		AVFrame *frameYUVJ420p = NULL;

		pthread_mutex_lock(&webClient->stream_cache.mutex);
		picItem = (struct AIInputPicItem *) webClient->stream_cache.data;
		if (picItem && webClient->stream_cache.changeflag)
		{
			frameYUVJ420p = ffmpeg_av_frame_alloc(AV_PIX_FMT_YUVJ420P, 640,
					320);
			{
				AVFrame *frameNV12 = av_frame_alloc();
				frameNV12->format = AV_PIX_FMT_NV12;
				frameNV12->width = picItem->width;
				frameNV12->height = picItem->height;
				av_image_fill_arrays(frameNV12->data, frameNV12->linesize,
						picItem->picDataArray, frameNV12->format,
						frameNV12->width, frameNV12->height, 1);
				ffmpeg_SwsScale(frameNV12, frameYUVJ420p);
				av_frame_free(&frameNV12); //nv12 to yuv420p
			}
			webClient->stream_cache.changeflag = 0;
		}
		pthread_mutex_unlock(&webClient->stream_cache.mutex);

		if (frameYUVJ420p)
		{
			AVPacket avpkt;
			av_init_packet(&avpkt);
			avpkt.data = NULL;
			avpkt.size = 0;

			ffmpeg_YUVJ420PAVFrameToJpgAVPacket(frameYUVJ420p, &avpkt);
			av_frame_free(&frameYUVJ420p);

			char *tbuf = strrchr(jpeghead, ':');
			sprintf(tbuf + 1, " %d\r\n\r\n", avpkt.size + 2);

			ret = send(webClient->fd, jpeghead, strlen(jpeghead), 0);
			ret = send(webClient->fd, avpkt.data, avpkt.size, 0);
			ret = send(webClient->fd, "\r\n", 2, 0);

			av_packet_unref(&avpkt);

			if (webClient->first_time == 0 || webClient->frame_count == 0)
				webClient->first_time = ToolOSClockMonotonicMs();

			webClient->frame_count++;
			webClient->last_time = ToolOSClockMonotonicMs();
			{
				uint64_t alltime = webClient->last_time - webClient->first_time;
				webClient->fps = webClient->frame_count / (alltime * 0.001f);
				printf("fps:%d \n", webClient->fps);
			}
			if (ret == 0)
				return -1;
		}
	}
	return 0;
}

int ToolOpenDir(const char *path,
		void (*handle)(const char *path, struct dirent *item, void *context),
		void *context)
{
	DIR *dir = opendir(path);
	if (dir)
	{
		struct dirent *pdp = NULL;
		while ((pdp = readdir(dir)) != NULL)
		{
			if (strcmp(pdp->d_name, ".") == 0)
				continue;
			if (strcmp(pdp->d_name, "..") == 0)
				continue;

			if (handle)
				handle(path, pdp, context);
		}
		closedir(dir);
		return 0;
	}
	return -1;
}

void _faceAttribInfoBackFunc(struct FaceImageCache *imgCache, void *context)
{
	FACE_DETECT_PIC_DATA_S *facePickupData = &imgCache->picData;
	cJSON *param = (cJSON*) context;
	cJSON *infoObj = cJSON_CreateObject();

	cJSON_AddStringToObjectEx(infoObj, "imgName", "%lld.jpg", imgCache->jid);
	cJSON_AddNumberToObject(infoObj, "chID", imgCache->chID);
	cJSON_AddNumberToObject(infoObj, "age",
			facePickupData->faceDetectResult.age); //年龄

	if (1 == facePickupData->faceDetectResult.gender) //性别
		cJSON_AddStringToObject(infoObj, "sex", "male");
	else if (0 == facePickupData->faceDetectResult.gender)
		cJSON_AddStringToObject(infoObj, "sex", "female");
	else
		cJSON_AddStringToObject(infoObj, "sex", "unknow");

	if (1 == facePickupData->faceDetectResult.glasses) //戴眼镜
		cJSON_AddStringToObject(infoObj, "glasses", "true");
	else
		cJSON_AddStringToObject(infoObj, "glasses", "false");

	if (1 == facePickupData->faceDetectResult.smile) //微笑
		cJSON_AddStringToObject(infoObj, "smile", "true");
	else
		cJSON_AddStringToObject(infoObj, "smile", "false");

	if (1 == facePickupData->faceDetectResult.beard) //胡须
		cJSON_AddStringToObject(infoObj, "beard", "true");
	else
		cJSON_AddStringToObject(infoObj, "beard", "false");

	cJSON_AddStringToObjectEx(infoObj, "time", "%d/%d/%d %d:%d:%d",
			facePickupData->tm.year + 2000, facePickupData->tm.mon,
			facePickupData->tm.day, facePickupData->tm.hour,
			facePickupData->tm.min, facePickupData->tm.sec);

	{
		cJSON *picInfoObj = cJSON_CreateObject();

		cJSON_AddStringToObjectEx(picInfoObj, "picNo", "%lld-%d",
				(long long int) facePickupData->timestamp,
				facePickupData->faceDetectResult.indexForOneFrame); //抠图图片序列号(序列号：xxxxxx-yyy，其中xxxxxx为抓图图片序列号，yyy为自增的人脸序号)
		cJSON_AddStringToObject(picInfoObj, "picType", "jpg");

		cJSON_AddStringToObjectEx(picInfoObj, "picResolution", "%dX%d",
				facePickupData->faceDetectResult.w,
				facePickupData->faceDetectResult.h);

		cJSON_AddNumberToObject(picInfoObj, "picSize",
				facePickupData->picDataSize);
		cJSON_AddNumberToObject(picInfoObj, "rectX",
				facePickupData->faceDetectResult.x);
		cJSON_AddNumberToObject(picInfoObj, "rectY",
				facePickupData->faceDetectResult.y);
		cJSON_AddNumberToObject(picInfoObj, "rectW",
				facePickupData->faceDetectResult.w);
		cJSON_AddNumberToObject(picInfoObj, "rectH",
				facePickupData->faceDetectResult.h);

		cJSON_AddItemToObject(infoObj, "picInfo", picInfoObj);
	}

	{
		char *separatorAddr = strchr(
				facePickupData->faceDetectResult.faceMatchData.matchPicName,
				'#');
		char *fileNameEndAddr = strrchr(
				facePickupData->faceDetectResult.faceMatchData.matchPicName,
				'.');

		char sampleLibIDStr[200];
		char samplePicIDStr[200];

		if (separatorAddr && fileNameEndAddr
				&& facePickupData->faceDetectResult.faceMatchData.similarityDegree
						> 0)
		{
			cJSON_AddNumberToObject(infoObj, "similar",
					facePickupData->faceDetectResult.faceMatchData.similarityDegree);

			memcpy(sampleLibIDStr,
					facePickupData->faceDetectResult.faceMatchData.matchPicName,
					separatorAddr
							- facePickupData->faceDetectResult.faceMatchData.matchPicName);
			sampleLibIDStr[separatorAddr
					- facePickupData->faceDetectResult.faceMatchData.matchPicName] =
					0;

			if (fileNameEndAddr - (separatorAddr + 1) < sizeof(sampleLibIDStr))
			{
				memcpy(samplePicIDStr, separatorAddr + 1,
						fileNameEndAddr - (separatorAddr + 1));
				samplePicIDStr[fileNameEndAddr - (separatorAddr + 1)] = 0;

				cJSON_AddStringToObject(infoObj, "sampleLibID", sampleLibIDStr);
				cJSON_AddStringToObject(infoObj, "samplePicID", samplePicIDStr);
			}
			else
			{
				cJSON_AddNumberToObject(infoObj, "similar", 0);
			}
		}
		else
		{
			cJSON_AddNumberToObject(infoObj, "similar", 0);
		}
	}

	if (facePickupData->faceDetectResult.verifyResult)
	{
		cJSON_AddStringToObject(infoObj, "verifyResult", "true");
	}
	else
	{
		cJSON_AddStringToObject(infoObj, "verifyResult", "false");
	}
	cJSON_AddItemToArray(param, infoObj);
}

int WebClientRecvHandle(struct WebClient *webClient)
{
	char method[10] =
	{ '\0' };
	char url[512] =
	{ '\0' };
	char protocol[10] =
	{ '\0' };

	int ret;

	if (NULL == strchr(webClient->buff, '\n'))
	{
		if (webClient->buff_rsize >= 1024 - 1)
			return -1;
		return 0;
	}

	if (strstr(webClient->buff, "\r\n\r\n") == NULL)
	{
		if (webClient->buff_rsize >= 1024 - 1)
			return -1;
		return 0;
	}

	ret = sscanf(webClient->buff, "%9s %511s %9s", method, url, protocol);
	webClient->buff[webClient->buff_rsize] = 0;
	if (ret != 3)
	{
		ret = write_nonblock(webClient->fd, bad_request_response,
				sizeof(bad_request_response));
		return -1;
	}

	printf("%s", webClient->buff);
	printf("method:%s\n", method);
	printf("url:%s\n", url);
	printf("protocol:%s\n", protocol);

	strncpy(webClient->url, url, 100 - 1);

	//获取视频
	if (ToolStringStartsWith(url, "/video/chid=", 0) == 0)
	{
		//开始发送视频哦
		int chId = 0;
		sscanf(url, "/video/chid=%d", &chId);
		printf("chid=%d\n", chId);
		webClient->work_mode = web_mode_video_jpeg_stream;
		ret = send(webClient->fd, http_boundary_header,
				strlen(http_boundary_header), 0);
		if (ret == 0)
			return -1;
		webClient->stream_cache.chID = chId;
		return 0;
	}

	static char http_json_resp_head[] = "HTTP/1.1 200 OK\r\n"
			"Content-Type: application/json;charset=UTF-8\r\n"
			"Access-Control-Allow-origin:*\r\n"
			"Connection: close\r\n\r\n";

	//获取区域
	if (ToolStringStartsWith(url, "/rect/chid=", 0) == 0)
	{
		int chID = 0;
		sscanf(url, "/rect/chid=%d", &chID);
		webClient->work_mode = web_mode_rect;

		ret = send(webClient->fd, http_json_resp_head,
				strlen(http_json_resp_head), 0);

		cJSON *json = cJSON_CreateObject();
		cJSON_AddStringToObject(json, "action", "rectinfo");
		cJSON *param = cJSON_CreateObject();
		cJSON_AddItemToObject(json, "param", param);
		{
			struct WebClient *client = NULL;
			struct list_head *p = NULL;
			int ret;

			pthread_mutex_lock(&_mutex);
			list_for_each(p, &clist)
			{
				client = list_entry(p, struct WebClient, list);

				if (client->stream_cache.chID == chID)
				{
					cJSON *jsonRects = cJSON_CreateArray();
					for (int i = 0; i < client->webRectInfo.rectCount; i++)
					{
						cJSON *jsonItem = cJSON_CreateObject();
						cJSON_AddNumberToObject(jsonItem, "id",
								client->webRectInfo.allRects[i].id);
						cJSON_AddNumberToObject(jsonItem, "x",
								client->webRectInfo.allRects[i].x);
						cJSON_AddNumberToObject(jsonItem, "y",
								client->webRectInfo.allRects[i].y);
						cJSON_AddNumberToObject(jsonItem, "w",
								client->webRectInfo.allRects[i].w);
						cJSON_AddNumberToObject(jsonItem, "h",
								client->webRectInfo.allRects[i].h);
						cJSON_AddItemToArray(jsonRects, jsonItem);
					}
					cJSON_AddItemToObject(param, "rects", jsonRects);
					cJSON_AddNumberToObject(param, "width",
							client->webRectInfo.w);
					cJSON_AddNumberToObject(param, "height",
							client->webRectInfo.h);
					break;
				}
			}
			pthread_mutex_unlock(&_mutex);
		}

		char *buff = cJSON_Print(json);
		cJSON_Delete(json);
		if (buff)
		{
			ret = send(webClient->fd, buff, strlen(buff), 0);
			free(buff);
		}
		return -1; //need close
	}

	if (ToolStringStartsWith(url, "/face/info", 0) == 0)
	{
		char tmpbuf[1024];

		ret = send(webClient->fd, http_json_resp_head,
				strlen(http_json_resp_head), 0);

		cJSON *json = cJSON_CreateObject();
		cJSON *param = cJSON_CreateArray();
		cJSON_AddItemToObject(json, "param", param);
		{
			FaceAttribCache_Foreach(_faceAttribInfoBackFunc, param);
		}
		char *buff = cJSON_Print(json);
		cJSON_Delete(json);
		if (buff)
		{
			ret = send(webClient->fd, buff, strlen(buff), 0);
			free(buff);
		}
		return -1; //need close
	}

	///image/child/[chid]/[jpg name]
	if (ToolStringStartsWith(url, "/image/child/", 0) == 0)
	{
		char fileName[100];
		int chID = 0;
		uint64_t jid = 0;

		sscanf(url, "/image/child/%d/%s", &chID, fileName);
		sscanf(fileName, "%ld.jpg", &jid);

		{
			static char resp[] = "HTTP/1.1 200 OK\r\n"
					"Server: LXVideo\r\n"
					"Connection: close\r\n"
					"Cache-Control: no-cache, private\r\n"
					"Content-Type: image/jpeg\r\n\r\n";

			ret = send(webClient->fd, resp, strlen(resp), 0);
			//倒序
			pthread_mutex_lock(&_mutexFaceAttrib);

			for (int i = _FaceAttribCache.front; i != _FaceAttribCache.rear;
					i = (i + 1) % 10)
			{
				if (_FaceAttribCache.picAttrib[i].jid == jid)
				{
					ret = send(webClient->fd,
							_FaceAttribCache.picAttrib[i].jdata,
							_FaceAttribCache.picAttrib[i].jsize, 0);
				}
			}
			pthread_mutex_unlock(&_mutexFaceAttrib);
			fsync(webClient->fd);
		}
	}
	return -1;
}

int webClientHandleWrite(struct WebClient *webClient)
{
	int ret = 0;
	switch (webClient->work_mode)
	{
		case web_mode_video_jpeg_stream:
			ret = WebClientWriteStreamVideo(webClient);
		break;
		default:
			ret = -1;
		break;
	}
	return ret;
}

void WebClientClose(struct WebClient *webClient)
{
	epoll_ctl(webClient->epfd, EPOLL_CTL_DEL, webClient->fd, NULL);
	shutdown(webClient->fd, SHUT_RDWR);
	close(webClient->fd);

	pthread_mutex_lock(&_mutex);
	list_del(&webClient->list);
	pthread_mutex_unlock(&_mutex);

	WebClient_free(webClient);
}

static void *_ThreadWebVideoStream(void *arg)
{
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	int val = 1;
	struct sockaddr_in in;

	pthread_detach(pthread_self());
	prctl(PR_SET_NAME, "WebVideo");

	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));
	in.sin_family = AF_INET;
	in.sin_port = htons(8082);
	in.sin_addr.s_addr = INADDR_ANY;

	bind(sfd, (struct sockaddr *) &in, sizeof(in));
	listen(sfd, 10);

	int epfd = epoll_create1(0);
	struct epoll_event *events = NULL;
	int eventCount = 0;
	struct list_head *p, *n;

	INIT_LIST_HEAD(&clist);

	int fdsize = 0;

	int i = 0;
	int ret = 0;
	int ewaitret = 0;

	{
		struct epoll_event ev;
		ev.data.fd = sfd;
		ev.events = EPOLLIN;
		epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev);
	}
	printf("Web listeing ....\n");
	while (1)
	{
		fdsize = 1 + WebClients_Count();

		if (eventCount != fdsize)
		{
			void *eallptr = realloc(events,
					fdsize * sizeof(struct epoll_event));
			if (eallptr)
			{
				events = eallptr;
				eventCount = fdsize;
			}
		}

		pthread_mutex_lock(&_mutex);
		list_for_each(p, &clist)
		{
			struct WebClient *webClient = NULL;
			struct epoll_event ev;

			webClient = list_entry(p, struct WebClient, list);
			ev.data.ptr = webClient;
			ev.events = EPOLLIN;
			if (webClient->stream_cache.changeflag == 1)
				ev.events |= EPOLLOUT;
			epoll_ctl(epfd, EPOLL_CTL_MOD, webClient->fd, &ev);
		}
		pthread_mutex_unlock(&_mutex);

		ewaitret = epoll_wait(epfd, events, eventCount, 100);
		if (ewaitret < 0)
			continue;
		if (ewaitret == 0)
			continue;

		for (i = 0; i < ewaitret; i++)
		{
			if (events[i].data.fd == sfd)
			{
				printf("listen ....\n");
				if (events[i].events & (EPOLLIN | EPOLLPRI))
				{
					struct sockaddr_in caddr;
					socklen_t len = sizeof(caddr);
					int cfd = accept(sfd, (struct sockaddr *) &caddr, &len);
					if (cfd > 0)
					{
						struct WebClient *webClient = WebClient_calloc();
						if (!webClient)
						{
							close(cfd);
							continue;
						}
						memcpy(&webClient->addr, &caddr, sizeof(caddr));
						webClient->epfd = epfd;
						webClient->fd = cfd;
						socket_nonblock(cfd, 1);
						printf("new tcp connect %d\n", cfd);
						WebClients_Add(webClient);

						struct epoll_event ev;
						ev.events = EPOLLIN;
						ev.data.ptr = webClient;
						epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
					}
				}
				continue;
			}

			{ //client
				struct WebClient *webClient =
						(struct WebClient *) events[i].data.ptr;
				uint32_t event = events[i].events;

				if (event & EPOLLIN)
				{ //输入
				  //GET /chid1/video HTTP/1.1
				  //Host: 192.168.0.29:8002
				  //User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:69.0) Gecko/20100101 Firefox/69.0
				  //Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
				  //Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2
				  //Accept-Encoding: gzip, deflate
				  //Connection: keep-alive
				  //Upgrade-Insecure-Requests: 1
					ret = WebClientRecv(webClient);
					if (0 == ret)
						ret = WebClientRecvHandle(webClient);

					if (ret == -1)
					{
						WebClientClose(webClient);
						webClient = NULL;
						continue;
					}
				}

				if (event & EPOLLOUT)
				{
					ret = webClientHandleWrite(webClient);
					if (ret == -1)
					{
						WebClientClose(webClient);
						webClient = NULL;
						continue;
					}
				}

				if (event & (EPOLLERR | EPOLLHUP))
				{
					WebClientClose(webClient);
					webClient = NULL;
				}
			}				  //client
		}
	}
	return NULL;
}

void AIWebHttpInit()
{
	pthread_t pt;
	pthread_create(&pt, NULL, _ThreadWebVideoStream, NULL);
}
