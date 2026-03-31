#ifndef SUPPORT_H
#define SUPPORT_H

#include <SDL2/SDL.h>
#include "../common.h"

void initSupport(SDL_Renderer* renderer);

/* After scroll milestone: wizard appears ahead on the path (still MENU_GAME). */
void supportSpawnWizardAhead(SDL_Renderer* renderer, float playerWorldX);

/* Each frame while approaching: animate + start dialogue when player is close. */
void supportUpdateApproach(SDL_Renderer* renderer, MenuState* currentMenu, float playerWorldX);

/* Draw distant wizard during gameplay (smaller when far). */
void supportRenderFieldWizard(SDL_Renderer* renderer, float cameraX, float playerWorldX, float groundY);

int supportIsAwaitingApproach(void);
int supportIsDialogueFinished(void);
void supportUpdateFarewell(float playerWorldX);
int supportShouldRenderFieldWizard(void);
void supportResetForNewRun(void);

void handleSupportEvent(SDL_Event* e, MenuState* currentMenu);
void updateSupport(void);
void renderSupport(SDL_Renderer* renderer);
void destroySupport(void);

#endif
