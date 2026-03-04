#include "menu.h"
#include <stdio.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include "common.h"
#include <math.h>

// ===== ANIMATION =====
static int currentFrame = 0;
static Uint32 lastFrameTime = 0;
static int frameDelay = 33; // ~30 FPS

// ===== BUTTONS =====
static Button buttons[BUTTON_COUNT];
static SDL_Texture* titleTexture = NULL;
static SDL_Rect titleRect;
static SDL_Rect titleBgRect;
static SDL_Texture* logoTexture = NULL;
static SDL_Rect logoRect;

TTF_Font* font = NULL;
TTF_Font* titleFont = NULL;

// ===== AUDIO =====
Mix_Music* menuMusic = NULL;
Mix_Music* gameMusic = NULL;
Mix_Chunk* hoverSound = NULL;
Mix_Chunk* clickSound = NULL;
static int lastHoveredButton = -1;

// ===== ICON PATHS =====
static const char* iconPaths[BUTTON_COUNT] = {
    "assets/images/ui/play.png",
    "assets/images/ui/options.png",
    "assets/images/ui/score.png",
    "assets/images/ui/story.png",
    "assets/images/ui/quit.png"
};

// ===== BUTTON LABELS =====
static const char* buttonLabels[BUTTON_COUNT] = {
    "PLAY [P]",
    "OPTIONS [O]",
    "BEST SCORE [B]",
    "STORY",
    "QUIT [ESC]"
};

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

void initMenu(SDL_Renderer* renderer)
{
    // ===== LOAD FONTS =====
    font = TTF_OpenFont("assets/fonts/ARIAL.TTF", 22);
    titleFont = TTF_OpenFont("assets/fonts/ARIAL.TTF", 72);

    SDL_Color gold = {220, 180, 90, 255};

    // ===== TITLE =====
    SDL_Surface* titleSurface =
        TTF_RenderText_Blended(titleFont, "SPHINX: THE LOST NOSE", gold);

    titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);

    titleRect.w = titleSurface->w;
    titleRect.h = titleSurface->h;
    titleRect.x = (SCREEN_WIDTH - titleRect.w) / 2;
    titleRect.y = 50;

    SDL_FreeSurface(titleSurface);

    titleBgRect.x = titleRect.x - 24;
    titleBgRect.y = titleRect.y - 16;
    titleBgRect.w = titleRect.w + 48;
    titleBgRect.h = titleRect.h + 32;

    // ===== BUTTONS =====
    int buttonWidth = 260;
    int buttonHeight = 60;
    int spacing = 20;

    int marginLeft = 120;
    int marginTop = 240;

    for(int i = 0; i < BUTTON_COUNT; i++)
    {
        buttons[i].rect.w = buttonWidth;
        buttons[i].rect.h = buttonHeight;

        if(i < 4)
        {
            buttons[i].rect.x = marginLeft;
            buttons[i].rect.y = marginTop + i * (buttonHeight + spacing);
        }
        else
        {
            buttons[i].rect.x = SCREEN_WIDTH - buttonWidth - 50;
            buttons[i].rect.y = SCREEN_HEIGHT - buttonHeight - 50;
        }

        buttons[i].hovered = 0;

        // ===== LOAD ICON =====
        SDL_Surface* iconSurface = IMG_Load(iconPaths[i]);
        if(iconSurface)
        {
            buttons[i].iconTexture = SDL_CreateTextureFromSurface(renderer, iconSurface);
            int iconSize = 28;
            buttons[i].iconRect.w = iconSize;
            buttons[i].iconRect.h = iconSize;
            buttons[i].iconRect.x = buttons[i].rect.x + 15;
            buttons[i].iconRect.y = buttons[i].rect.y + (buttonHeight - iconSize) / 2;
            SDL_FreeSurface(iconSurface);
        }
        else
        {
            buttons[i].iconTexture = NULL;
        }

        // ===== LOAD TEXT =====
        SDL_Surface* textSurface =
            TTF_RenderText_Blended(font, buttonLabels[i], gold);

        buttons[i].textTexture =
            SDL_CreateTextureFromSurface(renderer, textSurface);

        buttons[i].textRect.w = textSurface->w;
        buttons[i].textRect.h = textSurface->h;
        buttons[i].textRect.x = buttons[i].rect.x + 60;
        buttons[i].textRect.y = buttons[i].rect.y + (buttonHeight - textSurface->h) / 2;

        SDL_FreeSurface(textSurface);
    }

    // ===== AUDIO =====
    menuMusic = Mix_LoadMUS("assets/audio/music/menu.wav");
    if(menuMusic)
        Mix_PlayMusic(menuMusic, -1);

    gameMusic = Mix_LoadMUS("assets/audio/music/game.wav");
    if (!gameMusic)
        printf("Failed to load game music: %s\n", Mix_GetError());

    hoverSound = Mix_LoadWAV("assets/audio/sfx/hover.wav");
    clickSound = Mix_LoadWAV("assets/audio/sfx/click.wav");

    SDL_Surface* logoSurf = IMG_Load("assets/images/ui/sphinx.png");
    if (!logoSurf) logoSurf = IMG_Load("assets/images/sphinx.png");
    if (logoSurf)
    {
        logoTexture = SDL_CreateTextureFromSurface(renderer, logoSurf);
        int lw = logoSurf->w, lh = logoSurf->h;
        int maxW = 160, maxH = 160;
        float scale = 1.0f;
        if (lw > maxW || lh > maxH)
        {
            float sx = (float)maxW / (float)lw;
            float sy = (float)maxH / (float)lh;
            scale = sx < sy ? sx : sy;
        }
        logoRect.w = (int)(lw * scale);
        logoRect.h = (int)(lh * scale);
        logoRect.x = SCREEN_WIDTH - logoRect.w - 24;
        logoRect.y = 24;
        SDL_FreeSurface(logoSurf);
    }
}

