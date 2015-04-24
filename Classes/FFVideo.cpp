#include "ff.h"
#include "ffdepends.h"

using namespace ff;

static bool isInitFF = false;

FFVideo::FFVideo() :_ctx(nullptr)
{
	if (!isInitFF){
		initFF();
		isInitFF = true;
	}
}

FFVideo::~FFVideo()
{
	close();
}

bool FFVideo::open(const char *url)
{
	_ctx = stream_open(url, NULL);
	return isOpen();
}

void FFVideo::seek(float t)
{

}

float FFVideo::cur()
{
	return 0;
}

bool FFVideo::isPause() const
{
	return isOpen() && is_stream_pause((VideoState*)_ctx);
}

bool FFVideo::isPlaying() const
{
	return !isPause();
}

bool FFVideo::isOpen() const
{
	VideoState* _vs = (VideoState*)_ctx;
	return  (_vs && (_vs->audio_st || _vs->video_st));
}

int FFVideo::width() const
{
	if (!isOpen())return -1;
	VideoState* _vs = (VideoState*)_ctx;
	if (!_vs->video_st->codec)return -1;
	return _vs->video_st->codec->width;
}

int FFVideo::height() const
{
	if (!isOpen())return -1;
	VideoState* _vs = (VideoState*)_ctx;
	if (!_vs->video_st->codec)return -1;
	return _vs->video_st->codec->height;
}

void *FFVideo::image() const
{
	VideoState* _vs = (VideoState*)_ctx;
	if (_vs && _vs->pscreen)
	{
		double r = 1 / 30;
		video_refresh(_vs, &r);
		return _vs->pscreen->pixels;
	}
	return nullptr;
}

int FFVideo::length() const
{
	if (!isOpen())return -1;
	return 0;//not impentment
}

void FFVideo::pause()
{
	if (isOpen()&&!is_stream_pause((VideoState*)_ctx))
	{
		toggle_pause((VideoState*)_ctx);
	}
}

void FFVideo::play()
{
	if (isOpen() && is_stream_pause((VideoState*)_ctx))
	{
		toggle_pause((VideoState*)_ctx);
	}
}

void FFVideo::close()
{
	if (_ctx)
	{
		stream_close((VideoState*)_ctx);
		_ctx = nullptr;
	}
}
