#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>

#include "src/common.h"
#include "src/menu/menu.h"
#include "src/options/options.h"
#include "src/story/story.h"
#include "src/score/score.h"
#include "src/play/play.h"
#include "src/save/save.h"
#include "src/newgame/newgame.h"
#include "src/single/single.h"
#include "src/multi/multi.h"
#include "src/enigm/enigm.h"
#include "src/quiz/quiz.h"

// Global state
MenuState currentMenu = MENU_MAIN;

static TTF_Font* gLoadingTitleFont = NULL;
static TTF_Font* gLoadingSmallFont = NULL;
static SDL_Texture* gLoadingTitleTex = NULL;
static SDL_Rect gLoadingTitleRect;

static void renderLoading(SDL_Renderer* r, SDL_Texture* bg, float progress)
{
    if (bg) SDL_RenderCopy(r, bg, NULL, NULL);
    if (!gLoadingTitleFont) gLoadingTitleFont = TTF_OpenFont("assets/fonts/ARIAL.TTF", 64);
    if (!gLoadingSmallFont) gLoadingSmallFont = TTF_OpenFont("assets/fonts/ARIAL.TTF", 24);
    if (!gLoadingTitleTex && gLoadingTitleFont)
    {
        SDL_Color gold = {220,180,90,255};
        SDL_Surface* s = TTF_RenderText_Blended(gLoadingTitleFont, "SPHINX: THE LOST NOSE", gold);
        if (s)
        {
            gLoadingTitleTex = SDL_CreateTextureFromSurface(r, s);
            gLoadingTitleRect.w = s->w; gLoadingTitleRect.h = s->h;
            gLoadingTitleRect.x = (SCREEN_WIDTH - s->w)/2;
            gLoadingTitleRect.y = 80;
            SDL_FreeSurface(s);
        }
    }
    SDL_Rect titleBg = { gLoadingTitleRect.x - 24, gLoadingTitleRect.y - 16, gLoadingTitleRect.w + 48, gLoadingTitleRect.h + 32 };
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 140);
    SDL_RenderFillRect(r, &titleBg);
    if (gLoadingTitleTex) SDL_RenderCopy(r, gLoadingTitleTex, NULL, &gLoadingTitleRect);

    int w = (int)(SCREEN_WIDTH * 0.5f);
    int h = 24;
    int x = (SCREEN_WIDTH - w) / 2;
    int y = SCREEN_HEIGHT - 100;
    SDL_Rect track = (SDL_Rect){x, y, w, h};
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_RenderFillRect(r, &track);
    int fw = (int)((w - 4) * (progress < 0 ? 0 : (progress > 1 ? 1 : progress)));
    SDL_Rect fill = (SDL_Rect){x + 2, y + 2, fw, h - 4};
    SDL_SetRenderDrawColor(r, 220, 180, 90, 220);
    SDL_RenderFillRect(r, &fill);

    int pct = (int)(progress * 100.0f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    char buf[8]; snprintf(buf, sizeof(buf), "%d%%", pct);
    SDL_Color gold = {220,180,90,255};
    if (gLoadingSmallFont)
    {
        SDL_Surface* ps = TTF_RenderText_Blended(gLoadingSmallFont, buf, gold);
        if (ps)
        {
            SDL_Texture* pt = SDL_CreateTextureFromSurface(r, ps);
            SDL_Rect pr = { track.x + track.w + 16, track.y + (track.h - ps->h)/2, ps->w, ps->h };
            SDL_Rect pbg = { pr.x - 12, pr.y - 6, pr.w + 24, pr.h + 12 };
            SDL_SetRenderDrawColor(r, 0, 0, 0, 140);
            SDL_RenderFillRect(r, &pbg);
            SDL_RenderCopy(r, pt, NULL, &pr);
            SDL_DestroyTexture(pt);
            SDL_FreeSurface(ps);
        }
    }
}

