#include "save.h"
#include "common.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern TTF_Font* font;
extern Mix_Chunk* hoverSound;
extern Mix_Chunk* clickSound;
extern Mix_Music* menuMusic;
extern Mix_Music* gameMusic;

// ===== Shared Background From main.c =====
extern SDL_Texture* bgShared[SHARED_BG_FRAMES];
extern int gCurrentFrame;

// ===== Panel & Buttons =====
SDL_Rect savePanelRect;

#define SAVE_BUTTON_COUNT 2
SaveButton saveButtons[SAVE_BUTTON_COUNT];
int selectedSaveButton = 0;

// ===== Question Text =====
SDL_Texture* questionTexture = NULL;
SDL_Rect questionRect;

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
// ================= INIT ==================================
// =========================================================
void initSave(SDL_Renderer* renderer)
{
    if(!font) return;

    SDL_Color gold = {220,180,90,255};

    // ===== PANEL =====
    int panelWidth = SCREEN_WIDTH * 0.4;
    int panelHeight = SCREEN_HEIGHT * 0.3;

    savePanelRect = (SDL_Rect){
        (SCREEN_WIDTH - panelWidth) / 2,
        (SCREEN_HEIGHT - panelHeight) / 2,
        panelWidth,
        panelHeight
    };

    // ===== QUESTION TEXT =====
    SDL_Surface* questionSurface =
        TTF_RenderText_Blended(font, "DO YOU WANT TO SAVE ?", gold);

    questionTexture = SDL_CreateTextureFromSurface(renderer, questionSurface);

    questionRect.w = questionSurface->w;
    questionRect.h = questionSurface->h;
    questionRect.x = SCREEN_WIDTH/2 - questionSurface->w/2;
    questionRect.y = savePanelRect.y + 40;

    SDL_FreeSurface(questionSurface);

    // ===== BUTTONS =====
    const char* labels[SAVE_BUTTON_COUNT] = {"YES", "NO"};

    int buttonWidth = 160;
    int buttonHeight = 55;
    int spacing = 50;
    int centerX = SCREEN_WIDTH / 2;
    int posY = savePanelRect.y + savePanelRect.h - buttonHeight - 40;

    for(int i=0; i<SAVE_BUTTON_COUNT; i++)
    {
        saveButtons[i].rect = (SDL_Rect){
            centerX - buttonWidth - spacing/2 + i*(buttonWidth + spacing),
            posY,
            buttonWidth,
            buttonHeight
        };

        saveButtons[i].hovered = 0;

        SDL_Surface* surf =
            TTF_RenderText_Blended(font, labels[i], gold);

        saveButtons[i].textTexture =
            SDL_CreateTextureFromSurface(renderer, surf);

        saveButtons[i].textRect.w = surf->w;
        saveButtons[i].textRect.h = surf->h;
        saveButtons[i].textRect.x =
            saveButtons[i].rect.x + (buttonWidth - surf->w)/2;
        saveButtons[i].textRect.y =
            saveButtons[i].rect.y + (buttonHeight - surf->h)/2;

        SDL_FreeSurface(surf);
    }
}

// =========================================================
// ================= UPDATE ================================
// =========================================================
void updateSave()
{
    updateSharedBackground(30);
}

