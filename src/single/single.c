#include "single.h"
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <math.h>

extern TTF_Font* font;
extern Mix_Chunk* hoverSound;
extern Mix_Chunk* clickSound;
extern SDL_Texture* bgShared[SHARED_BG_FRAMES];
extern int gCurrentFrame;

static SDL_Texture* titleTexture;
static SDL_Rect titleRect;
static SDL_Rect panelRect;

typedef struct {
    SDL_Rect rect;
    SDL_Texture* textTexture;
    SDL_Rect textRect;
    SDL_Texture* iconTexture;
    SDL_Rect iconRect;
    int hovered;
} Btn;

static Btn avatarBoyBtn;
static Btn avatarGirlBtn;
static Btn inputKBMBtn;
static Btn inputCtrlBtn;
static Btn confirmBtn;
static Btn returnBtn;

static int selectedAvatar;   /* 0 = Boy, 1 = Girl, -1 = none */
static int selectedInput;    /* 0 = KBM, 1 = Controller, -1 = none */

static SDL_Color gold = {220,180,90,255};
static SDL_Color panelColor = {10,15,25,180};

static SDL_Texture* avatarLabelTex = NULL;
static SDL_Rect avatarLabelRect;
static SDL_Texture* inputLabelTex = NULL;
static SDL_Rect inputLabelRect;

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
static void setText(SDL_Renderer* r, SDL_Texture** tex, SDL_Rect* rect, const char* txt)
{
    if (*tex) { SDL_DestroyTexture(*tex); *tex = NULL; }
    SDL_Surface* s = TTF_RenderText_Blended(font, txt, gold);
    *tex = SDL_CreateTextureFromSurface(r, s);
    rect->w = s->w;
    rect->h = s->h;
    SDL_FreeSurface(s);
}

static void makeBtn(SDL_Renderer* r, Btn* b, const char* label, int x, int y, int w, int h)
{
    b->rect = (SDL_Rect){x,y,w,h};
    setText(r, &b->textTexture, &b->textRect, label);
    b->textRect.x = x + (w - b->textRect.w)/2;
    b->textRect.y = y + (h - b->textRect.h)/2;
    b->iconTexture = NULL;
    b->iconRect = (SDL_Rect){0,0,0,0};
    b->hovered = 0;
}

static void attachIcon(Btn* b, SDL_Renderer* r, const char* iconPath, int iconSize)
{
    SDL_Surface* icon = IMG_Load(iconPath);
    if (icon)
    {
        if (b->iconTexture) SDL_DestroyTexture(b->iconTexture);
        b->iconTexture = SDL_CreateTextureFromSurface(r, icon);
        SDL_FreeSurface(icon);
        int maxIcon = (b->rect.h < b->rect.w ? b->rect.h : b->rect.w) - 24;
        if (iconSize > maxIcon) iconSize = maxIcon;
        if (iconSize < 24) iconSize = 24;
        int spacingV = 8;
        int totalH = iconSize + spacingV + (b->textTexture ? b->textRect.h : 0);
        int startY = b->rect.y + (b->rect.h - totalH)/2;
        b->iconRect.w = iconSize;
        b->iconRect.h = iconSize;
        b->iconRect.x = b->rect.x + (b->rect.w - iconSize)/2;
        b->iconRect.y = startY;
        if (b->textTexture)
        {
            b->textRect.x = b->rect.x + (b->rect.w - b->textRect.w)/2;
            b->textRect.y = startY + iconSize + spacingV;
        }
    }
}

