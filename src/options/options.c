#include "options.h"
#include "common.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// ===== BACKGROUND ANIMATION =====
extern SDL_Texture* bgShared[SHARED_BG_FRAMES];
extern int gCurrentFrame;

// ===== UI Elements =====
SDL_Texture* displayLabelTexture = NULL;
SDL_Rect displayLabelRect;

SDL_Texture* volumeLabelTexture = NULL;
SDL_Rect volumeLabelRect;

SDL_Rect optionsPanelRect;

OptionButton optionsButtons[OPTIONS_BUTTON_COUNT];
int selectedOptionButton = 0;

#define VOLUME_BAR_WIDTH 350
#define VOLUME_BAR_HEIGHT 25
SDL_Rect volumeBarRect;
SDL_Rect volumeFillRect;

// ===== External Assets =====
extern TTF_Font* font;
extern Mix_Chunk* hoverSound;
extern Mix_Chunk* clickSound;
extern MenuState currentMenu;
extern Mix_Music* menuMusic;

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

void initOptions(SDL_Renderer* renderer)
{
    if (!font) return;

    int panelWidth = SCREEN_WIDTH * 0.65;
    int panelHeight = SCREEN_HEIGHT * 0.75;
    optionsPanelRect = (SDL_Rect){
        (SCREEN_WIDTH - panelWidth) / 2,
        (SCREEN_HEIGHT - panelHeight) / 2,
        panelWidth,
        panelHeight
    };

    SDL_Color gold = {220,180,90,255};
    const char* labels[OPTIONS_BUTTON_COUNT] = {"FULLSCREEN", "NORMAL", "+", "-", "BACK"};

    // ===== Labels =====
    SDL_Surface* displaySurface = TTF_RenderText_Blended(font, "DISPLAY MODE", gold);
    displayLabelTexture = SDL_CreateTextureFromSurface(renderer, displaySurface);
    displayLabelRect.w = displaySurface->w;
    displayLabelRect.h = displaySurface->h;
    SDL_FreeSurface(displaySurface);

    SDL_Surface* volumeSurface = TTF_RenderText_Blended(font, "VOLUME", gold);
    volumeLabelTexture = SDL_CreateTextureFromSurface(renderer, volumeSurface);
    volumeLabelRect.w = volumeSurface->w;
    volumeLabelRect.h = volumeSurface->h;
    SDL_FreeSurface(volumeSurface);

    int marginTop = 50;
    int spacing = 25;
    int buttonWidth = 260;
    int buttonHeight = 55;
    int centerX = optionsPanelRect.x + optionsPanelRect.w / 2;
    int currentY = optionsPanelRect.y + marginTop;

    // Display Label
    displayLabelRect.x = centerX - displayLabelRect.w / 2;
    displayLabelRect.y = currentY;
    currentY += displayLabelRect.h + 20;

    // Display mode buttons
    optionsButtons[0].rect = (SDL_Rect){centerX - buttonWidth/2, currentY, buttonWidth, buttonHeight};
    currentY += buttonHeight + spacing;
    optionsButtons[1].rect = (SDL_Rect){centerX - buttonWidth/2, currentY, buttonWidth, buttonHeight};
    currentY += buttonHeight + 50;

    // Volume Label
    volumeLabelRect.x = centerX - volumeLabelRect.w / 2;
    volumeLabelRect.y = currentY;
    currentY += volumeLabelRect.h + 20;

    // Volume Bar
    volumeBarRect = (SDL_Rect){centerX - VOLUME_BAR_WIDTH/2, currentY, VOLUME_BAR_WIDTH, VOLUME_BAR_HEIGHT};
    currentY += VOLUME_BAR_HEIGHT + 20;

    // Volume Buttons
    int volButtonWidth = 120;
    optionsButtons[3].rect = (SDL_Rect){centerX - volButtonWidth - 20, currentY, volButtonWidth, buttonHeight}; // "-"
    optionsButtons[2].rect = (SDL_Rect){centerX + 20, currentY, volButtonWidth, buttonHeight};              // "+"
    currentY += buttonHeight + 40;

    // Back Button
    optionsButtons[4].rect = (SDL_Rect){centerX - buttonWidth/2, optionsPanelRect.y + optionsPanelRect.h - 70, buttonWidth, buttonHeight};

    // Load Button Textures
    for(int i=0; i<OPTIONS_BUTTON_COUNT; i++)
    {
        optionsButtons[i].hovered = 0;
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, labels[i], gold);
        optionsButtons[i].textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        optionsButtons[i].textRect.w = textSurface->w;
        optionsButtons[i].textRect.h = textSurface->h;
        optionsButtons[i].textRect.x = optionsButtons[i].rect.x + (optionsButtons[i].rect.w - textSurface->w)/2;
        optionsButtons[i].textRect.y = optionsButtons[i].rect.y + (optionsButtons[i].rect.h - textSurface->h)/2;
        SDL_FreeSurface(textSurface);
    }
}

void updateOptions()
{
    updateSharedBackground(30);
}

