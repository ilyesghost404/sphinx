#ifndef STORY_H
#define STORY_H

#include <SDL2/SDL.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

void initStory(SDL_Renderer* renderer);
void handleStoryEvent(SDL_Event* e);
void updateStory();
void renderStory(SDL_Renderer* renderer);
void destroyStory();

#endif
