#ifndef NEWGAME_H
#define NEWGAME_H

#include <SDL2/SDL.h>
#include "common.h"

typedef enum { MODE_MONO, MODE_MULTI } GameMode;
typedef enum { AVATAR_BOY, AVATAR_GIRL } AvatarType;
typedef enum { INPUT_KBM, INPUT_CONTROLLER } InputType;

typedef struct {
    SDL_Rect rect;
    SDL_Texture* textTexture;
    SDL_Rect textRect;
    SDL_Texture* iconTexture;
    SDL_Rect iconRect;
    int hovered;
} NGButton;


void initNewGame(SDL_Renderer* renderer);
void handleNewGameEvent(SDL_Event* e, MenuState* currentMenu);
void updateNewGame();
void renderNewGame(SDL_Renderer* renderer);
void destroyNewGame();

#endif
