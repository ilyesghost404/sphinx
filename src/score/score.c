#include "score.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>

extern TTF_Font* font;
extern Mix_Music* menuMusic;
extern Mix_Music* gameMusic;

static SDL_Rect panelRect;
static SDL_Rect headerRect;

static PlayerScore scores[MAX_SCORES];
static int scoreCount = 0;

static SDL_Rect backButtonRect;
static SDL_Texture* backTextTexture = NULL;
static SDL_Rect backTextRect;
static SDL_Rect enigmButtonRect;
static SDL_Texture* enigmTextTexture = NULL;
static SDL_Rect enigmTextRect;

static SDL_Color panelColor  = {18, 14, 24, 215};
static SDL_Color borderColor = {240, 210, 120, 235};
static SDL_Color backColor   = {12, 10, 20, 210};
static SDL_Color textColor   = {245, 215, 130, 255};

static Mix_Music* scoreMusic = NULL;
static int scoreMusicActive = 0;
static SDL_Texture* scoreBgTex = NULL;

typedef enum { STAGE_INPUT, STAGE_LIST } ScoreStage;
static ScoreStage stage = STAGE_INPUT;
static SDL_Rect inputRect;
static SDL_Texture* inputLabelTex = NULL;
static SDL_Rect inputLabelRect;
static SDL_Rect confirmButtonRect;
static SDL_Texture* confirmTextTexture = NULL;
static SDL_Rect confirmTextRect;
static char nameBuffer[32] = {0};
static int nameLen = 0;
static int backHovered = 0;
static int confirmHovered = 0;
static int enigmHovered = 0;

static int inside(SDL_Rect r, int x, int y){ return x>=r.x && x<=r.x+r.w && y>=r.y && y<=r.y+r.h; }

static void restoreMenuMusic()
{
    if (!scoreMusicActive) return;
    if (menuMusic)
        Mix_PlayMusic(menuMusic, -1);
    else
        Mix_HaltMusic();
    scoreMusicActive = 0;
}

static void switchToGameMusic()
{
    if (!scoreMusicActive) return;
    if (gameMusic)
        Mix_PlayMusic(gameMusic, -1);
    else
        Mix_HaltMusic();
    scoreMusicActive = 0;
}

static void setInputButtonsRow() {
    int spacing = 24;
    int total = backButtonRect.w + spacing + confirmButtonRect.w + spacing + enigmButtonRect.w;
    int startX = panelRect.x + (panelRect.w - total) / 2;
    backButtonRect.x = startX;
    backButtonRect.y = inputRect.y + inputRect.h + 28;
    confirmButtonRect.x = startX + backButtonRect.w + spacing;
    confirmButtonRect.y = backButtonRect.y;
    enigmButtonRect.x = confirmButtonRect.x + confirmButtonRect.w + spacing;
    enigmButtonRect.y = backButtonRect.y;
    backTextRect.x = backButtonRect.x + (backButtonRect.w - backTextRect.w)/2;
    backTextRect.y = backButtonRect.y + (backButtonRect.h - backTextRect.h)/2;
    confirmTextRect.x = confirmButtonRect.x + (confirmButtonRect.w - confirmTextRect.w)/2;
    confirmTextRect.y = confirmButtonRect.y + (confirmButtonRect.h - confirmTextRect.h)/2;
    enigmTextRect.x = enigmButtonRect.x + (enigmButtonRect.w - enigmTextRect.w)/2;
    enigmTextRect.y = enigmButtonRect.y + (enigmButtonRect.h - enigmTextRect.h)/2;
}

