#include "common.h"
#include <SDL2/SDL.h>

// Main menu background
SDL_Texture* bgMenu[MENU_BG_FRAMES];

// Shared background (Play + Options)
SDL_Texture* bgShared[SHARED_BG_FRAMES];

// Shared background frame index
int gCurrentFrame = 0;

static Uint32 gSharedLastTick = 0;

void updateSharedBackground(int targetFps)
{
    if (targetFps <= 0) targetFps = 30;
    Uint32 delay = 1000 / (Uint32)targetFps;
    Uint32 now = SDL_GetTicks();
    if (now - gSharedLastTick >= delay)
    {
        gCurrentFrame = (gCurrentFrame + 1) % SHARED_BG_FRAMES;
        gSharedLastTick = now;
    }
}
