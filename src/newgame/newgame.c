#include "newgame.h"
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

static SDL_Texture* titleTexture = NULL;
static SDL_Rect titleRect;
static SDL_Texture* subtitleTexture = NULL;
static SDL_Rect subtitleRect;

static SDL_Rect panelRect;


static NGButton modeMonoBtn;
static NGButton modeMultiBtn;

static SDL_Color gold = {220,180,90,255};
static SDL_Color buttonColor = {10,15,25,180};
static SDL_Color buttonHover = {10,15,25,240};

static void setText(SDL_Renderer* r, SDL_Texture** tex, SDL_Rect* rect, const char* txt)
{
    if (*tex) { SDL_DestroyTexture(*tex); *tex = NULL; }
    SDL_Surface* surf = TTF_RenderText_Blended(font, txt, gold);
    *tex = SDL_CreateTextureFromSurface(r, surf);
    rect->w = surf->w;
    rect->h = surf->h;
    SDL_FreeSurface(surf);
}

static void setButton(SDL_Renderer* r, NGButton* b, const char* txt, int x, int y, int w, int h)
{
    b->rect = (SDL_Rect){x,y,w,h};
    setText(r, &b->textTexture, &b->textRect, txt);
    b->textRect.x = x + (w - b->textRect.w)/2;
    b->textRect.y = y + (h - b->textRect.h)/2;
    b->iconTexture = NULL;
    b->iconRect = (SDL_Rect){0,0,0,0};
    b->hovered = 0;
}

static NGButton backBtn;
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
void initNewGame(SDL_Renderer* renderer)
{
    TTF_Font* titleFontLocal = TTF_OpenFont("assets/fonts/ARIAL.TTF", 64);
    SDL_Surface* titleSurf = titleFontLocal
        ? TTF_RenderText_Blended(titleFontLocal, "SPHINX: THE LOST NOSE", gold)
        : TTF_RenderText_Blended(font, "SPHINX: THE LOST NOSE", gold);
    titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurf);
    titleRect.w = titleSurf->w;
    titleRect.h = titleSurf->h;
    if (titleFontLocal) TTF_CloseFont(titleFontLocal);
    SDL_FreeSurface(titleSurf);

    SDL_Surface* subSurf = TTF_RenderText_Blended(font, "New Game", gold);
    subtitleTexture = SDL_CreateTextureFromSurface(renderer, subSurf);
    subtitleRect.w = subSurf->w;
    subtitleRect.h = subSurf->h;
    SDL_FreeSurface(subSurf);

    panelRect.w = (int)(SCREEN_WIDTH * 0.82f);
    panelRect.h = (int)(SCREEN_HEIGHT * 0.78f);
    panelRect.x = (SCREEN_WIDTH - panelRect.w)/2;
    panelRect.y = (SCREEN_HEIGHT - panelRect.h)/2;

    int marginY = 64;
    titleRect.x = panelRect.x + (panelRect.w - titleRect.w)/2;
    titleRect.y = panelRect.y + marginY - 12;
    subtitleRect.x = panelRect.x + (panelRect.w - subtitleRect.w)/2;
    subtitleRect.y = titleRect.y + titleRect.h + 8;

    int marginX = 64;
    int btnWidth = panelRect.w - 2*marginX;
    if (btnWidth < 320) btnWidth = 320;
    int btnHeight = 84;
    int iconSizeBase = 36;
    int posY = subtitleRect.y + subtitleRect.h + 48;
    int startX = panelRect.x + (panelRect.w - btnWidth)/2;

    setButton(renderer, &modeMonoBtn, "Single", startX, posY, btnWidth, btnHeight);
    {
        SDL_Surface* iconSurf = IMG_Load("assets/images/ui/single.png");
        if (iconSurf) {
            modeMonoBtn.iconTexture = SDL_CreateTextureFromSurface(renderer, iconSurf);
            SDL_FreeSurface(iconSurf);
        }
        int iconSize = iconSizeBase;
        modeMonoBtn.iconRect.w = iconSize;
        modeMonoBtn.iconRect.h = iconSize;
        modeMonoBtn.iconRect.x = modeMonoBtn.rect.x + 24;
        modeMonoBtn.iconRect.y = modeMonoBtn.rect.y + (modeMonoBtn.rect.h - iconSize) / 2;
        modeMonoBtn.textRect.x = modeMonoBtn.iconRect.x + iconSize + 18;
        modeMonoBtn.textRect.y = modeMonoBtn.rect.y + (modeMonoBtn.rect.h - modeMonoBtn.textRect.h) / 2;
    }

    setButton(renderer, &modeMultiBtn, "Multi", startX, posY + btnHeight + 18, btnWidth, btnHeight);
    {
        SDL_Surface* iconSurf = IMG_Load("assets/images/ui/multi.png");
        if (iconSurf) {
            modeMultiBtn.iconTexture = SDL_CreateTextureFromSurface(renderer, iconSurf);
            SDL_FreeSurface(iconSurf);
        }
        int iconSize = iconSizeBase;
        modeMultiBtn.iconRect.w = iconSize;
        modeMultiBtn.iconRect.h = iconSize;
        modeMultiBtn.iconRect.x = modeMultiBtn.rect.x + 24;
        modeMultiBtn.iconRect.y = modeMultiBtn.rect.y + (modeMultiBtn.rect.h - iconSize) / 2;
        modeMultiBtn.textRect.x = modeMultiBtn.iconRect.x + iconSize + 18;
        modeMultiBtn.textRect.y = modeMultiBtn.rect.y + (modeMultiBtn.rect.h - modeMultiBtn.textRect.h) / 2;
    }

    int bw = 180, bh = 56;
    int bx = panelRect.x + (panelRect.w - bw)/2;
    int by = panelRect.y + panelRect.h - bh - 24;
    setButton(renderer, &backBtn, "BACK", bx, by, bw, bh);
}

