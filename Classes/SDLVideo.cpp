#include "SDLImp.h"
#if __cplusplus
extern "C" {
#endif
#include <SDL.h>
#if __cplusplus
}
#endif

namespace ff{

	void SDLog(const char *log)
	{
	}
	Surface *CreateRGBSurface(int width, int height)
	{
		return (Surface*)SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 24, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	}
	Surface * SetVideoMode(int width, int height, int bpp, Uint32 flags)
	{
		//return (Surface*)SDL_SetVideoMode(width, height, bpp, flags);
		return nullptr; 
	}
	int LockYUVOverlay(Overlay *overlay)
	{
		//return SDL_LockYUVOverlay((SDL_Overlay*)overlay);
		//软件Overlay不需要锁定
		return 0;
	}

	void UnlockYUVOverlay(Overlay *overlay)
	{
		//SDL_UnlockYUVOverlay((SDL_Overlay*)overlay);
	}

	int DisplayYUVOverlay(Overlay *overlay, Rect *dstrect)
	{
		return SDL_DisplayYUVOverlay((SDL_Overlay*)overlay, (SDL_Rect*)dstrect);
		//软件模式不需要，但是该函数给出一个更新节点
		return 0;
	}

	void FreeYUVOverlay(Overlay *overlay)
	{
		if (overlay == NULL) {
			return;
		}
		/* 软件模式省略
		if (overlay->hwfuncs) {
			overlay->hwfuncs->FreeHW(current_video, overlay);
		}
		*/
		free(overlay);
		//SDL_FreeYUVOverlay((SDL_Overlay*)overlay);
	}

	/*
	* How many 1 bits are there in the Uint32.
	* Low performance, do not call often.
	*/
	static int number_of_bits_set(Uint32 a)
	{
		if (!a) return 0;
		if (a & 1) return 1 + number_of_bits_set(a >> 1);
		return(number_of_bits_set(a >> 1));
	}

	/*
	* How many 0 bits are there at least significant end of Uint32.
	* Low performance, do not call often.
	*/
	static int free_bits_at_bottom(Uint32 a)
	{
		/* assume char is 8 bits */
		if (!a) return sizeof(Uint32)* 8;
		if (((Sint32)a) & 1l) return 0;
		return 1 + free_bits_at_bottom(a >> 1);
	}

