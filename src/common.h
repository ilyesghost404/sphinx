#ifndef COMMON_H
#define COMMON_H

#include <SDL2/SDL.h>

// ================= SCREEN =================
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

// ================= MENU STATES =================
typedef enum {
    MENU_MAIN,
    MENU_OPTIONS,
    MENU_GAME,
    MENU_STORY,
    MENU_SCORE,
    MENU_PLAY,
    MENU_SAVE_PROMPT,
    MENU_NEWGAME,
    MENU_SINGLE,
    MENU_MULTI,
    MENU_ENIGM,
    MENU_QUIZ
} MenuState;

// ================= BACKGROUND FRAME COUNTS =================
#define MENU_BG_FRAMES        258
#define SHARED_BG_FRAMES      152   // Play + Options

// ===== GLOBAL ANIMATED BACKGROUNDS =====
extern SDL_Texture* bgMenu[MENU_BG_FRAMES];
extern SDL_Texture* bgShared[SHARED_BG_FRAMES];

// ===== SHARED BACKGROUND FRAME INDEX =====
extern int gCurrentFrame;

void updateSharedBackground(int targetFps);

#endif
