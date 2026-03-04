#ifndef SAVE_H
#define SAVE_H

#include <SDL2/SDL.h>
#include "common.h"

// ===== Save Button Struct =====
typedef struct {
    SDL_Rect rect;
    SDL_Texture* textTexture;
    SDL_Rect textRect;
    int hovered;
} SaveButton;

// ===== Constants =====
#define SAVE_BUTTON_COUNT 2  // Example: NEW SAVE, LOAD SAVE

// ===== Functions =====
void initSave(SDL_Renderer* renderer);                       // Initialize Save menu
void handleSaveEvent(SDL_Event* e, MenuState* currentMenuPtr); // Handle mouse/keyboard events
void updateSave();                                           // Update animation or logic
void renderSave(SDL_Renderer* renderer);                     // Render Save menu
void destroySave();                                          // Free resources
void saveGame();                                             // Actual save game logic

#endif
