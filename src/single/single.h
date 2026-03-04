#ifndef SINGLE_H
#define SINGLE_H

#include <SDL2/SDL.h>
#include "common.h"

void initSingle(SDL_Renderer* renderer);
void handleSingleEvent(SDL_Event* e, MenuState* currentMenu);
void updateSingle();
void renderSingle(SDL_Renderer* renderer);
void destroySingle();

#endif
