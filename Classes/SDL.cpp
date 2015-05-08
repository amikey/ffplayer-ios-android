#include "SDLImp.h"
#ifdef WIN32
#include <ctype.h>
#endif
/*
	SDL 库的初始化实现
*/
namespace ff
{
	static Uint8 SDL_SubsystemRefCount[32];

	int SDL_MostSignificantBitIndex32(Uint32 x)
	{
#if defined(__GNUC__) && __GNUC__ >= 4
			/* Count Leading Zeroes builtin in GCC.
			* http://gcc.gnu.org/onlinedocs/gcc-4.3.4/gcc/Other-Builtins.html
			*/
			if (x == 0) {
				return -1;
			}
			return 31 - __builtin_clz(x);
#else
			/* Based off of Bit Twiddling Hacks by Sean Eron Anderson
			* <seander@cs.stanford.edu>, released in the public domain.
			* http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
			*/
			const Uint32 b[] = { 0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000 };
			const int    S[] = { 1, 2, 4, 8, 16 };

			int msbIndex = 0;
			int i;

			if (x == 0) {
				return -1;
			}

			for (i = 4; i >= 0; i--)
			{
				if (x & b[i])
				{
					x >>= S[i];
					msbIndex |= S[i];
				}
			}

			return msbIndex;
#endif
		}

	Uint32 SDL_WasInit(Uint32 flags)
	{
		int i;
		int num_subsystems = SDL_arraysize(SDL_SubsystemRefCount);
		Uint32 initialized = 0;

		if (!flags) {
			flags = SDL_INIT_AUDIO;
		}

		num_subsystems = SDL_min(num_subsystems, SDL_MostSignificantBitIndex32(flags) + 1);

		/* Iterate over each bit in flags, and check the matching subsystem. */
		for (i = 0; i < num_subsystems; ++i) {
			if ((flags & 1) && SDL_SubsystemRefCount[i] > 0) {
				initialized |= (1 << i);
			}

			flags >>= 1;
		}

		return initialized;
	}

	static bool
		SDL_PrivateShouldInitSubsystem(Uint32 subsystem)
	{
			int subsystem_index = SDL_MostSignificantBitIndex32(subsystem);
		//	SDL_assert(SDL_SubsystemRefCount[subsystem_index] < 255);
			return (SDL_SubsystemRefCount[subsystem_index] == 0);
	}

	static void
		SDL_PrivateSubsystemRefCountIncr(Uint32 subsystem)
	{
			int subsystem_index = SDL_MostSignificantBitIndex32(subsystem);
		//	SDL_assert(SDL_SubsystemRefCount[subsystem_index] < 255);
			++SDL_SubsystemRefCount[subsystem_index];
	}

	int SDL_InitSubSystem(Uint32 flags)
	{
		/* Initialize the audio subsystem */
		if ((flags & SDL_INIT_AUDIO)){
#if !SDL_AUDIO_DISABLED
			if (SDL_PrivateShouldInitSubsystem(SDL_INIT_AUDIO)) {
				if (SDL_AudioInit(NULL) < 0) {
					return (-1);
				}
			}
			SDL_PrivateSubsystemRefCountIncr(SDL_INIT_AUDIO);
#else
			return SDLog("SDL not built with audio support");
#endif
		}

		return (0);
	}

	int
		SDL_strncasecmp(const char *str1, const char *str2, size_t maxlen)
	{
#ifdef HAVE_STRNCASECMP
			return strncasecmp(str1, str2, maxlen);
#elif defined(HAVE__STRNICMP)
			return _strnicmp(str1, str2, maxlen);
#else
			char a = 0;
			char b = 0;
			while (*str1 && *str2 && maxlen) {
				a = tolower((unsigned char)*str1);
				b = tolower((unsigned char)*str2);
				if (a != b)
					break;
				++str1;
				++str2;
				--maxlen;
			}
			if (maxlen == 0) {
				return 0;
			}
			else {
				a = tolower((unsigned char)*str1);
				b = tolower((unsigned char)*str2);
				return (int)((unsigned char)a - (unsigned char)b);
			}
#endif /* HAVE_STRNCASECMP */
		}


}