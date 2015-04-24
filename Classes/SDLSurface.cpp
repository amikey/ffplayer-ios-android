#include "SDLImp.h"

#if __cplusplus
extern "C" {
#endif
#include <SDL.h>
#if __cplusplus
}
#endif

namespace ff{
	Surface * CreateRGBSurface(Uint32 flags,
		int width, int height, int depth,
		Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
	{
	}

	void FreeSurface(Surface *)
	{
	}

#if 0
	Surface * CreateRGBSurface(Uint32 flags,
		int width, int height, int depth,
		Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
	{
		Surface *surface;

		/* Make sure the size requested doesn't overflow our datatypes */
		/* Next time I write a library like SDL, I'll use int for size. :) */
		if (width >= 16384 || height >= 65536) {
			SDLog("Width or height is too large");
			return(NULL);
		}

		flags &= ~HWSURFACE;

		/* Allocate the surface */
		surface = (Surface *)malloc(sizeof(*surface));
		if (surface == NULL) {
			return(NULL);
		}
		surface->flags = SWSURFACE;

		surface->format = AllocFormat(depth, Rmask, Gmask, Bmask, Amask);
		if (surface->format == NULL) {
			free(surface);
			return(NULL);
		}
		if (Amask) {
			surface->flags |= SRCALPHA;
		}
		surface->w = width;
		surface->h = height;
		surface->pitch = CalculatePitch(surface);
		surface->pixels = NULL;
		surface->offset = 0;
		surface->hwdata = NULL;
		surface->locked = 0;
		surface->map = NULL;
		surface->unused1 = 0;
		SetClipRect(surface, NULL);
		FormatChanged(surface);

		/* Get the pixels */
		if (((flags&HWSURFACE) == SWSURFACE) ||
			(video->AllocHWSurface(this, surface) < 0)) {
			if (surface->w && surface->h) {
				surface->pixels = malloc(surface->h*surface->pitch);
				if (surface->pixels == NULL) {
					FreeSurface(surface);
					return(NULL);
				}
				/* This is important for bitmaps */
				memset(surface->pixels, 0, surface->h*surface->pitch);
			}
		}

		/* Allocate an empty mapping */
		surface->map = AllocBlitMap();
		if (surface->map == NULL) {
			FreeSurface(surface);
			return(NULL);
		}

		/* The surface is ready to go */
		surface->refcount = 1;
#ifdef CHECK_LEAKS
		++surfaces_allocated;
#endif
		return(surface);
	}


	void FreeSurface(Surface *surface)
	{
		/* Free anything that's not NULL, and not the screen surface */
		if ((surface == NULL) ||
			(current_video &&
			((surface == SDL_ShadowSurface) || (surface == SDL_VideoSurface)))) {
			return;
		}
		if (--surface->refcount > 0) {
			return;
		}
		while (surface->locked > 0) {
			UnlockSurface(surface);
		}
		if ((surface->flags & SDL_RLEACCEL) == SDL_RLEACCEL) {
			SDL_UnRLESurface(surface, 0);
		}
		if (surface->format) {
			SDL_FreeFormat(surface->format);
			surface->format = NULL;
		}
		if (surface->map != NULL) {
			SDL_FreeBlitMap(surface->map);
			surface->map = NULL;
		}
		if (surface->hwdata) {
			SDL_VideoDevice *video = current_video;
			SDL_VideoDevice *this = current_video;
			video->FreeHWSurface(this, surface);
		}
		if (surface->pixels &&
			((surface->flags & SDL_PREALLOC) != SDL_PREALLOC)) {
			free(surface->pixels);
		}
		free(surface);
#ifdef CHECK_LEAKS
		--surfaces_allocated;
#endif
	}
#endif
}