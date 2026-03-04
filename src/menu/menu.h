#ifndef MENU_H
#define MENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include "common.h"

#define BUTTON_COUNT 5

// ===== BUTTON STRUCTURE =====
typedef struct {
    SDL_Rect rect;           // Button rectangle
    SDL_Texture* textTexture;
    SDL_Rect textRect;
    SDL_Texture* iconTexture;
    SDL_Rect iconRect;
    int hovered;
} Button;

// ===== EXTERNAL VARIABLES =====
extern Mix_Chunk* clickSound;
extern Mix_Chunk* hoverSound;
extern TTF_Font* font;
extern TTF_Font* titleFont;

// ===== MENU FUNCTIONS =====
void initMenu(SDL_Renderer* renderer);
void handleMenuEvent(SDL_Event* e, int* running, MenuState* currentMenu);
void updateMenu();
void renderMenu(SDL_Renderer* renderer);
void destroyMenu();

#endif // MENU_H
