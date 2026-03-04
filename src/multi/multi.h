#ifndef MULTI_H
#define MULTI_H

#include <SDL2/SDL.h>
#include "common.h"

void initMulti(SDL_Renderer* renderer);
void handleMultiEvent(SDL_Event* e, MenuState* currentMenu);
void updateMulti();
void renderMulti(SDL_Renderer* renderer);
void destroyMulti();

#endif
