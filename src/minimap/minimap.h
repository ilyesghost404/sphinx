#ifndef MINIMAP_H
#define MINIMAP_H

#include <SDL2/SDL.h>
#include "../common.h"
#include "../character/character.h"
#include "../enemy/enemy.h"

void initMiniMap(void);
void handleMiniMapEvent(SDL_Event* e);
void renderMiniMap(SDL_Renderer* r, const Character* player, const Ennemi* enemy, int enemySpawned);

#endif
