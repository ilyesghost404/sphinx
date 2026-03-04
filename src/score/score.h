#ifndef SCORE_H
#define SCORE_H

#include "../common.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define MAX_SCORES 50
#define SCORE_FILE "assets/data/data.dat"

typedef struct {
    char name[32];
    int score;
} PlayerScore;

// ===== File operations =====
int loadScores(PlayerScore* scores, int maxScores);
void saveScore(PlayerScore newScore);
void sortScores(PlayerScore* scores, int count);

// ===== SDL functions =====
void initScore(SDL_Renderer* renderer);
void handleScoreEvent(SDL_Event* e, MenuState* currentMenu);
void updateScore();
void renderScore(SDL_Renderer* renderer);
void destroyScore();
void goToScoreList();

#endif