static int selectedButton = 0;

void handleMenuEvent(SDL_Event* e, int* running, MenuState* currentMenu)
{
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    int currentHovered = -1;

    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        buttons[i].hovered =
            mouseX >= buttons[i].rect.x &&
            mouseX <= buttons[i].rect.x + buttons[i].rect.w &&
            mouseY >= buttons[i].rect.y &&
            mouseY <= buttons[i].rect.y + buttons[i].rect.h;

        if (buttons[i].hovered)
            currentHovered = i;
    }

    if (currentHovered != -1 && currentHovered != lastHoveredButton)
    {
        Mix_PlayChannel(-1, hoverSound, 0);
        selectedButton = currentHovered;
    }

    lastHoveredButton = currentHovered;

    if(e->type == SDL_MOUSEBUTTONDOWN)
    {
        for(int i = 0; i < BUTTON_COUNT; i++)
        {
            if(buttons[i].hovered)
            {
                Mix_PlayChannel(-1, clickSound, 0);
                switch(i)
                {
                    case 0:
                        *currentMenu = MENU_SAVE_PROMPT;
                        if (gameMusic) Mix_PlayMusic(gameMusic, -1);
                        break;
                    case 1:
                        *currentMenu = MENU_OPTIONS;
                        if (gameMusic) Mix_PlayMusic(gameMusic, -1);
                        break;
                    case 2:
                        *currentMenu = MENU_SCORE;
                        break;
                    case 3:
                        *currentMenu = MENU_STORY;
                        if (gameMusic) Mix_PlayMusic(gameMusic, -1);
                        break;
                    case 4:
                        *running = 0;
                        break;
                }
            }
        }
    }

    if(e->type == SDL_KEYDOWN)
    {
        switch(e->key.keysym.sym)
        {
            case SDLK_p:
                *currentMenu = MENU_SAVE_PROMPT;
                if (gameMusic) Mix_PlayMusic(gameMusic, -1);
                break;
            case SDLK_o:
                *currentMenu = MENU_OPTIONS;
                if (gameMusic) Mix_PlayMusic(gameMusic, -1);
                break;
            case SDLK_b:
                *currentMenu = MENU_SCORE;
                break;
            case SDLK_ESCAPE: *running = 0; break;

            case SDLK_DOWN:
                selectedButton = (selectedButton + 1) % BUTTON_COUNT;
                Mix_PlayChannel(-1, hoverSound, 0);
                break;

            case SDLK_UP:
                selectedButton = (selectedButton - 1 + BUTTON_COUNT) % BUTTON_COUNT;
                Mix_PlayChannel(-1, hoverSound, 0);
                break;

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                Mix_PlayChannel(-1, clickSound, 0);
                switch(selectedButton)
                {
                    case 0: *currentMenu = MENU_SAVE_PROMPT; break;
                    case 1: *currentMenu = MENU_OPTIONS; break;
                    case 2: *currentMenu = MENU_SCORE; break;
                    case 3: *currentMenu = MENU_STORY; break;
                    case 4: *running = 0; break;
                }
                break;
        }
    }

    for(int i = 0; i < BUTTON_COUNT; i++)
        buttons[i].hovered = (i == selectedButton);
}

