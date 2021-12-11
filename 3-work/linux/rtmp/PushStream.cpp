#include "../libtools/libtools.hh"
#include "../ConfigIni.h"

#include <iostream>
#include "PushStream.h"
#include "ffmpegex.h"
#include "rtmp/rtmp_h264.h"

using namespace media;

//fps计算
class FPSCalc
{
	int _count;
	std::chrono::steady_clock::time_point _oldtm;
	int _fps;
	int stepms;
public:
	FPSCalc()
	{
		_count = 0;
		_fps = 0;
		stepms = 0;
	}

	void step() //增加一步
	{
		if (_count == 0)
			_oldtm = std::chrono::steady_clock::now();
		_count++;
		auto curr_time = std::chrono::steady_clock::now();
		auto exisms = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - _oldtm);
		stepms = exisms.count();
		if (stepms < 0)
			_oldtm = std::chrono::steady_clock::now();

		if (stepms > 1000)
		{
			_fps = (int)(_count * 1.0 / stepms * 1000);
			_count = 0;
		}
	}
	
	int getStepMs() {
		return stepms;
	}

	int getCount() {
		return _count;
	}

	int getFps()
	{
		return _fps;
	}
};


static int _string2hitm(const std::string& uri, struct histroy_time& t)
{
	memset(&t, 0, sizeof(t));
	const char* str = uri.c_str();
	return sscanf(str, "%d/%d/%d/%02d%02d%02d",
		&t.year, &t.mon, &t.day,
		&t.hour, &t.min, &t.sec);
}

bool media::ArgHistory::decoder(const std::string& uri, ArgHistory& arg)
{
	//std::string uri = "history://device123/2021/11/01/101112-2021/11/10/101300";
	bool ret = false;
	if (uri.find("history://") == 0)
	{
		const char* ptr = uri.c_str();
		const char* end = "";
		ptr += strlen("history://");
		end = strchr(ptr, '/');
		if (end)
		{
			std::string deviceid(ptr, end - ptr);

			arg._deviceId = deviceid;

			end++;
			std::vector<std::string> stv = tool::Split(end, "-");
			if (stv.size() == 2)
			{
				_string2hitm(stv[0], arg._tmstart);
				_string2hitm(stv[1], arg._tmend);
				ret = true;
			}
		}
	}
	return ret;
}

PushStream::Ptr PushStream::create()
{
	return std::make_shared<PushStream>();
}

//清理视频
void media::PushStream::ClearVideos()
{
	std::string path = ConfigIni::Instance()[ConfigIniName_VideoPath]; //需要遍历目录
	std::vector<std::string> names = tool::DirList(path);

	for (auto name : names)
	{
		std::string dir = tool::PathAppend(path, name);
		std::vector<std::string> years = tool::DirList(dir);
		std::cout << "设备目录:" << dir << std::endl;

		for (auto year : years)
		{
			std::string ypath = tool::PathAppend(dir, year);
			std::vector<std::string> mons = tool::DirList(ypath);
			//std::cout << "年:" << ypath << std::endl;
			for (auto mon : mons)
			{
				std::string mpath = tool::PathAppend(ypath, mon);
				std::vector<std::string> days = tool::DirList(mpath); //需要排序
				//std::cout << "月:" << mpath << std::endl;

				if (days.size() > 0)
				{
					std::string dpath = tool::PathAppend(mpath, days[0]);
					LInfo("Clear") << "删除天, remove day:" << dpath << std::endl;
					tool::FileRemove(dpath);
				}

				days = tool::DirList(mpath);
				if (days.size() == 0)
				{
					LInfo("Clear") << "月 remove:" << mpath << ", sub file count =0" << std::endl;
					tool::FileRemove(mpath);
				}
				//for (auto day : days)
				//{
				//	std::string dpath = PathAppend(mpath, day);
				//	std::cout << "日:" << dpath << std::endl; //删除最小的天目录
				//}
			}
			mons = tool::DirList(ypath);
			if (mons.size() == 0)
			{
				LInfo("Clear") << "年 remove:" << ypath << ", sub file count =0" << std::endl;
				tool::FileRemove(ypath);
			}
		}
	}
}

PushStream::PushStream()
{
	_runflag = false;
	_argModify = 0;
}

void PushStream::setInURL(const std::string& url)
{
	_inUrl = url;
	//
	if (_inUrl.find("history://") == 0)
	{
		//历史文件类型
		ArgHistory::decoder(url, _argHistoryFile);
	}
	_argModify = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); //当前时间
}

void PushStream::setOutURL(const std::string& url)
{
	_outUrl = url;
}

void PushStream::start()
{
	if (_runflag)
		throw std::runtime_error("task is running!");
	_runflag = true;
	PushStream::Ptr myself = shared_from_this();

	OSThreadRun("Push", [myself]
		{
			myself->loop();
		});
}

void PushStream::stop()
{
	_runflag = false;
}

static void ffmpgein2des(struct ffmpegIn* fin, std::string& _desc)
{
	_desc = "in:";
	if (fin->fmt_ctx)
	{
		_desc += fin->fmt_ctx->url;
		_desc += ",";
	}
	_desc += std::to_string(fin->width) + "x" + std::to_string(fin->height) + "/";
	_desc += fin->video_codec_name;
}

extern "C" void test_nal_data(uint8_t * data, size_t size);

//使用ffmpeg进行推流