static void setListButtonsRow() {
    int spacing = 24;
    int total = backButtonRect.w + spacing + enigmButtonRect.w;
    int startX = panelRect.x + (panelRect.w - total) / 2;
    backButtonRect.x = startX;
    backButtonRect.y = panelRect.y + panelRect.h - 24 - backButtonRect.h;
    enigmButtonRect.x = startX + backButtonRect.w + spacing;
    enigmButtonRect.y = backButtonRect.y;
    backTextRect.x = backButtonRect.x + (backButtonRect.w - backTextRect.w)/2;
    backTextRect.y = backButtonRect.y + (backButtonRect.h - backTextRect.h)/2;
    enigmTextRect.x = enigmButtonRect.x + (enigmButtonRect.w - enigmTextRect.w)/2;
    enigmTextRect.y = enigmButtonRect.y + (enigmButtonRect.h - enigmTextRect.h)/2;
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

// ===== FILE OPERATIONS =====
int loadScores(PlayerScore* out, int maxScores) {
    FILE* file = fopen(SCORE_FILE, "rb");
    if (!file) return 0;
    int n = 0;
    PlayerScore temp;
    while (n < maxScores && fread(&temp, sizeof(PlayerScore), 1, file) == 1) {
        out[n++] = temp;
    }
    fclose(file);
    return n;
}

void saveScore(PlayerScore newScore) {
    FILE* file = fopen(SCORE_FILE, "ab");
    if (!file) {
        printf("Cannot open score file!\n");
        return;
    }
    fwrite(&newScore, sizeof(PlayerScore), 1, file);
    fclose(file);
}

void sortScores(PlayerScore* scores, int count) {
    for (int i = 0; i < count - 1; i++)
        for (int j = i + 1; j < count; j++)
            if (scores[j].score > scores[i].score) {
                PlayerScore tmp = scores[i];
                scores[i] = scores[j];
                scores[j] = tmp;
            }
}

static void populateFakeScores() {
    const char* names[] = {"Alex","Sam","Nora"};
    int values[] = {980,920,880};
    int n = 3;
    if (n > MAX_SCORES) n = MAX_SCORES;
    scoreCount = n;
    for (int i=0;i<n;i++) {
        strncpy(scores[i].name, names[i], sizeof(scores[i].name)-1);
        scores[i].name[sizeof(scores[i].name)-1] = 0;
        scores[i].score = values[i];
    }
}

// ===== SDL INIT =====
void initScore(SDL_Renderer* renderer) {
    if (!font) { printf("Font not loaded!\n"); return; }

    if (!scoreMusic)
    {
        scoreMusic = Mix_LoadMUS("assets/audio/music/score.wav");
        if (!scoreMusic)
            printf("Failed to load score music: %s\n", Mix_GetError());
    }
    scoreMusicActive = 0;

    if (!scoreBgTex)
    {
        SDL_Surface* bg = IMG_Load("assets/images/backgrounds/score.jpg");
        if (!bg)
            printf("Failed to load score background: %s\n", IMG_GetError());
        else
        {
            scoreBgTex = SDL_CreateTextureFromSurface(renderer, bg);
            SDL_FreeSurface(bg);
        }
    }

    panelRect.w = 700;
    panelRect.h = 520;
    panelRect.x = (SCREEN_WIDTH - panelRect.w) / 2;
    panelRect.y = 60;

    headerRect.x = panelRect.x + 8;
    headerRect.y = panelRect.y + 8;
    headerRect.w = panelRect.w - 16;
    headerRect.h = 72;

    int spacing = 24;
    int btnW = 200;
    int btnH = 56;
    int total = btnW * 2 + spacing;
    int startX = panelRect.x + (panelRect.w - total) / 2;
    // BACK left, ENIGM right
    backButtonRect.w = btnW;
    backButtonRect.h = btnH;
    backButtonRect.x = startX;
    backButtonRect.y = panelRect.y + panelRect.h - 24 - btnH;
    enigmButtonRect.w = btnW;
    enigmButtonRect.h = btnH;
    enigmButtonRect.x = startX + btnW + spacing;
    enigmButtonRect.y = backButtonRect.y;

    SDL_Surface* backSurface = TTF_RenderText_Blended(font, "BACK TO MENU", borderColor);
    backTextTexture = SDL_CreateTextureFromSurface(renderer, backSurface);
    backTextRect.w = backSurface->w;
    backTextRect.h = backSurface->h;
    backTextRect.x = backButtonRect.x + (backButtonRect.w - backSurface->w) / 2;
    backTextRect.y = backButtonRect.y + (backButtonRect.h - backSurface->h) / 2;
    SDL_FreeSurface(backSurface);

    SDL_Surface* enigms = TTF_RenderText_Blended(font, "ENIGM [E]", borderColor);
    enigmTextTexture = SDL_CreateTextureFromSurface(renderer, enigms);
    enigmTextRect.w = enigms->w;
    enigmTextRect.h = enigms->h;
    enigmTextRect.x = enigmButtonRect.x + (enigmButtonRect.w - enigms->w)/2;
    enigmTextRect.y = enigmButtonRect.y + (enigmButtonRect.h - enigms->h)/2;
    SDL_FreeSurface(enigms);

    scoreCount = loadScores(scores, MAX_SCORES);
    if (scoreCount == 0) {
        populateFakeScores();
    } else {
        sortScores(scores, scoreCount);
    }

    stage = STAGE_INPUT;
    inputRect.w = 420;
    inputRect.h = 64;
    inputRect.x = panelRect.x + (panelRect.w - inputRect.w)/2;
    inputRect.y = headerRect.y + headerRect.h + 32;

    SDL_Surface* lab = TTF_RenderText_Blended(font, "ENTER YOUR NAME", borderColor);
    inputLabelTex = SDL_CreateTextureFromSurface(renderer, lab);
    inputLabelRect.w = lab->w;
    inputLabelRect.h = lab->h;
    inputLabelRect.x = headerRect.x + (headerRect.w - lab->w)/2;
    inputLabelRect.y = headerRect.y + (headerRect.h - lab->h)/2;
    SDL_FreeSurface(lab);

    confirmButtonRect.w = 200;
    confirmButtonRect.h = 56;
    confirmButtonRect.x = panelRect.x + (panelRect.w - confirmButtonRect.w) / 2;
    confirmButtonRect.y = inputRect.y + inputRect.h + 28;

    SDL_Surface* conf = TTF_RenderText_Blended(font, "CONFIRM", borderColor);
    confirmTextTexture = SDL_CreateTextureFromSurface(renderer, conf);
    confirmTextRect.w = conf->w;
    confirmTextRect.h = conf->h;
    confirmTextRect.x = confirmButtonRect.x + (confirmButtonRect.w - conf->w)/2;
    confirmTextRect.y = confirmButtonRect.y + (confirmButtonRect.h - conf->h)/2;
    SDL_FreeSurface(conf);

    nameBuffer[0] = 0;
    nameLen = 0;
    SDL_StartTextInput();
    setInputButtonsRow();
}

void handleScoreEvent(SDL_Event* e, MenuState* currentMenu) {
    if (!currentMenu) return;

    if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_ESCAPE) {
        stage = STAGE_INPUT;
        nameLen = 0; nameBuffer[0] = 0;
        SDL_StopTextInput();
        *currentMenu = MENU_MAIN;
        restoreMenuMusic();
    }
    if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_e) {
        SDL_StopTextInput();
        *currentMenu = MENU_ENIGM;
        switchToGameMusic();
    }

    {
        int mx,my; SDL_GetMouseState(&mx,&my);
        if (stage == STAGE_INPUT) setInputButtonsRow();
        if (stage == STAGE_LIST) setListButtonsRow();
        backHovered = inside(backButtonRect,mx,my);
        confirmHovered = inside(confirmButtonRect,mx,my) && stage == STAGE_INPUT;
        enigmHovered = inside(enigmButtonRect,mx,my);
    }

    if (stage == STAGE_INPUT) {
        if (e->type == SDL_TEXTINPUT) {
            const char* t = e->text.text;
            while (*t && nameLen < (int)sizeof(nameBuffer)-1) {
                nameBuffer[nameLen++] = *t++;
            }
            nameBuffer[nameLen] = 0;
        }
        if (e->type == SDL_KEYDOWN) {
            if (e->key.keysym.sym == SDLK_BACKSPACE && nameLen > 0) {
                nameBuffer[--nameLen] = 0;
            }
            if ((e->key.keysym.sym == SDLK_RETURN || e->key.keysym.sym == SDLK_KP_ENTER) && nameLen > 0) {
                stage = STAGE_LIST;
                SDL_StopTextInput();
                setListButtonsRow();
            }
        }
        if (e->type == SDL_MOUSEBUTTONDOWN) {
            int x = e->button.x, y = e->button.y;
            if (x >= confirmButtonRect.x && x <= confirmButtonRect.x + confirmButtonRect.w &&
                y >= confirmButtonRect.y && y <= confirmButtonRect.y + confirmButtonRect.h) {
                if (nameLen > 0) {
                    stage = STAGE_LIST;
                    SDL_StopTextInput();
                    setListButtonsRow();
                }
            }
            if (x >= backButtonRect.x && x <= backButtonRect.x + backButtonRect.w &&
                y >= backButtonRect.y && y <= backButtonRect.y + backButtonRect.h) {
                stage = STAGE_INPUT;
                nameLen = 0; nameBuffer[0] = 0;
                SDL_StopTextInput();
                *currentMenu = MENU_MAIN;
                restoreMenuMusic();
            }
            if (x >= enigmButtonRect.x && x <= enigmButtonRect.x + enigmButtonRect.w &&
                y >= enigmButtonRect.y && y <= enigmButtonRect.y + enigmButtonRect.h) {
                SDL_StopTextInput();
                *currentMenu = MENU_ENIGM;
                switchToGameMusic();
            }
        }
    } else {
        if (e->type == SDL_MOUSEBUTTONDOWN) {
            int x = e->button.x, y = e->button.y;
            if (x >= backButtonRect.x && x <= backButtonRect.x + backButtonRect.w &&
                y >= backButtonRect.y && y <= backButtonRect.y + backButtonRect.h) {
                stage = STAGE_INPUT;
                nameLen = 0; nameBuffer[0] = 0;
                SDL_StopTextInput();
                *currentMenu = MENU_MAIN;
                restoreMenuMusic();
            }
            if (x >= enigmButtonRect.x && x <= enigmButtonRect.x + enigmButtonRect.w &&
                y >= enigmButtonRect.y && y <= enigmButtonRect.y + enigmButtonRect.h) {
                *currentMenu = MENU_ENIGM;
                switchToGameMusic();
            }
        }
    }
}

