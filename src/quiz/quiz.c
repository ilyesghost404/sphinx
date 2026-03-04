#include "quiz.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <string.h>
#include <math.h>

extern TTF_Font* font;
extern Mix_Chunk* hoverSound;
extern Mix_Chunk* clickSound;
extern SDL_Texture* bgShared[SHARED_BG_FRAMES];
extern int gCurrentFrame;
extern Mix_Music* gameMusic;

typedef struct {
    const char* q;
    const char* opts[3];
    int correct;
} QA;

static QA questions[3] = {
    { "What does BMW stand for?",
      { "Bayerische Motoren Werke", "British Motor Works", "Bavarian Motor Wagon" }, 0 },
    { "Real Madrid is based in which city?",
      { "Madrid", "Barcelona", "Seville" }, 0 },
    { "Main protagonist of God of War (2018)?",
      { "Kratos", "Zeus", "Atreus" }, 0 }
};

static int currentQ = 0;
static int selected = -1;
static int revealed = 0;
static int quizFinished = 0;
static SDL_Renderer* quizRenderer = NULL;
static Uint32 highlightStart = 0;
static int highlightPending = 0;

static SDL_Rect panelRect;
static SDL_Texture* titleTexture = NULL;
static SDL_Rect titleRect;
static SDL_Rect headerRect;
static SDL_Texture* qTexture = NULL;
static SDL_Rect qRect;

typedef struct {
    SDL_Rect rect;
    SDL_Texture* textTexture;
    SDL_Rect textRect;
    int hovered;
} QuizButton;

static QuizButton optBtns[3];
static QuizButton nextBtn;
static QuizButton backBtn;

static SDL_Color gold = {220,180,90,220};
static SDL_Color buttonColor = {10,15,25,180};
static SDL_Color buttonHover = {10,15,25,240};
static int bgStripeOffset = 0;
static Uint32 bgLastTick = 0;
static SDL_Texture* quizBgTex = NULL;

static void setText(SDL_Renderer* r, SDL_Texture** tex, SDL_Rect* rect, const char* txt, SDL_Color col)
{
    if (*tex) { SDL_DestroyTexture(*tex); *tex = NULL; }
    SDL_Surface* surf = TTF_RenderText_Blended(font, txt, col);
    *tex = SDL_CreateTextureFromSurface(r, surf);
    rect->w = surf->w;
    rect->h = surf->h;
    SDL_FreeSurface(surf);
}

