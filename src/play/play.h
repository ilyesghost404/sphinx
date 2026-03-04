#ifndef PLAY_H
#define PLAY_H

#include <SDL2/SDL.h>
#include "common.h"

// ===== Functions =====
void initPlay(SDL_Renderer* renderer);
void handlePlayEvent(SDL_Event* e, MenuState* currentMenu);
void updatePlay();
void renderPlay(SDL_Renderer* renderer);
void destroyPlay();

#endif // PLAY_H
