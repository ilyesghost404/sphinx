#include "play.h"
#include "save/save.h"
#include "common.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <math.h>

// ===== External shared resources =====
extern TTF_Font* font;
extern Mix_Chunk* hoverSound;
extern Mix_Chunk* clickSound;
extern Mix_Music* menuMusic;

// Shared background from main (Play + Options)
extern SDL_Texture* bgShared[SHARED_BG_FRAMES];
extern int gCurrentFrame;

// ===== Play Buttons =====
typedef struct {
    SDL_Rect rect;
    SDL_Texture* textTexture;
    SDL_Rect textRect;
    SDL_Texture* iconTexture;
    SDL_Rect iconRect;
    int hovered;
} PlayButton;

#define PLAY_BUTTON_COUNT 2
static PlayButton playButtons[PLAY_BUTTON_COUNT];
static int selectedPlayButton = 0;

// ===== Title =====
static SDL_Texture* titleTexture = NULL;
static SDL_Rect titleRect;
static SDL_Texture* subtitleTexture = NULL;
static SDL_Rect subtitleRect;

// ===== Panel =====
static SDL_Rect panelRect;

// Colors
static SDL_Color borderColor = {220, 180, 90, 220};
static SDL_Color buttonColor = {10, 15, 25, 180};
static SDL_Color buttonHover = {10, 15, 25, 240};
static SDL_Color textColor = {220, 180, 90, 255};

// ===== Save prompt flag =====
static int showSavePrompt = 1;

static PlayButton backButton;
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
// =========================================================
// ================= INIT PLAY ============================
// =========================================================
void initPlay(SDL_Renderer* renderer)
{
    showSavePrompt = 1;
    initSave(renderer);

    // Render title with a larger font to avoid pixelation
    TTF_Font* titleFontLocal = TTF_OpenFont("assets/fonts/ARIAL.TTF", 64);
    SDL_Surface* titleSurf = titleFontLocal
        ? TTF_RenderText_Blended(titleFontLocal, "SPHINX: THE LOST NOSE", textColor)
        : TTF_RenderText_Blended(font, "SPHINX: THE LOST NOSE", textColor);
    titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurf);

    titleRect.w = titleSurf->w;
    titleRect.h = titleSurf->h;
    if (titleFontLocal) TTF_CloseFont(titleFontLocal);
    SDL_FreeSurface(titleSurf);

    SDL_Surface* subSurf = TTF_RenderText_Blended(font, "Choose how to start", textColor);
    subtitleTexture = SDL_CreateTextureFromSurface(renderer, subSurf);
    subtitleRect.w = subSurf->w;
    subtitleRect.h = subSurf->h;
    SDL_FreeSurface(subSurf);

    const char* labels[PLAY_BUTTON_COUNT] = {"NEW GAME [N]", "LOAD GAME"};
    const char* icons[PLAY_BUTTON_COUNT] = {"assets/images/ui/new.png", "assets/images/ui/load.png"};
    int marginX = 64;
    int marginY = 64;
    // Bigger, modern panel
    panelRect.w = (int)(SCREEN_WIDTH * 0.82f);
    panelRect.h = (int)(SCREEN_HEIGHT * 0.78f);
    panelRect.x = (SCREEN_WIDTH - panelRect.w) / 2;
    panelRect.y = (SCREEN_HEIGHT - panelRect.h) / 2;
    // Center the title and place subtitle beneath, both centered
    titleRect.x = panelRect.x + (panelRect.w - titleRect.w) / 2;
    titleRect.y = panelRect.y + marginY - 12;
    subtitleRect.x = panelRect.x + (panelRect.w - subtitleRect.w) / 2;
    subtitleRect.y = titleRect.y + titleRect.h + 8;
    // Stacked wide buttons for a clean look
    int btnWidth = panelRect.w - 2 * marginX;
    if (btnWidth < 320) btnWidth = 320;
    int btnHeight = 84;
    int iconSizeBase = 36;
    int posY = subtitleRect.y + subtitleRect.h + 48;
    int startX = panelRect.x + (panelRect.w - btnWidth) / 2;

    for (int i = 0; i < PLAY_BUTTON_COUNT; i++)
    {
        int by = posY + i * (btnHeight + 18);
        int bx = startX;
        playButtons[i].rect = (SDL_Rect){bx, by, btnWidth, btnHeight};

        SDL_Surface* surf = TTF_RenderText_Blended(font, labels[i], textColor);
        playButtons[i].textTexture = SDL_CreateTextureFromSurface(renderer, surf);

        playButtons[i].textRect.w = surf->w;
        playButtons[i].textRect.h = surf->h;
        int iconSize = iconSizeBase;
        playButtons[i].iconTexture = NULL;
        SDL_Surface* iconSurf = IMG_Load(icons[i]);
        if (iconSurf)
        {
            playButtons[i].iconTexture = SDL_CreateTextureFromSurface(renderer, iconSurf);
            SDL_FreeSurface(iconSurf);
        }
        playButtons[i].iconRect.w = iconSize;
        playButtons[i].iconRect.h = iconSize;
        playButtons[i].iconRect.x = playButtons[i].rect.x + 24;
        playButtons[i].iconRect.y = playButtons[i].rect.y + (playButtons[i].rect.h - iconSize) / 2;
        playButtons[i].textRect.x = playButtons[i].iconRect.x + iconSize + 18;
        playButtons[i].textRect.y = playButtons[i].rect.y + (playButtons[i].rect.h - surf->h) / 2;

        playButtons[i].hovered = 0;

        SDL_FreeSurface(surf);
    }

    // Move BACK to top-left inside the panel to declutter the lower area
    int bw = 180, bh = 56;
    int bx = panelRect.x + (panelRect.w - bw)/2;
    int by = panelRect.y + panelRect.h - bh - 24;
    backButton.rect = (SDL_Rect){bx, by, bw, bh};
    SDL_Surface* backSurf = TTF_RenderText_Blended(font, "BACK", textColor);
    backButton.textTexture = SDL_CreateTextureFromSurface(renderer, backSurf);
    backButton.textRect.w = backSurf->w;
    backButton.textRect.h = backSurf->h;
    backButton.textRect.x = backButton.rect.x + (bw - backSurf->w)/2;
    backButton.textRect.y = backButton.rect.y + (bh - backSurf->h)/2;
    backButton.iconTexture = NULL;
    backButton.iconRect = (SDL_Rect){0,0,0,0};
    backButton.hovered = 0;
    SDL_FreeSurface(backSurf);
}

