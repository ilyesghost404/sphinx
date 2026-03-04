#ifndef ENIGM_H
#define ENIGM_H

#include "../common.h"
#include <SDL2/SDL.h>

void initEnigm(SDL_Renderer* renderer);
void handleEnigmEvent(SDL_Event* e, MenuState* currentMenu);
void updateEnigm();
void renderEnigm(SDL_Renderer* renderer);
void destroyEnigm();

#endif
