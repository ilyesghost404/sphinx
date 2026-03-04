#ifndef OPTIONS_H
#define OPTIONS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#define OPTIONS_BUTTON_COUNT 5  // FULLSCREEN, NORMAL, VOL+, VOL-, BACK

typedef struct {
    SDL_Rect rect;
    SDL_Texture* textTexture;
    SDL_Rect textRect;
    int hovered;
} OptionButton;

// ===== Core Functions =====
void initOptions(SDL_Renderer* renderer);
void handleOptionsEvent(SDL_Event* e, int* running, SDL_Window* window);
void renderOptions(SDL_Renderer* renderer, SDL_Window* window);
void updateOptions();
void destroyOptions();

#endif // OPTIONS_H
