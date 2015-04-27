#include "SDLImp.h"
/*
	迁移SDL YUVOverlay
*/
namespace ff{

	void SDLog(const char *log)
	{
		CCLog(log);
	}
	int LockYUVOverlay(Overlay *overlay)
	{
		if (overlay == NULL) {
			SDLog("Passed NULL overlay");
			return -1;
		}
		return overlay->hwfuncs->Lock(overlay);
	}

	void UnlockYUVOverlay(Overlay *overlay)
	{
		if (overlay == NULL) {
			return;
		}
		overlay->hwfuncs->Unlock(overlay);
	}

	int DisplayYUVOverlay(Overlay *overlay, Rect *dstrect)
	{
		Rect src, dst;
		int srcx, srcy, srcw, srch;
		int dstx, dsty, dstw, dsth;

		if (overlay == NULL || dstrect == NULL) {
			SDLog("Passed NULL overlay or dstrect");
			return -1;
		}

		/* Clip the rectangle to the screen area */
		srcx = 0;
		srcy = 0;
		srcw = overlay->w;
		srch = overlay->h;
		dstx = dstrect->x;
		dsty = dstrect->y;
		dstw = dstrect->w;
		dsth = dstrect->h;
		if (dstx < 0) {
			srcw += (dstx * overlay->w) / dstrect->w;
			dstw += dstx;
			srcx -= (dstx * overlay->w) / dstrect->w;
			dstx = 0;
		}
		/* 不存在current_video
		if ((dstx + dstw) > current_video->screen->w) {
			int extra = (dstx + dstw - current_video->screen->w);
			srcw -= (extra * overlay->w) / dstrect->w;
			dstw -= extra;
		}
		*/
		if (dsty < 0) {
			srch += (dsty * overlay->h) / dstrect->h;
			dsth += dsty;
			srcy -= (dsty * overlay->h) / dstrect->h;
			dsty = 0;
		}
		/*不存在current_video
		if ((dsty + dsth) > current_video->screen->h) {
			int extra = (dsty + dsth - current_video->screen->h);
			srch -= (extra * overlay->h) / dstrect->h;
			dsth -= extra;
		}
		*/
		if (srcw <= 0 || srch <= 0 ||
			srch <= 0 || dsth <= 0) {
			return 0;
		}
		/* Ugh, I can't wait for SDL_Rect to be int values */
		src.x = srcx;
		src.y = srcy;
		src.w = srcw;
		src.h = srch;
		dst.x = dstx;
		dst.y = dsty;
		dst.w = dstw;
		dst.h = dsth;
		return overlay->hwfuncs->Display(overlay, &src, &dst);
	}

	void FreeYUVOverlay(Overlay *overlay)
	{
		if (overlay == NULL) {
			return;
		}
		if (overlay->hwfuncs) {
			overlay->hwfuncs->FreeHW(overlay);
		}
		free(overlay);
	}

	Overlay * CreateYUVOverlay(int width, int height,
		Uint32 format, Surface *display)
	{
		return CreateYUV_SW(width, height, format, display);
	}
}