// =========================================================
// ================= UPDATE PLAY ==========================
// =========================================================
void updatePlay()
{
    if (!showSavePrompt)
        updateSharedBackground(30);

    if (showSavePrompt)
        updateSave();
}

// =========================================================
// ================= HANDLE EVENTS ========================
// =========================================================
void handlePlayEvent(SDL_Event* e, MenuState* currentMenuPtr)
{
    // ===== If save prompt is active =====
    if (showSavePrompt)
    {
        handleSaveEvent(e, currentMenuPtr);
        if (*currentMenuPtr == MENU_PLAY || *currentMenuPtr == MENU_MAIN)
            showSavePrompt = 0;
        return;
    }

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    int hovered = -1;
    for (int i = 0; i < PLAY_BUTTON_COUNT; i++)
    {
        playButtons[i].hovered =
            mx >= playButtons[i].rect.x &&
            mx <= playButtons[i].rect.x + playButtons[i].rect.w &&
            my >= playButtons[i].rect.y &&
            my <= playButtons[i].rect.y + playButtons[i].rect.h;

        if (playButtons[i].hovered)
            hovered = i;
    }

    if (hovered != -1 && hovered != selectedPlayButton && hoverSound)
        Mix_PlayChannel(-1, hoverSound, 0);

    if (hovered != -1)
        selectedPlayButton = hovered;

    int was = backButton.hovered;
    backButton.hovered =
        mx >= backButton.rect.x &&
        mx <= backButton.rect.x + backButton.rect.w &&
        my >= backButton.rect.y &&
        my <= backButton.rect.y + backButton.rect.h;
    if (backButton.hovered && !was && hoverSound)
        Mix_PlayChannel(-1, hoverSound, 0);

    if (e->type == SDL_MOUSEBUTTONDOWN)
    {
        if (selectedPlayButton != -1 && playButtons[selectedPlayButton].hovered && clickSound)
            Mix_PlayChannel(-1, clickSound, 0);

        if (backButton.hovered)
        {
            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
            *currentMenuPtr = MENU_MAIN;
            if (menuMusic) Mix_PlayMusic(menuMusic, -1);
            return;
        }

        switch (selectedPlayButton)
        {
            case 0: *currentMenuPtr = MENU_NEWGAME; break;
            case 1: printf("LOAD GAME pressed!\n"); break;
        }
    }

    if (e->type == SDL_KEYDOWN)
    {
        switch (e->key.keysym.sym)
        {
            case SDLK_RIGHT:
                selectedPlayButton = (selectedPlayButton + 1) % PLAY_BUTTON_COUNT;
                if (hoverSound) Mix_PlayChannel(-1, hoverSound, 0);
                break;
            case SDLK_LEFT:
                selectedPlayButton = (selectedPlayButton - 1 + PLAY_BUTTON_COUNT) % PLAY_BUTTON_COUNT;
                if (hoverSound) Mix_PlayChannel(-1, hoverSound, 0);
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                if (selectedPlayButton == 0) *currentMenuPtr = MENU_NEWGAME;
                break;
            case 'n':
            case 'N':
                *currentMenuPtr = MENU_NEWGAME;
                break;
            case SDLK_ESCAPE:
                *currentMenuPtr = MENU_MAIN;
                if (menuMusic) Mix_PlayMusic(menuMusic, -1);
                break;
        }
    }

    for (int i = 0; i < PLAY_BUTTON_COUNT; i++)
        playButtons[i].hovered = (i == selectedPlayButton);
}