// =========================================================
// ================= RENDER ================================
// =========================================================
void renderSave(SDL_Renderer* renderer)
{
    // ===== Shared Background =====
    if(bgShared[gCurrentFrame])
        SDL_RenderCopy(renderer, bgShared[gCurrentFrame], NULL, NULL);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Dark screen fade
    SDL_SetRenderDrawColor(renderer, 0,0,0,180);
    SDL_Rect screenRect = {0,0,SCREEN_WIDTH,SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &screenRect);

    // Panel shadow
    SDL_SetRenderDrawColor(renderer,0,0,0,180);
    SDL_Rect shadow = {
        savePanelRect.x+8,
        savePanelRect.y+8,
        savePanelRect.w,
        savePanelRect.h
    };
    SDL_RenderFillRect(renderer,&shadow);

    // Panel
    fillRounded(renderer, savePanelRect, 16, (SDL_Color){10,15,25,180});
    strokeRounded(renderer, savePanelRect, 16, (SDL_Color){180,140,60,220});

    // Question text
    SDL_RenderCopy(renderer, questionTexture, NULL, &questionRect);

    // Buttons
    for(int i=0;i<SAVE_BUTTON_COUNT;i++)
    {
        SDL_Rect rect = saveButtons[i].rect;

        if(saveButtons[i].hovered)
        {
            rect.x -= 6;
            rect.y -= 4;
            rect.w += 12;
            rect.h += 8;
        }

        fillRounded(renderer, rect, 12, (SDL_Color){10,15,25,180});
        SDL_Color border = {
            (Uint8)(saveButtons[i].hovered?255:180),
            (Uint8)(saveButtons[i].hovered?215:140),
            (Uint8)(saveButtons[i].hovered?0:60),
            220
        };
        strokeRounded(renderer, rect, 12, border);

        SDL_RenderCopy(renderer,
            saveButtons[i].textTexture,
            NULL,
            &saveButtons[i].textRect);
    }
}

// =========================================================
// ================= EVENTS ================================
// =========================================================
void handleSaveEvent(SDL_Event* e, MenuState* currentMenuPtr)
{
    int mx, my;
    SDL_GetMouseState(&mx,&my);

    int hovered = -1;

    for(int i=0;i<SAVE_BUTTON_COUNT;i++)
    {
        saveButtons[i].hovered =
            mx >= saveButtons[i].rect.x &&
            mx <= saveButtons[i].rect.x + saveButtons[i].rect.w &&
            my >= saveButtons[i].rect.y &&
            my <= saveButtons[i].rect.y + saveButtons[i].rect.h;

        if(saveButtons[i].hovered)
            hovered = i;
    }

    if(hovered != -1 && hovered != selectedSaveButton && hoverSound)
        Mix_PlayChannel(-1, hoverSound, 0);

    selectedSaveButton = (hovered != -1) ? hovered : selectedSaveButton;

    if(e->type == SDL_MOUSEBUTTONDOWN)
    {
        if(selectedSaveButton != -1 &&
           saveButtons[selectedSaveButton].hovered)
        {
            if(clickSound)
                Mix_PlayChannel(-1, clickSound, 0);

            if(selectedSaveButton == 0)
                saveGame();

            *currentMenuPtr = MENU_PLAY;
            if (gameMusic) Mix_PlayMusic(gameMusic, -1);
        }
    }

    if(e->type == SDL_KEYDOWN)
    {
        switch(e->key.keysym.sym)
        {
            case SDLK_RIGHT:
                selectedSaveButton =
                    (selectedSaveButton + 1) % SAVE_BUTTON_COUNT;
                break;

            case SDLK_LEFT:
                selectedSaveButton =
                    (selectedSaveButton - 1 + SAVE_BUTTON_COUNT) % SAVE_BUTTON_COUNT;
                break;

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if(selectedSaveButton == 0)
                    saveGame();
                *currentMenuPtr = MENU_PLAY;
                if (gameMusic) Mix_PlayMusic(gameMusic, -1);
                break;

            case SDLK_ESCAPE:
                *currentMenuPtr = MENU_MAIN;
                if (menuMusic) Mix_PlayMusic(menuMusic, -1);
                break;
        }
    }

    for(int i=0;i<SAVE_BUTTON_COUNT;i++)
        saveButtons[i].hovered = (i == selectedSaveButton);
}

// =========================================================
// ================= DESTROY ===============================
// =========================================================
void destroySave()
{
    for(int i=0;i<SAVE_BUTTON_COUNT;i++)
        if(saveButtons[i].textTexture)
            SDL_DestroyTexture(saveButtons[i].textTexture);

    if(questionTexture)
        SDL_DestroyTexture(questionTexture);
}

// =========================================================
// ================= SAVE GAME =============================
// =========================================================
void saveGame()
{
    printf("Game saved successfully!\n");
}
