#ifndef _SDLImp_H_
#define _SDLImp_H_
/*
	SDLImp 一个脱离SDL的本地实现
*/
/*
	使用std::thread 替代 SDL_Thread函数
*/
#include <mutex>
#include <condition_variable>
#include <thread>

namespace ff{
	typedef int8_t		Sint8;
	typedef uint8_t		Uint8;
	typedef int16_t		Sint16;
	typedef uint16_t	Uint16;
	typedef int32_t		Sint32;
	typedef uint32_t	Uint32;

	typedef std::mutex mutex_t;
	typedef std::condition_variable cond_t;
	typedef std::thread thread_t;

	void Delay(Uint32 ms);

	mutex_t* createMutex();
	int lockMutex(mutex_t *);
	int unlockMutex(mutex_t *);
	void destroyMutex(mutex_t *);

	cond_t * createCond();
	int waitCond(cond_t*, mutex_t *);
	int signalCond(cond_t*);
	void destroyCond(cond_t *);
	int waitCondTimeout(cond_t* p, mutex_t*, unsigned int ms);

	thread_t* createThread(int(*)(void*), void *);
	void waitThread(thread_t *, int* status);

	struct Rect {
		Sint16 x, y;
		Uint16 w, h;
	};

	/*
		从SDL_video.h复制而来
		*/
#define SWSURFACE	0x00000000	/**< Surface is in system memory */
#define HWSURFACE	0x00000001	/**< Surface is in video memory */
#define ASYNCBLIT	0x00000004	/**< Use asynchronous blits if possible */
#define HWACCEL	0x00000100	/**< Blit uses hardware acceleration */
#define FULLSCREEN	0x80000000	/**< Surface is a full screen display */
#define RESIZABLE 	0x00000010	/**< This video mode may be resized */

#define YV12_OVERLAY  0x32315659	/**< Planar mode: Y + V + U  (3 planes) */
#define IYUV_OVERLAY  0x56555949	/**< Planar mode: Y + U + V  (3 planes) */
#define YUY2_OVERLAY  0x32595559	/**< Packed mode: Y0+U0+Y1+V0 (1 plane) */
#define UYVY_OVERLAY  0x59565955	/**< Packed mode: U0+Y0+V0+Y1 (1 plane) */
#define YVYU_OVERLAY  0x55595659	/**< Packed mode: Y0+V0+Y1+U0 (1 plane) */

#define HWACCEL	0x00000100	/**< Blit uses hardware acceleration */
#define SRCCOLORKEY	0x00001000	/**< Blit uses a source color key */
#define RLEACCELOK	0x00002000	/**< Private flag */
#define RLEACCEL	0x00004000	/**< Surface is RLE encoded */
#define SRCALPHA	0x00010000	/**< Blit uses source alpha blending */
#define PREALLOC	0x01000000	/**< Surface uses preallocated memory */

#define ALPHA_OPAQUE 255
#define ALPHA_TRANSPARENT 0

	/* RGB conversion lookup tables */
	struct Surface;
	struct Overlay;

	struct Palette {
		int       ncolors;
		SDL_Color *colors;
	};

	struct private_yuvhwfuncs {
		int(*Lock)(Overlay *overlay);
		void(*Unlock)(Overlay *overlay);
		int(*Display)(Overlay *overlay, Rect *src, Rect *dst);
		void(*FreeHW)(Overlay *overlay);
	};

	struct private_yuvhwdata {
		Surface *stretch;
		Surface *display;
		Uint8 *pixels;
		int *colortab;
		Uint32 *rgb_2_pix;
		void(*Display1X)(int *colortab, Uint32 *rgb_2_pix,
			unsigned char *lum, unsigned char *cr,
			unsigned char *cb, unsigned char *out,
			int rows, int cols, int mod);
		void(*Display2X)(int *colortab, Uint32 *rgb_2_pix,
			unsigned char *lum, unsigned char *cr,
			unsigned char *cb, unsigned char *out,
			int rows, int cols, int mod);

		/* These are just so we don't have to allocate them separately */
		Uint16 pitches[3];
		Uint8 *planes[3];
	};

	struct Overlay {
		Uint32 format;				/**< Read-only */
		int w, h;				/**< Read-only */
		int planes;				/**< Read-only */
		Uint16 *pitches;			/**< Read-only */
		Uint8 **pixels;				/**< Read-write */

		/** @name Hardware-specific surface info */
		/*@{*/
		struct private_yuvhwfuncs *hwfuncs;
		struct private_yuvhwdata *hwdata;
		/*@{*/

		/** @name Special flags */
		/*@{*/
		Uint32 hw_overlay : 1;	/**< Flag: This overlay hardware accelerated? */
		Uint32 UnusedBits : 31;
		/*@}*/
	};

	struct Color {
		Uint8 r;
		Uint8 g;
		Uint8 b;
		Uint8 unused;
	};

	struct Palette {
		int       ncolors;
		Color *colors;
	};

	/** Everything in the pixel format structure is read-only */
	struct PixelFormat {
		Palette *palette;
		Uint8  BitsPerPixel;
		Uint8  BytesPerPixel;
		Uint8  Rloss;
		Uint8  Gloss;
		Uint8  Bloss;
		Uint8  Aloss;
		Uint8  Rshift;
		Uint8  Gshift;
		Uint8  Bshift;
		Uint8  Ashift;
		Uint32 Rmask;
		Uint32 Gmask;
		Uint32 Bmask;
		Uint32 Amask;

		/** RGB color key information */
		Uint32 colorkey;
		/** Alpha value information (per-surface alpha) */
		Uint8  alpha;
	};

