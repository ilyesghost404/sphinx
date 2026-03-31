#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <SDL2/SDL.h>
#include "../common.h"

void initBackground(SDL_Renderer* renderer);
void handleBackgroundEvent(SDL_Event* e, MenuState* currentMenu);
void updateBackground(MenuState* currentMenu);
void renderBackground(SDL_Renderer* renderer);
void destroyBackground();

float backgroundGetCameraX(void);
float backgroundGetPlayerWorldX(void);
float backgroundGetPlayerGroundY(void);
void backgroundOnEnterGameplay(void);

/* Clears held move/jump/attack flags (e.g. after UI swallowed key ups). */
void backgroundClearGameplayInput(void);

void backgroundGrantOneMoreLife(void);

#endif
