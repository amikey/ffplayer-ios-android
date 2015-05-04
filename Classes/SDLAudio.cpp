#include "SDLImp.h"

/*
#ifdef WIN32
	#if __cplusplus
	extern "C" {
	#endif
	#include <SDL.h>
	#if __cplusplus
	}
	#endif
*/

namespace ff{
	/*
		重新实现SDL Audio的部分函数
	*/

	static AudioDriver current_audio;
	static AudioDevice *open_devices[16];

	static AudioDeviceID
		open_audio_device(const char *devname, int iscapture,
		const AudioSpec * desired, AudioSpec * obtained,
		int allowed_changes, int min_id)
	{
			AudioDeviceID id = 0;
			AudioSpec _obtained;
			AudioDevice *device;
			bool build_cvt;
			int i = 0;

			if (!SDL_WasInit(SDL_INIT_AUDIO)) {
				SDLog("Audio subsystem is not initialized");
				return 0;
			}

			if ((iscapture) && (!current_audio.impl.HasCaptureSupport)) {
				SDLog("No capture support");
				return 0;
			}

			if (!obtained) {
				obtained = &_obtained;
			}
			if (!prepare_audiospec(desired, obtained)) {
				return 0;
			}

			/* If app doesn't care about a specific device, let the user override. */
			if (devname == NULL) {
				devname = getenv("SDL_AUDIO_DEVICE_NAME");
			}

			/*
			* Catch device names at the high level for the simple case...
			* This lets us have a basic "device enumeration" for systems that
			*  don't have multiple devices, but makes sure the device name is
			*  always NULL when it hits the low level.
			*
			* Also make sure that the simple case prevents multiple simultaneous
			*  opens of the default system device.
			*/

			if ((iscapture) && (current_audio.impl.OnlyHasDefaultInputDevice)) {
				if ((devname) && (strcmp(devname, DEFAULT_INPUT_DEVNAME) != 0)) {
					SDLog("No such device");
					return 0;
				}
				devname = NULL;

				for (i = 0; i < SDL_arraysize(open_devices); i++) {
					if ((open_devices[i]) && (open_devices[i]->iscapture)) {
						SDLog("Audio device already open");
						return 0;
					}
				}
			}

			if ((!iscapture) && (current_audio.impl.OnlyHasDefaultOutputDevice)) {
				if ((devname) && (strcmp(devname, DEFAULT_OUTPUT_DEVNAME) != 0)) {
					SDLog("No such device");
					return 0;
				}
				devname = NULL;

				for (i = 0; i < SDL_arraysize(open_devices); i++) {
					if ((open_devices[i]) && (!open_devices[i]->iscapture)) {
						SDLog("Audio device already open");
						return 0;
					}
				}
			}

			device = (AudioDevice *)SDL_AllocAudioMem(sizeof(AudioDevice));
			if (device == NULL) {
				OutOfMemory();
				return 0;
			}
			memset(device, '\0', sizeof(SDL_AudioDevice));
			device->spec = *obtained;
			device->enabled = 1;
			device->paused = 1;
			device->iscapture = iscapture;

			/* Create a semaphore for locking the sound buffers */
			if (!current_audio.impl.SkipMixerLock) {
				device->mixer_lock = SDL_CreateMutex();
				if (device->mixer_lock == NULL) {
					close_audio_device(device);
					SDLog("Couldn't create mixer lock");
					return 0;
				}
			}

			/* force a device detection if we haven't done one yet. */
			if (((iscapture) && (current_audio.inputDevices == NULL)) ||
				((!iscapture) && (current_audio.outputDevices == NULL)))
				SDL_GetNumAudioDevices(iscapture);

			if (current_audio.impl.OpenDevice(device, devname, iscapture) < 0) {
				close_audio_device(device);
				return 0;
			}
			device->opened = 1;

			/* Allocate a fake audio memory buffer */
			device->fake_stream = (Uint8 *)SDL_AllocAudioMem(device->spec.size);
			if (device->fake_stream == NULL) {
				close_audio_device(device);
				OutOfMemory();
				return 0;
			}

			/* See if we need to do any conversion */
			build_cvt = false;
			if (obtained->freq != device->spec.freq) {
				if (allowed_changes & SDL_AUDIO_ALLOW_FREQUENCY_CHANGE) {
					obtained->freq = device->spec.freq;
				}
				else {
					build_cvt = true;
				}
			}
			if (obtained->format != device->spec.format) {
				if (allowed_changes & SDL_AUDIO_ALLOW_FORMAT_CHANGE) {
					obtained->format = device->spec.format;
				}
				else {
					build_cvt = true;
				}
			}
			if (obtained->channels != device->spec.channels) {
				if (allowed_changes & SDL_AUDIO_ALLOW_CHANNELS_CHANGE) {
					obtained->channels = device->spec.channels;
				}
				else {
					build_cvt = true;
				}
			}

			/* If the audio driver changes the buffer size, accept it.
			This needs to be done after the format is modified above,
			otherwise it might not have the correct buffer size.
			*/
			if (device->spec.samples != obtained->samples) {
				obtained->samples = device->spec.samples;
				SDL_CalculateAudioSpec(obtained);
			}

			if (build_cvt) {
				/* Build an audio conversion block */
				if (SDL_BuildAudioCVT(&device->convert,
					obtained->format, obtained->channels,
					obtained->freq,
					device->spec.format, device->spec.channels,
					device->spec.freq) < 0) {
					close_audio_device(device);
					return 0;
				}
				if (device->convert.needed) {
					device->convert.len = (int)(((double)device->spec.size) /
						device->convert.len_ratio);

					device->convert.buf =
						(Uint8 *)SDL_AllocAudioMem(device->convert.len *
						device->convert.len_mult);
					if (device->convert.buf == NULL) {
						close_audio_device(device);
						OutOfMemory();
						return 0;
					}
				}
			}

			/* Find an available device ID and store the structure... */
			for (id = min_id - 1; id < SDL_arraysize(open_devices); id++) {
				if (open_devices[id] == NULL) {
					open_devices[id] = device;
					break;
				}
			}

			if (id == SDL_arraysize(open_devices)) {
				SDLog("Too many open audio devices");
				close_audio_device(device);
				return 0;
			}

			/* Start the audio thread if necessary */
			if (!current_audio.impl.ProvidesOwnCallbackThread) {
				/* Start the audio thread */
				char name[64];
				SDL_snprintf(name, sizeof (name), "SDLAudioDev%d", (int)(id + 1));
				/* !!! FIXME: this is nasty. */
#if defined(__WIN32__) && !defined(HAVE_LIBC)
#undef SDL_CreateThread
#if SDL_DYNAMIC_API
				device->thread = SDL_CreateThread_REAL(SDL_RunAudio, name, device, NULL, NULL);
#else
				device->thread = SDL_CreateThread(SDL_RunAudio, name, device, NULL, NULL);
#endif
#else
				device->thread = SDL_CreateThread(SDL_RunAudio, name, device);
#endif
				if (device->thread == NULL) {
					SDL_CloseAudioDevice(id + 1);
					SDLog("Couldn't create audio thread");
					return 0;
				}
			}

			return id + 1;
		}

	int OpenAudio(AudioSpec *desired, AudioSpec *obtained)
	{
		AudioDeviceID id = 0;

		/* Start up the audio driver, if necessary. This is legacy behaviour! */
		if (!SDL_WasInit(SDL_INIT_AUDIO)) {
			if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
				return (-1);
			}
		}

		/* SDL_OpenAudio() is legacy and can only act on Device ID #1. */
		if (open_devices[0] != NULL) {
			SDLog("Audio device is already opened");
			return (-1);
		}

		if (obtained) {
			id = open_audio_device(NULL, 0, desired, obtained,
				SDL_AUDIO_ALLOW_ANY_CHANGE, 1);
		}
		else {
			id = open_audio_device(NULL, 0, desired, desired, 0, 1);
		}

		SDL_assert((id == 0) || (id == 1));
		return ((id == 0) ? -1 : 0);
	}

	/*
		SDL_CloseAudio
		*/
	void CloseAudio(void)
	{
		//SDL_CloseAudio();
	}

	void PauseAudio(int pause_on)
	{
		//SDL_PauseAudio(pause_on);
	}
}