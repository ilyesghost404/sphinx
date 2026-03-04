#ifndef QUIZ_H
#define QUIZ_H

#include "../common.h"
#include <SDL2/SDL.h>

void initQuiz(SDL_Renderer* renderer);
void handleQuizEvent(SDL_Event* e, MenuState* currentMenu);
void updateQuiz();
void renderQuiz(SDL_Renderer* renderer);
void destroyQuiz();
void resetQuiz();

#endif
