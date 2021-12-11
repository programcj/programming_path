#ifndef _PUSH_STREAM_H
#define _PUSH_STREAM_H

#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <mutex>
#include <memory>
#include <string>

#include <exception>
#include <functional>
#include <unordered_map>
#include <future>
#include <stdexcept>

#include <iostream>
#include <fstream>
#include <list>
#include <vector>

namespace media {

	//历史时间
	struct histroy_time {
		int year; //年 2021....
		int mon;  //月 1-12
		int day;  //日 1-31
		int hour; //时 0-23
		int min;  //分 0-59
		int sec;  //秒 0-59
	};

	class FileItem {
	public:
		std::string _path; //文件路径
		size_t _size; //文件大小

		long _fid;

		//不需要
		struct histroy_time _tmcreate;
		struct histroy_time _tmwrite;
	};

	//参数
	class ArgHistory {
	public:
		std::string _deviceId; //设备ID
		struct histroy_time _tmstart; //开始时间
		struct histroy_time _tmend; //结束时间
		ArgHistory() {
			memset(&_tmstart, 0, sizeof(_tmstart));
			memset(&_tmend, 0, sizeof(_tmend));
		}

		//inUrl: history://{设备ID}/{年}/{月}/{日}/时分秒-{年}/月/日/时分秒
		static bool decoder(const std::string& uri, ArgHistory& arg);
		static time_t histroy2time_t(struct histroy_time* t);
		static void time_t2historytm(time_t t, struct histroy_time* ht);

		std::list<FileItem> getHistorys();
	};
	
	class StreamAttrib {
	public:
		std::string status; //在线,离线
		int width; //宽
		int height; //高
		std::string videoCodeName; //视频编码名称
		float fps;

		StreamAttrib() {
			status = "";
			width = 0;
			height = 0;
		}

		void setAttr(int w, int h, float _fps, const std::string& codestr) {
			width = w;
			height = h;
			fps = _fps;
			videoCodeName = codestr;
		}

		void clear() {
			width = 0;
			height = 0;
			videoCodeName = "";
			fps = 0;
		}
	};

	class PushStream : public std::enable_shared_from_this<PushStream>
	{
		ArgHistory _argHistoryFile; //历史文件
		std::string _inUrl; //输入地址
		std::string _outUrl; //输出地址
		bool _runflag;

		std::string _saveFileName; //保存到文件
		time_t _argModify;

		StreamAttrib _inUrlAttrib;
	public:
		typedef std::shared_ptr<PushStream> Ptr;
		friend int PushStreamTaskRtspLoop(PushStream* pushStreamTask);
		static void ClearVideos(); //清理视频

		PushStream();
		static PushStream::Ptr create();

		const std::string& getInURL() { return _inUrl; }
		const std::string& getOutURL() { return _outUrl; }

		void setInURL(const std::string &url);
		void setOutURL(const std::string &url);
		StreamAttrib& getInAttrib() { return _inUrlAttrib; } //获取输入源的属性

		void setSaveFileName(const std::string& name) { _saveFileName = name; }
		std::string& getSaveFileName() { return _saveFileName; }

		void start();

		void stop();

		bool isRun() { return _runflag; }

		void loop();

		const std::string& debugInfo() { return _desc; }

		void setBackFunDone(const std::function<void()>& func) {
			_callDone = func;
		}

	protected:
		std::string  _desc;  //注释

		std::function<void()> _callDone;
		void loopRtsp(); //rtsp推流
		void loopHistory(); //历史文件推流
		void loopFile(); //本地文件推流
	};

	int PushStreamTaskRtspLoop(PushStream* pushStreamTask);
}

#endif