void updateScore() {}

void renderScore(SDL_Renderer* renderer) {
    if (!renderer) return;

    if (!scoreMusicActive && scoreMusic)
    {
        Mix_PlayMusic(scoreMusic, -1);
        scoreMusicActive = 1;
    }

    if (stage == STAGE_INPUT && !SDL_IsTextInputActive()) SDL_StartTextInput();

    if (scoreBgTex)
        SDL_RenderCopy(renderer, scoreBgTex, NULL, NULL);
    else if (bgShared[gCurrentFrame])
        SDL_RenderCopy(renderer, bgShared[gCurrentFrame], NULL, NULL);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect shadow = {panelRect.x+10,panelRect.y+10,panelRect.w,panelRect.h};
    SDL_SetRenderDrawColor(renderer, 0,0,0,140);
    SDL_RenderFillRect(renderer, &shadow);
    fillRounded(renderer, panelRect, 16, panelColor);
    strokeRounded(renderer, panelRect, 16, borderColor);

    if (stage == STAGE_INPUT) {
        fillRounded(renderer, headerRect, 16, (SDL_Color){0,0,0,140});
        strokeRounded(renderer, headerRect, 12, borderColor);
        if (inputLabelTex) SDL_RenderCopy(renderer, inputLabelTex, NULL, &inputLabelRect);
        fillRounded(renderer, inputRect, 10, (SDL_Color){10,15,25,200});
        strokeRounded(renderer, inputRect, 10, borderColor);
        const char* display = nameLen > 0 ? nameBuffer : "enter name...";
        SDL_Color col = nameLen > 0 ? textColor : (SDL_Color){180,180,180,255};
        SDL_Surface* s = TTF_RenderText_Blended(font, display, col);
        if (s) {
            SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
            SDL_Rect r = { inputRect.x + 12, inputRect.y + (inputRect.h - s->h)/2, s->w, s->h };
            SDL_RenderCopy(renderer, t, NULL, &r);
            SDL_FreeSurface(s);
            SDL_DestroyTexture(t);
            Uint32 ticks = SDL_GetTicks();
            if ((ticks/500)%2 == 0 && nameLen < (int)sizeof(nameBuffer)-1) {
                int cx = r.x + r.w + 3;
                SDL_SetRenderDrawColor(renderer, 220,180,90,255);
                SDL_RenderDrawLine(renderer, cx, inputRect.y+10, cx, inputRect.y+inputRect.h-10);
            }
        }
        fillRounded(renderer, confirmButtonRect, 12, backColor);
        strokeRounded(renderer, confirmButtonRect, 12, (SDL_Color){ (Uint8)(confirmHovered?255:180), (Uint8)(confirmHovered?215:140), (Uint8)(confirmHovered?0:60), 220 });
        if (confirmTextTexture)
            SDL_RenderCopy(renderer, confirmTextTexture, NULL, &confirmTextRect);
        fillRounded(renderer, backButtonRect, 12, backColor);
        strokeRounded(renderer, backButtonRect, 12, (SDL_Color){ (Uint8)(backHovered?255:180), (Uint8)(backHovered?215:140), (Uint8)(backHovered?0:60), 220 });
        if (backTextTexture)
            SDL_RenderCopy(renderer, backTextTexture, NULL, &backTextRect);
        fillRounded(renderer, enigmButtonRect, 12, backColor);
        strokeRounded(renderer, enigmButtonRect, 12, (SDL_Color){ (Uint8)(enigmHovered?255:180), (Uint8)(enigmHovered?215:140), (Uint8)(enigmHovered?0:60), 220 });
        if (enigmTextTexture)
            SDL_RenderCopy(renderer, enigmTextTexture, NULL, &enigmTextRect);
    } else {
        SDL_Rect header = {panelRect.x+8, panelRect.y+8, panelRect.w-16, 72};
        fillRounded(renderer, header, 16, (SDL_Color){0,0,0,140});
        strokeRounded(renderer, header, 12, borderColor);
        SDL_Surface* h = TTF_RenderText_Blended(font, "BEST 3 SCORES", borderColor);
        if (h) {
            SDL_Texture* ht = SDL_CreateTextureFromSurface(renderer, h);
            SDL_Rect hr = { header.x + (header.w - h->w)/2, header.y + (72 - h->h)/2, h->w, h->h };
            SDL_RenderCopy(renderer, ht, NULL, &hr);
            SDL_FreeSurface(h); SDL_DestroyTexture(ht);
        }
        int startY = header.y + header.h + 20;
        int padding = 14;
        int limit = scoreCount < 3 ? scoreCount : 3;
        for (int i = 0; i < limit; i++) {
            SDL_Color stripeCol = (SDL_Color){16,22,32,170};
            SDL_Color nameCol = textColor;
            SDL_Color scoreCol = borderColor;
            if (i == 0) { stripeCol = (SDL_Color){38,32,18,210}; scoreCol = (SDL_Color){220,180,90,255}; }
            else if (i == 1) { stripeCol = (SDL_Color){32,34,40,210}; scoreCol = (SDL_Color){200,200,220,255}; }
            else if (i == 2) { stripeCol = (SDL_Color){40,30,28,210}; scoreCol = (SDL_Color){220,140,80,255}; }
            SDL_Rect stripe = { panelRect.x + 16, startY - 6, panelRect.w - 32, 44 };
            fillRounded(renderer, stripe, 12, stripeCol);
            char rankBuf[8]; snprintf(rankBuf, sizeof(rankBuf), "%d", i+1);
            SDL_Surface* rankS = TTF_RenderText_Blended(font, rankBuf, borderColor);
            if (rankS) {
                SDL_Texture* rankT = SDL_CreateTextureFromSurface(renderer, rankS);
                SDL_Rect rr = { stripe.x + 16, startY + (44 - rankS->h)/2, rankS->w, rankS->h };
                SDL_RenderCopy(renderer, rankT, NULL, &rr);
                SDL_FreeSurface(rankS); SDL_DestroyTexture(rankT);
            }
            SDL_Surface* nameS = TTF_RenderText_Blended(font, scores[i].name, nameCol);
            if (nameS) {
                SDL_Texture* nameT = SDL_CreateTextureFromSurface(renderer, nameS);
                SDL_Rect nr = { stripe.x + 64, startY + (44 - nameS->h)/2, nameS->w, nameS->h };
                SDL_RenderCopy(renderer, nameT, NULL, &nr);
                SDL_FreeSurface(nameS); SDL_DestroyTexture(nameT);
            }
            char scoreBuf[16]; snprintf(scoreBuf, sizeof(scoreBuf), "%d", scores[i].score);
            SDL_Surface* scS = TTF_RenderText_Blended(font, scoreBuf, scoreCol);
            if (scS) {
                SDL_Texture* scT = SDL_CreateTextureFromSurface(renderer, scS);
                SDL_Rect sr = { stripe.x + stripe.w - 20 - scS->w, startY + (44 - scS->h)/2, scS->w, scS->h };
                SDL_RenderCopy(renderer, scT, NULL, &sr);
                SDL_FreeSurface(scS); SDL_DestroyTexture(scT);
            }
            startY += 44 + padding;
        }
        fillRounded(renderer, backButtonRect, 12, backColor);
        strokeRounded(renderer, backButtonRect, 12, (SDL_Color){ (Uint8)(backHovered?255:180), (Uint8)(backHovered?215:140), (Uint8)(backHovered?0:60), 220 });
        if (backTextTexture)
            SDL_RenderCopy(renderer, backTextTexture, NULL, &backTextRect);
        fillRounded(renderer, enigmButtonRect, 12, backColor);
        strokeRounded(renderer, enigmButtonRect, 12, (SDL_Color){ (Uint8)(enigmHovered?255:180), (Uint8)(enigmHovered?215:140), (Uint8)(enigmHovered?0:60), 220 });
        if (enigmTextTexture)
            SDL_RenderCopy(renderer, enigmTextTexture, NULL, &enigmTextRect);
    }
}

void destroyScore() {
    if (backTextTexture) SDL_DestroyTexture(backTextTexture);
    if (inputLabelTex) SDL_DestroyTexture(inputLabelTex);
    if (confirmTextTexture) SDL_DestroyTexture(confirmTextTexture);
    if (enigmTextTexture) SDL_DestroyTexture(enigmTextTexture);
    if (scoreMusic) { Mix_FreeMusic(scoreMusic); scoreMusic = NULL; }
    if (scoreBgTex) { SDL_DestroyTexture(scoreBgTex); scoreBgTex = NULL; }
    SDL_StopTextInput();
}

void goToScoreList() {
    stage = STAGE_LIST;
    SDL_StopTextInput();
}
