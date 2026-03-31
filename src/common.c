#include "common.h"
#include <SDL2/SDL.h>
#include <math.h>

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

void applyDisplayMode(SDL_Window* window, SDL_Renderer* renderer, int fullscreen)
{
    if (!window) return;
    if (!renderer) renderer = SDL_GetRenderer(window);

    if (fullscreen)
    {
        SDL_DisplayMode mode;
        SDL_zero(mode);
        mode.w = SCREEN_WIDTH;
        mode.h = SCREEN_HEIGHT;
        SDL_SetWindowDisplayMode(window, &mode);
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    }
    else
    {
        SDL_SetWindowFullscreen(window, 0);
        SDL_SetWindowDisplayMode(window, NULL);
        SDL_SetWindowSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    if (renderer)
    {
        SDL_RenderSetLogicalSize(renderer, 0, 0);
        SDL_RenderSetScale(renderer, 1.0f, 1.0f);
        SDL_RenderSetIntegerScale(renderer, SDL_FALSE);
        SDL_RenderSetViewport(renderer, NULL);
        SDL_RenderSetClipRect(renderer, NULL);
    }
}

void windowToLogical(SDL_Window* window, SDL_Renderer* renderer, int winX, int winY, int* outX, int* outY)
{
    if (outX) *outX = winX;
    if (outY) *outY = winY;
    if (!window || !renderer) return;

    int lw = 0, lh = 0;
    SDL_RenderGetLogicalSize(renderer, &lw, &lh);
    if (lw <= 0 || lh <= 0) return;

    int lx = winX;
    int ly = winY;

#if SDL_VERSION_ATLEAST(2,0,18)
    {
        float fx = (float)winX;
        float fy = (float)winY;
        SDL_RenderWindowToLogical(renderer, winX, winY, &fx, &fy);
        lx = (int)lroundf(fx);
        ly = (int)lroundf(fy);
    }
#else
    int ww = 0, wh = 0;
    SDL_GetWindowSize(window, &ww, &wh);
    if (ww <= 0 || wh <= 0) return;

    int ow = 0, oh = 0;
    if (SDL_GetRendererOutputSize(renderer, &ow, &oh) != 0) return;
    if (ow <= 0 || oh <= 0) return;

    float w2ox = (float)ow / (float)ww;
    float w2oy = (float)oh / (float)wh;
    float ox = (float)winX * w2ox;
    float oy = (float)winY * w2oy;

    float sx = (float)ow / (float)lw;
    float sy = (float)oh / (float)lh;
    float s = sx < sy ? sx : sy;
    if (s <= 0.0f) return;

    float vpW = (float)lw * s;
    float vpH = (float)lh * s;
    float vpX = ((float)ow - vpW) * 0.5f;
    float vpY = ((float)oh - vpH) * 0.5f;

    lx = (int)lroundf((ox - vpX) / s);
    ly = (int)lroundf((oy - vpY) / s);
#endif

    lx = lx < 0 ? 0 : (lx > lw ? lw : lx);
    ly = ly < 0 ? 0 : (ly > lh ? lh : ly);

    if (outX) *outX = lx;
    if (outY) *outY = ly;
}

void normalizeEventCoords(SDL_Window* window, SDL_Renderer* renderer, SDL_Event* e)
{
    if (!e || !window || !renderer) return;

    if (e->type == SDL_MOUSEMOTION)
    {
        int x, y;
        windowToLogical(window, renderer, e->motion.x, e->motion.y, &x, &y);
        e->motion.x = x;
        e->motion.y = y;
    }
    else if (e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP)
    {
        int x, y;
        windowToLogical(window, renderer, e->button.x, e->button.y, &x, &y);
        e->button.x = x;
        e->button.y = y;
    }
}

void uiFillRounded(SDL_Renderer* r, SDL_Rect rect, int radius, SDL_Color col)
{
    if (radius <= 0) { SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a); SDL_RenderFillRect(r, &rect); return; }
    if (radius * 2 > rect.w) radius = rect.w / 2;
    if (radius * 2 > rect.h) radius = rect.h / 2;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    SDL_Rect mid = { rect.x, rect.y + radius, rect.w, rect.h - 2 * radius };
    SDL_RenderFillRect(r, &mid);
    SDL_Rect center = { rect.x + radius, rect.y, rect.w - 2 * radius, rect.h };
    SDL_RenderFillRect(r, &center);
    for (int y = 0; y < radius; ++y)
    {
        int dx = (int)(sqrtf((float)(radius * radius - y * y)) + 0.5f);
        int xL = rect.x + radius - dx;
        int xR = rect.x + rect.w - radius + dx - 1;
        SDL_RenderDrawLine(r, xL, rect.y + y, xR, rect.y + y);
        SDL_RenderDrawLine(r, xL, rect.y + rect.h - 1 - y, xR, rect.y + rect.h - 1 - y);
    }
}

void uiStrokeRounded(SDL_Renderer* r, SDL_Rect rect, int radius, SDL_Color col)
{
    if (radius <= 0) { SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a); SDL_RenderDrawRect(r, &rect); return; }
    if (radius * 2 > rect.w) radius = rect.w / 2;
    if (radius * 2 > rect.h) radius = rect.h / 2;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    SDL_RenderDrawLine(r, rect.x + radius, rect.y, rect.x + rect.w - radius - 1, rect.y);
    SDL_RenderDrawLine(r, rect.x + radius, rect.y + rect.h - 1, rect.x + rect.w - radius - 1, rect.y + rect.h - 1);
    SDL_RenderDrawLine(r, rect.x, rect.y + radius, rect.x, rect.y + rect.h - radius - 1);
    SDL_RenderDrawLine(r, rect.x + rect.w - 1, rect.y + radius, rect.x + rect.w - 1, rect.y + rect.h - radius - 1);
    for (int y = 0; y < radius; ++y)
    {
        int dx = (int)(sqrtf((float)(radius * radius - y * y)) + 0.5f);
        int x1 = rect.x + radius - dx;
        int x2 = rect.x + rect.w - radius + dx - 1;
        int yt = rect.y + y;
        int yb = rect.y + rect.h - 1 - y;
        SDL_RenderDrawPoint(r, x1, yt);
        SDL_RenderDrawPoint(r, x2, yt);
        SDL_RenderDrawPoint(r, x1, yb);
        SDL_RenderDrawPoint(r, x2, yb);
    }
}
