#include "story.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include "../common.h"
#include <math.h>

extern TTF_Font* font;
extern MenuState currentMenu;
extern Mix_Music* menuMusic;

#define STORY_WIDTH 700
#define SCROLL_SPEED 1.0f

// ===== PANEL =====
static SDL_Rect panelRect;

// ===== TEXTURE =====
static SDL_Texture* storyTexture = NULL;
static SDL_Rect storyRect;
static float scrollY = 0;
static SDL_Texture* storyBgTex = NULL;

// ===== SCROLL BAR =====
static SDL_Rect scrollBarBg;
static SDL_Rect scrollBarHandle;

// ===== BUTTONS =====
static SDL_Rect backButtonRect;
static SDL_Rect autoButtonRect;

static SDL_Texture* backTextTexture = NULL;
static SDL_Texture* autoTextTexture = NULL;

static SDL_Rect backTextRect;
static SDL_Rect autoTextRect;

// ===== AUTO SCROLL =====
static int autoScroll = 1;

// ===== RENDERER REFERENCE =====
static SDL_Renderer* gRenderer = NULL;

// ===== STORY TEXT =====
const char* storyText =
"For 4,000 years, the Great Sphinx of Giza has watched the world in silence.\n\n"
"But tonight... it moved.\n\n"
"Without warning, a violent shockwave tears through the desert. Sand explodes into the sky as the Sphinx's missing Nose rockets upward like a missile. Satellites lose signal. Alarms sound across Egypt. Something ancient has awakened.\n\n"
"The world thinks it's a disaster.\n\n"
"They're wrong.\n\n"
"It's a test.\n\n"
"You are a young archaeologist chosen for one reason: you believe artifacts aren't just objects — they're alive. Now the runaway Nose is traveling across the globe, leaving behind impossible puzzles in forgotten ruins. It isn't hiding.\n\n"
"It's challenging you.\n\n"
"Solve its trials. Prove your mind. Earn its respect.\n\n"
"Because the Nose is not just a piece of stone.\n\n"
"It is a key.\n\n"
"And when you return it to the Sphinx...\n\n"
"...you won't just restore history.\n\n"
"You will wake it up.\n\n"
"And this time -- it's watching back.\n";

void initStory(SDL_Renderer* renderer)
{
    gRenderer = renderer;

    SDL_Color gold = {220, 180, 90, 255};

    if (!storyBgTex)
    {
        SDL_Surface* bg = IMG_Load("assets/images/backgrounds/bg.png");
        if (!bg)
            printf("Failed to load story background: %s\n", IMG_GetError());
        else
        {
            storyBgTex = SDL_CreateTextureFromSurface(renderer, bg);
            SDL_FreeSurface(bg);
        }
    }

    // ===== PANEL =====
    panelRect.w = STORY_WIDTH + 60;
    panelRect.h = SCREEN_HEIGHT - 160;
    panelRect.x = (SCREEN_WIDTH - panelRect.w) / 2;
    panelRect.y = 100;

    // ===== STORY TEXT =====
    SDL_Surface* surface =
        TTF_RenderText_Blended_Wrapped(font, storyText, gold, STORY_WIDTH);

    storyTexture = SDL_CreateTextureFromSurface(renderer, surface);

    storyRect.w = surface->w;
    storyRect.h = surface->h;
    storyRect.x = panelRect.x + 30;

    scrollY = panelRect.y + panelRect.h;
    storyRect.y = (int)scrollY;

    SDL_FreeSurface(surface);

    // ===== BACK BUTTON =====
    backButtonRect = (SDL_Rect){
        panelRect.x,
        panelRect.y - 60,
        180, 45
    };

    SDL_Surface* backSurface =
        TTF_RenderText_Blended(font, "BACK", gold);

    backTextTexture =
        SDL_CreateTextureFromSurface(renderer, backSurface);

    backTextRect = (SDL_Rect){
        backButtonRect.x + (180 - backSurface->w)/2,
        backButtonRect.y + (45 - backSurface->h)/2,
        backSurface->w,
        backSurface->h
    };

    SDL_FreeSurface(backSurface);

    // ===== AUTO BUTTON =====
    autoButtonRect = (SDL_Rect){
        panelRect.x + panelRect.w - 220,
        panelRect.y - 60,
        220, 45
    };

    SDL_Surface* autoSurface =
        TTF_RenderText_Blended(font, "AUTO: ON", gold);

    autoTextTexture =
        SDL_CreateTextureFromSurface(renderer, autoSurface);

    autoTextRect = (SDL_Rect){
        autoButtonRect.x + (220 - autoSurface->w)/2,
        autoButtonRect.y + (45 - autoSurface->h)/2,
        autoSurface->w,
        autoSurface->h
    };

    SDL_FreeSurface(autoSurface);

    // ===== SCROLL BAR =====
    scrollBarBg = (SDL_Rect){
        panelRect.x + panelRect.w - 10,
        panelRect.y + 10,
        6,
        panelRect.h - 20
    };
}

