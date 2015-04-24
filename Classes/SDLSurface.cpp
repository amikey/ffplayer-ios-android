#include "SDLImp.h"
#if __cplusplus
extern "C" {
#endif
#include <SDL.h>
#if __cplusplus
}
#endif
namespace ff{
	/* used to save the destination format in the encoding. Designed to be
	macro-compatible with SDL_PixelFormat but without the unneeded fields */
	struct RLEDestFormat{
		Uint8  BytesPerPixel;
		Uint8  Rloss;
		Uint8  Gloss;
		Uint8  Bloss;
		Uint8  Rshift;
		Uint8  Gshift;
		Uint8  Bshift;
		Uint8  Ashift;
		Uint32 Rmask;
		Uint32 Gmask;
		Uint32 Bmask;
		Uint32 Amask;
	};
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
	/* Load pixel of the specified format from a buffer and get its R-G-B values */
	/* FIXME: rescale values to 0..255 here? */
#define RGB_FROM_PIXEL(Pixel, fmt, r, g, b)				\
	{									\
	r = (((Pixel&fmt->Rmask) >> fmt->Rshift) << fmt->Rloss); 		\
	g = (((Pixel&fmt->Gmask) >> fmt->Gshift) << fmt->Gloss); 		\
	b = (((Pixel&fmt->Bmask) >> fmt->Bshift) << fmt->Bloss); 		\
	}

	/* FIXME: this isn't correct, especially for Alpha (maximum != 255) */
#ifdef __NDS__ /* FIXME */
#define PIXEL_FROM_RGBA(Pixel, fmt, r, g, b, a)				\
	{									\
	Pixel = ((r >> fmt->Rloss) << fmt->Rshift) | \
	((g >> fmt->Gloss) << fmt->Gshift) | \
	((b >> fmt->Bloss) << fmt->Bshift) | \
	((a >> fmt->Aloss) << fmt->Ashift) | (1 << 15);				\
	}
#else
#define PIXEL_FROM_RGBA(Pixel, fmt, r, g, b, a)				\
	{									\
	Pixel = ((r >> fmt->Rloss) << fmt->Rshift) | \
	((g >> fmt->Gloss) << fmt->Gshift) | \
	((b >> fmt->Bloss) << fmt->Bshift) | \
	((a >> fmt->Aloss) << fmt->Ashift);				\
	}
#endif /* __NDS__ FIXME */

#define RGBA_FROM_8888(Pixel, fmt, r, g, b, a)	\
	{						\
	r = (Pixel&fmt->Rmask)>>fmt->Rshift;	\
	g = (Pixel&fmt->Gmask)>>fmt->Gshift;	\
	b = (Pixel&fmt->Bmask)>>fmt->Bshift;	\
	a = (Pixel&fmt->Amask)>>fmt->Ashift;	\
	}

	/* Assemble R-G-B values into a specified pixel format and store them */
#ifdef __NDS__ /* FIXME */
#define PIXEL_FROM_RGB(Pixel, fmt, r, g, b)				\
	{									\
	Pixel = ((r >> fmt->Rloss) << fmt->Rshift) | \
	((g >> fmt->Gloss) << fmt->Gshift) | \
	((b >> fmt->Bloss) << fmt->Bshift) | (1 << 15);				\
	}
#else
#define PIXEL_FROM_RGB(Pixel, fmt, r, g, b)				\
	{									\
	Pixel = ((r >> fmt->Rloss) << fmt->Rshift) | \
	((g >> fmt->Gloss) << fmt->Gshift) | \
	((b >> fmt->Bloss) << fmt->Bshift);				\
	}
#endif /* __NDS__ FIXME */

