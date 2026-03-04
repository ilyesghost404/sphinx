#include "enigm.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include "../score/score.h"

extern TTF_Font* font;
extern Mix_Chunk* hoverSound;
extern Mix_Chunk* clickSound;
extern SDL_Texture* bgShared[SHARED_BG_FRAMES];
extern int gCurrentFrame;
extern Mix_Music* gameMusic;

static SDL_Texture* titleTexture = NULL;
static SDL_Rect titleRect;
static SDL_Texture* subtitleTexture = NULL;
static SDL_Rect subtitleRect;
static SDL_Rect headerRect;
static int bgStripeOffset = 0;
static Uint32 bgLastTick = 0;

typedef struct {
    SDL_Rect rect;
    SDL_Texture* textTexture;
    SDL_Rect textRect;
    SDL_Texture* iconTexture;
    SDL_Rect iconRect;
    int hovered;
} EnigmButton;

static SDL_Rect panelRect;
static EnigmButton quizBtn;
static EnigmButton puzzleBtn;
static EnigmButton backBtn;

static SDL_Color gold = {220,180,90,220};
static SDL_Color buttonColor = {10,15,25,180};
static SDL_Color buttonHover = {10,15,25,240};
static SDL_Texture* enigmBgTex = NULL;

static void setText(SDL_Renderer* r, SDL_Texture** tex, SDL_Rect* rect, const char* txt)
{
    if (*tex) { SDL_DestroyTexture(*tex); *tex = NULL; }
    SDL_Surface* surf = TTF_RenderText_Blended(font, txt, gold);
    *tex = SDL_CreateTextureFromSurface(r, surf);
    rect->w = surf->w;
    rect->h = surf->h;
    SDL_FreeSurface(surf);
}

static void setButton(SDL_Renderer* r, EnigmButton* b, const char* txt, int x, int y, int w, int h)
{
    b->rect = (SDL_Rect){x,y,w,h};
    setText(r, &b->textTexture, &b->textRect, txt);
    b->textRect.x = x + (w - b->textRect.w)/2;
    b->textRect.y = y + (h - b->textRect.h)/2;
    b->iconTexture = NULL;
    b->iconRect = (SDL_Rect){0,0,0,0};
    b->hovered = 0;
}

static void fillRounded(SDL_Renderer* r, SDL_Rect rect, int radius, SDL_Color col)
{
    if (radius <= 0) { SDL_SetRenderDrawColor(r, col.r,col.g,col.b,col.a); SDL_RenderFillRect(r, &rect); return; }
    if (radius*2 > rect.w) radius = rect.w/2;
    if (radius*2 > rect.h) radius = rect.h/2;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    SDL_Rect mid = {rect.x, rect.y + radius, rect.w, rect.h - 2*radius};
    SDL_RenderFillRect(r, &mid);
    SDL_Rect center = {rect.x + radius, rect.y, rect.w - 2*radius, rect.h};
    SDL_RenderFillRect(r, &center);
    for (int y = 0; y < radius; ++y)
    {
        int dx = (int)(sqrtf((float)(radius*radius - y*y)) + 0.5f);
        int xL = rect.x + radius - dx;
        int xR = rect.x + rect.w - radius + dx - 1;
        SDL_RenderDrawLine(r, xL, rect.y + y, xR, rect.y + y);
        SDL_RenderDrawLine(r, xL, rect.y + rect.h - 1 - y, xR, rect.y + rect.h - 1 - y);
    }
}

static void strokeRounded(SDL_Renderer* r, SDL_Rect rect, int radius, SDL_Color col)
{
    if (radius <= 0) { SDL_SetRenderDrawColor(r, col.r,col.g,col.b,col.a); SDL_RenderDrawRect(r, &rect); return; }
    if (radius*2 > rect.w) radius = rect.w/2;
    if (radius*2 > rect.h) radius = rect.h/2;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    SDL_RenderDrawLine(r, rect.x + radius, rect.y, rect.x + rect.w - radius - 1, rect.y);
    SDL_RenderDrawLine(r, rect.x + radius, rect.y + rect.h - 1, rect.x + rect.w - radius - 1, rect.y + rect.h - 1);
    SDL_RenderDrawLine(r, rect.x, rect.y + radius, rect.x, rect.y + rect.h - radius - 1);
    SDL_RenderDrawLine(r, rect.x + rect.w - 1, rect.y + radius, rect.x + rect.w - 1, rect.y + rect.h - radius - 1);
    for (int y = 0; y < radius; ++y)
    {
        int dx = (int)(sqrtf((float)(radius*radius - y*y)) + 0.5f);
        int x1 = rect.x + radius - dx;
        int x2 = rect.x + rect.w - radius + dx - 1;
        int yt = rect.y + y;
        int yb = rect.y + rect.h - 1 - y;
        SDL_RenderDrawPoint(r, x1, yt);
        SDL_RenderDrawPoint(r, x2, yt);
        SDL_RenderDrawPoint(r, x1, yb);
        SDL_RenderDrawPoint(r, x2, yb);
    }
}

