#include "SDLImp.h"
#include <stdlib.h>

namespace ff{
	char *my_getenv(const char *name)
	{
		return getenv(name);
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

	// µœ÷SDL_GetError()
	char *GetError(void)
	{
		return "";
	}
}