	/** Evaluates to true if the surface needs to be locked before access */
#define SDL_MUSTLOCK(surface)	\
	(surface->offset || \
	((surface->flags & (SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_RLEACCEL)) != 0))
	/* Public routines */
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
		format = (PixelFormat *)malloc(sizeof(*format));
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
			int ncolors = 1 << bpp;
			format->palette = (Palette *)malloc(sizeof(Palette));
			if (format->palette == NULL) {
				FreeFormat(format);
				SDL_OutOfMemory();
				return(NULL);
			}
			(format->palette)->ncolors = ncolors;
			(format->palette)->colors = (Color *)SDL_malloc(
				(format->palette)->ncolors*sizeof(Color));
			if ((format->palette)->colors == NULL) {
				FreeFormat(format);
				SDL_OutOfMemory();
				return(NULL);
			}
			if (Rmask || Bmask || Gmask) {
				/* create palette according to masks */
				int i;
				int Rm = 0, Gm = 0, Bm = 0;
				int Rw = 0, Gw = 0, Bw = 0;
#ifdef ENABLE_PALETTE_ALPHA
				int Am = 0, Aw = 0;
#endif
				if (Rmask)
				{
					Rw = 8 - format->Rloss;
					for (i = format->Rloss; i>0; i -= Rw)
						Rm |= 1 << i;
				}
#ifdef DEBUG_PALETTE
				fprintf(stderr, "Rw=%d Rm=0x%02X\n", Rw, Rm);
#endif
				if (Gmask)
				{
					Gw = 8 - format->Gloss;
					for (i = format->Gloss; i>0; i -= Gw)
						Gm |= 1 << i;
				}
#ifdef DEBUG_PALETTE
				fprintf(stderr, "Gw=%d Gm=0x%02X\n", Gw, Gm);
#endif
				if (Bmask)
				{
					Bw = 8 - format->Bloss;
					for (i = format->Bloss; i>0; i -= Bw)
						Bm |= 1 << i;
				}
#ifdef DEBUG_PALETTE
				fprintf(stderr, "Bw=%d Bm=0x%02X\n", Bw, Bm);
#endif
#ifdef ENABLE_PALETTE_ALPHA
				if (Amask)
				{
					Aw = 8 - format->Aloss;
					for (i = format->Aloss; i>0; i -= Aw)
						Am |= 1 << i;
				}
# ifdef DEBUG_PALETTE
				fprintf(stderr, "Aw=%d Am=0x%02X\n", Aw, Am);
# endif
#endif
				for (i = 0; i < ncolors; ++i) {
					int r, g, b;
					r = (i&Rmask) >> format->Rshift;
					r = (r << format->Rloss) | ((r*Rm) >> Rw);
					format->palette->colors[i].r = r;

					g = (i&Gmask) >> format->Gshift;
					g = (g << format->Gloss) | ((g*Gm) >> Gw);
					format->palette->colors[i].g = g;

					b = (i&Bmask) >> format->Bshift;
					b = (b << format->Bloss) | ((b*Bm) >> Bw);
					format->palette->colors[i].b = b;

#ifdef ENABLE_PALETTE_ALPHA
					a = (i&Amask) >> format->Ashift;
					a = (a << format->Aloss) | ((a*Am) >> Aw);
					format->palette->colors[i].unused = a;
#else
					format->palette->colors[i].unused = 0;
#endif
				}
			}
			else if (ncolors == 2) {
				/* Create a black and white bitmap palette */
				format->palette->colors[0].r = 0xFF;
				format->palette->colors[0].g = 0xFF;
				format->palette->colors[0].b = 0xFF;
				format->palette->colors[1].r = 0x00;
				format->palette->colors[1].g = 0x00;
				format->palette->colors[1].b = 0x00;
			}
			else {
				/* Create an empty palette */
				memset((format->palette)->colors, 0,
					(format->palette)->ncolors*sizeof(Color));
			}
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
			return SDL_FALSE;
		}

		/* Set up the full surface rectangle */
		full_rect.x = 0;
		full_rect.y = 0;
		full_rect.w = surface->w;
		full_rect.h = surface->h;

		/* Set the clipping rectangle */
		if (!rect) {
			surface->clip_rect = full_rect;
			return 1;
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
			SDL_free(map->table);
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
		map->sw_data = (private_swaccel *)malloc(sizeof(*map->sw_data));
		if (map->sw_data == NULL) {
			FreeBlitMap(map);
			return(NULL);
		}
		memset(map->sw_data, 0, sizeof(*map->sw_data));

		/* It's ready to go */
		return(map);
	}

#define PIXEL_COPY(to, from, len, bpp)			\
	do {\
	if (bpp == 4) {\
		SDL_memcpy4(to, from, (size_t)(len));		\
	}else {\
		SDL_memcpy(to, from, (size_t)(len)* (bpp));	\
	}							\
	} while (0)

#define OPAQUE_BLIT(to, from, length, bpp, alpha)	\
	PIXEL_COPY(to, from, length, bpp)

	/*
	* The general slow catch-all function, for remaining depths and formats
	*/
#define ALPHA_BLIT_ANY(to, from, length, bpp, alpha)			\
	do {\
	int i;								\
	Uint8 *src = from;						\
	Uint8 *dst = to;						\
	for (i = 0; i < (int)(length); i++) {\
	Uint32 s, d;						\
	unsigned rs, gs, bs, rd, gd, bd;				\
	switch (bpp) {\
	case 2:							\
	s = *(Uint16 *)src;					\
	d = *(Uint16 *)dst;					\
	break;							\
	case 3:							\
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {\
			s = (src[0] << 16) | (src[1] << 8) | src[2];	\
			d = (dst[0] << 16) | (dst[1] << 8) | dst[2];	\
		}else {\
			s = (src[2] << 16) | (src[1] << 8) | src[0];	\
			d = (dst[2] << 16) | (dst[1] << 8) | dst[0];	\
		}							\
		break;							\
	case 4:							\
	s = *(Uint32 *)src;					\
	d = *(Uint32 *)dst;					\
	break;							\
	}								\
	RGB_FROM_PIXEL(s, fmt, rs, gs, bs);				\
	RGB_FROM_PIXEL(d, fmt, rd, gd, bd);				\
	rd += (rs - rd) * alpha >> 8;				\
	gd += (gs - gd) * alpha >> 8;				\
	bd += (bs - bd) * alpha >> 8;				\
	PIXEL_FROM_RGB(d, fmt, rd, gd, bd);				\
	switch (bpp) {\
	case 2:							\
	*(Uint16 *)dst = (Uint16)d;					\
	break;							\
	case 3:							\
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {\
				dst[0] = (Uint8)(d >> 16);					\
				dst[1] = (Uint8)(d >> 8);					\
				dst[2] = (Uint8)(d);						\
			}else {\
				dst[0] = (Uint8)d;						\
				dst[1] = (Uint8)(d >> 8);					\
				dst[2] = (Uint8)(d >> 16);					\
			}							\
			break;							\
	case 4:							\
	*(Uint32 *)dst = d;					\
	break;							\
			}								\
			src += bpp;							\
			dst += bpp;							\
	}								\
	} while (0)