static void drawModernBackground(SDL_Renderer* r)
{
    SDL_SetRenderDrawColor(r, 16, 18, 30, 255);
    SDL_RenderClear(r);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 220, 180, 90, 18);
    int step = 36;
    for (int i = -SCREEN_HEIGHT; i < SCREEN_WIDTH; i += step)
    {
        int x1 = i + bgStripeOffset;
        int y1 = 0;
        int x2 = x1 + SCREEN_HEIGHT;
        int y2 = SCREEN_HEIGHT;
        SDL_RenderDrawLine(r, x1, y1, x2, y2);
    }
    for (int k = 0; k < 10; ++k)
    {
        int pad = k * 10;
        SDL_Rect vr = { pad, pad, SCREEN_WIDTH - 2*pad, SCREEN_HEIGHT - 2*pad };
        SDL_SetRenderDrawColor(r, 0, 0, 0, 12);
        SDL_RenderDrawRect(r, &vr);
        SDL_RenderFillRect(r, &vr);
    }
}

void initEnigm(SDL_Renderer* renderer)
{
    if (!enigmBgTex)
    {
        SDL_Surface* bg = IMG_Load("assets/images/backgrounds/bg.png");
        if (!bg)
            printf("Failed to load enigm background: %s\n", IMG_GetError());
        else
        {
            enigmBgTex = SDL_CreateTextureFromSurface(renderer, bg);
            SDL_FreeSurface(bg);
        }
    }

    TTF_Font* titleFontLocal = TTF_OpenFont("assets/fonts/ARIAL.TTF", 64);
    SDL_Surface* titleSurf = titleFontLocal
        ? TTF_RenderText_Blended(titleFontLocal, "SPHINX: THE LOST NOSE", gold)
        : TTF_RenderText_Blended(font, "SPHINX: THE LOST NOSE", gold);
    titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurf);
    titleRect.w = titleSurf->w;
    titleRect.h = titleSurf->h;
    if (titleFontLocal) TTF_CloseFont(titleFontLocal);
    SDL_FreeSurface(titleSurf);

    SDL_Surface* subSurf = TTF_RenderText_Blended(font, "Enigm", gold);
    subtitleTexture = SDL_CreateTextureFromSurface(renderer, subSurf);
    subtitleRect.w = subSurf->w;
    subtitleRect.h = subSurf->h;
    SDL_FreeSurface(subSurf);

    panelRect.w = (int)(SCREEN_WIDTH * 0.86f);
    panelRect.h = (int)(SCREEN_HEIGHT * 0.76f);
    panelRect.x = (SCREEN_WIDTH - panelRect.w)/2;
    panelRect.y = (SCREEN_HEIGHT - panelRect.h)/2;

    subtitleRect.x = panelRect.x + 32;
    subtitleRect.y = panelRect.y + 24;
    headerRect.x = subtitleRect.x - 20;
    headerRect.y = subtitleRect.y - 12;
    headerRect.w = subtitleRect.w + 40;
    headerRect.h = subtitleRect.h + 24;

    int marginX = 32;
    int spacing = 24;
    int tileW = (panelRect.w - 2*marginX - spacing)/2;
    int tileH = 200;
    int startX = panelRect.x + marginX;
    int posY = headerRect.y + headerRect.h + 28;

    setButton(renderer, &quizBtn, "QUIZ", startX, posY, tileW, tileH);
    setButton(renderer, &puzzleBtn, "PUZZLE", startX + tileW + spacing, posY, tileW, tileH);

    // optional icons
    SDL_Surface* qIcon = IMG_Load("assets/images/ui/quiz.png");
    if (qIcon) {
        quizBtn.iconTexture = SDL_CreateTextureFromSurface(renderer, qIcon);
        int is = 64;
        quizBtn.iconRect = (SDL_Rect){ quizBtn.rect.x + 24, quizBtn.rect.y + 24, is, is };
        SDL_FreeSurface(qIcon);
        quizBtn.textRect.x = quizBtn.rect.x + 24 + is + 16;
        quizBtn.textRect.y = quizBtn.rect.y + 36;
    }
    SDL_Surface* pIcon = IMG_Load("assets/images/ui/puzzle.png");
    if (pIcon) {
        puzzleBtn.iconTexture = SDL_CreateTextureFromSurface(renderer, pIcon);
        int is = 64;
        puzzleBtn.iconRect = (SDL_Rect){ puzzleBtn.rect.x + 24, puzzleBtn.rect.y + 24, is, is };
        SDL_FreeSurface(pIcon);
        puzzleBtn.textRect.x = puzzleBtn.rect.x + 24 + is + 16;
        puzzleBtn.textRect.y = puzzleBtn.rect.y + 36;
    }

    int bw = 200, bh = 50;
    int bx = panelRect.x + 24;
    int by = panelRect.y + panelRect.h - bh - 24;
    setButton(renderer, &backBtn, "BACK", bx, by, bw, bh);
}

