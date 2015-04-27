#ifndef __FFMPEG_H__
#define __FFMPEG_H__

namespace ff
{
	class FFVideo
	{
	public:
		FFVideo();
		virtual ~FFVideo();
		bool open(const char *url);
		void seek(double t); //跳的指定位置进行播放，单位秒
		double cur() const; //视频当前播放位置,单位秒
		double length() const; //视频时间长度,单位秒
		bool isPause() const; //视频是否在暂停状态
		bool isOpen() const; //视频是否打开
		bool isPlaying() const; //视频是否在播放
		bool hasVideo() const; //是否有视频流
		bool hasAudio() const; //是否有音频流
		/*
			播放到结尾了返回true.
			也可以通过cur,来确定是否到结尾了.
		*/
		bool isEnd() const;
		void pause(); //暂停视频
		void play(); //播放视频
		void close(); //关闭视频，并释放内存
		int width() const; //视频的宽度
		int height() const; //视频的高度

		/*
			刷新,播放程序需要以一定的帧率调用该函数。比如1/30s
			函数成功返回一个RGB raw指针，可以格式为Texture2D::PixelFormat::RGB888
			因此你可以直接用来作为材质使用
		*/
		void *refresh() const;
	private:
		void* _ctx;
	};
}
#endif