	static void Color16DitherYV12Mod1X(int *colortab, Uint32 *rgb_2_pix,
		unsigned char *lum, unsigned char *cr,
		unsigned char *cb, unsigned char *out,
		int rows, int cols, int mod)
	{
		unsigned short* row1;
		unsigned short* row2;
		unsigned char* lum2;
		int x, y;
		int cr_r;
		int crb_g;
		int cb_b;
		int cols_2 = cols / 2;

		row1 = (unsigned short*)out;
		row2 = row1 + cols + mod;
		lum2 = lum + cols;

		mod += cols + mod;

		y = rows / 2;
		while (y--)
		{
			x = cols_2;
			while (x--)
			{
				register int L;

				cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
				crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
					+ colortab[*cb + 2 * 256];
				cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
				++cr; ++cb;

				L = *lum++;
				*row1++ = (unsigned short)(rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);

				L = *lum++;
				*row1++ = (unsigned short)(rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);


				/* Now, do second row.  */

				L = *lum2++;
				*row2++ = (unsigned short)(rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);

				L = *lum2++;
				*row2++ = (unsigned short)(rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
			}

			/*
			* These values are at the start of the next line, (due
			* to the ++'s above),but they need to be at the start
			* of the line after that.
			*/
			lum += cols;
			lum2 += cols;
			row1 += mod;
			row2 += mod;
		}
	}
	/*
	* In this function I make use of a nasty trick. The tables have the lower
	* 16 bits replicated in the upper 16. This means I can write ints and get
	* the horisontal doubling for free (almost).
	*/
	static void Color16DitherYV12Mod2X(int *colortab, Uint32 *rgb_2_pix,
		unsigned char *lum, unsigned char *cr,
		unsigned char *cb, unsigned char *out,
		int rows, int cols, int mod)
	{
		unsigned int* row1 = (unsigned int*)out;
		const int next_row = cols + (mod / 2);
		unsigned int* row2 = row1 + 2 * next_row;
		unsigned char* lum2;
		int x, y;
		int cr_r;
		int crb_g;
		int cb_b;
		int cols_2 = cols / 2;

		lum2 = lum + cols;

		mod = (next_row * 3) + (mod / 2);

		y = rows / 2;
		while (y--)
		{
			x = cols_2;
			while (x--)
			{
				register int L;

				cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
				crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
					+ colortab[*cb + 2 * 256];
				cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
				++cr; ++cb;

				L = *lum++;
				row1[0] = row1[next_row] = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row1++;

				L = *lum++;
				row1[0] = row1[next_row] = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row1++;


				/* Now, do second row. */

				L = *lum2++;
				row2[0] = row2[next_row] = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row2++;

				L = *lum2++;
				row2[0] = row2[next_row] = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row2++;
			}

			/*
			* These values are at the start of the next line, (due
			* to the ++'s above),but they need to be at the start
			* of the line after that.
			*/
			lum += cols;
			lum2 += cols;
			row1 += mod;
			row2 += mod;
		}
	}
	static void Color24DitherYV12Mod1X(int *colortab, Uint32 *rgb_2_pix,
		unsigned char *lum, unsigned char *cr,
		unsigned char *cb, unsigned char *out,
		int rows, int cols, int mod)
	{
		unsigned int value;
		unsigned char* row1;
		unsigned char* row2;
		unsigned char* lum2;
		int x, y;
		int cr_r;
		int crb_g;
		int cb_b;
		int cols_2 = cols / 2;

		row1 = out;
		row2 = row1 + cols * 3 + mod * 3;
		lum2 = lum + cols;

		mod += cols + mod;
		mod *= 3;

		y = rows / 2;
		while (y--)
		{
			x = cols_2;
			while (x--)
			{
				register int L;

				cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
				crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
					+ colortab[*cb + 2 * 256];
				cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
				++cr; ++cb;

				L = *lum++;
				value = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				*row1++ = (value)& 0xFF;
				*row1++ = (value >> 8) & 0xFF;
				*row1++ = (value >> 16) & 0xFF;

				L = *lum++;
				value = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				*row1++ = (value)& 0xFF;
				*row1++ = (value >> 8) & 0xFF;
				*row1++ = (value >> 16) & 0xFF;


				/* Now, do second row.  */

				L = *lum2++;
				value = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				*row2++ = (value)& 0xFF;
				*row2++ = (value >> 8) & 0xFF;
				*row2++ = (value >> 16) & 0xFF;

				L = *lum2++;
				value = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				*row2++ = (value)& 0xFF;
				*row2++ = (value >> 8) & 0xFF;
				*row2++ = (value >> 16) & 0xFF;
			}

			/*
			* These values are at the start of the next line, (due
			* to the ++'s above),but they need to be at the start
			* of the line after that.
			*/
			lum += cols;
			lum2 += cols;
			row1 += mod;
			row2 += mod;
		}
	}
	static void Color24DitherYV12Mod2X(int *colortab, Uint32 *rgb_2_pix,
		unsigned char *lum, unsigned char *cr,
		unsigned char *cb, unsigned char *out,
		int rows, int cols, int mod)
	{
		unsigned int value;
		unsigned char* row1 = out;
		const int next_row = (cols * 2 + mod) * 3;
		unsigned char* row2 = row1 + 2 * next_row;
		unsigned char* lum2;
		int x, y;
		int cr_r;
		int crb_g;
		int cb_b;
		int cols_2 = cols / 2;

		lum2 = lum + cols;

		mod = next_row * 3 + mod * 3;

		y = rows / 2;
		while (y--)
		{
			x = cols_2;
			while (x--)
			{
				register int L;

				cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
				crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
					+ colortab[*cb + 2 * 256];
				cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
				++cr; ++cb;

				L = *lum++;
				value = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row1[0 + 0] = row1[3 + 0] = row1[next_row + 0] = row1[next_row + 3 + 0] =
					(value)& 0xFF;
				row1[0 + 1] = row1[3 + 1] = row1[next_row + 1] = row1[next_row + 3 + 1] =
					(value >> 8) & 0xFF;
				row1[0 + 2] = row1[3 + 2] = row1[next_row + 2] = row1[next_row + 3 + 2] =
					(value >> 16) & 0xFF;
				row1 += 2 * 3;

				L = *lum++;
				value = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row1[0 + 0] = row1[3 + 0] = row1[next_row + 0] = row1[next_row + 3 + 0] =
					(value)& 0xFF;
				row1[0 + 1] = row1[3 + 1] = row1[next_row + 1] = row1[next_row + 3 + 1] =
					(value >> 8) & 0xFF;
				row1[0 + 2] = row1[3 + 2] = row1[next_row + 2] = row1[next_row + 3 + 2] =
					(value >> 16) & 0xFF;
				row1 += 2 * 3;


				/* Now, do second row. */

				L = *lum2++;
				value = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row2[0 + 0] = row2[3 + 0] = row2[next_row + 0] = row2[next_row + 3 + 0] =
					(value)& 0xFF;
				row2[0 + 1] = row2[3 + 1] = row2[next_row + 1] = row2[next_row + 3 + 1] =
					(value >> 8) & 0xFF;
				row2[0 + 2] = row2[3 + 2] = row2[next_row + 2] = row2[next_row + 3 + 2] =
					(value >> 16) & 0xFF;
				row2 += 2 * 3;

				L = *lum2++;
				value = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row2[0 + 0] = row2[3 + 0] = row2[next_row + 0] = row2[next_row + 3 + 0] =
					(value)& 0xFF;
				row2[0 + 1] = row2[3 + 1] = row2[next_row + 1] = row2[next_row + 3 + 1] =
					(value >> 8) & 0xFF;
				row2[0 + 2] = row2[3 + 2] = row2[next_row + 2] = row2[next_row + 3 + 2] =
					(value >> 16) & 0xFF;
				row2 += 2 * 3;
			}

			/*
			* These values are at the start of the next line, (due
			* to the ++'s above),but they need to be at the start
			* of the line after that.
			*/
			lum += cols;
			lum2 += cols;
			row1 += mod;
			row2 += mod;
		}
	}

	static void Color32DitherYV12Mod1X(int *colortab, Uint32 *rgb_2_pix,
		unsigned char *lum, unsigned char *cr,
		unsigned char *cb, unsigned char *out,
		int rows, int cols, int mod)
	{
		unsigned int* row1;
		unsigned int* row2;
		unsigned char* lum2;
		int x, y;
		int cr_r;
		int crb_g;
		int cb_b;
		int cols_2 = cols / 2;

		row1 = (unsigned int*)out;
		row2 = row1 + cols + mod;
		lum2 = lum + cols;

		mod += cols + mod;

		y = rows / 2;
		while (y--)
		{
			x = cols_2;
			while (x--)
			{
				register int L;

				cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
				crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
					+ colortab[*cb + 2 * 256];
				cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
				++cr; ++cb;

				L = *lum++;
				*row1++ = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);

				L = *lum++;
				*row1++ = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);


				/* Now, do second row.  */

				L = *lum2++;
				*row2++ = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);

				L = *lum2++;
				*row2++ = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
			}