int main(int argc, char* argv[])
{
    // ================= SDL INIT =================
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        printf("SDL init failed: %s\n", SDL_GetError());
        return -1;
    }

    if (TTF_Init() < 0)
    {
        printf("TTF init failed: %s\n", TTF_GetError());
        return -1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        printf("SDL_image init failed: %s\n", IMG_GetError());
        return -1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        printf("SDL_mixer init failed: %s\n", Mix_GetError());
        return -1;
    }

    Mix_AllocateChannels(16);

    // ================= WINDOW =================
    SDL_Window* window = SDL_CreateWindow(
        "SPHINX: THE LOST NOSE",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window)
    {
        printf("Window creation failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Surface* iconSurf = IMG_Load("assets/images/ui/sphinx.png");
    if (iconSurf)
    {
        SDL_SetWindowIcon(window, iconSurf);
        SDL_FreeSurface(iconSurf);
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer)
    {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Texture* loadingBg = NULL;
    {
        SDL_Surface* s0 = IMG_Load("assets/images/backgrounds/menu/frame_000000.png");
        if (s0) { loadingBg = SDL_CreateTextureFromSurface(renderer, s0); SDL_FreeSurface(s0); }
    }

    char path[256];
    SDL_Surface* surface;

    // -------- MENU BACKGROUND --------
    for (int i = 0; i < MENU_BG_FRAMES; i++)
    {
        sprintf(path, "assets/images/backgrounds/menu/frame_%06d.png", i);
        surface = IMG_Load(path);

        if (!surface)
        {
            printf("Failed to load %s\n", path);
            bgMenu[i] = NULL;
        } else {
            bgMenu[i] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
        }

        float progress = (float)(i + 1) / (MENU_BG_FRAMES + SHARED_BG_FRAMES);
        SDL_RenderClear(renderer);
        renderLoading(renderer, loadingBg ? loadingBg : NULL, progress);
        SDL_RenderPresent(renderer);
        SDL_Event ev; while (SDL_PollEvent(&ev)) { if (ev.type == SDL_QUIT) { goto cleanup; } }
    }

    // -------- SHARED BACKGROUND (PLAY + OPTIONS) --------
    surface = IMG_Load("assets/images/backgrounds/play.png");
    if (!surface)
    {
        printf("Failed to load assets/images/backgrounds/play.png: %s\n", IMG_GetError());
        for (int i = 0; i < SHARED_BG_FRAMES; i++)
            bgShared[i] = NULL;
    }
    else
    {
        for (int i = 0; i < SHARED_BG_FRAMES; i++)
            bgShared[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    float progress = (float)(MENU_BG_FRAMES + SHARED_BG_FRAMES) / (MENU_BG_FRAMES + SHARED_BG_FRAMES);
    SDL_RenderClear(renderer);
    renderLoading(renderer, loadingBg ? loadingBg : NULL, progress);
    SDL_RenderPresent(renderer);
    SDL_Event ev; while (SDL_PollEvent(&ev)) { if (ev.type == SDL_QUIT) { goto cleanup; } }

    
 

   

    // Save uses the shared background

    // ================= INIT SECTIONS =================
    if (loadingBg) { SDL_DestroyTexture(loadingBg); loadingBg = NULL; }
    if (gLoadingTitleTex) { SDL_DestroyTexture(gLoadingTitleTex); gLoadingTitleTex = NULL; }
    if (gLoadingTitleFont) { TTF_CloseFont(gLoadingTitleFont); gLoadingTitleFont = NULL; }
    if (gLoadingSmallFont) { TTF_CloseFont(gLoadingSmallFont); gLoadingSmallFont = NULL; }
    initMenu(renderer);
    initOptions(renderer);
    initStory(renderer);
    initScore(renderer);
    initPlay(renderer);
    initSave(renderer);
    initNewGame(renderer);
    initSingle(renderer);
    initMulti(renderer);
    initEnigm(renderer);
    initQuiz(renderer);

    int running = 1;
    SDL_Event e;

    // ================= GAME LOOP =================
    while (running)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = 0;

            if (currentMenu == MENU_MAIN)
                handleMenuEvent(&e, &running, &currentMenu);
            else if (currentMenu == MENU_OPTIONS)
                handleOptionsEvent(&e, &running, window);
            else if (currentMenu == MENU_STORY)
                handleStoryEvent(&e);
            else if (currentMenu == MENU_SCORE)
                handleScoreEvent(&e, &currentMenu);
            else if (currentMenu == MENU_SAVE_PROMPT)
                handleSaveEvent(&e, &currentMenu);
            else if (currentMenu == MENU_PLAY)
                handlePlayEvent(&e, &currentMenu);
            else if (currentMenu == MENU_NEWGAME)
                handleNewGameEvent(&e, &currentMenu);
            else if (currentMenu == MENU_SINGLE)
                handleSingleEvent(&e, &currentMenu);
            else if (currentMenu == MENU_MULTI)
                handleMultiEvent(&e, &currentMenu);
            else if (currentMenu == MENU_ENIGM)
                handleEnigmEvent(&e, &currentMenu);
            else if (currentMenu == MENU_QUIZ)
                handleQuizEvent(&e, &currentMenu);
        }

        if (currentMenu == MENU_MAIN)
            updateMenu();
        else if (currentMenu == MENU_OPTIONS)
            updateOptions();
        else if (currentMenu == MENU_STORY)
            updateStory();
        else if (currentMenu == MENU_SCORE)
            updateScore();
        else if (currentMenu == MENU_SAVE_PROMPT)
            updateSave();
        else if (currentMenu == MENU_PLAY)
            updatePlay();
        else if (currentMenu == MENU_NEWGAME)
            updateNewGame();
        else if (currentMenu == MENU_SINGLE)
            updateSingle();
        else if (currentMenu == MENU_MULTI)
            updateMulti();
        else if (currentMenu == MENU_ENIGM)
            updateEnigm();
        else if (currentMenu == MENU_QUIZ)
            updateQuiz();

        SDL_RenderClear(renderer);

        if (currentMenu == MENU_MAIN)
            renderMenu(renderer);
        else if (currentMenu == MENU_OPTIONS)
            renderOptions(renderer, window);
        else if (currentMenu == MENU_STORY)
            renderStory(renderer);
        else if (currentMenu == MENU_SCORE)
            renderScore(renderer);
        else if (currentMenu == MENU_SAVE_PROMPT)
            renderSave(renderer);
        else if (currentMenu == MENU_PLAY)
            renderPlay(renderer);
        else if (currentMenu == MENU_NEWGAME)
            renderNewGame(renderer);
        else if (currentMenu == MENU_SINGLE)
            renderSingle(renderer);
        else if (currentMenu == MENU_MULTI)
            renderMulti(renderer);
        else if (currentMenu == MENU_ENIGM)
            renderEnigm(renderer);
        else if (currentMenu == MENU_QUIZ)
            renderQuiz(renderer);

        SDL_RenderPresent(renderer);
    }

    // ================= CLEANUP =================

cleanup:
    for (int i = 0; i < MENU_BG_FRAMES; i++)
        if (bgMenu[i]) SDL_DestroyTexture(bgMenu[i]);

    for (int i = 0; i < SHARED_BG_FRAMES; i++)
        if (bgShared[i]) SDL_DestroyTexture(bgShared[i]);

    

    destroyMenu();
    destroyOptions();
    destroyStory();
    destroyScore();
    destroyPlay();
    destroySave();
    destroyNewGame();
    destroySingle();
    destroyMulti();
    destroyEnigm();
    destroyQuiz();

    Mix_CloseAudio();
    Mix_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}
