#ifdef WIN32
/*
	windows使用mmsystem为SDL实现的驱动
*/
#include "SDLImp.h"

namespace ff
{
	#define NUM_FORMATS 10
	static int format_idx;
	static int format_idx_sub;
	static AudioFormat format_list[NUM_FORMATS][NUM_FORMATS] = {
		{ AUDIO_U8, AUDIO_S8, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB,
		AUDIO_U16MSB, AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB },
		{ AUDIO_S8, AUDIO_U8, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB,
		AUDIO_U16MSB, AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB },
		{ AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_S32LSB,
		AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_U8, AUDIO_S8 },
		{ AUDIO_S16MSB, AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_S32MSB,
		AUDIO_S32LSB, AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_U8, AUDIO_S8 },
		{ AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_S32LSB,
		AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_U8, AUDIO_S8 },
		{ AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_S16MSB, AUDIO_S16LSB, AUDIO_S32MSB,
		AUDIO_S32LSB, AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_U8, AUDIO_S8 },
		{ AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_S16LSB,
		AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_U8, AUDIO_S8 },
		{ AUDIO_S32MSB, AUDIO_S32LSB, AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_S16MSB,
		AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_U8, AUDIO_S8 },
		{ AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_S16LSB,
		AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_U8, AUDIO_S8 },
		{ AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_S32MSB, AUDIO_S32LSB, AUDIO_S16MSB,
		AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_U8, AUDIO_S8 },
	};

	void CalculateAudioSpec(AudioSpec * spec)
	{
			switch (spec->format) {
			case AUDIO_U8:
				spec->silence = 0x80;
				break;
			default:
				spec->silence = 0x00;
				break;
			}
			spec->size = AUDIO_BITSIZE(spec->format) / 8;
			spec->size *= spec->channels;
			spec->size *= spec->samples;
		}

	AudioFormat NextAudioFormat(void)
	{
			if ((format_idx == NUM_FORMATS) || (format_idx_sub == NUM_FORMATS)) {
				return (0);
			}
			return (format_list[format_idx][format_idx_sub++]);
	}

	AudioFormat FirstAudioFormat(AudioFormat format)
	{
		for (format_idx = 0; format_idx < NUM_FORMATS; ++format_idx) {
			if (format_list[format_idx][0] == format) {
				break;
			}
		}
		format_idx_sub = 0;
		return (NextAudioFormat());
	}