void renderOptions(SDL_Renderer* renderer, SDL_Window* window)
{
    // ===== Background =====
    if(bgShared[gCurrentFrame])
        SDL_RenderCopy(renderer, bgShared[gCurrentFrame], NULL, NULL);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Panel Shadow
    SDL_SetRenderDrawColor(renderer, 0,0,0,180);
    SDL_Rect shadow = {optionsPanelRect.x+10, optionsPanelRect.y+10, optionsPanelRect.w, optionsPanelRect.h};
    SDL_RenderFillRect(renderer, &shadow);

    // Panel
    fillRounded(renderer, optionsPanelRect, 16, (SDL_Color){10,15,25,180});
    strokeRounded(renderer, optionsPanelRect, 16, (SDL_Color){180,140,60,220});

    // Labels
    SDL_RenderCopy(renderer, displayLabelTexture, NULL, &displayLabelRect);
    SDL_RenderCopy(renderer, volumeLabelTexture, NULL, &volumeLabelRect);

    // Buttons
    for(int i=0; i<OPTIONS_BUTTON_COUNT; i++)
    {
        SDL_Rect rect = optionsButtons[i].rect;
        if(optionsButtons[i].hovered)
        {
            rect.x -= 6; rect.y -= 4;
            rect.w += 12; rect.h += 8;
        }
        fillRounded(renderer, rect, 12, (SDL_Color){10,15,25,180});
        SDL_Color border = {
            (Uint8)(optionsButtons[i].hovered?255:180),
            (Uint8)(optionsButtons[i].hovered?215:140),
            (Uint8)(optionsButtons[i].hovered?0:60),
            220
        };
        strokeRounded(renderer, rect, 12, border);
        SDL_RenderCopy(renderer, optionsButtons[i].textTexture, NULL, &optionsButtons[i].textRect);
    }

    // Volume Bar
    int volume = Mix_VolumeMusic(-1);
    if(volume < 0) volume = 0;
    if(volume > 128) volume = 128;

    float percent = volume / 128.0f;
    volumeFillRect = (SDL_Rect){volumeBarRect.x+2, volumeBarRect.y+2, (int)((VOLUME_BAR_WIDTH-4)*percent), VOLUME_BAR_HEIGHT-4};

    fillRounded(renderer, volumeBarRect, 8, (SDL_Color){60,60,100,200});
    fillRounded(renderer, volumeFillRect, 8, (SDL_Color){255,215,0,220});
    strokeRounded(renderer, volumeBarRect, 8, (SDL_Color){180,140,60,220});
}

void handleOptionsEvent(SDL_Event* e, int* running, SDL_Window* window)
{
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    int currentHovered = -1;

    for(int i=0; i<OPTIONS_BUTTON_COUNT; i++)
    {
        optionsButtons[i].hovered = mouseX >= optionsButtons[i].rect.x &&
                                    mouseX <= optionsButtons[i].rect.x + optionsButtons[i].rect.w &&
                                    mouseY >= optionsButtons[i].rect.y &&
                                    mouseY <= optionsButtons[i].rect.y + optionsButtons[i].rect.h;
        if(optionsButtons[i].hovered) currentHovered = i;
    }

    if(currentHovered != -1 && currentHovered != selectedOptionButton && hoverSound)
        Mix_PlayChannel(-1, hoverSound, 0);

    selectedOptionButton = currentHovered != -1 ? currentHovered : selectedOptionButton;

    if(e->type == SDL_MOUSEBUTTONDOWN)
    {
        if(clickSound) Mix_PlayChannel(-1, clickSound, 0);

        switch(selectedOptionButton)
        {
            case 0: SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN); break;
            case 1: SDL_SetWindowFullscreen(window, 0); break;
            case 2: Mix_VolumeMusic(Mix_VolumeMusic(-1)+16); break;
            case 3: Mix_VolumeMusic(Mix_VolumeMusic(-1)-16); break;
            case 4:
                currentMenu = MENU_MAIN;
                if (menuMusic) Mix_PlayMusic(menuMusic, -1);
                break;
        }
    }

    if(e->type == SDL_KEYDOWN)
    {
        SDL_Keycode key = e->key.keysym.sym;
        switch (key)
        {
            case SDLK_ESCAPE:
                currentMenu = MENU_MAIN;
                if (menuMusic) Mix_PlayMusic(menuMusic, -1);
                break;
            case 'f':
            case 'F':
            {
                if(clickSound) Mix_PlayChannel(-1, clickSound, 0);
                Uint32 flags = SDL_GetWindowFlags(window);
                if (flags & SDL_WINDOW_FULLSCREEN)
                {
                    SDL_SetWindowFullscreen(window, 0);
                    selectedOptionButton = 1; // NORMAL
                }
                else
                {
                    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
                    selectedOptionButton = 0; // FULLSCREEN
                }
                break;
            }
            case '+':
            case '=':
            case SDLK_KP_PLUS:
            {
                int v = Mix_VolumeMusic(-1);
                v += 16;
                if (v > 128) v = 128;
                Mix_VolumeMusic(v);
                if(clickSound) Mix_PlayChannel(-1, clickSound, 0);
                selectedOptionButton = 2; // VOL+
                break;
            }
            case '-':
            case SDLK_KP_MINUS:
            {
                int v = Mix_VolumeMusic(-1);
                v -= 16;
                if (v < 0) v = 0;
                Mix_VolumeMusic(v);
                if(clickSound) Mix_PlayChannel(-1, clickSound, 0);
                selectedOptionButton = 3; // VOL-
                break;
            }
        }
    }

    for(int i=0; i<OPTIONS_BUTTON_COUNT; i++)
        optionsButtons[i].hovered = (i == selectedOptionButton);
}

void destroyOptions()
{
    for(int i=0; i<OPTIONS_BUTTON_COUNT; i++)
        if(optionsButtons[i].textTexture) SDL_DestroyTexture(optionsButtons[i].textTexture);

    if(displayLabelTexture) SDL_DestroyTexture(displayLabelTexture);
    if(volumeLabelTexture) SDL_DestroyTexture(volumeLabelTexture);
}
