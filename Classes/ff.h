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
private:
	void* _ctx;
};

#endif