	char * WIN_StringToUTF8(TCHAR* tstr)
	{
		int len = WideCharToMultiByte(CP_UTF8, 0, tstr, -1, NULL, NULL, NULL, NULL);
		if (len == 0)
		{
			return NULL;
		}
		char * buf = (char *)malloc(len+3);
		len = WideCharToMultiByte(CP_UTF8, 0, tstr, -1, buf, len+3, NULL, NULL);
		return  buf;
	}

#define DETECT_DEV_IMPL(typ, capstyp) \
	static void DetectWave##typ##Devs(AddAudioDevice addfn) {\
	const UINT devcount = wave##typ##GetNumDevs(); \
	capstyp caps; \
	UINT i; \
	for (i = 0; i < devcount; i++) {\
	if (wave##typ##GetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {\
		char *name = WIN_StringToUTF8(caps.szPname); \
		if (name != NULL) {\
			addfn(name); \
			free(name); \
		} \
	} \
	} \
	}

	DETECT_DEV_IMPL(Out, WAVEOUTCAPS)
	DETECT_DEV_IMPL(In, WAVEINCAPS)

	static void
	WINMM_DetectDevices(int iscapture, AddAudioDevice addfn)
			{
		if (iscapture) {
			DetectWaveInDevs(addfn);
		}
		else {
			DetectWaveOutDevs(addfn);
		}
	}

			static void CALLBACK
				CaptureSound(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance,
				DWORD_PTR dwParam1, DWORD_PTR dwParam2)
			{
					AudioDevice *_this = (AudioDevice *)dwInstance;

					/* Only service "buffer is filled" messages */
					if (uMsg != WIM_DATA)
						return;

					/* Signal that we have a new buffer of data */
					ReleaseSemaphore(_this->hidden->audio_sem, 1, NULL);
				}


			/* The Win32 callback for filling the WAVE device */
			static void CALLBACK
				FillSound(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
				DWORD_PTR dwParam1, DWORD_PTR dwParam2)
			{
					AudioDevice *_this = (AudioDevice *)dwInstance;

					/* Only service "buffer done playing" messages */
					if (uMsg != WOM_DONE)
						return;

					/* Signal that we are done playing a buffer */
					ReleaseSemaphore(_this->hidden->audio_sem, 1, NULL);
				}

			static int
				SetMMerror(char *function, MMRESULT code)
			{
					int len;
					char errbuf[MAXERRORLENGTH];
					wchar_t werrbuf[MAXERRORLENGTH];

					_snprintf(errbuf, SDL_arraysize(errbuf), "%s: ", function);
					len = SDL_static_cast(int, strlen(errbuf));

					waveOutGetErrorText(code, werrbuf, MAXERRORLENGTH - len);
					WideCharToMultiByte(CP_ACP, 0, werrbuf, -1, errbuf + len,
						MAXERRORLENGTH - len, NULL, NULL);

					return CCLog("%s", errbuf);
				}

			static void
				WINMM_WaitDevice(AudioDevice *_this)
			{
					/* Wait for an audio chunk to finish */
					WaitForSingleObject(_this->hidden->audio_sem, INFINITE);
				}

			static Uint8 *
				WINMM_GetDeviceBuf(AudioDevice *_this)
			{
					return (Uint8 *)(_this->hidden->
						wavebuf[_this->hidden->next_buffer].lpData);
				}

			static void
				WINMM_PlayDevice(AudioDevice *_this)
			{
					/* Queue it up */
					waveOutWrite(_this->hidden->hout,
						&_this->hidden->wavebuf[_this->hidden->next_buffer],
						sizeof(_this->hidden->wavebuf[0]));
					_this->hidden->next_buffer = (_this->hidden->next_buffer + 1) % NUM_BUFFERS;
				}

			static void
				WINMM_WaitDone(AudioDevice *_this)
			{
					int i, left;

					do {
						left = NUM_BUFFERS;
						for (i = 0; i < NUM_BUFFERS; ++i) {
							if (_this->hidden->wavebuf[i].dwFlags & WHDR_DONE) {
								--left;
							}
						}
						if (left > 0) {
							Delay(100);
						}
					} while (left > 0);
				}

			static void
				WINMM_CloseDevice(AudioDevice *_this)
			{
					/* Close up audio */
					if (_this->hidden != NULL) {
						int i;

						if (_this->hidden->audio_sem) {
							CloseHandle(_this->hidden->audio_sem);
							_this->hidden->audio_sem = 0;
						}

						/* Clean up mixing buffers */
						for (i = 0; i < NUM_BUFFERS; ++i) {
							if (_this->hidden->wavebuf[i].dwUser != 0xFFFF) {
								waveOutUnprepareHeader(_this->hidden->hout,
									&_this->hidden->wavebuf[i],
									sizeof(_this->hidden->wavebuf[i]));
								_this->hidden->wavebuf[i].dwUser = 0xFFFF;
							}
						}

						/* Free raw mixing buffer */
						free(_this->hidden->mixbuf);
						_this->hidden->mixbuf = NULL;

						if (_this->hidden->hin) {
							waveInClose(_this->hidden->hin);
							_this->hidden->hin = 0;
						}

						if (_this->hidden->hout) {
							waveOutClose(_this->hidden->hout);
							_this->hidden->hout = 0;
						}

						free(_this->hidden);
						_this->hidden = NULL;
					}
				}

			static bool
				PrepWaveFormat(AudioDevice *_this, UINT devId, WAVEFORMATEX *pfmt, const int iscapture)
			{
					SDL_zerop(pfmt);

					if (SDL_AUDIO_ISFLOAT(_this->spec.format)) {
						pfmt->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
					}
					else {
						pfmt->wFormatTag = WAVE_FORMAT_PCM;
					}
					pfmt->wBitsPerSample = AUDIO_BITSIZE(_this->spec.format);

					pfmt->nChannels = _this->spec.channels;
					pfmt->nSamplesPerSec = _this->spec.freq;
					pfmt->nBlockAlign = pfmt->nChannels * (pfmt->wBitsPerSample / 8);
					pfmt->nAvgBytesPerSec = pfmt->nSamplesPerSec * pfmt->nBlockAlign;

					if (iscapture) {
						return (waveInOpen(0, devId, pfmt, 0, 0, WAVE_FORMAT_QUERY) == 0);
					}
					else {
						return (waveOutOpen(0, devId, pfmt, 0, 0, WAVE_FORMAT_QUERY) == 0);
					}
				}

			static int
				WINMM_OpenDevice(AudioDevice *_this, const char *devname, int iscapture)
			{
					AudioFormat test_format = FirstAudioFormat(_this->spec.format);
					int valid_datatype = 0;
					MMRESULT result;
					WAVEFORMATEX waveformat;
					UINT devId = WAVE_MAPPER;  /* WAVE_MAPPER == choose system's default */
					char *utf8 = NULL;
					UINT i;

					if (devname != NULL) {  /* specific device requested? */
						if (iscapture) {
							const UINT devcount = waveInGetNumDevs();
							WAVEINCAPS caps;
							for (i = 0; (i < devcount) && (devId == WAVE_MAPPER); i++) {
								result = waveInGetDevCaps(i, &caps, sizeof (caps));
								if (result != MMSYSERR_NOERROR)
									continue;
								else if ((utf8 = WIN_StringToUTF8(caps.szPname)) == NULL)
									continue;
								else if (strcmp(devname, utf8) == 0)
									devId = i;
								free(utf8);
							}
						}
						else {
							const UINT devcount = waveOutGetNumDevs();
							WAVEOUTCAPS caps;
							for (i = 0; (i < devcount) && (devId == WAVE_MAPPER); i++) {
								result = waveOutGetDevCaps(i, &caps, sizeof (caps));
								if (result != MMSYSERR_NOERROR)
									continue;
								else if ((utf8 = WIN_StringToUTF8(caps.szPname)) == NULL)
									continue;
								else if (strcmp(devname, utf8) == 0)
									devId = i;
								free(utf8);
							}
						}

						if (devId == WAVE_MAPPER) {
							return CCLog("Requested device not found");
						}
					}

					/* Initialize all variables that we clean on shutdown */
					_this->hidden = (PrivateAudioData *)
						malloc((sizeof *_this->hidden));
					if (_this->hidden == NULL) {
						return OutOfMemory();
					}
					memset(_this->hidden, 0, (sizeof *_this->hidden));

					/* Initialize the wavebuf structures for closing */
					for (i = 0; i < NUM_BUFFERS; ++i)
						_this->hidden->wavebuf[i].dwUser = 0xFFFF;

					if (_this->spec.channels > 2)
						_this->spec.channels = 2;        /* !!! FIXME: is _this right? */

					/* Check the buffer size -- minimum of 1/4 second (word aligned) */
					if (_this->spec.samples < (_this->spec.freq / 4))
						_this->spec.samples = ((_this->spec.freq / 4) + 3) & ~3;

					while ((!valid_datatype) && (test_format)) {
						switch (test_format) {
						case AUDIO_U8:
						case AUDIO_S16:
						case AUDIO_S32:
						case AUDIO_F32:
							_this->spec.format = test_format;
							if (PrepWaveFormat(_this, devId, &waveformat, iscapture)) {
								valid_datatype = 1;
							}
							else {
								test_format = NextAudioFormat();
							}
							break;

						default:
							test_format = NextAudioFormat();
							break;
						}
					}

					if (!valid_datatype) {
						WINMM_CloseDevice(_this);
						return CCLog("Unsupported audio format");
					}

					/* Update the fragment size as size in bytes */
					CalculateAudioSpec(&_this->spec);

					/* Open the audio device */
					if (iscapture) {
						result = waveInOpen(&_this->hidden->hin, devId, &waveformat,
							(DWORD_PTR)CaptureSound, (DWORD_PTR)_this,
							CALLBACK_FUNCTION);
					}
					else {
						result = waveOutOpen(&_this->hidden->hout, devId, &waveformat,
							(DWORD_PTR)FillSound, (DWORD_PTR)_this,
							CALLBACK_FUNCTION);
					}

					if (result != MMSYSERR_NOERROR) {
						WINMM_CloseDevice(_this);
						return SetMMerror("waveOutOpen()", result);
					}
#ifdef SOUND_DEBUG
					/* Check the sound device we retrieved */
					{
						WAVEOUTCAPS caps;

						result = waveOutGetDevCaps((UINT) _this->hidden->hout,
							&caps, sizeof(caps));
						if (result != MMSYSERR_NOERROR) {
							WINMM_CloseDevice(_this);
							return SetMMerror("waveOutGetDevCaps()", result);
						}
						printf("Audio device: %s\n", caps.szPname);
					}
#endif

					/* Create the audio buffer semaphore */
					_this->hidden->audio_sem =
						CreateSemaphore(NULL, NUM_BUFFERS - 1, NUM_BUFFERS, NULL);
					if (_this->hidden->audio_sem == NULL) {
						WINMM_CloseDevice(_this);
						return CCLog("Couldn't create semaphore");
					}

					/* Create the sound buffers */
					_this->hidden->mixbuf =
						(Uint8 *)malloc(NUM_BUFFERS * _this->spec.size);
					if (_this->hidden->mixbuf == NULL) {
						WINMM_CloseDevice(_this);
						return OutOfMemory();
					}
					for (i = 0; i < NUM_BUFFERS; ++i) {
						memset(&_this->hidden->wavebuf[i], 0,
							sizeof(_this->hidden->wavebuf[i]));
						_this->hidden->wavebuf[i].dwBufferLength = _this->spec.size;
						_this->hidden->wavebuf[i].dwFlags = WHDR_DONE;
						_this->hidden->wavebuf[i].lpData =
							(LPSTR)& _this->hidden->mixbuf[i * _this->spec.size];
						result = waveOutPrepareHeader(_this->hidden->hout,
							&_this->hidden->wavebuf[i],
							sizeof(_this->hidden->wavebuf[i]));
						if (result != MMSYSERR_NOERROR) {
							WINMM_CloseDevice(_this);
							return SetMMerror("waveOutPrepareHeader()", result);
						}
					}

					return 0;                   /* Ready to go! */
				}


			static int
				WINMM_Init(AudioDriverImpl * impl)
			{
					/* Set the function pointers */
					impl->DetectDevices = WINMM_DetectDevices;
					impl->OpenDevice = WINMM_OpenDevice;
					impl->PlayDevice = WINMM_PlayDevice;
					impl->WaitDevice = WINMM_WaitDevice;
					impl->WaitDone = WINMM_WaitDone;
					impl->GetDeviceBuf = WINMM_GetDeviceBuf;
					impl->CloseDevice = WINMM_CloseDevice;

					return 1;   /* _this audio target is available. */
				}

			AudioBootStrap WINMM_bootstrap = {
				"winmm", "Windows Waveform Audio", WINMM_Init, 0
			};
}
#endif