// =========================================================
// ================= RENDER PLAY ==========================
// =========================================================
void renderPlay(SDL_Renderer* renderer)
{
    // ===== Save prompt =====
    if (showSavePrompt)
    {
        renderSave(renderer);
        return;
    }

    // ===== Background =====
    if (bgShared[gCurrentFrame])
        SDL_RenderCopy(renderer, bgShared[gCurrentFrame], NULL, NULL);

    // ===== Panel Shadow =====
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 120);
    SDL_Rect shadow = {panelRect.x + 10, panelRect.y + 10, panelRect.w, panelRect.h};
    SDL_RenderFillRect(renderer, &shadow);

    // ===== Panel =====
    fillRounded(renderer, panelRect, 16, buttonColor);
    strokeRounded(renderer, panelRect, 16, borderColor);

    // ===== Title =====
    SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
    SDL_RenderCopy(renderer, subtitleTexture, NULL, &subtitleRect);

    // ===== Buttons =====
    for (int i = 0; i < PLAY_BUTTON_COUNT; i++)
    {
        SDL_Rect rect = playButtons[i].rect;
        if (playButtons[i].hovered)
        {
            rect.x -= 6; rect.y -= 4;
            rect.w += 12; rect.h += 8;
        }

        SDL_Color fill = playButtons[i].hovered ? buttonHover : buttonColor;
        fillRounded(renderer, rect, 12, fill);
        strokeRounded(renderer, rect, 12, borderColor);
        if (playButtons[i].iconTexture)
            SDL_RenderCopy(renderer, playButtons[i].iconTexture, NULL, &playButtons[i].iconRect);
        SDL_RenderCopy(renderer, playButtons[i].textTexture, NULL, &playButtons[i].textRect);
    }

    SDL_Rect br = backButton.rect;
    if (backButton.hovered) { br.x -= 6; br.y -= 4; br.w += 12; br.h += 8; }
    fillRounded(renderer, br, 12, buttonColor);
    strokeRounded(renderer, br, 12, borderColor);
    SDL_RenderCopy(renderer, backButton.textTexture, NULL, &backButton.textRect);
}

// =========================================================
// ================= DESTROY PLAY =========================
// =========================================================
void destroyPlay()
{
    destroySave();

    for (int i = 0; i < PLAY_BUTTON_COUNT; i++)
    {
        if (playButtons[i].textTexture)
            SDL_DestroyTexture(playButtons[i].textTexture);
        if (playButtons[i].iconTexture)
            SDL_DestroyTexture(playButtons[i].iconTexture);
    }

    if (titleTexture)
        SDL_DestroyTexture(titleTexture);
    if (subtitleTexture)
        SDL_DestroyTexture(subtitleTexture);
    if (backButton.textTexture)
        SDL_DestroyTexture(backButton.textTexture);
}