int media::PushStreamTaskRtspLoop(PushStream* pushStreamTask)
{
	const char* in_filename = pushStreamTask->getInURL().c_str();
	const char* out_filename = pushStreamTask->getOutURL().c_str();

	AVOutputFormat* ofmt = NULL;
	AVFormatContext* ifmt_ctx = NULL;
	AVFormatContext* ofmt_ctx = NULL;
	AVPacket pkt;
	int ret, i;
	int video_stream_index = -1;
	int64_t start_time = 0;
	uint64_t pts = 0;

	AVDictionary* opts = NULL;
	av_dict_set(&opts, "rtsp_transport", "tcp", 0);		//设置tcp or udp，默认一般优先tcp再尝试udp
	av_dict_set(&opts, "stimeout", "3000000", 0);		//设置超时3秒
	LDebugP("push", "in:%s, to:%s\n", in_filename, out_filename);

	AVStream* vstream = NULL;

	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, &opts)) < 0)
	{
		printf("Could not open input file.");

		pushStreamTask->_desc = "in open error";
		goto end;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0)
	{
		printf("Failed to retrieve input stream information");
		goto end;
	}

	for (i = 0; i < ifmt_ctx->nb_streams; i++)
	{
		if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_stream_index = i;
			break;
		}
	}
	av_dump_format(ifmt_ctx, 0, in_filename, 0);

	vstream = ifmt_ctx->streams[video_stream_index];

	{
		const char* codename = avcodec_get_name(vstream->codecpar->codec_id);
		pushStreamTask->_desc = "in ok ";
		pushStreamTask->_desc += std::to_string(vstream->codecpar->width);
		pushStreamTask->_desc += "x";
		pushStreamTask->_desc += std::to_string(vstream->codecpar->height);
		pushStreamTask->_desc += "/";
		pushStreamTask->_desc += codename;
	}

	//Output
	avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", out_filename);		//RTMP
	//avformat_alloc_output_context2(&ofmt_ctx, NULL, "mpegts", out_filename);//UDP

	if (!ofmt_ctx)
	{
		printf("Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt = ofmt_ctx->oformat;
	for (i = 0; i < ifmt_ctx->nb_streams; i++)
	{
		//Create output AVStream according to input AVStream
		AVStream* in_stream = ifmt_ctx->streams[i];
		if (in_stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
			continue;

		//注2：因为codecpar没有codec这个成员变量，所以不能简单地将codec替换成codecpar，可以通过avcodec_find_decoder()方法获取
		//AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
		AVCodec* avcodec = avcodec_find_decoder(in_stream->codecpar->codec_id);
		AVStream* out_stream = avformat_new_stream(ofmt_ctx, avcodec);
		if (!out_stream)
		{
			printf("Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}
		//Copy the settings of AVCodecContext
		//注3：avcodec_copy_context()方法已被avcodec_parameters_to_context()和avcodec_parameters_from_context()所替代，
		//ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
		AVCodecContext* codec_ctx = avcodec_alloc_context3(avcodec);
		ret = avcodec_parameters_to_context(codec_ctx, in_stream->codecpar);
		if (ret < 0)
		{
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
	av_dump_format(ofmt_ctx, 0, out_filename, 1);

	//Open output URL
	if (!(ofmt->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			printf("Could not open output URL '%s'", out_filename);

			pushStreamTask->_desc += ", out open error";
			goto end;
		}
	}
	//Write file header
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0)
	{
		printf("Error occurred when opening output URL\n");
		goto end;
	}
	pushStreamTask->_desc += ", out start";

	start_time = av_gettime();
	while (pushStreamTask->isRun())
	{
		AVStream* in_stream, * out_stream;
		//Get an AVPacket
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
			break;
		if (pkt.stream_index != video_stream_index)
		{
			av_packet_unref(&pkt);
			continue; //不是视频流
		}

		in_stream = ifmt_ctx->streams[pkt.stream_index];
		out_stream = ofmt_ctx->streams[0];

		//printf("pts:%ld dts:%ld duration:%ld\n", pkt.pts, pkt.dts, pkt.duration);

		pkt.pts = pts; //重建pts
		pkt.dts = pts;
		pts += pkt.duration;
		//重新给视频包打时间戳 还不知道怎么处理?

		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,
			(enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base,
			(enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);

		//av_packet_rescale_ts(&pkt, in_stream->time_base, out_stream->time_base);
		//printf("> pts:%ld, dts:%ld, duration:%ld\n", pkt.pts, pkt.dts, pkt.duration);
		pkt.pos = -1;
		//ret = av_write_frame(ofmt_ctx, &pkt);
		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);

		if (ret < 0)
		{
			char estr[AV_ERROR_MAX_STRING_SIZE];
			av_make_error_string(estr, AV_ERROR_MAX_STRING_SIZE, ret);
			fprintf(stderr, "Error muxing packet, %s\n", estr);
			break;
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
	av_dict_free(&opts);

	pushStreamTask->_desc = "continue";

	if (ret < 0 && ret != AVERROR_EOF)
	{
		printf("Error occurred.\n");
		return -1;
	}
	return 0;
}

#if 0
//一堆视频帧保存
int Packet2Mp4(const std::list<std::shared_ptr<Packet>>& packetList, struct ffmpegEnc* fenc)
{
	//std::list<std::shared_ptr<Packet>> packetList = list;
	uint64_t pts = 0;
	int ret;

	for (auto itr = packetList.begin(); itr != packetList.end(); itr++)
	{
		AVPacket* pkt = av_packet_alloc();
		if (av_packet_ref(pkt, (*itr)->getPacket()) == 0)
		{
			pkt->pts = pts; //重建pts
			pkt->dts = pts;
			pts += pkt->duration;
			av_packet_rescale_ts(pkt, fenc->src_time_base, fenc->format_ctx->streams[0]->time_base);
		}
		ret = av_interleaved_write_frame(fenc->format_ctx, pkt);
		av_packet_free(&pkt);
	}
	ffmpegEncClose(fenc);
	return 0;
}
#endif

class OutFileStream
{
	std::string _streamid;
	struct ffmpegEnc fenc;
	time_t _oldtime;
public:
	typedef std::shared_ptr<OutFileStream> Ptr;
	OutFileStream()
	{
		memset(&fenc, 0, sizeof(fenc));
		_oldtime = 0;
	}

	~OutFileStream();

	void setStreamID(const std::string& id)
	{
		_streamid = id;
	}

	bool savePacket(struct ffmpegIn* ffin, void* inpkt, int ms);

	void outClose();
};

OutFileStream::~OutFileStream()
{
	if (fenc.openflag)
	{
		//需要关闭
		LInfo("VideoWrite") << "close " << _streamid << std::endl;
		outClose();
	}
}

//steady_clock 是单调的时钟，相当于教练手中的秒表；只会增长，适合用于记录程序耗时；
//system_clock 是系统的时钟；因为系统的时钟可以修改；甚至可以网络对时； 所以用系统时间计算时间差可能不准。
//high_resolution_clock 是当前系统能够提供的最高精度的时钟；它也是不可以修改的。相当于 steady_clock 的高精度版本。
bool OutFileStream::savePacket(struct ffmpegIn* ffin, void* inpkt, int ms)
{
	AVPacket* avpkt = (AVPacket*)inpkt;
	//得到时间 年/月/日/时分秒
	//如果需要保存为MP4呢
	std::string filePath = ConfigIni::Instance()[ConfigIniName_VideoPath];
	time_t cur_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); //当前时间
	std::string curTimePath = filePath;
	std::string exname = ConfigIni::Instance()[ConfigIniName_VideoType];

	if (ConfigIni::Instance().exists(ConfigIniName_VideoType) &&
		ConfigIni::Instance()[ConfigIniName_VideoType].length() > 0)
	{
		exname = ConfigIni::Instance().get(ConfigIniName_VideoType);
	}

	{
		char subpath[100];
		struct tm tm;
		localtime_r(&cur_time, &tm);

		//开始不能为Mp4,必须执行写结束后才变成mp4
		//年/月/日/时分秒
		snprintf(subpath, sizeof(subpath), "%s/%d/%02d/%02d/%02d%02d%02d.%s.tmp",
			_streamid.c_str(),
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, exname.c_str());

		filePath = tool::PathAppend(filePath, subpath);

		//年/月/日/时分 用于按分钟判断是否相同
		snprintf(subpath, sizeof(subpath), "%s/%d/%02d/%02d/%02d%02d",
			_streamid.c_str(),
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min);
		curTimePath = tool::PathAppend(curTimePath, subpath);
	}

	int isKeyFlag = (AV_PKT_FLAG_KEY & avpkt->flags) == AV_PKT_FLAG_KEY;
	int ret;

	//判断存储文件是否改变,如果改变则重新打开
	if (fenc.openflag)
	{
		//判断是否创建新的文件
		//首帧为关键帧
		//按每分钟进行比较
		if (isKeyFlag &&
			curTimePath.compare(0, curTimePath.length(),
				fenc.format_ctx->url, 0, curTimePath.length()) != 0)
		{
			outClose();
		}
	}

	if (fenc.openflag == 0)
	{ //创建文件目录
		if (!tool::FileExists(filePath))
		{
			LInfo("VideoSave") << "Create file:" << filePath << std::endl;
			if (!tool::CreateFileEx(filePath))
				LWarn("VideoWrite") << "不能创建文件:" << filePath << "," << tool::GetErrorStr() << std::endl;
		}

		ret = ffmpegEncOpenType(&fenc, ffin->vstream, filePath.c_str(), exname.c_str());
		if(ret!=0)
		{
			LErr("VideoWrite") << "not create file:" << filePath << "," << fenc.errstr << std::endl;
			tool::FileRemove(filePath);
		}
	}

	if (fenc.openflag)
	{
		int64_t pts, dts = 0, duration;
		AVPacket* pkt = av_packet_alloc();
		if (av_packet_ref(pkt, avpkt) == 0)
		{
			pkt->pts = fenc.pts; //重建pts
			pkt->dts = fenc.pts;
			pkt->duration = avpkt->duration;
			fenc.pts += avpkt->duration;

			av_packet_rescale_ts(pkt, fenc.src_time_base, fenc.format_ctx->streams[0]->time_base);
			pts = pkt->pts;
			dts = pkt->dts;
			duration = pkt->duration;
			//std::cout << "pts:" << pts << ",dts:" << dts << ",dur:" << duration << std::endl;
			ret = av_interleaved_write_frame(fenc.format_ctx, pkt);
			if (ret < 0)
			{
				//如果本次计算结果等于上次计算结果??? 如何处理
				LErr("VideoWrite") << "av_interleaved_write_frame()"
					<< ret << (isKeyFlag ? ",KFrame" : ",BFrame")
					<< "src(" << fenc.src_time_base.den << "," << fenc.src_time_base.num << ")"
					<< "enc(" << fenc.format_ctx->streams[0]->time_base.den << "," << fenc.format_ctx->streams[0]->time_base.num << ")"
					<< ",pkt:P="<<pts<<",D="<<dts<<",du="<<duration
					<<",spkt:P="<<avpkt->pts<<",D="<<avpkt->dts<<",du:"<<avpkt->duration
					<< std::endl;
			}
			av_packet_unref(pkt);
		}
		av_packet_free(&pkt);
	}
	return true;
}

void OutFileStream::outClose()
{
	if (fenc.openflag) //需要关闭
	{
		LInfo("VideoWrite") << "close " << _streamid << std::endl;
		std::string oldpath = fenc.format_ctx->url;
		ffmpegEncClose(&fenc);
		//去掉oldpath的末尾.tmp
		size_t pos = oldpath.find(".tmp");
		if (pos != std::string::npos)
		{
			std::string newpath = oldpath.substr(0, pos);
			tool::FileRenName(oldpath, newpath);
			LInfo("VideoWrite") << "OK," << newpath << std::endl;
		}
	}
}

void PushStream::loop()
{
#if 0
	while (isRun())
	{
		PushStreamTaskRtspLoop(this);
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
#else
	while (isRun())
	{
		if (_inUrl.find("rtsp://") == 0)
		{
			loopRtsp(); //rtsp地址推送
			continue;
		}

		if (_inUrl.find("history://") == 0)
		{
			loopHistory();
			break;
		}

		//判断是否是本地文件推流
		std::string exname = tool::FileExName(_inUrl);
		if (strcasecmp(exname.c_str(), "mp4") == 0) //mp4
		{
			//本地MP4文件推流
			loopFile();
		}
	}
	//av_frame_free(&avframe);

	if (_callDone)
		_callDone();

	_callDone = nullptr;
#endif
}

void media::PushStream::loopRtsp()
{
	int ret;
	int64_t pts_old = 0;
	int rtmp_out_ret = 0;
	struct ffmpegIn fin;

	RTMPOut* out = RTMPStreamOut_calloc();
	AVPacket* avpkt = av_packet_alloc(); // 创建一个packet

	memset(&fin, 0, sizeof(fin));
	av_init_packet(avpkt);

	OutFileStream::Ptr _outFileStream = nullptr;
	OSThreadSetDes("in:%s,out:%s", _inUrl.c_str(), _outUrl.c_str());
	_inUrlAttrib.status = "start";
	while (isRun())
	{

		memset(&fin, 0, sizeof(fin));
		ret = ffmpegInOpen(&fin, _inUrl.c_str());

		if (ret == 401)
		{
			_desc = _inUrl + ", error:401";
			_inUrlAttrib.status = "error:401";
			LWarn("PushStream") << _inUrl << ", 认证错误 open error " << ret << std::endl;
			break; //认证错误
		}

		if (ret != 0)
		{
			_inUrlAttrib.status = "error";			
			_desc = _inUrl + ", error:" + fin.errstr;
			LWarn("Push") << _inUrl << ", open error " << ret << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(3));
			continue;
		}
		_inUrlAttrib.status = "online";
		_inUrlAttrib.setAttr(fin.width, fin.height, fin.fps, fin.video_codec_name);

		ffmpgein2des(&fin, _desc);
		//av_rescale_q 把时间戳从一个时基调整到另外一个时基
		AVRational _AV_TIME_BASE_Q = { 1, AV_TIME_BASE };
		uint64_t nb_packets = 0;
		FPSCalc _fps;
		std::string _outdes = "";

		while (isRun())
		{
			ret = ffmpegInRead(&fin, avpkt); //AVERROR(EAGAIN)
			if (ret != 0)
				break;

			if (avpkt->stream_index != fin.video_stream_index)
			{
				av_packet_unref(avpkt);
				continue; //不是视频流
			}
			//test_nal_data(avpkt->data, avpkt->size);
			nb_packets++; //成功读取数据包数量
			_fps.step();  //重建fps

			//if (pkt.dts != AV_NOPTS_VALUE)
			//	pkt.dts += av_rescale_q(ts_offset, _AV_TIME_BASE_Q, fin.vstream->time_base);
			//if (pkt.pts != AV_NOPTS_VALUE)
			//	pkt.pts += av_rescale_q(ts_offset, _AV_TIME_BASE_Q, fin.vstream->time_base);

			if (!fin.saw_first_ts) { //首帧
				//dts = fin.d->avg_frame_rate.num ? -fin.dec_ctx->has_b_frames * AV_TIME_BASE / av_q2d(ist->st->avg_frame_rate) : 0;
				fin.pts = 0;
				fin.dts = 0;				
				avpkt->pts = 0;
				avpkt->dts = 0;	//dts需要重建
				fin.saw_first_ts = 1;
			}

			//if (fin.next_dts == AV_NOPTS_VALUE)
			//	fin.next_dts = fin.dts;
			//if (fin.next_pts == AV_NOPTS_VALUE)
			//	fin.next_pts = fin.pts;
			
			//if (avpkt && avpkt->dts != AV_NOPTS_VALUE) { //时间基数变化
				//fin.next_dts = fin.dts = av_rescale_q(avpkt->dts, fin.vstream->time_base, _AV_TIME_BASE_Q);
				//fin.next_pts = fin.pts = fin.dts;
			//}
			//fin.dts = fin.next_dts;
			//std::cout << "pts:" << avpkt->pts << ",dts:" << avpkt->dts << ",dur:" << avpkt->duration << std::endl;
#if 0
			if (fin.fmt_ctx.framerate.num) {
				// TODO: Remove work-around for c99-to-c89 issue 7
				AVRational time_base_q = AV_TIME_BASE_Q;
				int64_t next_dts = av_rescale_q(ist->next_dts, time_base_q, av_inv_q(ist->framerate));
				ist->next_dts = av_rescale_q(next_dts + 1, av_inv_q(ist->framerate), time_base_q);

				av_log(NULL, AV_LOG_INFO, ">>> num=%d next:%ld\n", ist->framerate.num, ist->next_dts);
			} else
#endif
			//if (avpkt->duration!=0) {
			//	fin.next_dts += av_rescale_q(avpkt->duration, fin.vstream->time_base, _AV_TIME_BASE_Q);
			//}
			//else if (fin.vstream->avg_frame_rate.num != 0) {
				//int ticks = av_stream_get_parser(fin.vstream) ? av_stream_get_parser(fin.vstream)->repeat_pict + 1 : 1;
				//num=25, den=1
				//int64_t n = ((int64_t)AV_TIME_BASE * fin.vstream->avg_frame_rate.den * ticks) / fin.vstream->avg_frame_rate.num / 1;
				//int64_t n = (int64_t)AV_TIME_BASE / av_q2d(fin.vstream->avg_frame_rate);
				//fin.next_dts += n;
				//还需要-offset
				//std::cout << "next:" << fin.next_dts << " step:" << n << std::endl;				
			//}
			//fin.pts = fin.dts;
			//fin.next_pts = fin.next_dts;
			
			//重建pts
			//*此数据包的持续时间（以AVStream->time_为基本单位），如果未知，则为0。
			//* 等于下一个pts - 按演示顺序显示此pts。
			if (avpkt->duration <= 0)
			{   //如果平均帧率存在
				int fps = fin.vstream->avg_frame_rate.den && fin.vstream->avg_frame_rate.num;
				if (!fps)
				{	//没有平均帧率时
					fin.fps = (int)_fps.getFps();
				}
				_inUrlAttrib.fps = fin.fps;

				if (fin.fps > 0) {
					int64_t duration = (int64_t)AV_TIME_BASE / fin.fps;
					avpkt->pts = av_rescale_q(fin.pts, _AV_TIME_BASE_Q, fin.vstream->time_base);
					avpkt->dts = avpkt->pts;
					avpkt->duration = av_rescale_q(duration, _AV_TIME_BASE_Q, fin.vstream->time_base);
					fin.pts += duration;
				}
			}
			//std::cout << "pts:" << avpkt->pts << ",dts:" << avpkt->dts << ",dur:" << avpkt->duration << std::endl;
#if 1	
			//开始推流
			if (fin.fps > 0)
			{
				AVRational dst_time_base = { 1, 1000 }; //1sec 计算ms
				int64_t pts = av_rescale_q_rnd(avpkt->pts, fin.vstream->time_base, dst_time_base, AV_ROUND_NEAR_INF);

				int isKeyFlag = (AV_PKT_FLAG_KEY & avpkt->flags) == AV_PKT_FLAG_KEY;
				int ms = (int)(pts - pts_old);

				pts_old = pts;

				if (_saveFileName.length() > 0)
				{
					//需要录像存储 {总路径}/{设备名}/年/月/日/录像文件
					if (!_outFileStream)
						_outFileStream = std::make_shared<OutFileStream>();
					if (_outFileStream)
					{
						_outFileStream->setStreamID(_saveFileName);
						_outFileStream->savePacket(&fin, avpkt, ms);
					}
				}

				if (out && out->isopen == 0)
				{
					ret = RTMPStreamOut_open(out, _outUrl.c_str());
					if (ret == -1)
					{
						LDebugP("PushStream", "Out Stream open error:%p\n", _outUrl.c_str());
						_outdes = "error";
					}
					else
					{
						LDebugP("PushStream", "Out Stream open ok:%p\n", _outUrl.c_str());
						RTMPStreamOut_SetInfo(out, fin.width, fin.height, fin.fps);
						_outdes = "open";
					}
				}

				//需要录像并写到文件中
				if (out && out->isopen == 1)
				{
					if (fin.video_codec_id == AV_CODEC_ID_H264)
						rtmp_out_ret = RTMPStreamOut_Send(out, avpkt->data, avpkt->size, ms);
					if (fin.video_codec_id == AV_CODEC_ID_H265)
						rtmp_out_ret = RTMPStreamOut_SendH265(out, avpkt->data, avpkt->size, ms);

					_outdes = "out " + std::to_string(ms) + "ms";
				}
				else
				{
					rtmp_out_ret = -1;
				}

				if (rtmp_out_ret == -1)
				{
					if (out)
					{
						LDebugP("PushStream", "Out Stream error:%p\n", out);
						RTMPStreamOut_close(out);
						_outdes = "out error";
					}
				}
			}//推流完成
#endif
			ffmpgein2des(&fin, _desc);
			_desc += ",fps:" + std::to_string(_fps.getFps())+"=>"; //接收到的真实fps流
			_desc += _outUrl + "," + _outdes;
			///
			av_packet_unref(avpkt);
		}
		_inUrlAttrib.clear();

		ffmpegInClose(&fin);
		if (out)
			RTMPStreamOut_close(out);

		if (_outFileStream)
		{
			_outFileStream->outClose();
		}

		if (isRun())
		{
			LWarnP("Push", "stream restart [%s]=>[%s]\n", _inUrl.c_str(), _outUrl.c_str());
			_inUrlAttrib.status = "restart";
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
	}
	if (out)
	{
		RTMPStreamOut_close(out);
		free(out);
		out = NULL;
	}
	av_packet_free(&avpkt);
	_inUrlAttrib.status = "close";
	LInfoP("Push", "Push Stream Close [%s]=>[%s]\n", _inUrl.c_str(), _outUrl.c_str());
}

//获取历史目录天
std::string _getHistoryDayDir(const std::string& streamid, struct histroy_time& t)
{
	std::string path = ConfigIni::Instance()[ConfigIniName_VideoPath];
	path = tool::PathAppend(path, streamid.c_str());

	char subpath[100];
	snprintf(subpath, sizeof(subpath), "/%d/%02d/%02d",
		t.year, t.mon, t.day);
	path = tool::PathAppend(path, subpath);
	return path;
}

time_t media::ArgHistory::histroy2time_t(struct histroy_time* t1)
{
	struct tm tm1;
	memset(&tm1, 0, sizeof(tm1));
	tm1.tm_year = t1->year - 1900;
	tm1.tm_mon = t1->mon - 1;
	tm1.tm_mday = t1->day;
	tm1.tm_hour = t1->hour;
	tm1.tm_min = t1->min;
	tm1.tm_sec = t1->sec;
	return mktime(&tm1);
}

void media::ArgHistory::time_t2historytm(time_t t, struct histroy_time* ht)
{
	struct tm tm;
	//localtime_s(&tm, &t);
	localtime_r(&t, &tm);

	ht->year = tm.tm_year + 1900;
	ht->mon = tm.tm_mon + 1;
	ht->day = tm.tm_mday;
	ht->hour = tm.tm_hour;
	ht->min = tm.tm_min;
}

std::list<FileItem> media::ArgHistory::getHistorys()
{
	struct histroy_time ht;
	//得到播放时间的开始和结束
	struct histroy_time tstart;
	tstart = _tmstart;
	time_t play_start_time = media::ArgHistory::histroy2time_t(&tstart);
	time_t play_end_time = media::ArgHistory::histroy2time_t(&_tmend);
	std::list<FileItem> list;

	while (play_start_time < play_end_time)
	{
		memset(&ht, 0, sizeof(ht));
		media::ArgHistory::time_t2historytm(play_start_time, &ht);
		std::string path = _getHistoryDayDir(_deviceId, ht);

		//查看具体的文件是否存在
		//遍历目录
		std::vector<std::string> videos = tool::DirList(path);
		for (auto name : videos)
		{
			std::string filePath = tool::PathAppend(path, name);
			struct stat st;
			memset(&st, 0, sizeof(st));
			stat(filePath.c_str(), &st);

			struct histroy_time htc, htw;
			memset(&htw, 0, sizeof(htw));
			//memset(&htc, 0, sizeof(htc));
			//media::ArgHistory::time_t2historytm(findData.time_create, &htc);
			//std::cout << findData.name <<" write: "<< ht.year << "-" << ht.mon << "-" << " " << ht.hour << ":" << ht.min << ":" << ht.sec << std::endl;
			long v = 0;
			long vs = 0;
			long ve = 0;
			if (1 == sscanf(name.c_str(), "%ld", &v))
			{
				vs = _tmstart.hour * 10000 + _tmstart.min * 100 + _tmstart.sec;
				ve = _tmend.hour * 10000 + _tmend.min * 100 + _tmend.sec;

				if (v >= vs && v <= ve)
				{
					FileItem f;
					f._path = filePath;
					f._size = 0;
					f._fid = v;
					f._tmcreate = ht;
					sscanf(name.c_str(), "%02d%02d%02d", &f._tmcreate.hour, &f._tmcreate.min, &f._tmcreate.sec);
					media::ArgHistory::time_t2historytm(st.st_mtime, &f._tmwrite);
					list.push_back(f);
				}
			}
		}

		//增加一天
		tstart.day++;
		if (tstart.day > 31)
		{
			tstart.day = 1;
			tstart.mon++;
			if (tstart.mon > 12)
			{
				tstart.mon = 1;
				tstart.year++;
			}
		}
		play_start_time = media::ArgHistory::histroy2time_t(&tstart);
	}

	//对list排序
	list.sort([](FileItem& a, FileItem& b)
		{	return a._fid < b._fid; });
	return list;
}

void media::PushStream::loopHistory()
{
	//需要找到对应的文件
	//得到时间 年/月/日/时分秒
	//查找下一个文件
	int ret;
	int64_t pts_old = 0;
	int rtmp_out_ret = 0;
	int ms = 0;
	struct ffmpegIn fin;
	time_t _argModifyOld = _argModify;

	RTMPOut* out = RTMPStreamOut_calloc();
	AVPacket* avpkt = av_packet_alloc(); // 创建一个packet
	int64_t avpkt_next_pts = 0;
	memset(&fin, 0, sizeof(fin));
	av_init_packet(avpkt);

	std::list<media::FileItem> list;
	bool pushflag = false;

	_inUrlAttrib.status = "start";
	do
	{
		pushflag = false; //默认不重新推流

		OSThreadSetDes("in:%s,out:%s", _inUrl.c_str(), _outUrl.c_str());

		list = _argHistoryFile.getHistorys();
		LInfo("PushHistory") << "start push size=" << list.size() << std::endl;
		for (auto itr = list.begin(); itr != list.end(); itr++)
		{
			media::FileItem& item = *itr;

			if (_argModifyOld != _argModify) //判断是否修改了参数, 如何让rtmp不close, 并继续推流呢
				break;

			FPSCalc _fps;
			memset(&fin, 0, sizeof(fin));
			ret = ffmpegInOpen(&fin, item._path.c_str());
			if (ret != 0)
			{
				if (ret != 0)
				{
					_desc = itr->_path + ", error:" + std::to_string(ret);
					LWarn("Push") << itr->_path << ", open error " << ret << std::endl;
					//std::this_thread::sleep_for(std::chrono::seconds(3));
					continue;
				}
				continue;
			}
			_inUrlAttrib.status = "online";
			_inUrlAttrib.setAttr(fin.width, fin.height, fin.fps, fin.video_codec_name);

			//需要判断文件是否为mp4, 
			std::string exname = path_extension_name(item._path.c_str());

			AVBSFContext* bsf_ctx = NULL;
			const AVBitStreamFilter* pfilter = NULL;

			//扩展名不能是h264/h265
			if (exname.compare("h264") != 0 &&
				exname.compare("h265") != 0)
			{
				if (fin.video_codec_id == AV_CODEC_ID_H264)
					pfilter = av_bsf_get_by_name("h264_mp4toannexb");
				if (fin.video_codec_id == AV_CODEC_ID_H265)
					pfilter = av_bsf_get_by_name("hevc_mp4toannexb");
				if (pfilter)
					av_bsf_alloc(pfilter, &bsf_ctx);
				//3. 添加解码器属性
				if (fin.vstream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
					avcodec_parameters_copy(bsf_ctx->par_in, fin.vstream->codecpar);
				//4. 初始化过滤器上下文
				av_bsf_init(bsf_ctx);
			}
#if 0
			//声明：
			AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
			//使用
			av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
			//释放：
			av_bitstream_filter_close(h264bsfc);

			//---新办吧
			//声明
			AVBSFContext* bsf_ctx = nullptr;
			const AVBitStreamFilter* pfilter = av_bsf_get_by_name("h264_mp4toannexb");
			av_bsf_alloc(pfilter, &bsf_ctx);
			//使用：
			av_bsf_send_packet(bsf_ctx, &packet);
			av_bsf_receive_packet(bsf_ctx, &packet);
			//释放：
			av_bsf_free(&bsf_ctx);
#endif

			ffmpgein2des(&fin, _desc);
			while (isRun())
			{
				if (_argModifyOld != _argModify) //判断是否修改了参数
					break;

				ret = ffmpegInRead(&fin, avpkt);
				if (ret != 0)
					break;
				if (avpkt->stream_index != fin.video_stream_index)
				{
					av_packet_unref(avpkt);
					continue; //不是视频流
				}
				if (avpkt->pts < 0)
				{
					printf("ms <0, pts:%lld\n", avpkt->pts);
					avpkt->pts = 0;
				}
				if (avpkt->duration <= 0)
				{
					avpkt->duration = avpkt->pts - avpkt_next_pts;
					avpkt_next_pts = avpkt->pts;
				}

				{ //开始推流
					AVRational dst_time_base =
					{ 1, 1000 };
					int64_t pts = av_rescale_q_rnd(avpkt->pts, fin.vstream->time_base, dst_time_base, AV_ROUND_NEAR_INF);
					int isKeyFlag = (AV_PKT_FLAG_KEY & avpkt->flags) == AV_PKT_FLAG_KEY;
					ms = (int)(pts - pts_old);
					pts_old = pts;

					if (out->isopen == 0)
					{
						printf("out:<%p>\n", out);
						ret = RTMPStreamOut_open(out, _outUrl.c_str());
						if (ret == -1)
						{
							LDebugP("PushStream", "Out Stream open error:%s\n", _outUrl.c_str());

							ffmpgein2des(&fin, _desc);
							_desc += "=>" + _outUrl + ", error";
						}
						else
						{
							LInfoP("PushStream", "Out Stream open ok:%s\n", _outUrl.c_str());

							RTMPStreamOut_SetInfo(out, fin.width, fin.height, fin.fps);
							ffmpgein2des(&fin, _desc);
							_desc += "=>" + _outUrl + ", open";
						}
					}

					if (out && out->isopen == 1)
					{
						ffmpgein2des(&fin, _desc);
						_desc += "=>" + _outUrl + ", send " + std::to_string(ms);

						//如果是mp4文件,需要转换为NALU单元格式
						if (bsf_ctx)
						{
							AVPacket opkt =
							{ 0 };
							av_init_packet(&opkt);
							opkt.dts = avpkt->dts;
							opkt.pts = avpkt->pts;
							opkt.duration = avpkt->duration;
							opkt.flags = avpkt->flags;

							if (avpkt->buf)
							{
								opkt.buf = av_buffer_ref(avpkt->buf);
								if (!opkt.buf)
								{
									LErr("Push") << "error" << std::endl;
								}
							}
							opkt.data = avpkt->data;
							opkt.size = avpkt->size;
							av_copy_packet_side_data(&opkt, avpkt);

							ret = av_bsf_send_packet(bsf_ctx, &opkt);
							if (ret == 0)
							{
								int eof = 0;
								int idx = 1;
								while (idx)
								{
									ret = av_bsf_receive_packet(bsf_ctx, &opkt);
									if (ret == AVERROR(EAGAIN))
									{
										ret = 0;
										idx--;
										continue;
									}
									else if (ret == AVERROR_EOF)
									{
										eof = 1;
									}
									else if (ret < 0)
										break;

									if (fin.video_codec_id == AV_CODEC_ID_H264)
										rtmp_out_ret = RTMPStreamOut_Send(out, opkt.data, opkt.size, ms);
									if (fin.video_codec_id == AV_CODEC_ID_H265)
										rtmp_out_ret = RTMPStreamOut_SendH265(out, opkt.data, opkt.size, ms);
									av_packet_unref(&opkt);
								}
							}
						}	//if(bsf_ctx) else
						else
						{
							if (fin.video_codec_id == AV_CODEC_ID_H264)
								rtmp_out_ret = RTMPStreamOut_Send(out, avpkt->data, avpkt->size, ms);
							if (fin.video_codec_id == AV_CODEC_ID_H265)
								rtmp_out_ret = RTMPStreamOut_SendH265(out, avpkt->data, avpkt->size, ms);
						} //end 						
					}
					else
					{
						rtmp_out_ret = -1;
					}

					if (rtmp_out_ret == -1)
					{
						ffmpgein2des(&fin, _desc);
						_desc += "=>" + _outUrl + ", send error";

						if (out)
						{
							LWarnP("PushStream", "Out Stream error:%p, %s\n", out, _outUrl.c_str());
							RTMPStreamOut_close(out);
						}
					}
				}
				av_packet_unref(avpkt);

				std::this_thread::sleep_for(std::chrono::milliseconds(ms));
			}

			LWarnP("Push", "stream restart [%s]=>[%s]\n", item._path.c_str(), _outUrl.c_str());

			ffmpegInClose(&fin);

			//6. 释放资源
			if (bsf_ctx)
				av_bsf_free(&bsf_ctx);
			bsf_ctx = NULL;
		}

		if (_argModify != _argModifyOld)
		{
			_argModifyOld = _argModify;
			//需要重新去推流
			pushflag = true;
		}

	} while (pushflag);

	if (out)
	{
		LInfo("PushStream") << "push stream close =>" << _outUrl << std::endl;
		RTMPStreamOut_close(out);
		free(out);
		out = NULL;
	}
	av_packet_free(&avpkt);

	_inUrlAttrib.status = "close";
	LInfo("PushStream") << "push stream quit." << std::endl;
}


void media::PushStream::loopFile() //本地文件推流
{
	//需要找到对应的文件
	//得到时间 年/月/日/时分秒
	//查找下一个文件
	int ret;
	int64_t pts_old = 0;
	int rtmp_out_ret = 0;
	int ms = 0;
	struct ffmpegIn fin;

	RTMPOut* out = RTMPStreamOut_calloc();
	AVPacket* avpkt = av_packet_alloc(); // 创建一个packet
	int64_t avpkt_next_pts = 0;
	memset(&fin, 0, sizeof(fin));
	av_init_packet(avpkt);

	bool pushflag = false;

	_inUrlAttrib.status = "start";

	OSThreadSetDes("in:%s,out:%s", _inUrl.c_str(), _outUrl.c_str());

	LInfo("PushFile") << "start push size=" << _outUrl << std::endl;

	while (isRun())
	{
		FPSCalc _fps;
		memset(&fin, 0, sizeof(fin));
		ret = ffmpegInOpen(&fin, _inUrl.c_str());
		if (ret != 0)
		{
			if (ret != 0)
			{
				_desc = _inUrl + ", error:" + std::to_string(ret);
				LWarn("Push") << _inUrl << ", open error " << ret << std::endl;
				//std::this_thread::sleep_for(std::chrono::seconds(3));
				continue;
			}
			continue;
		}
		_inUrlAttrib.status = "online";
		_inUrlAttrib.setAttr(fin.width, fin.height, fin.fps, fin.video_codec_name);

		//需要判断文件是否为mp4, 
		std::string exname = path_extension_name(_inUrl.c_str());

		AVBSFContext* bsf_ctx = NULL;
		const AVBitStreamFilter* pfilter = NULL;

		//扩展名不能是h264/h265
		if (exname.compare("h264") != 0 &&
			exname.compare("h265") != 0)
		{
			if (fin.video_codec_id == AV_CODEC_ID_H264)
				pfilter = av_bsf_get_by_name("h264_mp4toannexb");
			if (fin.video_codec_id == AV_CODEC_ID_H265)
				pfilter = av_bsf_get_by_name("hevc_mp4toannexb");
			if (pfilter)
				av_bsf_alloc(pfilter, &bsf_ctx);
			//3. 添加解码器属性
			if (fin.vstream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
				avcodec_parameters_copy(bsf_ctx->par_in, fin.vstream->codecpar);
			//4. 初始化过滤器上下文
			av_bsf_init(bsf_ctx);
		}

		ffmpgein2des(&fin, _desc);
		while (isRun())
		{
			ret = ffmpegInRead(&fin, avpkt);
			if (ret != 0)
				break;
			if (avpkt->stream_index != fin.video_stream_index)
			{
				av_packet_unref(avpkt);
				continue; //不是视频流
			}
			if (avpkt->pts < 0)
			{
				printf("ms <0, pts:%lld\n", avpkt->pts);
				avpkt->pts = 0;
			}
			if (avpkt->duration <= 0)
			{
				avpkt->duration = avpkt->pts - avpkt_next_pts;
				avpkt_next_pts = avpkt->pts;
			}

			{ //开始推流
				AVRational dst_time_base =
				{ 1, 1000 };
				int64_t pts = av_rescale_q_rnd(avpkt->pts, fin.vstream->time_base, dst_time_base, AV_ROUND_NEAR_INF);
				int isKeyFlag = (AV_PKT_FLAG_KEY & avpkt->flags) == AV_PKT_FLAG_KEY;
				ms = (int)(pts - pts_old);
				pts_old = pts;

				if (out->isopen == 0)
				{
					printf("out:<%p>\n", out);
					ret = RTMPStreamOut_open(out, _outUrl.c_str());
					if (ret == -1)
					{
						LDebugP("PushStream", "Out Stream open error:%s\n", _outUrl.c_str());

						ffmpgein2des(&fin, _desc);
						_desc += "=>" + _outUrl + ", error";
					}
					else
					{
						LInfoP("PushStream", "Out Stream open ok:%s\n", _outUrl.c_str());

						RTMPStreamOut_SetInfo(out, fin.width, fin.height, fin.fps);
						ffmpgein2des(&fin, _desc);
						_desc += "=>" + _outUrl + ", open";
					}
				}

				if (out && out->isopen == 1)
				{
					ffmpgein2des(&fin, _desc);
					_desc += "=>" + _outUrl + ", send " + std::to_string(ms);

					//如果是mp4文件,需要转换为NALU单元格式
					if (bsf_ctx)
					{
						AVPacket opkt =
						{ 0 };
						av_init_packet(&opkt);
						opkt.dts = avpkt->dts;
						opkt.pts = avpkt->pts;
						opkt.duration = avpkt->duration;
						opkt.flags = avpkt->flags;

						if (avpkt->buf)
						{
							opkt.buf = av_buffer_ref(avpkt->buf);
							if (!opkt.buf)
							{
								LErr("Push") << "error" << std::endl;
							}
						}
						opkt.data = avpkt->data;
						opkt.size = avpkt->size;
						av_copy_packet_side_data(&opkt, avpkt);

						ret = av_bsf_send_packet(bsf_ctx, &opkt);
						if (ret == 0)
						{
							int eof = 0;
							int idx = 1;
							while (idx)
							{
								ret = av_bsf_receive_packet(bsf_ctx, &opkt);
								if (ret == AVERROR(EAGAIN))
								{
									ret = 0;
									idx--;
									continue;
								}
								else if (ret == AVERROR_EOF)
								{
									eof = 1;
								}
								else if (ret < 0)
									break;

								if (fin.video_codec_id == AV_CODEC_ID_H264)
									rtmp_out_ret = RTMPStreamOut_Send(out, opkt.data, opkt.size, ms);
								if (fin.video_codec_id == AV_CODEC_ID_H265)
									rtmp_out_ret = RTMPStreamOut_SendH265(out, opkt.data, opkt.size, ms);
								av_packet_unref(&opkt);
							}
						}
					}	//if(bsf_ctx) else
					else
					{
						if (fin.video_codec_id == AV_CODEC_ID_H264)
							rtmp_out_ret = RTMPStreamOut_Send(out, avpkt->data, avpkt->size, ms);
						if (fin.video_codec_id == AV_CODEC_ID_H265)
							rtmp_out_ret = RTMPStreamOut_SendH265(out, avpkt->data, avpkt->size, ms);
					} //end 						
				}
				else
				{
					rtmp_out_ret = -1;
				}

				if (rtmp_out_ret == -1)
				{
					ffmpgein2des(&fin, _desc);
					_desc += "=>" + _outUrl + ", send error";

					if (out)
					{
						LWarnP("PushStream", "Out Stream error:%p, %s\n", out, _outUrl.c_str());
						RTMPStreamOut_close(out);
					}
				}
			}
			av_packet_unref(avpkt);

			std::this_thread::sleep_for(std::chrono::milliseconds(ms));
		}

		LWarnP("Push", "stream restart [%s]=>[%s]\n", _inUrl.c_str(), _outUrl.c_str());

		ffmpegInClose(&fin);

		//6. 释放资源
		if (bsf_ctx)
			av_bsf_free(&bsf_ctx);
		bsf_ctx = NULL;
	}

	if (out)
	{
		LInfo("PushStream") << "push stream close =>" << _outUrl << std::endl;
		RTMPStreamOut_close(out);
		free(out);
		out = NULL;
	}
	av_packet_free(&avpkt);

	_inUrlAttrib.status = "close";
	LInfo("PushStream") << "push stream quit." << std::endl;
}