static int inside(SDL_Rect r, int x, int y)
{
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

void handleNewGameEvent(SDL_Event* e, MenuState* currentMenu)
{
    int mx,my;
    SDL_GetMouseState(&mx,&my);

    modeMonoBtn.hovered = inside(modeMonoBtn.rect,mx,my);
    modeMultiBtn.hovered = inside(modeMultiBtn.rect,mx,my);
    backBtn.hovered = inside(backBtn.rect,mx,my);

    if (e->type == SDL_MOUSEBUTTONDOWN)
    {
        if (modeMonoBtn.hovered) { if(clickSound) Mix_PlayChannel(-1, clickSound, 0); *currentMenu = MENU_SINGLE; }
        else if (modeMultiBtn.hovered) { if(clickSound) Mix_PlayChannel(-1, clickSound, 0); *currentMenu = MENU_MULTI; }
        else if (backBtn.hovered) { if(clickSound) Mix_PlayChannel(-1, clickSound, 0); *currentMenu = MENU_PLAY; }
    }

    if (e->type == SDL_KEYDOWN)
    {
        if (e->key.keysym.sym == SDLK_ESCAPE) { *currentMenu = MENU_PLAY; return; }
        if (e->key.keysym.sym == SDLK_LEFT) { *currentMenu = MENU_SINGLE; }
        if (e->key.keysym.sym == SDLK_RIGHT) { *currentMenu = MENU_MULTI; }
    }
}

void updateNewGame()
{
    updateSharedBackground(30);
}

static void drawButton(SDL_Renderer* renderer, NGButton* b, int active)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect rect = b->rect;
    if (active) { rect.x -= 6; rect.y -= 4; rect.w += 12; rect.h += 8; }
    SDL_Color fill = active ? buttonHover : buttonColor;
    fillRounded(renderer, rect, 12, fill);
    SDL_Color border = { (Uint8)(active?255:180), (Uint8)(active?215:140), (Uint8)(active?0:60), 220 };
    strokeRounded(renderer, rect, 12, border);
    if (b->iconTexture)
        SDL_RenderCopy(renderer, b->iconTexture, NULL, &b->iconRect);
    SDL_RenderCopy(renderer, b->textTexture, NULL, &b->textRect);
}

void renderNewGame(SDL_Renderer* renderer)
{
    if (bgShared[gCurrentFrame])
        SDL_RenderCopy(renderer, bgShared[gCurrentFrame], NULL, NULL);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0,0,0,120);
    SDL_Rect shadow = {panelRect.x+10, panelRect.y+10, panelRect.w, panelRect.h};
    SDL_RenderFillRect(renderer, &shadow);

    fillRounded(renderer, panelRect, 16, buttonColor);
    strokeRounded(renderer, panelRect, 16, (SDL_Color){220,180,90,220});

    SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
    SDL_RenderCopy(renderer, subtitleTexture, NULL, &subtitleRect);

    drawButton(renderer, &modeMonoBtn, modeMonoBtn.hovered);
    drawButton(renderer, &modeMultiBtn, modeMultiBtn.hovered);
    drawButton(renderer, &backBtn, backBtn.hovered);
}

void destroyNewGame()
{
    if (titleTexture) SDL_DestroyTexture(titleTexture);
    if (subtitleTexture) SDL_DestroyTexture(subtitleTexture);
    if (modeMonoBtn.textTexture) SDL_DestroyTexture(modeMonoBtn.textTexture);
    if (modeMultiBtn.textTexture) SDL_DestroyTexture(modeMultiBtn.textTexture);
    if (modeMonoBtn.iconTexture) SDL_DestroyTexture(modeMonoBtn.iconTexture);
    if (modeMultiBtn.iconTexture) SDL_DestroyTexture(modeMultiBtn.iconTexture);
    if (backBtn.textTexture) SDL_DestroyTexture(backBtn.textTexture);
}
