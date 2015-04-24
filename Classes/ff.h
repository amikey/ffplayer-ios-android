#ifndef __FFMPEG_H__
#define __FFMPEG_H__

class FFVideo
{
public:
	FFVideo();
	virtual ~FFVideo();
	bool open( const char *url);
	void seek(float t);
	float cur();
	bool isPause() const;
	bool isOpen() const;
	bool isPlaying() const;
	void pause();
	void play();
	void close();
	int width() const;
	int height() const;
	int length() const; //时间长度ms
	void *image() const;
private:
	void* _ctx;
};

#endif