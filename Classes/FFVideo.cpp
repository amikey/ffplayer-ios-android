#include "ff.h"
#include "ffdepends.h"

using namespace ff;

FFVideo::FFVideo() :_ctx(nullptr)
{
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
	return _ctx != nullptr;
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