void updateMenu()
{
    if(SDL_GetTicks() - lastFrameTime > frameDelay)
    {
        currentFrame = (currentFrame + 1) % MENU_BG_FRAMES; // 258 frames
        lastFrameTime = SDL_GetTicks();
    }
}

void renderMenu(SDL_Renderer* renderer)
{
    // ===== BACKGROUND =====
    if(bgMenu[currentFrame])
        SDL_RenderCopy(renderer, bgMenu[currentFrame], NULL, NULL);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 140);
    fillRounded(renderer, titleBgRect, 12, (SDL_Color){0,0,0,140});
    strokeRounded(renderer, titleBgRect, 12, (SDL_Color){220,180,90,220});
    SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
    if (logoTexture) SDL_RenderCopy(renderer, logoTexture, NULL, &logoRect);

    for(int i = 0; i < BUTTON_COUNT; i++)
    {
        SDL_Rect rect = buttons[i].rect;

        if(buttons[i].hovered)
        {
            rect.x -= 5;
            rect.y -= 3;
            rect.w += 10;
            rect.h += 6;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 120);
        SDL_Rect shadow = {rect.x + 4, rect.y + 4, rect.w, rect.h};
        SDL_RenderFillRect(renderer, &shadow);

        fillRounded(renderer, rect, 12, (SDL_Color){10,15,25,180});
        SDL_Color border = {
            (Uint8)(buttons[i].hovered ? 255 : 180),
            (Uint8)(buttons[i].hovered ? 215 : 140),
            (Uint8)(buttons[i].hovered ? 0 : 60),
            220
        };
        strokeRounded(renderer, rect, 12, border);

        SDL_RenderCopy(renderer, buttons[i].iconTexture, NULL, &buttons[i].iconRect);
        SDL_RenderCopy(renderer, buttons[i].textTexture, NULL, &buttons[i].textRect);
    }
}

void destroyMenu()
{
    for(int i = 0; i < BUTTON_COUNT; i++)
    {
        if(buttons[i].textTexture)
            SDL_DestroyTexture(buttons[i].textTexture);
        if(buttons[i].iconTexture)
            SDL_DestroyTexture(buttons[i].iconTexture);
    }

    if(titleTexture)
        SDL_DestroyTexture(titleTexture);

    if(menuMusic)
        Mix_FreeMusic(menuMusic);
    if(gameMusic)
        Mix_FreeMusic(gameMusic);

    if(hoverSound)
        Mix_FreeChunk(hoverSound);

    if(clickSound)
        Mix_FreeChunk(clickSound);

    if (logoTexture)
        SDL_DestroyTexture(logoTexture);

    TTF_CloseFont(font);
    TTF_CloseFont(titleFont);
}