static int inside(SDL_Rect r, int x, int y)
{
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

void handleEnigmEvent(SDL_Event* e, MenuState* currentMenu)
{
    int mx,my; SDL_GetMouseState(&mx,&my);
    quizBtn.hovered = inside(quizBtn.rect,mx,my);
    puzzleBtn.hovered = inside(puzzleBtn.rect,mx,my);
    backBtn.hovered = inside(backBtn.rect,mx,my);

    if (e->type == SDL_MOUSEBUTTONDOWN)
    {
        if (quizBtn.hovered) {
            if(clickSound) Mix_PlayChannel(-1, clickSound, 0);
            *currentMenu = MENU_QUIZ;
            if (gameMusic) Mix_PlayMusic(gameMusic, -1);
        }
        else if (puzzleBtn.hovered) { if(clickSound) Mix_PlayChannel(-1, clickSound, 0); }
        else if (backBtn.hovered) { if(clickSound) Mix_PlayChannel(-1, clickSound, 0); goToScoreList(); *currentMenu = MENU_SCORE; }
    }

    if (e->type == SDL_KEYDOWN)
    {
        if (e->key.keysym.sym == SDLK_ESCAPE) { goToScoreList(); *currentMenu = MENU_SCORE; }
        if (e->key.keysym.sym == SDLK_RETURN || e->key.keysym.sym == SDLK_KP_ENTER) {
            *currentMenu = MENU_QUIZ;
            if (gameMusic) Mix_PlayMusic(gameMusic, -1);
        }
    }
}

void updateEnigm()
{
    Uint32 now = SDL_GetTicks();
    if (now - bgLastTick > 33)
    {
        bgStripeOffset = (bgStripeOffset + 2) % 36;
        bgLastTick = now;
    }
}

static void drawButton(SDL_Renderer* renderer, EnigmButton* b)
{
    SDL_Rect rect = b->rect;
    if (b->hovered) { rect.x -= 4; rect.y -= 2; rect.w += 8; rect.h += 4; }
    SDL_Color fill = b->hovered ? buttonHover : buttonColor;
    fillRounded(renderer, rect, 12, fill);
    SDL_Color border = { (Uint8)(b->hovered?255:180), (Uint8)(b->hovered?215:140), (Uint8)(b->hovered?0:60), 220 };
    strokeRounded(renderer, rect, 12, border);
    if (b->iconTexture) SDL_RenderCopy(renderer, b->iconTexture, NULL, &b->iconRect);
    SDL_RenderCopy(renderer, b->textTexture, NULL, &b->textRect);
}

void renderEnigm(SDL_Renderer* renderer)
{
    if (enigmBgTex)
        SDL_RenderCopy(renderer, enigmBgTex, NULL, NULL);
    else
        drawModernBackground(renderer);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect shadow = { panelRect.x + 10, panelRect.y + 10, panelRect.w, panelRect.h };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 140);
    SDL_RenderFillRect(renderer, &shadow);
    fillRounded(renderer, panelRect, 18, (SDL_Color){10,15,25,180});
    strokeRounded(renderer, panelRect, 18, gold);

    fillRounded(renderer, headerRect, 12, (SDL_Color){0,0,0,140});
    strokeRounded(renderer, headerRect, 12, gold);
    SDL_RenderCopy(renderer, subtitleTexture, NULL, &subtitleRect);

    drawButton(renderer, &quizBtn);
    drawButton(renderer, &puzzleBtn);
    drawButton(renderer, &backBtn);
}

void destroyEnigm()
{
    if (titleTexture) SDL_DestroyTexture(titleTexture);
    if (subtitleTexture) SDL_DestroyTexture(subtitleTexture);
    if (quizBtn.textTexture) SDL_DestroyTexture(quizBtn.textTexture);
    if (puzzleBtn.textTexture) SDL_DestroyTexture(puzzleBtn.textTexture);
    if (backBtn.textTexture) SDL_DestroyTexture(backBtn.textTexture);
    if (enigmBgTex) SDL_DestroyTexture(enigmBgTex);
}
