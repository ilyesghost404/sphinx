#include "game.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>

SDL_Color textColor   = {255, 140, 0, 255};
SDL_Color shadowColor = {0, 0, 0, 150};
SDL_Color btnBase     = {0, 0, 0, 150};
SDL_Color btnHover    = {255, 180, 100, 150};

typedef struct {
    SDL_Rect rect;
    char* text;
    float hoverFactor;
    int hoveredLastFrame;
} Button;

float lerp(float a, float b, float t) { return a + t * (b - a); }

static int isMouseOver(SDL_Rect rect, int x, int y) {
    return x >= rect.x && x <= rect.x + rect.w &&
           y >= rect.y && y <= rect.y + rect.h;
}

void drawButton(SDL_Renderer* renderer, SDL_Rect rect, float hoverFactor) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, btnBase.r, btnBase.g, btnBase.b, btnBase.a);
    SDL_RenderFillRect(renderer, &rect);

    if (hoverFactor > 0.01f) {
        SDL_SetRenderDrawColor(renderer, btnHover.r, btnHover.g, btnHover.b,
                               (Uint8)(btnHover.a * hoverFactor));
        SDL_RenderFillRect(renderer, &rect);
    }
}

int game_loop(SDL_Window* window, SDL_Renderer* renderer) {
    TTF_Font* font = TTF_OpenFont("assets/fonts/PatrickHand-Regular.ttf", 64);
    TTF_Font* buttonFont = TTF_OpenFont("assets/fonts/PatrickHand-Regular.ttf", 48);

    if (!font || !buttonFont) {
        printf("Font load error!\n");
        return 0;
    }

    // Background placeholder (just fill with dark color)
    SDL_Color bgColor = {20, 20, 40, 255};

    // Go Back button
    Button backButton = {{50, 50, 200, 70}, "Go Back", 0, 0};

    int running = 1;
    SDL_Event event;
    int mouseX, mouseY;
    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        Uint32 current = SDL_GetTicks();
        float dt = (current - lastTime)/1000.0f;
        lastTime = current;

        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) return -1;

            if(event.type == SDL_MOUSEBUTTONDOWN) {
                SDL_GetMouseState(&mouseX, &mouseY);
                if(isMouseOver(backButton.rect, mouseX, mouseY)) {
                    return 0; // back to menu
                }
            }
        }

        SDL_GetMouseState(&mouseX, &mouseY);
        int hover = isMouseOver(backButton.rect, mouseX, mouseY);
        backButton.hoveredLastFrame = hover;
        backButton.hoverFactor = lerp(backButton.hoverFactor, hover?1.0f:0.0f, dt*10);

        // Render
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderClear(renderer);

        // Render "Coming Soon"
        SDL_Surface* textSurf = TTF_RenderText_Blended(font, "Coming Soon", textColor);
        SDL_Texture* textTex = SDL_CreateTextureFromSurface(renderer, textSurf);
        int tw, th;
        SDL_QueryTexture(textTex, NULL, NULL, &tw, &th);
        SDL_Rect textRect = {1280/2 - tw/2, 720/2 - th/2, tw, th};
        SDL_RenderCopy(renderer, textTex, NULL, &textRect);
        SDL_FreeSurface(textSurf);
        SDL_DestroyTexture(textTex);

        // Draw back button
        float scale = 1.0f + 0.1f*backButton.hoverFactor;
        SDL_Rect rect = backButton.rect;
        rect.w *= scale;
        rect.h *= scale;
        rect.x = backButton.rect.x - (rect.w - backButton.rect.w)/2;
        rect.y = backButton.rect.y - (rect.h - backButton.rect.h)/2;
        drawButton(renderer, rect, backButton.hoverFactor);

        // Button text
        SDL_Surface* btnSurf = TTF_RenderText_Blended(buttonFont, backButton.text, textColor);
        SDL_Texture* btnTex = SDL_CreateTextureFromSurface(renderer, btnSurf);
        SDL_QueryTexture(btnTex, NULL, NULL, &tw, &th);
        SDL_Rect btnRect = {rect.x + (rect.w - tw)/2, rect.y + (rect.h - th)/2, tw, th};
        SDL_RenderCopy(renderer, btnTex, NULL, &btnRect);
        SDL_FreeSurface(btnSurf);
        SDL_DestroyTexture(btnTex);

        SDL_RenderPresent(renderer);
    }

    return 0;
}
