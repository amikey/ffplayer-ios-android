#include "SDLImp.h"
#if __cplusplus
extern "C" {
#endif
#include <SDL.h>
#if __cplusplus
}
#endif
namespace ff{
	char *my_getenv(const char *name)
	{
		return SDL_getenv(name);
	}

	void WM_SetCaption(const char *title, const char *icon)
	{
	}

	int ShowCursor(int toggle)
	{
		return 0;
	}

	void Quit(void)
	{
	}

	char *GetError(void)
	{
		return "";
	}
}