static void fillRounded(SDL_Renderer* r, SDL_Rect rect, int radius, SDL_Color col)
{
    if (radius <= 0) { SDL_SetRenderDrawColor(r, col.r,col.g,col.b,col.a); SDL_RenderFillRect(r, &rect); return; }
    if (radius*2 > rect.w) radius = rect.w/2;
    if (radius*2 > rect.h) radius = rect.h/2;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    SDL_Rect mid = {rect.x, rect.y + radius, rect.w, rect.h - 2*radius};
    SDL_RenderFillRect(r, &mid);
    SDL_Rect center = {rect.x + radius, rect.y, rect.w - 2*radius, rect.h};
    SDL_RenderFillRect(r, &center);
    for (int y = 0; y < radius; ++y)
    {
        int dx = (int)(sqrtf((float)(radius*radius - y*y)) + 0.5f);
        int xL = rect.x + radius - dx;
        int xR = rect.x + rect.w - radius + dx - 1;
        SDL_RenderDrawLine(r, xL, rect.y + y, xR, rect.y + y);
        SDL_RenderDrawLine(r, xL, rect.y + rect.h - 1 - y, xR, rect.y + rect.h - 1 - y);
    }
}

static void strokeRounded(SDL_Renderer* r, SDL_Rect rect, int radius, SDL_Color col)
{
    if (radius <= 0) { SDL_SetRenderDrawColor(r, col.r,col.g,col.b,col.a); SDL_RenderDrawRect(r, &rect); return; }
    if (radius*2 > rect.w) radius = rect.w/2;
    if (radius*2 > rect.h) radius = rect.h/2;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    SDL_RenderDrawLine(r, rect.x + radius, rect.y, rect.x + rect.w - radius - 1, rect.y);
    SDL_RenderDrawLine(r, rect.x + radius, rect.y + rect.h - 1, rect.x + rect.w - radius - 1, rect.y + rect.h - 1);
    SDL_RenderDrawLine(r, rect.x, rect.y + radius, rect.x, rect.y + rect.h - radius - 1);
    SDL_RenderDrawLine(r, rect.x + rect.w - 1, rect.y + radius, rect.x + rect.w - 1, rect.y + rect.h - radius - 1);
    for (int y = 0; y < radius; ++y)
    {
        int dx = (int)(sqrtf((float)(radius*radius - y*y)) + 0.5f);
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

void handleStoryEvent(SDL_Event* e)
{
    if (e->type == SDL_KEYDOWN &&
        e->key.keysym.sym == SDLK_ESCAPE)
    {
        currentMenu = MENU_MAIN;
        if (menuMusic) Mix_PlayMusic(menuMusic, -1);
    }

    if (e->type == SDL_MOUSEBUTTONDOWN)
    {
        int x = e->button.x;
        int y = e->button.y;

        // BACK
        if (x >= backButtonRect.x && x <= backButtonRect.x + backButtonRect.w &&
            y >= backButtonRect.y && y <= backButtonRect.y + backButtonRect.h)
        {
            currentMenu = MENU_MAIN;
            if (menuMusic) Mix_PlayMusic(menuMusic, -1);
        }

        // AUTO TOGGLE
        if (x >= autoButtonRect.x && x <= autoButtonRect.x + autoButtonRect.w &&
            y >= autoButtonRect.y && y <= autoButtonRect.y + autoButtonRect.h)
        {
            autoScroll = !autoScroll;

            SDL_DestroyTexture(autoTextTexture);

            SDL_Color gold = {220, 180, 90, 255};

            const char* text =
                autoScroll ? "AUTO: ON" : "AUTO: OFF";

            SDL_Surface* surf =
                TTF_RenderText_Blended(font, text, gold);

            autoTextTexture =
                SDL_CreateTextureFromSurface(gRenderer, surf);

            autoTextRect.w = surf->w;
            autoTextRect.h = surf->h;
            autoTextRect.x =
                autoButtonRect.x + (autoButtonRect.w - surf->w)/2;
            autoTextRect.y =
                autoButtonRect.y + (autoButtonRect.h - surf->h)/2;

            SDL_FreeSurface(surf);
        }
    }

    // Manual scroll (only when auto OFF)
    if (e->type == SDL_MOUSEWHEEL)
{
    // Turn auto scroll OFF when user scrolls manually
    if (autoScroll)
    {
        autoScroll = 0;

        SDL_DestroyTexture(autoTextTexture);

        SDL_Color gold = {220, 180, 90, 255};

        SDL_Surface* surf =
            TTF_RenderText_Blended(font, "AUTO: OFF", gold);

        autoTextTexture =
            SDL_CreateTextureFromSurface(gRenderer, surf);

        autoTextRect.w = surf->w;
        autoTextRect.h = surf->h;
        autoTextRect.x =
            autoButtonRect.x + (autoButtonRect.w - surf->w)/2;
        autoTextRect.y =
            autoButtonRect.y + (autoButtonRect.h - surf->h)/2;

        SDL_FreeSurface(surf);
    }

    // Manual scrolling
    scrollY += e->wheel.y * 20;
}

}

void updateStory()
{
    float minY = panelRect.y + 10 - storyRect.h;
    float maxY = panelRect.y + panelRect.h - 20;

    if (autoScroll)
        scrollY -= SCROLL_SPEED;

    // ===== REPEAT WHEN END =====
    if (scrollY < minY)
        scrollY = panelRect.y + panelRect.h;

    // Clamp upper limit
    if (scrollY > maxY)
        scrollY = maxY;

    storyRect.y = (int)scrollY;

    // ===== UPDATE SCROLL HANDLE =====
    float progress =
        (maxY - scrollY) / (storyRect.h + panelRect.h);

    scrollBarHandle.h = 60;
    scrollBarHandle.w = 6;
    scrollBarHandle.x = scrollBarBg.x;
    scrollBarHandle.y =
        scrollBarBg.y +
        progress * (scrollBarBg.h - scrollBarHandle.h);
}

void renderStory(SDL_Renderer* renderer)
{
    if (storyBgTex)
        SDL_RenderCopy(renderer, storyBgTex, NULL, NULL);
    else
    {
        SDL_SetRenderDrawColor(renderer, 18, 20, 35, 255);
        SDL_RenderClear(renderer);
    }

    // ===== PANEL =====
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    fillRounded(renderer, panelRect, 16, (SDL_Color){10,15,25,180});
    strokeRounded(renderer, panelRect, 16, (SDL_Color){220,180,90,220});

    // ===== STORY TEXT =====
    SDL_RenderSetClipRect(renderer, &panelRect);
    SDL_RenderCopy(renderer, storyTexture, NULL, &storyRect);
    SDL_RenderSetClipRect(renderer, NULL);

    // ===== SCROLL BAR =====
    fillRounded(renderer, scrollBarBg, 3, (SDL_Color){0,0,0,140});

    fillRounded(renderer, scrollBarHandle, 3, (SDL_Color){220,180,90,255});

    // ===== BACK BUTTON =====
    fillRounded(renderer, backButtonRect, 10, (SDL_Color){10,15,25,180});
    strokeRounded(renderer, backButtonRect, 10, (SDL_Color){220,180,90,220});
    SDL_RenderCopy(renderer, backTextTexture, NULL, &backTextRect);

    // ===== AUTO BUTTON =====
    fillRounded(renderer, autoButtonRect, 10, (SDL_Color){10,15,25,180});
    strokeRounded(renderer, autoButtonRect, 10, (SDL_Color){220,180,90,220});
    SDL_RenderCopy(renderer, autoTextTexture, NULL, &autoTextRect);
}

void destroyStory()
{
    if (storyTexture) SDL_DestroyTexture(storyTexture);
    if (backTextTexture) SDL_DestroyTexture(backTextTexture);
    if (autoTextTexture) SDL_DestroyTexture(autoTextTexture);
    if (storyBgTex) SDL_DestroyTexture(storyBgTex);
}