void initSingle(SDL_Renderer* renderer)
{
    setText(renderer, &titleTexture, &titleRect, "Single Player");
    int w = (int)(SCREEN_WIDTH * 0.70f);
    int h = (w * 3)/4;
    int maxH = (int)(SCREEN_HEIGHT * 0.80f);
    if (h > maxH) { h = maxH; w = (h*4)/3; }
    panelRect.w = w;
    panelRect.h = h;
    panelRect.x = (SCREEN_WIDTH - w)/2;
    panelRect.y = (SCREEN_HEIGHT - h)/2;
    titleRect.x = panelRect.x + (panelRect.w - titleRect.w)/2;
    titleRect.y = panelRect.y + 36;
    int marginX = 48;
    int spacing = 32;
    int availableWidth = panelRect.w - 2*marginX;
    setText(renderer, &avatarLabelTex, &avatarLabelRect, "AVATAR");
    avatarLabelRect.x = panelRect.x + (panelRect.w - avatarLabelRect.w)/2;
    avatarLabelRect.y = titleRect.y + titleRect.h + 16;
    int row1Y = avatarLabelRect.y + avatarLabelRect.h + 12;

    setText(renderer, &inputLabelTex, &inputLabelRect, "INPUT METHOD");
    int bottomBtnH = 56, bottomMargin = 24, gapAboveBottom = 24;
    int bottomLimitY = panelRect.y + panelRect.h - (bottomBtnH + bottomMargin + gapAboveBottom);
    int availableH = bottomLimitY - row1Y;
    int sBetweenRows = 28;
    int sLabelSpacing = 12;
    int maxByWidth = (availableWidth - spacing)/2;
    int maxByHeight = (availableH - (sBetweenRows + inputLabelRect.h) - sLabelSpacing) / 2;
    int btnSize = maxByWidth < maxByHeight ? maxByWidth : maxByHeight;
    if (btnSize < 96) btnSize = 96;

    int totalBtnW = btnSize * 2 + spacing;
    int startX = panelRect.x + (panelRect.w - totalBtnW) / 2;
    int leftX = startX;
    int rightX = startX + btnSize + spacing;
    makeBtn(renderer, &avatarBoyBtn, "Boy", leftX, row1Y, btnSize, btnSize);
    makeBtn(renderer, &avatarGirlBtn, "Girl", rightX, row1Y, btnSize, btnSize);
    int iconSizeAvatar = (int)(btnSize * 0.5f);
    attachIcon(&avatarBoyBtn, renderer, "assets/images/ui/boy.png", iconSizeAvatar);
    attachIcon(&avatarGirlBtn, renderer, "assets/images/ui/girl.png", iconSizeAvatar);

    inputLabelRect.x = panelRect.x + (panelRect.w - inputLabelRect.w)/2;
    inputLabelRect.y = row1Y + btnSize + sBetweenRows;
    int row2Y = inputLabelRect.y + inputLabelRect.h + sLabelSpacing;
    makeBtn(renderer, &inputKBMBtn, "Keyboard", leftX, row2Y, btnSize, btnSize);
    makeBtn(renderer, &inputCtrlBtn, "Controller", rightX, row2Y, btnSize, btnSize);
    int iconSizeInput = (int)(btnSize * 0.48f);
    attachIcon(&inputKBMBtn, renderer, "assets/images/ui/keyboard.png", iconSizeInput);
    attachIcon(&inputCtrlBtn, renderer, "assets/images/ui/controller.png", iconSizeInput);

    int cW = 180, cH = 56, cSpacing = 24;
    int totalCW = cW*2 + cSpacing;
    int startCX = panelRect.x + (panelRect.w - totalCW)/2;
    int bottomY = panelRect.y + panelRect.h - cH - bottomMargin;
    makeBtn(renderer, &confirmBtn, "Confirm", startCX, bottomY, cW, cH);
    makeBtn(renderer, &returnBtn, "Return", startCX + cW + cSpacing, bottomY, cW, cH);
    selectedAvatar = -1;
    selectedInput = -1;
}

static int inside(SDL_Rect r, int x, int y){ return x>=r.x && x<=r.x+r.w && y>=r.y && y<=r.y+r.h; }

void handleSingleEvent(SDL_Event* e, MenuState* currentMenu)
{
    int mx,my; SDL_GetMouseState(&mx,&my);
    avatarBoyBtn.hovered = inside(avatarBoyBtn.rect,mx,my);
    avatarGirlBtn.hovered = inside(avatarGirlBtn.rect,mx,my);
    inputKBMBtn.hovered = inside(inputKBMBtn.rect,mx,my);
    inputCtrlBtn.hovered = inside(inputCtrlBtn.rect,mx,my);
    confirmBtn.hovered = inside(confirmBtn.rect,mx,my);
    returnBtn.hovered = inside(returnBtn.rect,mx,my);
    if (e->type == SDL_MOUSEBUTTONDOWN)
    {
        if (avatarBoyBtn.hovered)  { if(clickSound) Mix_PlayChannel(-1, clickSound, 0); selectedAvatar = 0; }
        else if (avatarGirlBtn.hovered) { if(clickSound) Mix_PlayChannel(-1, clickSound, 0); selectedAvatar = 1; }
        else if (inputKBMBtn.hovered)   { if(clickSound) Mix_PlayChannel(-1, clickSound, 0); selectedInput = 0; }
        else if (inputCtrlBtn.hovered)  { if(clickSound) Mix_PlayChannel(-1, clickSound, 0); selectedInput = 1; }
        else if (returnBtn.hovered) { 
            if(clickSound) Mix_PlayChannel(-1, clickSound, 0); 
            selectedAvatar = -1; 
            selectedInput = -1; 
            *currentMenu = MENU_NEWGAME; 
        }
        else if (confirmBtn.hovered && selectedAvatar != -1 && selectedInput != -1) { 
            if(clickSound) Mix_PlayChannel(-1, clickSound, 0); 
            *currentMenu = MENU_SCORE; 
            selectedAvatar = -1; 
            selectedInput = -1; 
        }
    }
    if (e->type == SDL_KEYDOWN)
    {
        if (e->key.keysym.sym == SDLK_ESCAPE) { 
            selectedAvatar = -1; 
            selectedInput = -1; 
            *currentMenu = MENU_NEWGAME; 
        }
        if (e->key.keysym.sym == SDLK_RETURN || e->key.keysym.sym == SDLK_KP_ENTER)
            if (selectedAvatar != -1 && selectedInput != -1) { 
                *currentMenu = MENU_SCORE; 
                selectedAvatar = -1; 
                selectedInput = -1; 
            }
    }
}

