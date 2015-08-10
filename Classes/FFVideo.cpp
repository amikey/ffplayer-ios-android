#include "ff.h"
#include "ffdepends.h"

namespace ff
{
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
		_first = true;
		close();
		_ctx = stream_open(url, NULL);
		return _ctx != nullptr;
	}

	void FFVideo::seek(double t)
	{
		VideoState* _vs = (VideoState*)_ctx;
		if (_vs)
		{
			if (t > length())
				t = length();
			if (t < 0)
				t = 0;
			double pos = cur_clock();
			int64_t ts = t * AV_TIME_BASE;
			int64_t ref = (int64_t)((t - pos) *AV_TIME_BASE);
			stream_seek(_vs, ts, ref, 0);
		}
	}

	bool FFVideo::isEnd() const
	{
		if (isOpen())
		{
			VideoState* is = (VideoState*)_ctx;
			if (is)
			{
				if ((!is->audio_st || (is->auddec.finished == is->audioq.serial && frame_queue_nb_remaining(&is->sampq) == 0)) &&
					(!is->video_st || (is->viddec.finished == is->videoq.serial && frame_queue_nb_remaining(&is->pictq) == 0)))
					return true;
			}
		}
		return false;
	}

	void FFVideo::set_preload_nb(int n)
	{
		VideoState* is = (VideoState*)_ctx;
		if (is)
		{
			is->nMIN_FRAMES = n;
		}
	}

	int FFVideo::preload_packet_nb() const
	{
		VideoState* is = (VideoState*)_ctx;
		if (is)
		{
			if (is->video_st)
				return is->videoq.nb_packets;
			else if (is->audio_st)
				return is->audioq.nb_packets;
		}
		return -1;
	}

	double FFVideo::cur_clock() const
	{
		VideoState* _vs = (VideoState*)_ctx;
		if (_vs)
		{
			double pos;
			pos = get_master_clock(_vs);
			if (isnan(pos))
				pos = (double)_vs->seek_pos / AV_TIME_BASE;
			return pos;
		}
		return -1;
	}

	double FFVideo::cur() const
	{
		VideoState* _vs = (VideoState*)_ctx;
		if (_vs)
		{
			return _vs->current;
		}
		return -1;
	}

	double FFVideo::length() const
	{
		VideoState* _vs = (VideoState*)_ctx;
		if (_vs && _vs->ic)
		{
			return (double)_vs->ic->duration / 1000000LL;
		}
		return 0;
	}

	bool FFVideo::hasVideo() const
	{
		VideoState* _vs = (VideoState*)_ctx;
		if (_vs)
		{
			return _vs->video_st ? true : false;
		}
		return false;
	}

	bool FFVideo::hasAudio() const
	{
		VideoState* _vs = (VideoState*)_ctx;
		if (_vs)
		{
			return _vs->audio_st ? true : false;
		}
		return false;
	}

	bool FFVideo::isPause() const
	{
		return isOpen() && is_stream_pause((VideoState*)_ctx);
	}

	bool FFVideo::isSeeking() const
	{
		if (!isOpen()) return false;

		VideoState* _vs = (VideoState*)_ctx;
		if (_vs == NULL) return false;
		return _vs->seek_req || _vs->step;
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

	int FFVideo::codec_width() const
	{
		if (!isOpen())return -1;
		VideoState* _vs = (VideoState*)_ctx;
		if (!_vs->video_st)return -1;
		if (!_vs->video_st->codec)return -1;
		return _vs->video_st->codec->width;
	}

	int FFVideo::codec_height() const
	{
		if (!isOpen())return -1;
		VideoState* _vs = (VideoState*)_ctx;
		if (!_vs->video_st)return -1;
		if (!_vs->video_st->codec)return -1;
		return _vs->video_st->codec->height;
	}

	int FFVideo::width() const
	{
		if (!isOpen())return -1;
		VideoState* _vs = (VideoState*)_ctx;
		if (!_vs->pscreen)return -1;
		return _vs->pscreen->w;
	}

	int FFVideo::height() const
	{
		if (!isOpen())return -1;
		VideoState* _vs = (VideoState*)_ctx;
		if (!_vs->pscreen)return -1;
		return _vs->pscreen->h;
	}

	void *FFVideo::refresh()
	{
		VideoState* _vs = (VideoState*)_ctx;
		if (_vs && _vs->pscreen2)
		{
			double r = 1.0 / 30.0;
			if (!is_stream_pause((VideoState*)_ctx))
				video_refresh(_vs, &r);
			if (_vs->pscreen){
				if (_first){
					pause();
					_first = false;
				}
				return _vs->pscreen->pixels;
			}
		}
		return nullptr;
	}

	void FFVideo::pause()
	{
		if (isOpen() && !is_stream_pause((VideoState*)_ctx) && !isSeeking())
		{
			toggle_pause((VideoState*)_ctx);
		}
	}

	void FFVideo::play()
	{
		if (isOpen() && is_stream_pause((VideoState*)_ctx) && !isSeeking())
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

	bool FFVideo::isError() const
	{
		VideoState* _vs = (VideoState*)_ctx;
		if (_vs)
		{
			return _vs->errcode != 0;
		}
		return false;
	}

	const char * FFVideo::errorMsg() const
	{
		VideoState* _vs = (VideoState*)_ctx;
		if (_vs)
		{
			return _vs->errmsg;
		}
		return nullptr;
	}

	double FFVideo::preload_time()
	{
		VideoState* is = (VideoState*)_ctx;
		if (is)
		{
			if (is->video_st &&is->video_st->avg_frame_rate.num!=0){
				return is->videoq.nb_packets*(double)is->video_st->avg_frame_rate.den / (double)is->video_st->avg_frame_rate.num;
			}
		}
		return -1;
	}

	bool FFVideo::set_preload_time(double t)
	{
		VideoState* is = (VideoState*)_ctx;
		if (is && t>=0 )
		{
			if (is->video_st && is->video_st->avg_frame_rate.den != 0)
			{
				int nb = (int)( t * (double)is->video_st->avg_frame_rate.num / (double)is->video_st->avg_frame_rate.den );
				set_preload_nb(nb);
				return true;
			}
		}
		return false;
	}
}