static void setButton(SDL_Renderer* r, QuizButton* b, const char* txt, int x, int y, int w, int h)
{
    b->rect = (SDL_Rect){x,y,w,h};
    setText(r, &b->textTexture, &b->textRect, txt, gold);
    b->textRect.x = x + (w - b->textRect.w)/2;
    b->textRect.y = y + (h - b->textRect.h)/2;
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

static int inside(SDL_Rect r, int x, int y)
{
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

static void loadQuestion(SDL_Renderer* renderer)
{
    setText(renderer, &qTexture, &qRect, questions[currentQ].q, gold);
}

static void refreshOptions(SDL_Renderer* renderer)
{
    for (int i=0;i<3;i++)
    {
        if (optBtns[i].textTexture) { SDL_DestroyTexture(optBtns[i].textTexture); optBtns[i].textTexture = NULL; }
        setText(renderer, &optBtns[i].textTexture, &optBtns[i].textRect, questions[currentQ].opts[i], gold);
        optBtns[i].textRect.x = optBtns[i].rect.x + (optBtns[i].rect.w - optBtns[i].textRect.w)/2;
        optBtns[i].textRect.y = optBtns[i].rect.y + (optBtns[i].rect.h - optBtns[i].textRect.h)/2;
    }
}

void initQuiz(SDL_Renderer* renderer)
{
    quizRenderer = renderer;
    currentQ = 0;
    selected = -1;
    revealed = 0;
    quizFinished = 0;

    if (!quizBgTex)
    {
        SDL_Surface* bg = IMG_Load("assets/images/backgrounds/bg.png");
        if (!bg)
            printf("Failed to load quiz background: %s\n", IMG_GetError());
        else
        {
            quizBgTex = SDL_CreateTextureFromSurface(renderer, bg);
            SDL_FreeSurface(bg);
        }
    }

    panelRect.w = (int)(SCREEN_WIDTH * 0.86f);
    panelRect.h = (int)(SCREEN_HEIGHT * 0.76f);
    panelRect.x = (SCREEN_WIDTH - panelRect.w)/2;
    panelRect.y = (SCREEN_HEIGHT - panelRect.h)/2;

    SDL_Surface* t = TTF_RenderText_Blended(font, "Quiz", gold);
    titleTexture = SDL_CreateTextureFromSurface(renderer, t);
    titleRect.w = t->w;
    titleRect.h = t->h;
    SDL_FreeSurface(t);

    titleRect.x = panelRect.x + 32;
    titleRect.y = panelRect.y + 24;
    headerRect.x = titleRect.x - 20;
    headerRect.y = titleRect.y - 12;
    headerRect.w = titleRect.w + 40;
    headerRect.h = titleRect.h + 24;

    loadQuestion(renderer);
    qRect.x = panelRect.x + (panelRect.w - qRect.w)/2;
    qRect.y = headerRect.y + headerRect.h + 28;

    int marginX = 64;
    int btnWidth = panelRect.w - 2*marginX;
    int btnHeight = 68;
    int startX = panelRect.x + (panelRect.w - btnWidth)/2;
    int startY = qRect.y + qRect.h + 24;
    for (int i=0;i<3;i++)
    {
        setButton(renderer, &optBtns[i], questions[currentQ].opts[i], startX, startY + i*(btnHeight + 12), btnWidth, btnHeight);
    }
    refreshOptions(renderer);

    int backW = 260;
    int backH = 56;
    int backX = panelRect.x + (panelRect.w - backW)/2;
    int backY = panelRect.y + panelRect.h - backH - 24;
    setButton(renderer, &backBtn, "BACK TO ENIGM", backX, backY, backW, backH);
}

void handleQuizEvent(SDL_Event* e, MenuState* currentMenu)
{
    if (gameMusic && !Mix_PlayingMusic())
        Mix_PlayMusic(gameMusic, -1);

    int mx,my; SDL_GetMouseState(&mx,&my);
    for (int i=0;i<3;i++) optBtns[i].hovered = inside(optBtns[i].rect,mx,my);
    backBtn.hovered = quizFinished && inside(backBtn.rect, mx, my);

    if (e->type == SDL_MOUSEBUTTONDOWN)
    {
        for (int i=0;i<3;i++)
        {
            if (optBtns[i].hovered) {
                selected = i; revealed = 1;
                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                if (currentQ < 2) {
                    highlightPending = 1;
                    highlightStart = SDL_GetTicks();
                } else {
                    quizFinished = 1;
                }
            }
        }

        if (quizFinished && backBtn.hovered)
        {
            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
            *currentMenu = MENU_ENIGM;
            resetQuiz();
            return;
        }
    }

    if (e->type == SDL_KEYDOWN)
    {
        if (quizFinished)
        {
            *currentMenu = MENU_ENIGM;
            resetQuiz();
            return;
        }

        if (e->key.keysym.sym == SDLK_1) {
            selected = 0; revealed = 1;
            if (currentQ < 2) {
                highlightPending = 1;
                highlightStart = SDL_GetTicks();
            } else {
                quizFinished = 1;
            }
        }
        if (e->key.keysym.sym == SDLK_2) {
            selected = 1; revealed = 1;
            if (currentQ < 2) {
                highlightPending = 1;
                highlightStart = SDL_GetTicks();
            } else {
                quizFinished = 1;
            }
        }
        if (e->key.keysym.sym == SDLK_3) {
            selected = 2; revealed = 1;
            if (currentQ < 2) {
                highlightPending = 1;
                highlightStart = SDL_GetTicks();
            } else {
                quizFinished = 1;
            }
        }
    }
}

void updateQuiz()
{
    if (!quizFinished && highlightPending && revealed)
    {
        if (SDL_GetTicks() - highlightStart >= 300)
        {
            currentQ++;
            selected = -1;
            revealed = 0;
            highlightPending = 0;
            if (qTexture) { SDL_DestroyTexture(qTexture); qTexture = NULL; }
            loadQuestion(quizRenderer);
            qRect.x = panelRect.x + (panelRect.w - qRect.w)/2;
            qRect.y = titleRect.y + titleRect.h + 28;
            refreshOptions(quizRenderer);
        }
    }
    Uint32 now = SDL_GetTicks();
    if (now - bgLastTick > 33)
    {
        bgStripeOffset = (bgStripeOffset + 2) % 36;
        bgLastTick = now;
    }
}

static void drawButton(SDL_Renderer* renderer, QuizButton* b, int idx, int selectedIdx, int correctIdx, int revealedLocal)
{
    SDL_Rect rect = b->rect;
    if (b->hovered) { rect.x -= 5; rect.y -= 3; rect.w += 10; rect.h += 6; }
    SDL_Color fill = b->hovered ? buttonHover : buttonColor;
    SDL_Color border = { (Uint8)(b->hovered?255:180), (Uint8)(b->hovered?215:140), (Uint8)(b->hovered?0:60), 220 };
    if (revealedLocal) {
        if (idx == correctIdx) {
            fill = (SDL_Color){30,60,30,200};
            border = (SDL_Color){0,220,0,220};
        } else if (idx == selectedIdx) {
            fill = (SDL_Color){60,30,30,200};
            border = (SDL_Color){220,0,0,220};
        }
    }
    fillRounded(renderer, rect, 12, fill);
    strokeRounded(renderer, rect, 12, border);
    SDL_RenderCopy(renderer, b->textTexture, NULL, &b->textRect);
}

void renderQuiz(SDL_Renderer* renderer)
{
    if (quizBgTex)
        SDL_RenderCopy(renderer, quizBgTex, NULL, NULL);
    else
        drawModernBackground(renderer);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0,0,0,140);
    SDL_Rect shadow = {panelRect.x+10, panelRect.y+10, panelRect.w, panelRect.h};
    SDL_RenderFillRect(renderer, &shadow);

    fillRounded(renderer, panelRect, 18, (SDL_Color){10,15,25,180});
    strokeRounded(renderer, panelRect, 18, gold);

    fillRounded(renderer, headerRect, 12, (SDL_Color){0,0,0,140});
    strokeRounded(renderer, headerRect, 12, gold);
    SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
    if (qTexture) SDL_RenderCopy(renderer, qTexture, NULL, &qRect);

    for (int i=0;i<3;i++)
    {
        drawButton(renderer, &optBtns[i], i, selected, questions[currentQ].correct, revealed);
    }

    if (quizFinished)
        drawButton(renderer, &backBtn, -1, -1, -1, 0);
}

void destroyQuiz()
{
    if (titleTexture) SDL_DestroyTexture(titleTexture);
    if (qTexture) SDL_DestroyTexture(qTexture);
    for (int i=0;i<4;i++) if (optBtns[i].textTexture) SDL_DestroyTexture(optBtns[i].textTexture);
    if (nextBtn.textTexture) SDL_DestroyTexture(nextBtn.textTexture);
    if (backBtn.textTexture) SDL_DestroyTexture(backBtn.textTexture);
    if (quizBgTex) SDL_DestroyTexture(quizBgTex);
}

void resetQuiz()
{
    currentQ = 0;
    selected = -1;
    revealed = 0;
    quizFinished = 0;
    highlightPending = 0;
    if (qTexture) { SDL_DestroyTexture(qTexture); qTexture = NULL; }
    loadQuestion(quizRenderer);
    qRect.x = panelRect.x + (panelRect.w - qRect.w)/2;
    qRect.y = titleRect.y + titleRect.h + 28;
    refreshOptions(quizRenderer);
    for (int i=0;i<3;i++) optBtns[i].hovered = 0;
    backBtn.hovered = 0;
    nextBtn.hovered = 0;
}