void updateSingle(){ updateSharedBackground(30); }

static void drawBtn(SDL_Renderer* r, Btn* b, int active)
{
    SDL_Rect rect = b->rect;
    if (active)
    {
        rect.x -= 6;
        rect.y -= 6;
        rect.w += 12;
        rect.h += 12;
    }
    else if (b->hovered)
    {
        rect.x -= 3;
        rect.y -= 3;
        rect.w += 6;
        rect.h += 6;
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    fillRounded(r, rect, 12, panelColor);
    SDL_Color border;
    if (active)
    {
        border = (SDL_Color){255, 215, 0, 255};
    }
    else
    {
        border = (SDL_Color){
            (Uint8)(b->hovered ? 255 : 180),
            (Uint8)(b->hovered ? 215 : 140),
            (Uint8)(b->hovered ? 0   : 60),
            220
        };
    }
    strokeRounded(r, rect, 12, border);
    if (b->iconTexture)
        SDL_RenderCopy(r, b->iconTexture, NULL, &b->iconRect);
    if (b->textTexture)
        SDL_RenderCopy(r, b->textTexture, NULL, &b->textRect);
}

void renderSingle(SDL_Renderer* renderer)
{
    if (bgShared[gCurrentFrame]) SDL_RenderCopy(renderer, bgShared[gCurrentFrame], NULL, NULL);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0,0,0,120);
    SDL_Rect shadow = {panelRect.x+10,panelRect.y+10,panelRect.w,panelRect.h};
    SDL_RenderFillRect(renderer, &shadow);
    fillRounded(renderer, panelRect, 16, panelColor);
    strokeRounded(renderer, panelRect, 16, (SDL_Color){220,180,90,220});
    SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
    if (avatarLabelTex) SDL_RenderCopy(renderer, avatarLabelTex, NULL, &avatarLabelRect);
    if (inputLabelTex) SDL_RenderCopy(renderer, inputLabelTex, NULL, &inputLabelRect);
    drawBtn(renderer, &avatarBoyBtn,  avatarBoyBtn.hovered  || selectedAvatar == 0);
    drawBtn(renderer, &avatarGirlBtn, avatarGirlBtn.hovered || selectedAvatar == 1);
    drawBtn(renderer, &inputKBMBtn,   inputKBMBtn.hovered   || selectedInput == 0);
    drawBtn(renderer, &inputCtrlBtn,  inputCtrlBtn.hovered  || selectedInput == 1);
    drawBtn(renderer, &returnBtn, 0);
    if (selectedAvatar != -1 && selectedInput != -1) drawBtn(renderer, &confirmBtn, confirmBtn.hovered);
}

void destroySingle()
{
    if (titleTexture) SDL_DestroyTexture(titleTexture);
    if (avatarLabelTex) SDL_DestroyTexture(avatarLabelTex);
    if (inputLabelTex) SDL_DestroyTexture(inputLabelTex);
    if (avatarBoyBtn.textTexture) SDL_DestroyTexture(avatarBoyBtn.textTexture);
    if (avatarGirlBtn.textTexture) SDL_DestroyTexture(avatarGirlBtn.textTexture);
    if (inputKBMBtn.textTexture) SDL_DestroyTexture(inputKBMBtn.textTexture);
    if (inputCtrlBtn.textTexture) SDL_DestroyTexture(inputCtrlBtn.textTexture);
    if (confirmBtn.textTexture) SDL_DestroyTexture(confirmBtn.textTexture);
    if (returnBtn.textTexture) SDL_DestroyTexture(returnBtn.textTexture);
    if (avatarBoyBtn.iconTexture) SDL_DestroyTexture(avatarBoyBtn.iconTexture);
    if (avatarGirlBtn.iconTexture) SDL_DestroyTexture(avatarGirlBtn.iconTexture);
    if (inputKBMBtn.iconTexture) SDL_DestroyTexture(inputKBMBtn.iconTexture);
    if (inputCtrlBtn.iconTexture) SDL_DestroyTexture(inputCtrlBtn.iconTexture);
}
