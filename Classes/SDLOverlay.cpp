#include "SDLImp.h"

/*
	SDL Overlay 软实现
*/
namespace ff{
	int DisplayYUV_SW(Overlay *overlay, Rect *src, Rect *dst);

	int LockYUV_SW(Overlay *overlay)
	{
		return(0);
	}

	void UnlockYUV_SW(Overlay *overlay)
	{
		return;
	}

	void FreeYUV_SW(Overlay *overlay)
	{
		struct private_yuvhwdata *swdata;

		swdata = overlay->hwdata;
		if (swdata) {
			if (swdata->stretch) {
				FreeSurface(swdata->stretch);
			}
			if (swdata->pixels) {
				free(swdata->pixels);
			}
			if (swdata->colortab) {
				free(swdata->colortab);
			}
			if (swdata->rgb_2_pix) {
				free(swdata->rgb_2_pix);
			}
			free(swdata);
			overlay->hwdata = NULL;
		}
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
	/* The functions used to manipulate software video overlays */
	static struct private_yuvhwfuncs sw_yuvfuncs = {
		LockYUV_SW,
		UnlockYUV_SW,
		DisplayYUV_SW,
		FreeYUV_SW
	};
	/* 打开USE_ASM_STRETCH 不能正常编译 */
	//#define USE_ASM_STRETCH
#ifdef USE_ASM_STRETCH

#ifdef HAVE_MPROTECT
#include <sys/types.h>
#include <sys/mman.h>
#endif
#ifdef __GNUC__
#define PAGE_ALIGNED __attribute__((__aligned__(4096)))
#else
#define PAGE_ALIGNED
#endif

#if defined(_M_IX86) || defined(i386)
#define PREFIX16	0x66
#define STORE_BYTE	0xAA
#define STORE_WORD	0xAB
#define LOAD_BYTE	0xAC
#define LOAD_WORD	0xAD
#define RETURN		0xC3
#else
#error Need assembly opcodes for this architecture
#endif

	static unsigned char copy_row[4096] PAGE_ALIGNED;

	static int generate_rowbytes(int src_w, int dst_w, int bpp)
	{
		static struct {
			int bpp;
			int src_w;
			int dst_w;
			int status;
		} last;

		int i;
		int pos, inc;
		unsigned char *eip, *fence;
		unsigned char load, store;

		/* See if we need to regenerate the copy buffer */
		if ((src_w == last.src_w) &&
			(dst_w == last.dst_w) && (bpp == last.bpp)) {
			return(last.status);
		}
		last.bpp = bpp;
		last.src_w = src_w;
		last.dst_w = dst_w;
		last.status = -1;

		switch (bpp) {
		case 1:
			load = LOAD_BYTE;
			store = STORE_BYTE;
			break;
		case 2:
		case 4:
			load = LOAD_WORD;
			store = STORE_WORD;
			break;
		default:
			SDLog("ASM stretch of %d bytes isn't supported\n");
			return(-1);
		}
#ifdef HAVE_MPROTECT
		/* Make the code writeable */
		if (mprotect(copy_row, sizeof(copy_row), PROT_READ | PROT_WRITE) < 0) {
			SDLog("Couldn't make copy buffer writeable");
			return(-1);
		}
#endif
		pos = 0x10000;
		inc = (src_w << 16) / dst_w;
		eip = copy_row;
		fence = copy_row + sizeof(copy_row) - 2;
		for (i = 0; i<dst_w && eip < end; ++i) {
			while (pos >= 0x10000L) {
				if (eip == fence) {
					return -1;
				}
				if (bpp == 2) {
					*eip++ = PREFIX16;
				}
				*eip++ = load;
				pos -= 0x10000L;
			}
			if (eip == fence) {
				return -1;
			}
			if (bpp == 2) {
				*eip++ = PREFIX16;
			}
			*eip++ = store;
			pos += inc;
		}
		*eip++ = RETURN;

#ifdef HAVE_MPROTECT
		/* Make the code executable but not writeable */
		if (mprotect(copy_row, sizeof(copy_row), PROT_READ | PROT_EXEC) < 0) {
			SDLog("Couldn't make copy buffer executable");
			return(-1);
		}
#endif
		last.status = 0;
		return(0);
	}

#endif /* USE_ASM_STRETCH */

#define DEFINE_COPY_ROW(name, type)			\
void name(type *src, int src_w, type *dst, int dst_w)	\
		{							\
	int i;						\
	int pos, inc;					\
	type pixel = 0;					\
							\
	pos = 0x10000;					\
	inc = (src_w << 16) / dst_w;			\
	for ( i=dst_w; i>0; --i ) {			\
			while ( pos >= 0x10000L ) {		\
			pixel = *src++;			\
			pos -= 0x10000L;		\
					}					\
		*dst++ = pixel;				\
		pos += inc;				\
		}						\
		}
	DEFINE_COPY_ROW(copy_row1, Uint8)
		DEFINE_COPY_ROW(copy_row2, Uint16)
		DEFINE_COPY_ROW(copy_row4, Uint32)

		/* The ASM code doesn't handle 24-bpp stretch blits */
		void copy_row3(Uint8 *src, int src_w, Uint8 *dst, int dst_w)
	{
		int i;
		int pos, inc;
		Uint8 pixel[3] = { 0, 0, 0 };

		pos = 0x10000;
		inc = (src_w << 16) / dst_w;
		for (i = dst_w; i>0; --i) {
			while (pos >= 0x10000L) {
				pixel[0] = *src++;
				pixel[1] = *src++;
				pixel[2] = *src++;
				pos -= 0x10000L;
			}
			*dst++ = pixel[0];
			*dst++ = pixel[1];
			*dst++ = pixel[2];
			pos += inc;
		}
	}

	/* Perform a stretch blit between two surfaces of the same format.
	NOTE:  This function is not safe to call from multiple threads!
	*/
	static int SoftStretch(Surface *src, Rect *srcrect,
		Surface *dst, Rect *dstrect)
	{
		int src_locked;
		int dst_locked;
		int pos, inc;
		int dst_maxrow;
		int src_row, dst_row;
		Uint8 *srcp = NULL;
		Uint8 *dstp;
		Rect full_src;
		Rect full_dst;
#ifdef USE_ASM_STRETCH
		bool use_asm = true;
#ifdef __GNUC__
		int u1, u2;
#endif
#endif /* USE_ASM_STRETCH */
		const int bpp = dst->format->BytesPerPixel;

		if (src->format->BitsPerPixel != dst->format->BitsPerPixel) {
			SDLog("Only works with same format surfaces");
			return(-1);
		}

		/* Verify the blit rectangles */
		if (srcrect) {
			if ((srcrect->x < 0) || (srcrect->y < 0) ||
				((srcrect->x + srcrect->w) > src->w) ||
				((srcrect->y + srcrect->h) > src->h)) {
				SDLog("Invalid source blit rectangle");
				return(-1);
			}
		}
		else {
			full_src.x = 0;
			full_src.y = 0;
			full_src.w = src->w;
			full_src.h = src->h;
			srcrect = &full_src;
		}
		if (dstrect) {
			if ((dstrect->x < 0) || (dstrect->y < 0) ||
				((dstrect->x + dstrect->w) > dst->w) ||
				((dstrect->y + dstrect->h) > dst->h)) {
				SDLog("Invalid destination blit rectangle");
				return(-1);
			}
		}
		else {
			full_dst.x = 0;
			full_dst.y = 0;
			full_dst.w = dst->w;
			full_dst.h = dst->h;
			dstrect = &full_dst;
		}

		/* Lock the destination if it's in hardware */
		dst_locked = 0;
		if (MUSTLOCK(dst)) {
			if (LockSurface(dst) < 0) {
				SDLog("Unable to lock destination surface");
				return(-1);
			}
			dst_locked = 1;
		}
		/* Lock the source if it's in hardware */
		src_locked = 0;
		if (MUSTLOCK(src)) {
			if (LockSurface(src) < 0) {
				if (dst_locked) {
					UnlockSurface(dst);
				}
				SDLog("Unable to lock source surface");
				return(-1);
			}
			src_locked = 1;
		}

		/* Set up the data... */
		pos = 0x10000;
		inc = (srcrect->h << 16) / dstrect->h;
		src_row = srcrect->y;
		dst_row = dstrect->y;

#ifdef USE_ASM_STRETCH
		/* Write the opcodes for this stretch */
		if ((bpp == 3) ||
			(generate_rowbytes(srcrect->w, dstrect->w, bpp) < 0)) {
			use_asm = false;
		}
#endif

		/* Perform the stretch blit */
		for (dst_maxrow = dst_row + dstrect->h; dst_row<dst_maxrow; ++dst_row) {
			dstp = (Uint8 *)dst->pixels + (dst_row*dst->pitch)
				+ (dstrect->x*bpp);
			while (pos >= 0x10000L) {
				srcp = (Uint8 *)src->pixels + (src_row*src->pitch)
					+ (srcrect->x*bpp);
				++src_row;
				pos -= 0x10000L;
			}
#ifdef USE_ASM_STRETCH
			if (use_asm) {
#ifdef __GNUC__
				__asm__ __volatile__(
					"call *%4"
					: "=&D" (u1), "=&S" (u2)
					: "0" (dstp), "1" (srcp), "r" (copy_row)
					: "memory");
#elif defined(_MSC_VER) || defined(__WATCOMC__)
				{ void *code = copy_row;
				__asm {
					push edi
						push esi

						mov edi, dstp
						mov esi, srcp
						call dword ptr code

						pop esi
						pop edi
				}
				}
#else
#error Need inline assembly for this compiler
#endif
			}
			else
#endif
				switch (bpp) {
				case 1:
					copy_row1(srcp, srcrect->w, dstp, dstrect->w);
					break;
				case 2:
					copy_row2((Uint16 *)srcp, srcrect->w,
						(Uint16 *)dstp, dstrect->w);
					break;
				case 3:
					copy_row3(srcp, srcrect->w, dstp, dstrect->w);
					break;
				case 4:
					copy_row4((Uint32 *)srcp, srcrect->w,
						(Uint32 *)dstp, dstrect->w);
					break;
			}
			pos += inc;
		}

		/* We need to unlock the surfaces if they're locked */
		if (dst_locked) {
			UnlockSurface(dst);
		}
		if (src_locked) {
			UnlockSurface(src);
		}
		return(0);
	}

	/*
		代码乃至于SDL_CreateYUV_SW
	*/
	Overlay * CreateYUV_SW(int width, int height,
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
			//SDL_OutOfMemory();
			return(NULL);
		}
		memset(overlay, 0, (sizeof *overlay));

		/* Fill in the basic members */
		overlay->format = format;
		overlay->w = width;
		overlay->h = height;

		/* Set up the YUV surface function structure */
		overlay->hwfuncs = &sw_yuvfuncs;

		/* Create the pixel data and lookup tables */
		swdata = (struct private_yuvhwdata *)malloc(sizeof *swdata);
		overlay->hwdata = swdata;
		if (swdata == NULL) {
			//SDL_OutOfMemory();
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
			//SDL_OutOfMemory();
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

	int DisplayYUV_SW(Overlay *overlay, Rect *src, Rect *dst)
	{
		struct private_yuvhwdata *swdata;
		int stretch;
		int scale_2x;
		Surface *display;
		Uint8 *lum, *Cr, *Cb;
		Uint8 *dstp;
		int mod;

		swdata = overlay->hwdata;
		stretch = 0;
		scale_2x = 0;
		if (src->x || src->y || src->w < overlay->w || src->h < overlay->h) {
			/* The source rectangle has been clipped.
			Using a scratch surface is easier than adding clipped
			source support to all the blitters, plus that would
			slow them down in the general unclipped case.
			*/
			stretch = 1;
		}
		else if ((src->w != dst->w) || (src->h != dst->h)) {
			if ((dst->w == 2 * src->w) &&
				(dst->h == 2 * src->h)) {
				scale_2x = 1;
			}
			else {
				stretch = 1;
			}
		}
		if (stretch) {
			if (!swdata->stretch) {
				display = swdata->display;
				swdata->stretch = CreateRGBSurface(
					SWSURFACE,
					overlay->w, overlay->h,
					display->format->BitsPerPixel,
					display->format->Rmask,
					display->format->Gmask,
					display->format->Bmask, 0);
				if (!swdata->stretch) {
					return(-1);
				}
			}
			display = swdata->stretch;
		}
		else {
			display = swdata->display;
		}
		switch (overlay->format) {
		case YV12_OVERLAY:
			lum = overlay->pixels[0];
			Cr = overlay->pixels[1];
			Cb = overlay->pixels[2];
			break;
		case IYUV_OVERLAY:
			lum = overlay->pixels[0];
			Cr = overlay->pixels[2];
			Cb = overlay->pixels[1];
			break;
		case YUY2_OVERLAY:
			lum = overlay->pixels[0];
			Cr = lum + 3;
			Cb = lum + 1;
			break;
		case UYVY_OVERLAY:
			lum = overlay->pixels[0] + 1;
			Cr = lum + 1;
			Cb = lum - 1;
			break;
		case YVYU_OVERLAY:
			lum = overlay->pixels[0];
			Cr = lum + 1;
			Cb = lum + 3;
			break;
		default:
			SDLog("Unsupported YUV format in blit");
			return(-1);
		}
		if (MUSTLOCK(display)) {
			if (LockSurface(display) < 0) {
				return(-1);
			}
		}
		if (stretch) {
			dstp = (Uint8 *)swdata->stretch->pixels;
		}
		else {
			dstp = (Uint8 *)display->pixels
				+ dst->x * display->format->BytesPerPixel
				+ dst->y * display->pitch;
		}
		mod = (display->pitch / display->format->BytesPerPixel);

		if (scale_2x) {
			mod -= (overlay->w * 2);
			swdata->Display2X(swdata->colortab, swdata->rgb_2_pix,
				lum, Cr, Cb, dstp, overlay->h, overlay->w, mod);
		}
		else {
			mod -= overlay->w;
			swdata->Display1X(swdata->colortab, swdata->rgb_2_pix,
				lum, Cr, Cb, dstp, overlay->h, overlay->w, mod);
		}
		if (MUSTLOCK(display)) {
			UnlockSurface(display);
		}
		
		if (stretch) {
			display = swdata->display;
			SoftStretch(swdata->stretch, src, display, dst);
		}
		
		//UpdateRects(display, 1, dst);

		return(0);
	}
}

