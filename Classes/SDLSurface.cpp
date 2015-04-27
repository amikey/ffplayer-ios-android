#include "SDLImp.h"

#if __cplusplus
extern "C" {
#endif
#include <SDL.h>
#if __cplusplus
}
#endif

namespace ff{
	void UnlockSurface(Surface *surface)
	{
		/* Only perform an unlock if we are locked */
		if (!surface->locked || (--surface->locked > 0)) {
			return;
		}

		/* Perform the unlock */
		surface->pixels = (Uint8 *)surface->pixels - surface->offset;
	}

	int LockSurface(Surface *surface)
	{
		if (!surface->locked) {
			/* This needs to be done here in case pixels changes value */
			surface->pixels = (Uint8 *)surface->pixels + surface->offset;
		}

		/* Increment the surface lock count, for recursive locks */
		++surface->locked;

		/* Ready to go.. */
		return(0);
	}

	/* Helper functions */
	/*
	* Allocate a pixel format structure and fill it according to the given info.
	*/
	PixelFormat *AllocFormat(int bpp,
		Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
	{
		PixelFormat *format;
		Uint32 mask;

		/* Allocate an empty pixel format structure */
		format = (PixelFormat*)malloc(sizeof(*format));
		if (format == NULL) {
			return(NULL);
		}
		memset(format, 0, sizeof(*format));
		format->alpha = ALPHA_OPAQUE;

		/* Set up the format */
		format->BitsPerPixel = bpp;
		format->BytesPerPixel = (bpp + 7) / 8;
		if (Rmask || Bmask || Gmask) { /* Packed pixels with custom mask */
			format->palette = NULL;
			format->Rshift = 0;
			format->Rloss = 8;
			if (Rmask) {
				for (mask = Rmask; !(mask & 0x01); mask >>= 1)
					++format->Rshift;
				for (; (mask & 0x01); mask >>= 1)
					--format->Rloss;
			}
			format->Gshift = 0;
			format->Gloss = 8;
			if (Gmask) {
				for (mask = Gmask; !(mask & 0x01); mask >>= 1)
					++format->Gshift;
				for (; (mask & 0x01); mask >>= 1)
					--format->Gloss;
			}
			format->Bshift = 0;
			format->Bloss = 8;
			if (Bmask) {
				for (mask = Bmask; !(mask & 0x01); mask >>= 1)
					++format->Bshift;
				for (; (mask & 0x01); mask >>= 1)
					--format->Bloss;
			}
			format->Ashift = 0;
			format->Aloss = 8;
			if (Amask) {
				for (mask = Amask; !(mask & 0x01); mask >>= 1)
					++format->Ashift;
				for (; (mask & 0x01); mask >>= 1)
					--format->Aloss;
			}
			format->Rmask = Rmask;
			format->Gmask = Gmask;
			format->Bmask = Bmask;
			format->Amask = Amask;
		}
		else if (bpp > 8) {		/* Packed pixels with standard mask */
			/* R-G-B */
			if (bpp > 24)
				bpp = 24;
			format->Rloss = 8 - (bpp / 3);
			format->Gloss = 8 - (bpp / 3) - (bpp % 3);
			format->Bloss = 8 - (bpp / 3);
			format->Rshift = ((bpp / 3) + (bpp % 3)) + (bpp / 3);
			format->Gshift = (bpp / 3);
			format->Bshift = 0;
			format->Rmask = ((0xFF >> format->Rloss) << format->Rshift);
			format->Gmask = ((0xFF >> format->Gloss) << format->Gshift);
			format->Bmask = ((0xFF >> format->Bloss) << format->Bshift);
		}
		else {
			/* Palettized formats have no mask info */
			format->Rloss = 8;
			format->Gloss = 8;
			format->Bloss = 8;
			format->Aloss = 8;
			format->Rshift = 0;
			format->Gshift = 0;
			format->Bshift = 0;
			format->Ashift = 0;
			format->Rmask = 0;
			format->Gmask = 0;
			format->Bmask = 0;
			format->Amask = 0;
		}
		if (bpp <= 8) {			/* Palettized mode */
			/* not support */
		}
		return(format);
	}

	/*
	* Calculate the pad-aligned scanline width of a surface
	*/
	Uint16 CalculatePitch(Surface *surface)
	{
		Uint16 pitch;

		/* Surface should be 4-byte aligned for speed */
		pitch = surface->w*surface->format->BytesPerPixel;
		switch (surface->format->BitsPerPixel) {
		case 1:
			pitch = (pitch + 7) / 8;
			break;
		case 4:
			pitch = (pitch + 1) / 2;
			break;
		default:
			break;
		}
		pitch = (pitch + 3) & ~3;	/* 4-byte aligning */
		return(pitch);
	}

	/*
	* A function to calculate the intersection of two rectangles:
	* return true if the rectangles intersect, false otherwise
	*/
	static bool IntersectRect(const Rect *A, const Rect *B, Rect *intersection)
	{
			int Amin, Amax, Bmin, Bmax;

			/* Horizontal intersection */
			Amin = A->x;
			Amax = Amin + A->w;
			Bmin = B->x;
			Bmax = Bmin + B->w;
			if (Bmin > Amin)
				Amin = Bmin;
			intersection->x = Amin;
			if (Bmax < Amax)
				Amax = Bmax;
			intersection->w = Amax - Amin > 0 ? Amax - Amin : 0;

			/* Vertical intersection */
			Amin = A->y;
			Amax = Amin + A->h;
			Bmin = B->y;
			Bmax = Bmin + B->h;
			if (Bmin > Amin)
				Amin = Bmin;
			intersection->y = Amin;
			if (Bmax < Amax)
				Amax = Bmax;
			intersection->h = Amax - Amin > 0 ? Amax - Amin : 0;

			return (intersection->w && intersection->h);
		}

	/*
	* Set the clipping rectangle for a blittable surface
	*/
	bool SetClipRect(Surface *surface, const Rect *rect)
	{
		Rect full_rect;

		/* Don't do anything if there's no surface to act on */
		if (!surface) {
			return false;
		}

		/* Set up the full surface rectangle */
		full_rect.x = 0;
		full_rect.y = 0;
		full_rect.w = surface->w;
		full_rect.h = surface->h;

		/* Set the clipping rectangle */
		if (!rect) {
			surface->clip_rect = full_rect;
			return true;
		}
		return IntersectRect(rect, &full_rect, &surface->clip_rect);
	}

	void InvalidateMap(BlitMap *map)
	{
		if (!map) {
			return;
		}
		map->dst = NULL;
		map->format_version = (unsigned int)-1;
		if (map->table) {
			free(map->table);
			map->table = NULL;
		}
	}

	/*
	* Change any previous mappings from/to the new surface format
	*/
	void FormatChanged(Surface *surface)
	{
		static int format_version = 0;
		++format_version;
		if (format_version < 0) { /* It wrapped... */
			format_version = 1;
		}
		surface->format_version = format_version;
		InvalidateMap(surface->map);
	}

	void FreeBlitMap(BlitMap *map)
	{
		if (map) {
			InvalidateMap(map);
			if (map->sw_data != NULL) {
				free(map->sw_data);
			}
			free(map);
		}
	}

	BlitMap *AllocBlitMap(void)
	{
		BlitMap *map;

		/* Allocate the empty map */
		map = (BlitMap *)malloc(sizeof(*map));
		if (map == NULL) {
			return(NULL);
		}
		memset(map, 0, sizeof(*map));

		/* Allocate the software blit data */
		map->sw_data = (struct private_swaccel *)malloc(sizeof(*map->sw_data));
		if (map->sw_data == NULL) {
			FreeBlitMap(map);
			return(NULL);
		}
		memset(map->sw_data, 0, sizeof(*map->sw_data));

		/* It's ready to go */
		return(map);
	}

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
		if ((flags&HWSURFACE) == SWSURFACE) {
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

	/*
	* Free a previously allocated format structure
	*/
	void FreeFormat(PixelFormat *format)
	{
		if (format) {
			if (format->palette) {
				if (format->palette->colors) {
					free(format->palette->colors);
				}
				free(format->palette);
			}
			free(format);
		}
	}

	void FreeSurface(Surface *surface)
	{
		/* Free anything that's not NULL, and not the screen surface */
		/*
		if ((surface == NULL) ||
			(current_video &&
			((surface == SDL_ShadowSurface) || (surface == SDL_VideoSurface)))) {
			return;
		}
		*/
		if (--surface->refcount > 0) {
			return;
		}
		while (surface->locked > 0) {
			UnlockSurface(surface);
		}
		/*
		if ((surface->flags & SDL_RLEACCEL) == SDL_RLEACCEL) {
			SDL_UnRLESurface(surface, 0);
		}
		*/
		if (surface->format) {
			FreeFormat(surface->format);
			surface->format = NULL;
		}
		if (surface->map != NULL) {
			FreeBlitMap(surface->map);
			surface->map = NULL;
		}
		/*
		if (surface->hwdata) {
			SDL_VideoDevice *video = current_video;
			SDL_VideoDevice *this = current_video;
			video->FreeHWSurface(this, surface);
		}
		*/
		if (surface->pixels &&
			((surface->flags & PREALLOC) != PREALLOC)) {
			free(surface->pixels);
		}
		free(surface);
#ifdef CHECK_LEAKS
		--surfaces_allocated;
#endif
	}
}