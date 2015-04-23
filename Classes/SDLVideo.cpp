#include "SDLImp.h"
#if __cplusplus
extern "C" {
#endif
#include <SDL.h>
#if __cplusplus
}
#endif
/*
	重新实现SDL Video的部分函数
*/
int FillRect(Surface *dst, Rect *dstrect, Uint32 color)
{
	return SDL_FillRect((SDL_Surface*)dst, (SDL_Rect*)dstrect, color);
}

int LockYUVOverlay(Overlay *overlay)
{
	return SDL_LockYUVOverlay((SDL_Overlay*)overlay);
}

void UnlockYUVOverlay(Overlay *overlay)
{
	SDL_UnlockYUVOverlay((SDL_Overlay*)overlay);
}

int DisplayYUVOverlay(Overlay *overlay, Rect *dstrect)
{
	return SDL_DisplayYUVOverlay((SDL_Overlay*)overlay, (SDL_Rect*)dstrect);
}

void FreeYUVOverlay(Overlay *overlay)
{
	SDL_FreeYUVOverlay((SDL_Overlay*)overlay);
}

Overlay * CreateYUVOverlay(int width, int height,
	Uint32 format, Surface *display)
{
	return  (Overlay*)SDL_CreateYUVOverlay(width, height, format, (SDL_Surface*)display);
}

void UpdateRect(Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h)
{
	SDL_UpdateRect((SDL_Surface*)screen, x, y, w, h);
}

Uint32 MapRGB(const PixelFormat * const format, const Uint8 r, const Uint8 g, const Uint8 b)
{
	return SDL_MapRGB((SDL_PixelFormat*)format, r, g, b);
}

Surface * SetVideoMode(int width, int height, int bpp, Uint32 flags)
{
	return (Surface*)SDL_SetVideoMode(width, height, bpp, flags);
}