	struct BlitInfo{
		Uint8 *s_pixels;
		int s_width;
		int s_height;
		int s_skip;
		Uint8 *d_pixels;
		int d_width;
		int d_height;
		int d_skip;
		void *aux_data;
		PixelFormat *src;
		Uint8 *table;
		PixelFormat *dst;
	};
	typedef void(*loblit)(BlitInfo *info);
	/* This is the private info structure for software accelerated blits */
	struct private_swaccel {
		loblit blit;
		void *aux_data;
	};

	/* Blit mapping definition */
	struct BlitMap {
		Surface *dst;
		int identity;
		Uint8 *table;
		SDL_blit hw_blit;
		SDL_blit sw_blit;
		struct private_hwaccel *hw_data;
		struct private_swaccel *sw_data;

		/* the version count matches the destination; mismatch indicates
		an invalid mapping */
		unsigned int format_version;
	};

	struct Surface {
		Uint32 flags;				/**< Read-only */
		PixelFormat *format;		/**< Read-only */
		int w, h;				/**< Read-only */
		Uint16 pitch;				/**< Read-only */
		void *pixels;				/**< Read-write */
		int offset;				/**< Private */

		/** Hardware-specific surface info */
		struct private_hwdata *hwdata;

		/** clipping information */
		Rect clip_rect;			/**< Read-only */
		Uint32 unused1;				/**< for binary compatibility */

		/** Allow recursive locks */
		Uint32 locked;				/**< Private */

		/** info for fast blit mapping to other surfaces */
		struct BlitMap *map;		/**< Private */

		/** format version, bumped at every change to invalidate blit maps */
		unsigned int format_version;		/**< Private */

		/** Reference count -- used when freeing surface */
		int refcount;				/**< Read-mostly */
	};

	void SDLog(const char *log);

	/*
		SDL Video替代函数
		*/
	int LockYUVOverlay(Overlay *overlay);
	void UnlockYUVOverlay(Overlay *overlay);
	int DisplayYUVOverlay(Overlay *overlay, Rect *dstrect);
	void FreeYUVOverlay(Overlay *overlay);
	Overlay * CreateYUVOverlay(int width, int height,
		Uint32 format, Surface *display);

	Surface * CreateRGBSurface(Uint32 flags,
		int width, int height, int depth,
		Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
	void FreeSurface(Surface *);
	int LockSurface(Surface *);
	void UnlockSurface(Surface *);
	int SoftStretch(Surface *src, Rect *srcrect,
		Surface *dst, Rect *dstrect);
	void UpdateRects
		(Surface *screen, int numrects, Rect *rects);
	/*
		SDL 辅助函数
		*/
	void WM_SetCaption(const char *title, const char *icon);
	int ShowCursor(int toggle);
	void Quit(void);
	char *GetError(void);
	char *my_getenv(const char *name);

	/*
		SDL Audio替代函数
		*/
#define AUDIO_U8	0x0008	/**< Unsigned 8-bit samples */
#define AUDIO_S8	0x8008	/**< Signed 8-bit samples */
#define AUDIO_U16LSB	0x0010	/**< Unsigned 16-bit samples */
#define AUDIO_S16LSB	0x8010	/**< Signed 16-bit samples */
#define AUDIO_U16MSB	0x1010	/**< As above, but big-endian byte order */
#define AUDIO_S16MSB	0x9010	/**< As above, but big-endian byte order */
#define AUDIO_U16	AUDIO_U16LSB
#define AUDIO_S16	AUDIO_S16LSB


#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define AUDIO_U16SYS	AUDIO_U16LSB
#define AUDIO_S16SYS	AUDIO_S16LSB
#else
#define AUDIO_U16SYS	AUDIO_U16MSB
#define AUDIO_S16SYS	AUDIO_S16MSB
#endif

	struct AudioSpec {
		int freq;		/**< DSP frequency -- samples per second */
		Uint16 format;		/**< Audio data format */
		Uint8  channels;	/**< Number of channels: 1 mono, 2 stereo */
		Uint8  silence;		/**< Audio buffer silence value (calculated) */
		Uint16 samples;		/**< Audio buffer size in samples (power of 2) */
		Uint16 padding;		/**< Necessary for some compile environments */
		Uint32 size;		/**< Audio buffer size in bytes (calculated) */
		/**
		*  This function is called when the audio device needs more data.
		*
		*  @param[out] stream	A pointer to the audio data buffer
		*  @param[in]  len	The length of the audio buffer in bytes.
		*
		*  Once the callback returns, the buffer will no longer be valid.
		*  Stereo samples are stored in a LRLRLR ordering.
		*/
		void(*callback)(void *userdata, Uint8 *stream, int len);
		void  *userdata;
	};

	int OpenAudio(AudioSpec *desired, AudioSpec *obtained);
	void CloseAudio(void);
	void PauseAudio(int pause_on);

	/*
		SDL Event
		*/
#define EVENTMASK(X)	(1<<(X))
#define USEREVENT 24
#define FF_ALLOC_EVENT   (USEREVENT)
#define FF_QUIT_EVENT    (USEREVENT + 2)
#define ALLEVENTS		0xFFFFFFFF
	typedef enum {
		ADDEVENT,
		PEEKEVENT,
		GETEVENT
	} eventaction;

	/** A user-defined event type */
	struct UserEvent {
		Uint8 type;	/**< SDL_USEREVENT through SDL_NUMEVENTS-1 */
		int code;	/**< User defined event code */
		void *data1;	/**< User defined data pointer */
		void *data2;	/**< User defined data pointer */
	};

	union Event {
		Uint8 type;
		UserEvent user;
	};

	int PushEvent(Event *event);
	int PeepEvents(Event *events, int numevents,
		eventaction action, Uint32 mask);
	void PumpEvents(void);
}
#endif