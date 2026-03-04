#include "multi.h"
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
    SDL_Texture* iconTexture;
    SDL_Rect iconRect;
    int hovered;
} BtnIcon;

static BtnIcon twoBtn;
static BtnIcon fourBtn;
static BtnIcon kbBtn;
static BtnIcon ctrlBtn;
static BtnIcon confirmBtn;
static BtnIcon returnBtn;

static int players;
static int avatars[4];
static int inputType; /* 0 = keyboard, 1 = controller, -1 = none */

static SDL_Color gold = {220,180,90,255};
static SDL_Color panelColor = {10,15,25,180};
static SDL_Color white = {255,255,255,255};
static SDL_Color disabledColor = {110,110,120,255};

static SDL_Texture* texBoy = NULL;
static SDL_Texture* texGirl = NULL;
static SDL_Texture* texKeyboard = NULL;
static SDL_Texture* texController = NULL;

static SDL_Texture* pLabelTex[4] = {NULL,NULL,NULL,NULL};
static SDL_Rect pLabelSize[4];

static SDL_Texture* confirmTextTex = NULL;
static SDL_Rect confirmTextSize;
static SDL_Texture* returnTextTex = NULL;
static SDL_Rect returnTextSize;

/* Minimal text helper for title */
static void setText(SDL_Renderer* r, SDL_Texture** tex, SDL_Rect* rect, const char* txt)
{
    if (*tex) { SDL_DestroyTexture(*tex); *tex = NULL; }
    SDL_Surface* s = TTF_RenderText_Blended(font, txt, gold);
    if (!s) { rect->w = rect->h = 0; return; }
    *tex = SDL_CreateTextureFromSurface(r, s);
    rect->w = s->w;
    rect->h = s->h;
    SDL_FreeSurface(s);
}

static void setTextColored(SDL_Renderer* r, SDL_Texture** tex, SDL_Rect* rect, const char* txt, SDL_Color col)
{
    if (*tex) { SDL_DestroyTexture(*tex); *tex = NULL; }
    SDL_Surface* s = TTF_RenderText_Blended(font, txt, col);
    if (!s) { rect->w = rect->h = 0; return; }
    *tex = SDL_CreateTextureFromSurface(r, s);
    rect->w = s->w;
    rect->h = s->h;
    SDL_FreeSurface(s);
}