			/*
			* These values are at the start of the next line, (due
			* to the ++'s above),but they need to be at the start
			* of the line after that.
			*/
			lum += cols;
			lum2 += cols;
			row1 += mod;
			row2 += mod;
		}
	}
	static void Color32DitherYV12Mod2X(int *colortab, Uint32 *rgb_2_pix,
		unsigned char *lum, unsigned char *cr,
		unsigned char *cb, unsigned char *out,
		int rows, int cols, int mod)
	{
		unsigned int* row1 = (unsigned int*)out;
		const int next_row = cols * 2 + mod;
		unsigned int* row2 = row1 + 2 * next_row;
		unsigned char* lum2;
		int x, y;
		int cr_r;
		int crb_g;
		int cb_b;
		int cols_2 = cols / 2;

		lum2 = lum + cols;

		mod = (next_row * 3) + mod;

		y = rows / 2;
		while (y--)
		{
			x = cols_2;
			while (x--)
			{
				register int L;

				cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
				crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
					+ colortab[*cb + 2 * 256];
				cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
				++cr; ++cb;

				L = *lum++;
				row1[0] = row1[1] = row1[next_row] = row1[next_row + 1] =
					(rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row1 += 2;

				L = *lum++;
				row1[0] = row1[1] = row1[next_row] = row1[next_row + 1] =
					(rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row1 += 2;


				/* Now, do second row. */

				L = *lum2++;
				row2[0] = row2[1] = row2[next_row] = row2[next_row + 1] =
					(rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row2 += 2;

				L = *lum2++;
				row2[0] = row2[1] = row2[next_row] = row2[next_row + 1] =
					(rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row2 += 2;
			}

			/*
			* These values are at the start of the next line, (due
			* to the ++'s above),but they need to be at the start
			* of the line after that.
			*/
			lum += cols;
			lum2 += cols;
			row1 += mod;
			row2 += mod;
		}
	}
	static void Color16DitherYUY2Mod1X(int *colortab, Uint32 *rgb_2_pix,
		unsigned char *lum, unsigned char *cr,
		unsigned char *cb, unsigned char *out,
		int rows, int cols, int mod)
	{
		unsigned short* row;
		int x, y;
		int cr_r;
		int crb_g;
		int cb_b;
		int cols_2 = cols / 2;

		row = (unsigned short*)out;

		y = rows;
		while (y--)
		{
			x = cols_2;
			while (x--)
			{
				register int L;

				cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
				crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
					+ colortab[*cb + 2 * 256];
				cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
				cr += 4; cb += 4;

				L = *lum; lum += 2;
				*row++ = (unsigned short)(rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);

				L = *lum; lum += 2;
				*row++ = (unsigned short)(rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);

			}

			row += mod;
		}
	}

	/*
	* In this function I make use of a nasty trick. The tables have the lower
	* 16 bits replicated in the upper 16. This means I can write ints and get
	* the horisontal doubling for free (almost).
	*/
	static void Color16DitherYUY2Mod2X(int *colortab, Uint32 *rgb_2_pix,
		unsigned char *lum, unsigned char *cr,
		unsigned char *cb, unsigned char *out,
		int rows, int cols, int mod)
	{
		unsigned int* row = (unsigned int*)out;
		const int next_row = cols + (mod / 2);
		int x, y;
		int cr_r;
		int crb_g;
		int cb_b;
		int cols_2 = cols / 2;

		y = rows;
		while (y--)
		{
			x = cols_2;
			while (x--)
			{
				register int L;

				cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
				crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
					+ colortab[*cb + 2 * 256];
				cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
				cr += 4; cb += 4;

				L = *lum; lum += 2;
				row[0] = row[next_row] = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row++;

				L = *lum; lum += 2;
				row[0] = row[next_row] = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row++;

			}
			row += next_row;
		}
	}


	static void Color24DitherYUY2Mod1X(int *colortab, Uint32 *rgb_2_pix,
		unsigned char *lum, unsigned char *cr,
		unsigned char *cb, unsigned char *out,
		int rows, int cols, int mod)
	{
		unsigned int value;
		unsigned char* row;
		int x, y;
		int cr_r;
		int crb_g;
		int cb_b;
		int cols_2 = cols / 2;

		row = (unsigned char*)out;
		mod *= 3;
		y = rows;
		while (y--)
		{
			x = cols_2;
			while (x--)
			{
				register int L;

				cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
				crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
					+ colortab[*cb + 2 * 256];
				cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
				cr += 4; cb += 4;

				L = *lum; lum += 2;
				value = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				*row++ = (value)& 0xFF;
				*row++ = (value >> 8) & 0xFF;
				*row++ = (value >> 16) & 0xFF;

				L = *lum; lum += 2;
				value = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				*row++ = (value)& 0xFF;
				*row++ = (value >> 8) & 0xFF;
				*row++ = (value >> 16) & 0xFF;

			}
			row += mod;
		}
	}

	static void Color24DitherYUY2Mod2X(int *colortab, Uint32 *rgb_2_pix,
		unsigned char *lum, unsigned char *cr,
		unsigned char *cb, unsigned char *out,
		int rows, int cols, int mod)
	{
		unsigned int value;
		unsigned char* row = out;
		const int next_row = (cols * 2 + mod) * 3;
		int x, y;
		int cr_r;
		int crb_g;
		int cb_b;
		int cols_2 = cols / 2;
		y = rows;
		while (y--)
		{
			x = cols_2;
			while (x--)
			{
				register int L;

				cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
				crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
					+ colortab[*cb + 2 * 256];
				cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
				cr += 4; cb += 4;

				L = *lum; lum += 2;
				value = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row[0 + 0] = row[3 + 0] = row[next_row + 0] = row[next_row + 3 + 0] =
					(value)& 0xFF;
				row[0 + 1] = row[3 + 1] = row[next_row + 1] = row[next_row + 3 + 1] =
					(value >> 8) & 0xFF;
				row[0 + 2] = row[3 + 2] = row[next_row + 2] = row[next_row + 3 + 2] =
					(value >> 16) & 0xFF;
				row += 2 * 3;

				L = *lum; lum += 2;
				value = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row[0 + 0] = row[3 + 0] = row[next_row + 0] = row[next_row + 3 + 0] =
					(value)& 0xFF;
				row[0 + 1] = row[3 + 1] = row[next_row + 1] = row[next_row + 3 + 1] =
					(value >> 8) & 0xFF;
				row[0 + 2] = row[3 + 2] = row[next_row + 2] = row[next_row + 3 + 2] =
					(value >> 16) & 0xFF;
				row += 2 * 3;

			}
			row += next_row;
		}
	}

	static void Color32DitherYUY2Mod1X(int *colortab, Uint32 *rgb_2_pix,
		unsigned char *lum, unsigned char *cr,
		unsigned char *cb, unsigned char *out,
		int rows, int cols, int mod)
	{
		unsigned int* row;
		int x, y;
		int cr_r;
		int crb_g;
		int cb_b;
		int cols_2 = cols / 2;

		row = (unsigned int*)out;
		y = rows;
		while (y--)
		{
			x = cols_2;
			while (x--)
			{
				register int L;

				cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
				crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
					+ colortab[*cb + 2 * 256];
				cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
				cr += 4; cb += 4;

				L = *lum; lum += 2;
				*row++ = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);

				L = *lum; lum += 2;
				*row++ = (rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);


			}
			row += mod;
		}
	}

	static void Color32DitherYUY2Mod2X(int *colortab, Uint32 *rgb_2_pix,
		unsigned char *lum, unsigned char *cr,
		unsigned char *cb, unsigned char *out,
		int rows, int cols, int mod)
	{
		unsigned int* row = (unsigned int*)out;
		const int next_row = cols * 2 + mod;
		int x, y;
		int cr_r;
		int crb_g;
		int cb_b;
		int cols_2 = cols / 2;
		mod += mod;
		y = rows;
		while (y--)
		{
			x = cols_2;
			while (x--)
			{
				register int L;

				cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
				crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
					+ colortab[*cb + 2 * 256];
				cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
				cr += 4; cb += 4;

				L = *lum; lum += 2;
				row[0] = row[1] = row[next_row] = row[next_row + 1] =
					(rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row += 2;

				L = *lum; lum += 2;
				row[0] = row[1] = row[next_row] = row[next_row + 1] =
					(rgb_2_pix[L + cr_r] |
					rgb_2_pix[L + crb_g] |
					rgb_2_pix[L + cb_b]);
				row += 2;


			}

			row += next_row;
		}
	}

	/*
	Overlay * CreateYUVOverlay(int width, int height,
		Uint32 format, Surface *display)
	{
		return (Overlay*)SDL_CreateYUVOverlay(width, height, format, (SDL_Surface*)display);
	}
	*/
	/*
		代码乃至于SDL_CreateYUV_SW
	*/
	Overlay * CreateYUVOverlay(int width, int height,
		Uint32 format, Surface *display)
	{
		Overlay *overlay;
		struct private_yuvhwdata *swdata;
		int *Cr_r_tab;
		int *Cr_g_tab;
		int *Cb_g_tab;
		int *Cb_b_tab;
		Uint32 *r_2_pix_alloc;
		Uint32 *g_2_pix_alloc;
		Uint32 *b_2_pix_alloc;
		int i;
		int CR, CB;
		Uint32 Rmask, Gmask, Bmask;
		/* Only RGB packed pixel conversion supported */
		if ((display->format->BytesPerPixel != 2) &&
			(display->format->BytesPerPixel != 3) &&
			(display->format->BytesPerPixel != 4)) {
			SDLog("Can't use YUV data on non 16/24/32 bit surfaces");
			return(NULL);
		}
		/* Verify that we support the format */
		switch (format) {
		case YV12_OVERLAY:
		case IYUV_OVERLAY:
		case YUY2_OVERLAY:
		case UYVY_OVERLAY:
		case YVYU_OVERLAY:
			break;
		default:
			SDLog("Unsupported YUV format");
			return(NULL);
		}

		/* Create the overlay structure */
		overlay = (Overlay *)malloc(sizeof *overlay);
		if (overlay == NULL) {
			SDL_OutOfMemory();
			return(NULL);
		}
		memset(overlay, 0, (sizeof *overlay));

		/* Fill in the basic members */
		overlay->format = format;
		overlay->w = width;
		overlay->h = height;

		/* Set up the YUV surface function structure */
		//省略
		//overlay->hwfuncs = &sw_yuvfuncs;

		/* Create the pixel data and lookup tables */
		swdata = (struct private_yuvhwdata *)malloc(sizeof *swdata);
		overlay->hwdata = swdata;
		if (swdata == NULL) {
			SDL_OutOfMemory();
			FreeYUVOverlay(overlay);
			return(NULL);
		}
		swdata->stretch = NULL;
		swdata->display = display;
		swdata->pixels = (Uint8 *)malloc(width*height * 2);
		swdata->colortab = (int *)malloc(4 * 256 * sizeof(int));
		Cr_r_tab = &swdata->colortab[0 * 256];
		Cr_g_tab = &swdata->colortab[1 * 256];
		Cb_g_tab = &swdata->colortab[2 * 256];
		Cb_b_tab = &swdata->colortab[3 * 256];
		swdata->rgb_2_pix = (Uint32 *)malloc(3 * 768 * sizeof(Uint32));
		r_2_pix_alloc = &swdata->rgb_2_pix[0 * 768];
		g_2_pix_alloc = &swdata->rgb_2_pix[1 * 768];
		b_2_pix_alloc = &swdata->rgb_2_pix[2 * 768];
		if (!swdata->pixels || !swdata->colortab || !swdata->rgb_2_pix) {
			SDL_OutOfMemory();
			FreeYUVOverlay(overlay);
			return(NULL);
		}

		/* Generate the tables for the display surface */
		for (i = 0; i<256; i++) {
			/* Gamma correction (luminescence table) and chroma correction
			would be done here.  See the Berkeley mpeg_play sources.
			*/
			CB = CR = (i - 128);
			Cr_r_tab[i] = (int)((0.419 / 0.299) * CR);
			Cr_g_tab[i] = (int)(-(0.299 / 0.419) * CR);
			Cb_g_tab[i] = (int)(-(0.114 / 0.331) * CB);
			Cb_b_tab[i] = (int)((0.587 / 0.331) * CB);
		}

		/*
		* Set up entries 0-255 in rgb-to-pixel value tables.
		*/
		Rmask = display->format->Rmask;
		Gmask = display->format->Gmask;
		Bmask = display->format->Bmask;
		for (i = 0; i<256; ++i) {
			r_2_pix_alloc[i + 256] = i >> (8 - number_of_bits_set(Rmask));
			r_2_pix_alloc[i + 256] <<= free_bits_at_bottom(Rmask);
			g_2_pix_alloc[i + 256] = i >> (8 - number_of_bits_set(Gmask));
			g_2_pix_alloc[i + 256] <<= free_bits_at_bottom(Gmask);
			b_2_pix_alloc[i + 256] = i >> (8 - number_of_bits_set(Bmask));
			b_2_pix_alloc[i + 256] <<= free_bits_at_bottom(Bmask);
		}

		/*
		* If we have 16-bit output depth, then we double the value
		* in the top word. This means that we can write out both
		* pixels in the pixel doubling mode with one op. It is
		* harmless in the normal case as storing a 32-bit value
		* through a short pointer will lose the top bits anyway.
		*/
		if (display->format->BytesPerPixel == 2) {
			for (i = 0; i<256; ++i) {
				r_2_pix_alloc[i + 256] |= (r_2_pix_alloc[i + 256]) << 16;
				g_2_pix_alloc[i + 256] |= (g_2_pix_alloc[i + 256]) << 16;
				b_2_pix_alloc[i + 256] |= (b_2_pix_alloc[i + 256]) << 16;
			}
		}

		/*
		* Spread out the values we have to the rest of the array so that
		* we do not need to check for overflow.
		*/
		for (i = 0; i<256; ++i) {
			r_2_pix_alloc[i] = r_2_pix_alloc[256];
			r_2_pix_alloc[i + 512] = r_2_pix_alloc[511];
			g_2_pix_alloc[i] = g_2_pix_alloc[256];
			g_2_pix_alloc[i + 512] = g_2_pix_alloc[511];
			b_2_pix_alloc[i] = b_2_pix_alloc[256];
			b_2_pix_alloc[i + 512] = b_2_pix_alloc[511];
		}

		/* You have chosen wisely... */
		switch (format) {
		case YV12_OVERLAY:
		case IYUV_OVERLAY:
			if (display->format->BytesPerPixel == 2) {
				swdata->Display1X = Color16DitherYV12Mod1X;
				swdata->Display2X = Color16DitherYV12Mod2X;
			}
			if (display->format->BytesPerPixel == 3) {
				swdata->Display1X = Color24DitherYV12Mod1X;
				swdata->Display2X = Color24DitherYV12Mod2X;
			}
			if (display->format->BytesPerPixel == 4) {
				swdata->Display1X = Color32DitherYV12Mod1X;
				swdata->Display2X = Color32DitherYV12Mod2X;
			}
			break;
		case YUY2_OVERLAY:
		case UYVY_OVERLAY:
		case YVYU_OVERLAY:
			if (display->format->BytesPerPixel == 2) {
				swdata->Display1X = Color16DitherYUY2Mod1X;
				swdata->Display2X = Color16DitherYUY2Mod2X;
			}
			if (display->format->BytesPerPixel == 3) {
				swdata->Display1X = Color24DitherYUY2Mod1X;
				swdata->Display2X = Color24DitherYUY2Mod2X;
			}
			if (display->format->BytesPerPixel == 4) {
				swdata->Display1X = Color32DitherYUY2Mod1X;
				swdata->Display2X = Color32DitherYUY2Mod2X;
			}
			break;
		default:
			/* We should never get here (caught above) */
			break;
		}

		/* Find the pitch and offset values for the overlay */
		overlay->pitches = swdata->pitches;
		overlay->pixels = swdata->planes;
		switch (format) {
		case YV12_OVERLAY:
		case IYUV_OVERLAY:
			overlay->pitches[0] = overlay->w;
			overlay->pitches[1] = overlay->pitches[0] / 2;
			overlay->pitches[2] = overlay->pitches[0] / 2;
			overlay->pixels[0] = swdata->pixels;
			overlay->pixels[1] = overlay->pixels[0] +
				overlay->pitches[0] * overlay->h;
			overlay->pixels[2] = overlay->pixels[1] +
				overlay->pitches[1] * overlay->h / 2;
			overlay->planes = 3;
			break;
		case YUY2_OVERLAY:
		case UYVY_OVERLAY:
		case YVYU_OVERLAY:
			overlay->pitches[0] = overlay->w * 2;
			overlay->pixels[0] = swdata->pixels;
			overlay->planes = 1;
			break;
		default:
			/* We should never get here (caught above) */
			break;
		}

		/* We're all done.. */
		return(overlay);
	}
}