#define CHOOSE_BLIT(blitter, alpha, fmt)				\
	do {\
	if (alpha == 255) {\
			switch (fmt->BytesPerPixel) {\
			case 1: blitter(1, Uint8, OPAQUE_BLIT); break;		\
			case 2: blitter(2, Uint8, OPAQUE_BLIT); break;		\
			case 3: blitter(3, Uint8, OPAQUE_BLIT); break;		\
			case 4: blitter(4, Uint16, OPAQUE_BLIT); break;		\
		}								\
	}else {\
			switch (fmt->BytesPerPixel) {\
			case 1:							\
			/* No 8bpp alpha blitting */				\
			break;							\
			\
			case 2:							\
											switch (fmt->Rmask | fmt->Gmask | fmt->Bmask) {\
											case 0xffff:						\
				if (fmt->Gmask == 0x07e0				\
				|| fmt->Rmask == 0x07e0				\
				|| fmt->Bmask == 0x07e0) {\
					if (alpha == 128)				\
					blitter(2, Uint8, ALPHA_BLIT16_565_50);	\
					else {\
						blitter(2, Uint8, ALPHA_BLIT16_565);	\
					}						\
				}else						\
				goto general16;					\
				break;						\
				\
											case 0x7fff:						\
				if (fmt->Gmask == 0x03e0				\
				|| fmt->Rmask == 0x03e0				\
				|| fmt->Bmask == 0x03e0) {\
					if (alpha == 128)				\
					blitter(2, Uint8, ALPHA_BLIT16_555_50);	\
					else {\
						blitter(2, Uint8, ALPHA_BLIT16_555);	\
					}						\
					break;						\
				}							\
				/* fallthrough */					\
				\
											default:						\
										general16 : \
										blitter(2, Uint8, ALPHA_BLIT_ANY);			\
			}							\
			break;							\
			\
			case 3:							\
			blitter(3, Uint8, ALPHA_BLIT_ANY);			\
			break;							\
			\
			case 4:							\
						if ((fmt->Rmask | fmt->Gmask | fmt->Bmask) == 0x00ffffff	\
						&& (fmt->Gmask == 0xff00 || fmt->Rmask == 0xff00	\
						|| fmt->Bmask == 0xff00)) {\
							if (alpha == 128)					\
							blitter(4, Uint16, ALPHA_BLIT32_888_50);	\
							else						\
							blitter(4, Uint16, ALPHA_BLIT32_888);		\
						}else							\
						blitter(4, Uint16, ALPHA_BLIT_ANY);			\
						break;							\
		}								\
	}								\
	} while (0)

	/*
	* This takes care of the case when the surface is clipped on the left and/or
	* right. Top clipping has already been taken care of.
	*/
	static void RLEClipBlit(int w, Uint8 *srcbuf, Surface *dst,
		Uint8 *dstbuf,Rect *srcrect, unsigned alpha)
	{
		PixelFormat *fmt = dst->format;

#define RLECLIPBLIT(bpp, Type, do_blit)					   \
		do {\
		int linecount = srcrect->h;					   \
		int ofs = 0;							   \
		int left = srcrect->x;						   \
		int right = left + srcrect->w;					   \
		dstbuf -= left * bpp;						   \
		for (;;) {\
			int run;							   \
			ofs += *(Type *)srcbuf;					   \
			run = ((Type *)srcbuf)[1];					   \
			srcbuf += 2 * sizeof(Type);					   \
			if (run) {\
				/* clip to left and right borders */			   \
				if (ofs < right) {\
					int start = 0;					   \
					int len = run;					   \
					int startcol;					   \
					if (left - ofs > 0) {\
						start = left - ofs;				   \
						len -= start;					   \
						if (len <= 0)					   \
						goto nocopy ## bpp ## do_blit;		   \
					}							   \
					startcol = ofs + start;				   \
						if (len > right - startcol)				   \
						len = right - startcol;				   \
						do_blit(dstbuf + startcol * bpp, srcbuf + start * bpp, \
						len, bpp, alpha);				   \
				}							   \
				nocopy ## bpp ## do_blit:					   \
				srcbuf += run * bpp;					   \
				ofs += run;						   \
			}else if (!ofs)						   \
						break;							   \
			if (ofs == w) {\
				ofs = 0;						   \
				dstbuf += dst->pitch;					   \
				if (!--linecount)					   \
				break;						   \
			}								   \
		}								   \
		} while (0)

		CHOOSE_BLIT(RLECLIPBLIT, alpha, fmt);

#undef RLECLIPBLIT

	}

	/* blit a colorkeyed RLE surface */
	int SDL_RLEBlit(Surface *src, Rect *srcrect,
		Surface *dst, Rect *dstrect)
	{
		Uint8 *dstbuf;
		Uint8 *srcbuf;
		int x, y;
		int w = src->w;
		unsigned alpha;

		/* Lock the destination if necessary */
		if (SDL_MUSTLOCK(dst)) {
			if (LockSurface(dst) < 0) {
				return(-1);
			}
		}

		/* Set up the source and destination pointers */
		x = dstrect->x;
		y = dstrect->y;
		dstbuf = (Uint8 *)dst->pixels
			+ y * dst->pitch + x * src->format->BytesPerPixel;
		srcbuf = (Uint8 *)src->map->sw_data->aux_data;

		{
			/* skip lines at the top if neccessary */
			int vskip = srcrect->y;
			int ofs = 0;
			if (vskip) {

#define RLESKIP(bpp, Type)			\
				for (;;) {\
				int run;			\
				ofs += *(Type *)srcbuf;	\
				run = ((Type *)srcbuf)[1];	\
				srcbuf += sizeof(Type)* 2;	\
				if (run) {\
					srcbuf += run * bpp;	\
					ofs += run;		\
				}else if (!ofs)		\
				goto done;		\
				if (ofs == w) {\
					ofs = 0;		\
					if (!--vskip)		\
					break;		\
				}				\
				}

				switch (src->format->BytesPerPixel) {
				case 1: RLESKIP(1, Uint8); break;
				case 2: RLESKIP(2, Uint8); break;
				case 3: RLESKIP(3, Uint8); break;
				case 4: RLESKIP(4, Uint16); break;
				}

#undef RLESKIP

			}
		}

		alpha = (src->flags & SRCALPHA) == SRCALPHA
			? src->format->alpha : 255;
		/* if left or right edge clipping needed, call clip blit */
		if (srcrect->x || srcrect->w != src->w) {
			RLEClipBlit(w, srcbuf, dst, dstbuf, srcrect, alpha);
		}
		else {
			PixelFormat *fmt = src->format;

#define RLEBLIT(bpp, Type, do_blit)					      \
			do {\
			int linecount = srcrect->h;				      \
			int ofs = 0;						      \
			for (;;) {\
				unsigned run;					      \
				ofs += *(Type *)srcbuf;				      \
				run = ((Type *)srcbuf)[1];				      \
				srcbuf += 2 * sizeof(Type);				      \
				if (run) {\
					do_blit(dstbuf + ofs * bpp, srcbuf, run, bpp, alpha); \
					srcbuf += run * bpp;				      \
					ofs += run;					      \
				}else if (!ofs)					      \
				break;						      \
				if (ofs == w) {\
					ofs = 0;					      \
					dstbuf += dst->pitch;				      \
					if (!--linecount)				      \
					break;					      \
				}							      \
			}							      \
			} while (0)

			CHOOSE_BLIT(RLEBLIT, alpha, fmt);

#undef RLEBLIT
		}

	done:
		/* Unlock the destination if necessary */
		if (SDL_MUSTLOCK(dst)) {
			UnlockSurface(dst);
		}
		return(0);
	}

	/* encode 32bpp rgb + a into 16bpp rgb, losing alpha */
	static int copy_opaque_16(void *dst, Uint32 *src, int n,
		PixelFormat *sfmt, PixelFormat *dfmt)
	{
		int i;
		Uint16 *d = (Uint16 *)dst;
		for (i = 0; i < n; i++) {
			unsigned r, g, b;
			RGB_FROM_PIXEL(*src, sfmt, r, g, b);
			PIXEL_FROM_RGB(*d, dfmt, r, g, b);
			src++;
			d++;
		}
		return n * 2;
	}

	/* decode opaque pixels from 16bpp to 32bpp rgb + a */
	static int uncopy_opaque_16(Uint32 *dst, void *src, int n,
		RLEDestFormat *sfmt, PixelFormat *dfmt)
	{
		int i;
		Uint16 *s = (Uint16 *)src;
		unsigned alpha = dfmt->Amask ? 255 : 0;
		for (i = 0; i < n; i++) {
			unsigned r, g, b;
			RGB_FROM_PIXEL(*s, sfmt, r, g, b);
			PIXEL_FROM_RGBA(*dst, dfmt, r, g, b, alpha);
			s++;
			dst++;
		}
		return n * 2;
	}
	/* decode translucent pixels from 32bpp GORAB to 32bpp rgb + a */
	static int uncopy_transl_16(Uint32 *dst, void *src, int n,
		RLEDestFormat *sfmt, PixelFormat *dfmt)
	{
		int i;
		Uint32 *s = (Uint32 *)src;
		for (i = 0; i < n; i++) {
			unsigned r, g, b, a;
			Uint32 pix = *s++;
			a = (pix & 0x3e0) >> 2;
			pix = (pix & ~0x3e0) | pix >> 16;
			RGB_FROM_PIXEL(pix, sfmt, r, g, b);
			PIXEL_FROM_RGBA(*dst, dfmt, r, g, b, a);
			dst++;
		}
		return n * 4;
	}
	/* encode 32bpp rgba into 32bpp rgba, keeping alpha (dual purpose) */
	static int copy_32(void *dst, Uint32 *src, int n,
		PixelFormat *sfmt, PixelFormat *dfmt)
	{
		int i;
		Uint32 *d = (Uint32 *)dst;
		for (i = 0; i < n; i++) {
			unsigned r, g, b, a;
			Uint32 pixel;
			RGBA_FROM_8888(*src, sfmt, r, g, b, a);
			PIXEL_FROM_RGB(pixel, dfmt, r, g, b);
			*d++ = pixel | a << 24;
			src++;
		}
		return n * 4;
	}
	/* decode 32bpp rgba into 32bpp rgba, keeping alpha (dual purpose) */
	static int uncopy_32(Uint32 *dst, void *src, int n,
		RLEDestFormat *sfmt, PixelFormat *dfmt)
	{
		int i;
		Uint32 *s = (Uint32 *)src;
		for (i = 0; i < n; i++) {
			unsigned r, g, b, a;
			Uint32 pixel = *s++;
			RGB_FROM_PIXEL(pixel, sfmt, r, g, b);
			a = pixel >> 24;
			PIXEL_FROM_RGBA(*dst, dfmt, r, g, b, a);
			dst++;
		}
		return n * 4;
	}

	static Uint32 getpix_8(Uint8 *srcbuf)
	{
		return *srcbuf;
	}

	static Uint32 getpix_16(Uint8 *srcbuf)
	{
		return *(Uint16 *)srcbuf;
	}

	static Uint32 getpix_24(Uint8 *srcbuf)
	{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		return srcbuf[0] + (srcbuf[1] << 8) + (srcbuf[2] << 16);
#else
		return (srcbuf[0] << 16) + (srcbuf[1] << 8) + srcbuf[2];
#endif
	}

	static Uint32 getpix_32(Uint8 *srcbuf)
	{
		return *(Uint32 *)srcbuf;
	}

	typedef Uint32(*getpix_func)(Uint8 *);

	static getpix_func getpixes[4] = {
		getpix_8, getpix_16, getpix_24, getpix_32
	};

	static int RLEColorkeySurface(Surface *surface)
	{
		Uint8 *rlebuf, *dst;
		int maxn;
		int y;
		Uint8 *srcbuf, *lastline;
		int maxsize = 0;
		int bpp = surface->format->BytesPerPixel;
		getpix_func getpix;
		Uint32 ckey, rgbmask;
		int w, h;

		/* calculate the worst case size for the compressed surface */
		switch (bpp) {
		case 1:
			/* worst case is alternating opaque and transparent pixels,
			starting with an opaque pixel */
			maxsize = surface->h * 3 * (surface->w / 2 + 1) + 2;
			break;
		case 2:
		case 3:
			/* worst case is solid runs, at most 255 pixels wide */
			maxsize = surface->h * (2 * (surface->w / 255 + 1)
				+ surface->w * bpp) + 2;
			break;
		case 4:
			/* worst case is solid runs, at most 65535 pixels wide */
			maxsize = surface->h * (4 * (surface->w / 65535 + 1)
				+ surface->w * 4) + 4;
			break;
		}

		rlebuf = (Uint8 *)SDL_malloc(maxsize);
		if (rlebuf == NULL) {
			SDL_OutOfMemory();
			return(-1);
		}

		/* Set up the conversion */
		srcbuf = (Uint8 *)surface->pixels;
		maxn = bpp == 4 ? 65535 : 255;
		dst = rlebuf;
		rgbmask = ~surface->format->Amask;
		ckey = surface->format->colorkey & rgbmask;
		lastline = dst;
		getpix = getpixes[bpp - 1];
		w = surface->w;
		h = surface->h;

#define ADD_COUNTS(n, m)			\
		if (bpp == 4) {\
		((Uint16 *)dst)[0] = n;		\
		((Uint16 *)dst)[1] = m;		\
		dst += 4;				\
		}else {\
			dst[0] = n;				\
			dst[1] = m;				\
			dst += 2;				\
		}

		for (y = 0; y < h; y++) {
			int x = 0;
			int blankline = 0;
			do {
				int run, skip, len;
				int runstart;
				int skipstart = x;

				/* find run of transparent, then opaque pixels */
				while (x < w && (getpix(srcbuf + x * bpp) & rgbmask) == ckey)
					x++;
				runstart = x;
				while (x < w && (getpix(srcbuf + x * bpp) & rgbmask) != ckey)
					x++;
				skip = runstart - skipstart;
				if (skip == w)
					blankline = 1;
				run = x - runstart;

				/* encode segment */
				while (skip > maxn) {
					ADD_COUNTS(maxn, 0);
					skip -= maxn;
				}
				len = MIN(run, maxn);
				ADD_COUNTS(skip, len);
				SDL_memcpy(dst, srcbuf + runstart * bpp, len * bpp);
				dst += len * bpp;
				run -= len;
				runstart += len;
				while (run) {
					len = MIN(run, maxn);
					ADD_COUNTS(0, len);
					SDL_memcpy(dst, srcbuf + runstart * bpp, len * bpp);
					dst += len * bpp;
					runstart += len;
					run -= len;
				}
				if (!blankline)
					lastline = dst;
			} while (x < w);

			srcbuf += surface->pitch;
		}
		dst = lastline;		/* back up bast trailing blank lines */
		ADD_COUNTS(0, 0);

#undef ADD_COUNTS

		/* Now that we have it encoded, release the original pixels */
		if ((surface->flags & PREALLOC) != PREALLOC
			&& (surface->flags & HWSURFACE) != HWSURFACE) {
			free(surface->pixels);
			surface->pixels = NULL;
		}

		/* realloc the buffer to release unused memory */
		{
			/* If realloc returns NULL, the original block is left intact */
			Uint8 *p = (Uint8 *)realloc(rlebuf, dst - rlebuf);
			if (!p)
				p = rlebuf;
			surface->map->sw_data->aux_data = p;
		}

		return(0);
	}

	/* encode 32bpp rgb + a into 32bpp G0RAB format for blitting into 565 */
	static int copy_transl_565(void *dst, Uint32 *src, int n,
		PixelFormat *sfmt, PixelFormat *dfmt)
	{
		int i;
		Uint32 *d = (Uint32 *)dst;
		for (i = 0; i < n; i++) {
			unsigned r, g, b, a;
			Uint16 pix;
			RGBA_FROM_8888(*src, sfmt, r, g, b, a);
			PIXEL_FROM_RGB(pix, dfmt, r, g, b);
			*d = ((pix & 0x7e0) << 16) | (pix & 0xf81f) | ((a << 2) & 0x7e0);
			src++;
			d++;
		}
		return n * 4;
	}

	/* encode 32bpp rgb + a into 32bpp G0RAB format for blitting into 555 */
	static int copy_transl_555(void *dst, Uint32 *src, int n,
		PixelFormat *sfmt, PixelFormat *dfmt)
	{
		int i;
		Uint32 *d = (Uint32 *)dst;
		for (i = 0; i < n; i++) {
			unsigned r, g, b, a;
			Uint16 pix;
			RGBA_FROM_8888(*src, sfmt, r, g, b, a);
			PIXEL_FROM_RGB(pix, dfmt, r, g, b);
			*d = ((pix & 0x3e0) << 16) | (pix & 0xfc1f) | ((a << 2) & 0x3e0);
			src++;
			d++;
		}
		return n * 4;
	}

	/* convert surface to be quickly alpha-blittable onto dest, if possible */
	static int RLEAlphaSurface(Surface *surface)
	{
		Surface *dest;
		PixelFormat *df;
		int maxsize = 0;
		int max_opaque_run;
		int max_transl_run = 65535;
		unsigned masksum;
		Uint8 *rlebuf, *dst;
		int(*copy_opaque)(void *, Uint32 *, int,
			PixelFormat *, PixelFormat *);
		int(*copy_transl)(void *, Uint32 *, int,
			PixelFormat *, PixelFormat *);

		dest = surface->map->dst;
		if (!dest)
			return -1;
		df = dest->format;
		if (surface->format->BitsPerPixel != 32)
			return -1;		/* only 32bpp source supported */

		/* find out whether the destination is one we support,
		and determine the max size of the encoded result */
		masksum = df->Rmask | df->Gmask | df->Bmask;
		switch (df->BytesPerPixel) {
		case 2:
			/* 16bpp: only support 565 and 555 formats */
			switch (masksum) {
			case 0xffff:
				if (df->Gmask == 0x07e0
					|| df->Rmask == 0x07e0 || df->Bmask == 0x07e0) {
					copy_opaque = copy_opaque_16;
					copy_transl = copy_transl_565;
				}
				else
					return -1;
				break;
			case 0x7fff:
				if (df->Gmask == 0x03e0
					|| df->Rmask == 0x03e0 || df->Bmask == 0x03e0) {
					copy_opaque = copy_opaque_16;
					copy_transl = copy_transl_555;
				}
				else
					return -1;
				break;
			default:
				return -1;
			}
			max_opaque_run = 255;	/* runs stored as bytes */

			/* worst case is alternating opaque and translucent pixels,
			with room for alignment padding between lines */
			maxsize = surface->h * (2 + (4 + 2) * (surface->w + 1)) + 2;
			break;
		case 4:
			if (masksum != 0x00ffffff)
				return -1;		/* requires unused high byte */
			copy_opaque = copy_32;
			copy_transl = copy_32;
			max_opaque_run = 255;	/* runs stored as short ints */

			/* worst case is alternating opaque and translucent pixels */
			maxsize = surface->h * 2 * 4 * (surface->w + 1) + 4;
			break;
		default:
			return -1;		/* anything else unsupported right now */
		}

		maxsize += sizeof(RLEDestFormat);
		rlebuf = (Uint8 *)SDL_malloc(maxsize);
		if (!rlebuf) {
			SDL_OutOfMemory();
			return -1;
		}
		{
			/* save the destination format so we can undo the encoding later */
			RLEDestFormat *r = (RLEDestFormat *)rlebuf;
			r->BytesPerPixel = df->BytesPerPixel;
			r->Rloss = df->Rloss;
			r->Gloss = df->Gloss;
			r->Bloss = df->Bloss;
			r->Rshift = df->Rshift;
			r->Gshift = df->Gshift;
			r->Bshift = df->Bshift;
			r->Ashift = df->Ashift;
			r->Rmask = df->Rmask;
			r->Gmask = df->Gmask;
			r->Bmask = df->Bmask;
			r->Amask = df->Amask;
		}
		dst = rlebuf + sizeof(RLEDestFormat);

		/* Do the actual encoding */
		{
			int x, y;
			int h = surface->h, w = surface->w;
			PixelFormat *sf = surface->format;
			Uint32 *src = (Uint32 *)surface->pixels;
			Uint8 *lastline = dst;	/* end of last non-blank line */

			/* opaque counts are 8 or 16 bits, depending on target depth */
#define ADD_OPAQUE_COUNTS(n, m)			\
			if (df->BytesPerPixel == 4) {\
			((Uint16 *)dst)[0] = n;		\
			((Uint16 *)dst)[1] = m;		\
			dst += 4;				\
			}else {\
				dst[0] = n;				\
				dst[1] = m;				\
				dst += 2;				\
			}

			/* translucent counts are always 16 bit */
#define ADD_TRANSL_COUNTS(n, m)		\
	(((Uint16 *)dst)[0] = n, ((Uint16 *)dst)[1] = m, dst += 4)

			for (y = 0; y < h; y++) {
				int runstart, skipstart;
				int blankline = 0;
				/* First encode all opaque pixels of a scan line */
				x = 0;
				do {
					int run, skip, len;
					skipstart = x;
					while (x < w && !ISOPAQUE(src[x], sf))
						x++;
					runstart = x;
					while (x < w && ISOPAQUE(src[x], sf))
						x++;
					skip = runstart - skipstart;
					if (skip == w)
						blankline = 1;
					run = x - runstart;
					while (skip > max_opaque_run) {
						ADD_OPAQUE_COUNTS(max_opaque_run, 0);
						skip -= max_opaque_run;
					}
					len = MIN(run, max_opaque_run);
					ADD_OPAQUE_COUNTS(skip, len);
					dst += copy_opaque(dst, src + runstart, len, sf, df);
					runstart += len;
					run -= len;
					while (run) {
						len = MIN(run, max_opaque_run);
						ADD_OPAQUE_COUNTS(0, len);
						dst += copy_opaque(dst, src + runstart, len, sf, df);
						runstart += len;
						run -= len;
					}
				} while (x < w);

				/* Make sure the next output address is 32-bit aligned */
				dst += (uintptr_t)dst & 2;

				/* Next, encode all translucent pixels of the same scan line */
				x = 0;
				do {
					int run, skip, len;
					skipstart = x;
					while (x < w && !ISTRANSL(src[x], sf))
						x++;
					runstart = x;
					while (x < w && ISTRANSL(src[x], sf))
						x++;
					skip = runstart - skipstart;
					blankline &= (skip == w);
					run = x - runstart;
					while (skip > max_transl_run) {
						ADD_TRANSL_COUNTS(max_transl_run, 0);
						skip -= max_transl_run;
					}
					len = MIN(run, max_transl_run);
					ADD_TRANSL_COUNTS(skip, len);
					dst += copy_transl(dst, src + runstart, len, sf, df);
					runstart += len;
					run -= len;
					while (run) {
						len = MIN(run, max_transl_run);
						ADD_TRANSL_COUNTS(0, len);
						dst += copy_transl(dst, src + runstart, len, sf, df);
						runstart += len;
						run -= len;
					}
					if (!blankline)
						lastline = dst;
				} while (x < w);

				src += surface->pitch >> 2;
			}
			dst = lastline;		/* back up past trailing blank lines */
			ADD_OPAQUE_COUNTS(0, 0);
		}

#undef ADD_OPAQUE_COUNTS
#undef ADD_TRANSL_COUNTS

		/* Now that we have it encoded, release the original pixels */
		if ((surface->flags & SDL_PREALLOC) != SDL_PREALLOC
			&& (surface->flags & SDL_HWSURFACE) != SDL_HWSURFACE) {
			SDL_free(surface->pixels);
			surface->pixels = NULL;
		}

		/* realloc the buffer to release unused memory */
		{
			Uint8 *p = SDL_realloc(rlebuf, dst - rlebuf);
			if (!p)
				p = rlebuf;
			surface->map->sw_data->aux_data = p;
		}

		return 0;
	}

	int RLESurface(Surface *surface)
	{
		int retcode;

		/* Clear any previous RLE conversion */
		if ((surface->flags & RLEACCEL) == RLEACCEL) {
			UnRLESurface(surface, 1);
		}

		/* We don't support RLE encoding of bitmaps */
		if (surface->format->BitsPerPixel < 8) {
			return(-1);
		}

		/* Lock the surface if it's in hardware */
		if (SDL_MUSTLOCK(surface)) {
			if (LockSurface(surface) < 0) {
				return(-1);
			}
		}

		/* Encode */
		if ((surface->flags & SRCCOLORKEY) == SRCCOLORKEY) {
			retcode = RLEColorkeySurface(surface);
		}
		else {
			if ((surface->flags & SDL_SRCALPHA) == SDL_SRCALPHA
				&& surface->format->Amask != 0)
				retcode = RLEAlphaSurface(surface);
			else
				retcode = -1;	/* no RLE for per-surface alpha sans ckey */
		}

		/* Unlock the surface if it's in hardware */
		if (SDL_MUSTLOCK(surface)) {
			UnlockSurface(surface);
		}

		if (retcode < 0)
			return -1;

		/* The surface is now accelerated */
		surface->flags |= SDL_RLEACCEL;

		return(0);
	}

	/*
	* Un-RLE a surface with pixel alpha
	* This may not give back exactly the image before RLE-encoding; all
	* completely transparent pixels will be lost, and colour and alpha depth
	* may have been reduced (when encoding for 16bpp targets).
	*/
	static bool UnRLEAlpha(Surface *surface)
	{
		Uint8 *srcbuf;
		Uint32 *dst;
		PixelFormat *sf = surface->format;
		RLEDestFormat *df = (RLEDestFormat *)surface->map->sw_data->aux_data;

		int(*uncopy_opaque)(Uint32 *, void *, int,
			RLEDestFormat *, PixelFormat *);
		int(*uncopy_transl)(Uint32 *, void *, int,
			RLEDestFormat *, PixelFormat *);
		int w = surface->w;
		int bpp = df->BytesPerPixel;

		if (bpp == 2) {
			uncopy_opaque = uncopy_opaque_16;
			uncopy_transl = uncopy_transl_16;
		}
		else {
			uncopy_opaque = uncopy_transl = uncopy_32;
		}

		surface->pixels = malloc(surface->h * surface->pitch);
		if (!surface->pixels) {
			return(SDL_FALSE);
		}
		/* fill background with transparent pixels */
		memset(surface->pixels, 0, surface->h * surface->pitch);

		dst = (Uint32 *)surface->pixels;
		srcbuf = (Uint8 *)(df + 1);
		for (;;) {
			/* copy opaque pixels */
			int ofs = 0;
			do {
				unsigned run;
				if (bpp == 2) {
					ofs += srcbuf[0];
					run = srcbuf[1];
					srcbuf += 2;
				}
				else {
					ofs += ((Uint16 *)srcbuf)[0];
					run = ((Uint16 *)srcbuf)[1];
					srcbuf += 4;
				}
				if (run) {
					srcbuf += uncopy_opaque(dst + ofs, srcbuf, run, df, sf);
					ofs += run;
				}
				else if (!ofs)
					return(SDL_TRUE);
			} while (ofs < w);

			/* skip padding if needed */
			if (bpp == 2)
				srcbuf += (uintptr_t)srcbuf & 2;

			/* copy translucent pixels */
			ofs = 0;
			do {
				unsigned run;
				ofs += ((Uint16 *)srcbuf)[0];
				run = ((Uint16 *)srcbuf)[1];
				srcbuf += 4;
				if (run) {
					srcbuf += uncopy_transl(dst + ofs, srcbuf, run, df, sf);
					ofs += run;
				}
			} while (ofs < w);
			dst += surface->pitch >> 2;
		}
		/* Make the compiler happy */
		return(SDL_TRUE);
	}


	void UnRLESurface(Surface *surface, int recode)
	{
		if ((surface->flags & RLEACCEL) == RLEACCEL) {
			surface->flags &= ~RLEACCEL;

			if (recode && (surface->flags & PREALLOC) != PREALLOC
				&& (surface->flags & HWSURFACE) != HWSURFACE) {
				if ((surface->flags & SRCCOLORKEY) == SRCCOLORKEY) {
					SDL_Rect full;
					unsigned alpha_flag;

					/* re-create the original surface */
					surface->pixels = malloc(surface->h * surface->pitch);
					if (!surface->pixels) {
						/* Oh crap... */
						surface->flags |= RLEACCEL;
						return;
					}

					/* fill it with the background colour */
					SDL_FillRect(surface, NULL, surface->format->colorkey);

					/* now render the encoded surface */
					full.x = full.y = 0;
					full.w = surface->w;
					full.h = surface->h;
					alpha_flag = surface->flags & SDL_SRCALPHA;
					surface->flags &= ~SDL_SRCALPHA; /* opaque blit */
					SDL_RLEBlit(surface, &full, surface, &full);
					surface->flags |= alpha_flag;
				}
				else {
					if (!UnRLEAlpha(surface)) {
						/* Oh crap... */
						surface->flags |= SDL_RLEACCEL;
						return;
					}
				}
			}

			if (surface->map && surface->map->sw_data->aux_data) {
				free(surface->map->sw_data->aux_data);
				surface->map->sw_data->aux_data = NULL;
			}
		}
	}


	/*
	* Create an empty RGB surface of the appropriate depth
	*/
	Surface * CreateRGBSurface(Uint32 flags,
		int width, int height, int depth,
		Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
	{
		Surface *screen;
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

	/*
	* Free a surface created by the above function.
	*/
	void FreeSurface(Surface *surface)
	{
		/* Free anything that's not NULL, and not the screen surface */

		if (--surface->refcount > 0) {
			return;
		}
		while (surface->locked > 0) {
			UnlockSurface(surface);
		}
		if ((surface->flags & RLEACCEL) == RLEACCEL) {
			UnRLESurface(surface, 0);
		}
		if (surface->format) {
			FreeFormat(surface->format);
			surface->format = NULL;
		}
		if (surface->map != NULL) {
			FreeBlitMap(surface->map);
			surface->map = NULL;
		}
		if (surface->hwdata) {
			SDL_VideoDevice *video = current_video;
			SDL_VideoDevice *this = current_video;
			video->FreeHWSurface(this, surface);
		}
		if (surface->pixels &&
			((surface->flags & PREALLOC) != PREALLOC)) {
			free(surface->pixels);
		}
		free(surface);
	}

	/*
	* Lock a surface to directly access the pixels
	*/
	int LockSurface(Surface *surface)
	{
		if (!surface->locked) {
			/* Perform the lock */
			if (surface->flags & (HWSURFACE | ASYNCBLIT)) {
				SDL_VideoDevice *video = current_video;
				SDL_VideoDevice *this = current_video;
				if (video->LockHWSurface(this, surface) < 0) {
					return(-1);
				}
			}
			if (surface->flags & RLEACCEL) {
				UnRLESurface(surface, 1);
				surface->flags |= RLEACCEL;	/* save accel'd state */
			}
			/* This needs to be done here in case pixels changes value */
			surface->pixels = (Uint8 *)surface->pixels + surface->offset;
		}

		/* Increment the surface lock count, for recursive locks */
		++surface->locked;

		/* Ready to go.. */
		return(0);
	}

	/*
	* Unlock a previously locked surface
	*/
	void UnlockSurface(Surface *surface)
	{
		/* Only perform an unlock if we are locked */
		if (!surface->locked || (--surface->locked > 0)) {
			return;
		}

		/* Perform the unlock */
		surface->pixels = (Uint8 *)surface->pixels - surface->offset;

		/* Unlock hardware or accelerated surfaces */
		if (surface->flags & (HWSURFACE | ASYNCBLIT)) {
			SDL_VideoDevice *video = current_video;
			SDL_VideoDevice *this = current_video;
			video->UnlockHWSurface(this, surface);
		}
		else {
			/* Update RLE encoded surface with new data */
			if ((surface->flags & RLEACCEL) == RLEACCEL) {
				surface->flags &= ~RLEACCEL; /* stop lying */
				RLESurface(surface);
			}
		}
	}

}