static void makeIconBtn(SDL_Renderer* r, BtnIcon* b, const char* iconPath, int x, int y, int w, int h, int iconSize)
{
    b->rect = (SDL_Rect){x,y,w,h};
    b->hovered = 0;
    b->iconTexture = NULL;
    b->iconRect = (SDL_Rect){0,0,0,0};

    if (iconPath && *iconPath)
    {
        SDL_Surface* icon = IMG_Load(iconPath);
        if (icon)
        {
            b->iconTexture = SDL_CreateTextureFromSurface(r, icon);
            SDL_FreeSurface(icon);
            int maxIcon = (w < h ? w : h) - 16;
            if (iconSize > maxIcon) iconSize = maxIcon;
            if (iconSize < 24) iconSize = 24;
            b->iconRect.w = iconSize;
            b->iconRect.h = iconSize;
            b->iconRect.x = x + (w - iconSize)/2;
            b->iconRect.y = y + (h - iconSize)/2;
        }
    }
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
void initMulti(SDL_Renderer* renderer)
{
    setText(renderer, &titleTexture, &titleRect, "Multiplayer");
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
    int spacing = 24;
    int rowY1 = titleRect.y + titleRect.h + 32;
    int btnH = 70;
    int btnW = (panelRect.w - 2*marginX - spacing)/2;
    int startX1 = panelRect.x + (panelRect.w - (2*btnW + spacing))/2;
    makeIconBtn(renderer, &twoBtn, "", startX1, rowY1, btnW, btnH, 40);
    makeIconBtn(renderer, &fourBtn,"", startX1 + btnW + spacing, rowY1, btnW, btnH, 40);
    int rowY2 = rowY1 + btnH + 16;
    int startX2 = panelRect.x + (panelRect.w - (2*btnW + spacing))/2;
    makeIconBtn(renderer, &kbBtn,  "assets/images/ui/keyboard.png",  startX2, rowY2, btnW, btnH, 36);
    makeIconBtn(renderer, &ctrlBtn,"assets/images/ui/controller.png", startX2 + btnW + spacing, rowY2, btnW, btnH, 36);

    int bottomY = panelRect.y + panelRect.h - 56 - 24;
    int cW = 180, cH = 56;
    makeIconBtn(renderer, &confirmBtn, "", panelRect.x + (panelRect.w - cW)/2, bottomY, cW, cH, 32);
    makeIconBtn(renderer, &returnBtn, "", panelRect.x + panelRect.w - marginX - cW, bottomY - cH - 16, cW, cH, 32);

    players = 0;
    inputType = -1;
    for (int i=0;i<4;i++) avatars[i] = -1;

    /* Load shared row icons */
    SDL_Surface* s = IMG_Load("assets/images/ui/boy.png");
    if (s) { texBoy = SDL_CreateTextureFromSurface(renderer, s); SDL_FreeSurface(s); }
    s = IMG_Load("assets/images/ui/girl.png");
    if (s) { texGirl = SDL_CreateTextureFromSurface(renderer, s); SDL_FreeSurface(s); }
    s = IMG_Load("assets/images/ui/keyboard.png");
    if (s) { texKeyboard = SDL_CreateTextureFromSurface(renderer, s); SDL_FreeSurface(s); }
    s = IMG_Load("assets/images/ui/controller.png");
    if (s) { texController = SDL_CreateTextureFromSurface(renderer, s); SDL_FreeSurface(s); }

    setText(renderer, &pLabelTex[0], &pLabelSize[0], "P1");
    setText(renderer, &pLabelTex[1], &pLabelSize[1], "P2");
    setText(renderer, &pLabelTex[2], &pLabelSize[2], "P3");
    setText(renderer, &pLabelTex[3], &pLabelSize[3], "P4");

    setTextColored(renderer, &confirmTextTex, &confirmTextSize, "Confirm", white);
    setTextColored(renderer, &returnTextTex, &returnTextSize, "Return", white);
}

static int inside(SDL_Rect r, int x, int y){ return x>=r.x && x<=r.x+r.w && y>=r.y && y<=r.y+r.h; }

void handleMultiEvent(SDL_Event* e, MenuState* currentMenu)
{
    int mx,my; SDL_GetMouseState(&mx,&my);
    {
        int cW = 180, cH = 56, cGap = 24;
        int bottomY = panelRect.y + panelRect.h - cH - 24;
        int total = cW*2 + cGap;
        int sx = panelRect.x + (panelRect.w - total)/2;
        returnBtn.rect = (SDL_Rect){sx, bottomY, cW, cH};
        confirmBtn.rect = (SDL_Rect){sx + cW + cGap, bottomY, cW, cH};
    }
    twoBtn.hovered = inside(twoBtn.rect,mx,my);
    fourBtn.hovered = inside(fourBtn.rect,mx,my);
    kbBtn.hovered  = inside(kbBtn.rect,mx,my);
    ctrlBtn.hovered= inside(ctrlBtn.rect,mx,my);
    confirmBtn.hovered = inside(confirmBtn.rect,mx,my);
    returnBtn.hovered  = inside(returnBtn.rect,mx,my);
    if (e->type == SDL_MOUSEBUTTONDOWN)
    {
        if (twoBtn.hovered)  { if(clickSound) Mix_PlayChannel(-1, clickSound, 0); players = 2; for(int i=0;i<4;i++) avatars[i]=-1; }
        else if (fourBtn.hovered) { if(clickSound) Mix_PlayChannel(-1, clickSound, 0); players = 4; for(int i=0;i<4;i++) avatars[i]=-1; }
        else if (kbBtn.hovered)   { if(clickSound) Mix_PlayChannel(-1, clickSound, 0); inputType = 0; }
        else if (ctrlBtn.hovered) { if(clickSound) Mix_PlayChannel(-1, clickSound, 0); inputType = 1; }
        else if (returnBtn.hovered) { 
            if(clickSound) Mix_PlayChannel(-1, clickSound, 0); 
            players = 0; 
            inputType = -1; 
            for(int i=0;i<4;i++) avatars[i]=-1; 
            *currentMenu = MENU_NEWGAME; 
        }
        else {
            if (players>0)
            {
                int startY = ctrlBtn.rect.y + ctrlBtn.rect.h + 24;
                int intraGap = 24;
                int groupGap = 28;
                int rowGap = 24;
                int marginX = 48;
                int availableW = panelRect.w - 2*marginX;
                int rows = (players > 2) ? 2 : 1;
                int row0 = (rows == 2) ? (players+1)/2 : players;
                int row1 = (rows == 2) ? (players - row0) : 0;
                int maxRowCount = row0 > row1 ? row0 : row1;
                int sq = 72;
                int groupW = 2*sq + intraGap;
                int rowTotalW = maxRowCount*groupW + (maxRowCount-1)*groupGap;
                if (rowTotalW > availableW)
                {
                    sq = (availableW - (maxRowCount-1)*groupGap - intraGap*maxRowCount) / (2*maxRowCount);
                    if (sq < 48) sq = 48;
                    groupW = 2*sq + intraGap;
                    rowTotalW = maxRowCount*groupW + (maxRowCount-1)*groupGap;
                }
                for (int i=0;i<players;i++)
                {
                    int r = (rows == 2 && i >= row0) ? 1 : 0;
                    int idx = (r==0) ? i : (i - row0);
                    int countInRow = (r==0) ? row0 : row1;
                    int baseX = panelRect.x + (panelRect.w - (countInRow*groupW + (countInRow-1)*groupGap))/2;
                    int gx = baseX + idx*(groupW + groupGap);
                    int gy = startY + r*(sq + rowGap);
                    SDL_Rect boy = {gx, gy, sq, sq};
                    SDL_Rect girl = {gx + sq + intraGap, gy, sq, sq};
                    if (inside(boy,mx,my)) { avatars[i] = 0; if(clickSound) Mix_PlayChannel(-1, clickSound, 0); }
                    if (inside(girl,mx,my)) { avatars[i] = 1; if(clickSound) Mix_PlayChannel(-1, clickSound, 0); }
                }
            }
        }
        if (confirmBtn.hovered)
        {
            int ok = players>0 && inputType!=-1;
            for (int i=0;i<players;i++) if (avatars[i]==-1) ok=0;
            if (ok) { 
                if(clickSound) Mix_PlayChannel(-1, clickSound, 0); 
                *currentMenu = MENU_SCORE; 
                players = 0; 
                inputType = -1; 
                for (int i=0;i<4;i++) avatars[i]=-1; 
            }
        }
    }
    if (e->type == SDL_KEYDOWN)
    {
        if (e->key.keysym.sym == SDLK_ESCAPE) { 
            players = 0; 
            inputType = -1; 
            for (int i=0;i<4;i++) avatars[i]=-1; 
            *currentMenu = MENU_NEWGAME; 
        }
        if ((e->key.keysym.sym == SDLK_RETURN || e->key.keysym.sym == SDLK_KP_ENTER))
        {
            int ok = players>0 && inputType!=-1;
            for (int i=0;i<players;i++) if (avatars[i]==-1) ok=0;
            if (ok) { 
                *currentMenu = MENU_SCORE; 
                players = 0; 
                inputType = -1; 
                for (int i=0;i<4;i++) avatars[i]=-1; 
            }
        }
        if (e->key.keysym.sym == SDLK_2) { players = 2; for(int i=0;i<4;i++) avatars[i]=-1; }
        if (e->key.keysym.sym == SDLK_4) { players = 4; for(int i=0;i<4;i++) avatars[i]=-1; }
    }
}

void updateMulti(){ updateSharedBackground(30); }

static void drawIconBtn(SDL_Renderer* r, BtnIcon* b, int active)
{
    SDL_Rect rect = b->rect;
    int expand = 0;
    if (b->hovered) expand = 4;
    if (active) expand = 8;
    if (expand > 0)
    {
        rect.x -= expand;
        rect.y -= expand;
        rect.w += 2*expand;
        rect.h += 2*expand;
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    fillRounded(r, rect, 12, panelColor);
    int highlight = active || b->hovered;
    SDL_Color border = { (Uint8)(highlight?255:180), (Uint8)(highlight?215:140), (Uint8)(highlight?0:60), 220 };
    strokeRounded(r, rect, 12, border);
    if (b->iconTexture)
        SDL_RenderCopy(r, b->iconTexture, NULL, &b->iconRect);
}

void renderMulti(SDL_Renderer* renderer)
{
    if (bgShared[gCurrentFrame]) SDL_RenderCopy(renderer, bgShared[gCurrentFrame], NULL, NULL);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0,0,0,120);
    SDL_Rect shadow = {panelRect.x+10,panelRect.y+10,panelRect.w,panelRect.h};
    SDL_RenderFillRect(renderer, &shadow);
    fillRounded(renderer, panelRect, 16, panelColor);
    strokeRounded(renderer, panelRect, 16, (SDL_Color){220,180,90,220});
    SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
    drawIconBtn(renderer, &twoBtn, players==2);
    drawIconBtn(renderer, &fourBtn, players==4);
    drawIconBtn(renderer, &kbBtn,  inputType==0);
    drawIconBtn(renderer, &ctrlBtn,inputType==1);
    {
        int cW = 180, cH = 56, cGap = 24;
        int bottomY = panelRect.y + panelRect.h - cH - 24;
        int total = cW*2 + cGap;
        int sx = panelRect.x + (panelRect.w - total)/2;
        returnBtn.rect = (SDL_Rect){sx, bottomY, cW, cH};
        confirmBtn.rect = (SDL_Rect){sx + cW + cGap, bottomY, cW, cH};
    }
    if (!twoBtn.iconTexture || !fourBtn.iconTexture)
    {
        if (!twoBtn.iconTexture)
        {
            SDL_Surface* t = TTF_RenderText_Blended(font, "2 Players", gold);
            SDL_Texture* tt = SDL_CreateTextureFromSurface(renderer, t);
            SDL_Rect r = { twoBtn.rect.x + (twoBtn.rect.w - t->w)/2, twoBtn.rect.y + (twoBtn.rect.h - t->h)/2, t->w, t->h };
            SDL_RenderCopy(renderer, tt, NULL, &r);
            SDL_FreeSurface(t); SDL_DestroyTexture(tt);
        }
        if (!fourBtn.iconTexture)
        {
            SDL_Surface* t = TTF_RenderText_Blended(font, "4 Players", gold);
            SDL_Texture* tt = SDL_CreateTextureFromSurface(renderer, t);
            SDL_Rect r = { fourBtn.rect.x + (fourBtn.rect.w - t->w)/2, fourBtn.rect.y + (fourBtn.rect.h - t->h)/2, t->w, t->h };
            SDL_RenderCopy(renderer, tt, NULL, &r);
            SDL_FreeSurface(t); SDL_DestroyTexture(tt);
        }
        if (!confirmBtn.iconTexture)
        {
            int okc = players>0 && inputType!=-1;
            for (int i=0;i<players;i++) if (avatars[i]==-1) okc=0;
            if (okc)
            {
                SDL_Surface* t = TTF_RenderText_Blended(font, "Confirm", gold);
                SDL_Texture* tt = SDL_CreateTextureFromSurface(renderer, t);
                SDL_Rect r = { confirmBtn.rect.x + (confirmBtn.rect.w - t->w)/2, confirmBtn.rect.y + (confirmBtn.rect.h - t->h)/2, t->w, t->h };
                SDL_RenderCopy(renderer, tt, NULL, &r);
                SDL_FreeSurface(t); SDL_DestroyTexture(tt);
            }
        }
    }
    int startY = ctrlBtn.rect.y + ctrlBtn.rect.h + 24;
    if (players>0)
    {
        int intraGap = 24;
        int groupGap = 28;
        int rowGap = 24;
        int marginX = 48;
        int availableW = panelRect.w - 2*marginX;
        int rows = (players > 2) ? 2 : 1;
        int row0 = (rows == 2) ? (players+1)/2 : players;
        int row1 = (rows == 2) ? (players - row0) : 0;
        int maxRowCount = row0 > row1 ? row0 : row1;
        int sq = 72;
        int groupW = 2*sq + intraGap;
        int rowTotalW = maxRowCount*groupW + (maxRowCount-1)*groupGap;
        if (rowTotalW > availableW)
        {
            sq = (availableW - (maxRowCount-1)*groupGap - intraGap*maxRowCount) / (2*maxRowCount);
            if (sq < 48) sq = 48;
            groupW = 2*sq + intraGap;
            rowTotalW = maxRowCount*groupW + (maxRowCount-1)*groupGap;
        }
        for (int i=0;i<players;i++)
        {
            int r = (rows == 2 && i >= row0) ? 1 : 0;
            int idx = (r==0) ? i : (i - row0);
            int countInRow = (r==0) ? row0 : row1;
            int baseX = panelRect.x + (panelRect.w - (countInRow*groupW + (countInRow-1)*groupGap))/2;
            int gx = baseX + idx*(groupW + groupGap);
            int gy = startY + r*(sq + rowGap);
            SDL_Rect boy =  {gx, gy, sq, sq};
            SDL_Rect girl = {gx + sq + intraGap, gy, sq, sq};
            /* Draw avatar squares */
            SDL_SetRenderDrawColor(renderer, panelColor.r, panelColor.g, panelColor.b, panelColor.a);
            SDL_RenderFillRect(renderer, &boy);
            SDL_RenderFillRect(renderer, &girl);
            SDL_SetRenderDrawColor(renderer, avatars[i]==0?255:180, avatars[i]==0?215:140, avatars[i]==0?0:60, 220);
            SDL_RenderDrawRect(renderer, &boy);
            SDL_SetRenderDrawColor(renderer, avatars[i]==1?255:180, avatars[i]==1?215:140, avatars[i]==1?0:60, 220);
            SDL_RenderDrawRect(renderer, &girl);
            /* Icons inside squares */
            if (texBoy) {
                int isz = sq - 16;
                SDL_Rect dr = {boy.x + (sq - isz)/2, boy.y + (sq - isz)/2, isz, isz};
                SDL_RenderCopy(renderer, texBoy, NULL, &dr);
            }
            if (texGirl) {
                int isz = sq - 16;
                SDL_Rect dr = {girl.x + (sq - isz)/2, girl.y + (sq - isz)/2, isz, isz};
                SDL_RenderCopy(renderer, texGirl, NULL, &dr);
            }
            if (i < 4 && pLabelTex[i])
            {
                SDL_Rect anchor = (avatars[i] == 1) ? girl : boy;
                int padX = 8, padY = 4;
                int lw = pLabelSize[i].w + 2*padX;
                int lh = pLabelSize[i].h + 2*padY;
                SDL_Rect bg = { anchor.x + 6, anchor.y + 6, lw, lh };
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
                SDL_RenderFillRect(renderer, &bg);
                SDL_SetRenderDrawColor(renderer, 220,180,90,255);
                SDL_RenderDrawRect(renderer, &bg);
                SDL_Rect lr = { bg.x + padX, bg.y + padY, pLabelSize[i].w, pLabelSize[i].h };
                SDL_RenderCopy(renderer, pLabelTex[i], NULL, &lr);
            }
        }
    }
    int ok = players>0 && inputType!=-1;
    for (int i=0;i<players;i++) if (avatars[i]==-1) ok=0;

    drawIconBtn(renderer, &returnBtn, returnBtn.hovered);
    drawIconBtn(renderer, &confirmBtn, ok && confirmBtn.hovered);

    if (returnTextTex)
    {
        SDL_Color col = gold;
        if (returnBtn.hovered) col = white;
        SDL_SetTextureColorMod(returnTextTex, col.r, col.g, col.b);
        SDL_Rect r = {
            returnBtn.rect.x + (returnBtn.rect.w - returnTextSize.w)/2,
            returnBtn.rect.y + (returnBtn.rect.h - returnTextSize.h)/2,
            returnTextSize.w,
            returnTextSize.h
        };
        SDL_RenderCopy(renderer, returnTextTex, NULL, &r);
    }

    if (confirmTextTex)
    {
        SDL_Color col = disabledColor;
        if (ok)
        {
            col = gold;
            if (confirmBtn.hovered) col = white;
        }
        SDL_SetTextureColorMod(confirmTextTex, col.r, col.g, col.b);
        SDL_Rect r = {
            confirmBtn.rect.x + (confirmBtn.rect.w - confirmTextSize.w)/2,
            confirmBtn.rect.y + (confirmBtn.rect.h - confirmTextSize.h)/2,
            confirmTextSize.w,
            confirmTextSize.h
        };
        SDL_RenderCopy(renderer, confirmTextTex, NULL, &r);
    }
}

void destroyMulti()
{
    if (titleTexture) SDL_DestroyTexture(titleTexture);
    if (twoBtn.iconTexture) SDL_DestroyTexture(twoBtn.iconTexture);
    if (fourBtn.iconTexture) SDL_DestroyTexture(fourBtn.iconTexture);
    if (kbBtn.iconTexture) SDL_DestroyTexture(kbBtn.iconTexture);
    if (ctrlBtn.iconTexture) SDL_DestroyTexture(ctrlBtn.iconTexture);
    if (confirmBtn.iconTexture) SDL_DestroyTexture(confirmBtn.iconTexture);
    if (returnBtn.iconTexture) SDL_DestroyTexture(returnBtn.iconTexture);
    if (confirmTextTex) SDL_DestroyTexture(confirmTextTex);
    if (returnTextTex) SDL_DestroyTexture(returnTextTex);
    if (texBoy) SDL_DestroyTexture(texBoy);
    if (texGirl) SDL_DestroyTexture(texGirl);
    if (texKeyboard) SDL_DestroyTexture(texKeyboard);
    if (texController) SDL_DestroyTexture(texController);
    for (int i=0;i<4;i++) if (pLabelTex[i]) SDL_DestroyTexture(pLabelTex[i]);
}
