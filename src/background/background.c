#include "background.h"
#include "../character/character.h"
#include "../enemy/enemy.h"
#include "../support/support.h"
#include "../minimap/minimap.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include "../save/save.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static float clampf(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

extern TTF_Font* font;
extern Mix_Chunk* hoverSound;
extern Mix_Chunk* clickSound;
extern Mix_Music* menuMusic;

typedef struct {
    const char* path;
    float speedMul;
    int anchorBottom;
    SDL_Texture* tex;
    int texW;
    int texH;
    int renderW;
    int renderH;
} Layer;

static Layer gLayers[] = {
    { "assets/images/backgrounds/gameplay_bg/first_bg/sky.png",         0.10f, 0, NULL, 0, 0, 0, 0 },
    { "assets/images/backgrounds/gameplay_bg/first_bg/sun.png",         0.00f, 0, NULL, 0, 0, 0, 0 },
    { "assets/images/backgrounds/gameplay_bg/first_bg/cloud.png",       0.25f, 0, NULL, 0, 0, 0, 0 },
    { "assets/images/backgrounds/gameplay_bg/first_bg/back_land.png",   0.45f, 1, NULL, 0, 0, 0, 0 },
    { "assets/images/backgrounds/gameplay_bg/first_bg/back_land_2.png", 0.65f, 1, NULL, 0, 0, 0, 0 },
    { "assets/images/backgrounds/gameplay_bg/first_bg/decor.png",       0.90f, 1, NULL, 0, 0, 0, 0 },
    { "assets/images/backgrounds/gameplay_bg/first_bg/land.png",        1.20f, 1, NULL, 0, 0, 0, 0 }
};

static const int gLayerCount = (int)(sizeof(gLayers) / sizeof(gLayers[0]));
static SDL_Renderer* gRenderer = NULL;
static Uint32 gLastTick = 0;
static float gCameraX = 0.0f;

static Character gPlayer;
static int gPlayerReady = 0;
static int gLeftDown = 0;
static int gRightDown = 0;
static int gJumpQueued = 0;
static int gAttackQueued = 0;
static int gPaused = 0;
static int gPauseHovered = 0;
static SDL_Rect gPauseRect = { SCREEN_WIDTH - 84, 16, 68, 48 };

static float gDisplayHealth = 100.0f; // For smooth lerp
static int gSouls = 3;
static float gHurtCooldown = 0.0f;
static float gScreenFlashTimer = 0.0f;

typedef struct {
    SDL_Rect rect;
    const char* label;
    SDL_Texture* textTex;
    SDL_Rect textSize;
    int hovered;
} PauseBtn;

static PauseBtn gPauseBtns[5];
static SDL_Rect gPauseMainPanelRect = { 0, 0, 520, 420 };
static SDL_Rect gSkinPanelRect = { 0, 0, 920, 520 };
static int gLastPauseHovered = -1;
static int gPauseSelected = 0;
static int gSkinIndex = 1;

typedef enum {
    PAUSE_MODE_MAIN = 0,
    PAUSE_MODE_SKIN = 1,
    PAUSE_MODE_SETTINGS = 2,
    PAUSE_MODE_QUIT = 3
} PauseMode;

typedef struct {
    SDL_Rect rect;
    int skinIndex;
    SDL_Texture* previewTex;
    int previewW;
    int previewH;
    SDL_Rect previewSrc;
    int hasPreviewSrc;
    SDL_Texture* labelTex;
    SDL_Rect labelSize;
    int hovered;
} SkinCard;

static PauseMode gPauseMode = PAUSE_MODE_MAIN;
static SkinCard gSkinCards[3];
static int gLastSkinHovered = -1;
static int gSkinSelected = 0;
static SDL_Texture* gSkinTitleTex = NULL;
static SDL_Rect gSkinTitleSize;
static SDL_Texture* gSkinHintTex = NULL;
static SDL_Rect gSkinHintSize;

static SDL_Rect gSettingsPanelRect = { 0, 0, 620, 420 };
static PauseBtn gSettingsBtns[7];
static int gLastSettingsHovered = -1;
static int gSettingsSelected = 0;
static SDL_Texture* gSettingsTitleTex = NULL;
static SDL_Rect gSettingsTitleSize;
static SDL_Texture* gSettingsHintTex = NULL;
static SDL_Rect gSettingsHintSize;
static SDL_Texture* gSettingsMusicLabelTex = NULL;
static SDL_Rect gSettingsMusicLabelSize;
static SDL_Texture* gSettingsSfxLabelTex = NULL;
static SDL_Rect gSettingsSfxLabelSize;
static SDL_Texture* gSettingsMusicValTex = NULL;
static SDL_Rect gSettingsMusicValSize;
static SDL_Texture* gSettingsSfxValTex = NULL;
static SDL_Rect gSettingsSfxValSize;
static SDL_Texture* gSettingsScreenLabelTex = NULL;
static SDL_Rect gSettingsScreenLabelSize;
static SDL_Texture* gSettingsScreenValTex = NULL;
static SDL_Rect gSettingsScreenValSize;

static Uint32 gLastWindowID = 0;

static float gSupportScrollAccum = 0.0f;
static float gSupportLastCamX = 0.0f;
static int gSupportEncounterDone = 0;

static int gEnemySpawned = 0;
static Ennemi gEnemy;

static SDL_Rect gQuitPanelRect = { 0, 0, 640, 320 };
static PauseBtn gQuitBtns[3];
static int gLastQuitHovered = -1;
static int gQuitSelected = 0;
static SDL_Texture* gQuitTitleTex = NULL;
static SDL_Rect gQuitTitleSize;

static SDL_Texture* gToastTex = NULL;
static SDL_Rect gToastSize;
static Uint32 gToastUntil = 0;

static int gGameOverPrompt = 0;
static int gGameOverSelected = 0;
static SDL_Rect gGameOverPanelRect;
static SDL_Rect gGameOverBtnRects[2];
static SDL_Texture* gGameOverTitleTex = NULL;
static SDL_Rect gGameOverTitleSize;
static SDL_Texture* gGameOverBtnTex[2] = { NULL, NULL };
static SDL_Rect gGameOverBtnSize[2];

static int clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void strokeRectThick(SDL_Renderer* r, SDL_Rect rect, SDL_Color c, int thickness)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    for (int i = 0; i < thickness; i++)
    {
        SDL_Rect rr = { rect.x - i, rect.y - i, rect.w + 2 * i, rect.h + 2 * i };
        SDL_RenderDrawRect(r, &rr);
    }
}

static void fillRect(SDL_Renderer* r, SDL_Rect rect, SDL_Color c)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(r, &rect);
}

static void strokeRect(SDL_Renderer* r, SDL_Rect rect, SDL_Color c)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderDrawRect(r, &rect);
}

static void destroyBtnText(PauseBtn* b)
{
    if (b->textTex)
        SDL_DestroyTexture(b->textTex);
    b->textTex = NULL;
    b->textSize = (SDL_Rect){0,0,0,0};
    b->hovered = 0;
}

static void destroyToast()
{
    if (gToastTex)
        SDL_DestroyTexture(gToastTex);
    gToastTex = NULL;
    gToastSize = (SDL_Rect){0,0,0,0};
    gToastUntil = 0;
}

static void destroyGameOverPromptTextures(void)
{
    if (gGameOverTitleTex) SDL_DestroyTexture(gGameOverTitleTex);
    gGameOverTitleTex = NULL;
    gGameOverTitleSize = (SDL_Rect){0,0,0,0};
    for (int i = 0; i < 2; i++)
    {
        if (gGameOverBtnTex[i]) SDL_DestroyTexture(gGameOverBtnTex[i]);
        gGameOverBtnTex[i] = NULL;
        gGameOverBtnSize[i] = (SDL_Rect){0,0,0,0};
    }
}

static void layoutGameOverPrompt(void)
{
    gGameOverPanelRect.w = 640;
    gGameOverPanelRect.h = 260;
    gGameOverPanelRect.x = (SCREEN_WIDTH - gGameOverPanelRect.w) / 2;
    gGameOverPanelRect.y = (SCREEN_HEIGHT - gGameOverPanelRect.h) / 2;

    int bw = 240;
    int bh = 64;
    int gap = 26;
    int totalW = bw * 2 + gap;
    int sx = gGameOverPanelRect.x + (gGameOverPanelRect.w - totalW) / 2;
    int by = gGameOverPanelRect.y + gGameOverPanelRect.h - bh - 36;
    gGameOverBtnRects[0] = (SDL_Rect){ sx, by, bw, bh };
    gGameOverBtnRects[1] = (SDL_Rect){ sx + bw + gap, by, bw, bh };
}

static void ensureGameOverPromptTextures(SDL_Renderer* r)
{
    if (!r || !font) return;
    layoutGameOverPrompt();
    SDL_Color gold = { 220, 180, 90, 255 };

    if (!gGameOverTitleTex)
    {
        SDL_Surface* s = TTF_RenderText_Blended(font, "NO SOULS LEFT", gold);
        if (s)
        {
            gGameOverTitleTex = SDL_CreateTextureFromSurface(r, s);
            gGameOverTitleSize = (SDL_Rect){ 0, 0, s->w, s->h };
            SDL_FreeSurface(s);
        }
    }

    const char* labels[2] = { "ONE MORE LIFE", "QUIT" };
    for (int i = 0; i < 2; i++)
    {
        if (!gGameOverBtnTex[i])
        {
            SDL_Surface* s = TTF_RenderText_Blended(font, labels[i], gold);
            if (s)
            {
                gGameOverBtnTex[i] = SDL_CreateTextureFromSurface(r, s);
                gGameOverBtnSize[i] = (SDL_Rect){ 0, 0, s->w, s->h };
                SDL_FreeSurface(s);
            }
        }
    }
}

static void renderGameOverPrompt(SDL_Renderer* r)
{
    if (!gGameOverPrompt) return;
    ensureGameOverPromptTextures(r);

    SDL_Color dim = { 0, 0, 0, 160 };
    fillRect(r, (SDL_Rect){0,0,SCREEN_WIDTH,SCREEN_HEIGHT}, dim);

    SDL_Color shadow = { 0, 0, 0, 170 };
    SDL_Rect sh = { gGameOverPanelRect.x + 10, gGameOverPanelRect.y + 10, gGameOverPanelRect.w, gGameOverPanelRect.h };
    fillRect(r, sh, shadow);

    SDL_Color panel = { 10, 15, 25, 210 };
    SDL_Color border = { 220, 180, 90, 220 };
    fillRect(r, gGameOverPanelRect, panel);
    strokeRectThick(r, gGameOverPanelRect, border, 3);

    if (gGameOverTitleTex)
    {
        SDL_Rect tr = { gGameOverPanelRect.x + (gGameOverPanelRect.w - gGameOverTitleSize.w) / 2,
                        gGameOverPanelRect.y + 34,
                        gGameOverTitleSize.w, gGameOverTitleSize.h };
        SDL_RenderCopy(r, gGameOverTitleTex, NULL, &tr);
    }

    for (int i = 0; i < 2; i++)
    {
        SDL_Rect br = gGameOverBtnRects[i];
        int sel = (i == gGameOverSelected);
        SDL_Color bfill = sel ? (SDL_Color){ 20, 28, 44, 240 } : (SDL_Color){ 10, 15, 25, 200 };
        SDL_Color bbrd = sel ? (SDL_Color){ 255, 215, 120, 240 } : (SDL_Color){ 180, 140, 60, 210 };
        fillRect(r, br, bfill);
        strokeRectThick(r, br, bbrd, sel ? 3 : 2);
        if (gGameOverBtnTex[i])
        {
            SDL_Rect tt = { br.x + (br.w - gGameOverBtnSize[i].w) / 2,
                            br.y + (br.h - gGameOverBtnSize[i].h) / 2,
                            gGameOverBtnSize[i].w, gGameOverBtnSize[i].h };
            SDL_RenderCopy(r, gGameOverBtnTex[i], NULL, &tt);
        }
    }
}

static void showToast(SDL_Renderer* r, const char* msg)
{
    if (!r || !font || !msg) return;
    destroyToast();
    SDL_Color gold = { 220, 180, 90, 255 };
    SDL_Surface* s = TTF_RenderText_Blended(font, msg, gold);
    if (!s) return;
    gToastTex = SDL_CreateTextureFromSurface(r, s);
    gToastSize.w = s->w;
    gToastSize.h = s->h;
    SDL_FreeSurface(s);
    gToastUntil = SDL_GetTicks() + 1400;
}

static void renderToast(SDL_Renderer* r, SDL_Rect panelRect)
{
    if (!gToastTex) return;
    Uint32 now = SDL_GetTicks();
    if (now >= gToastUntil)
    {
        destroyToast();
        return;
    }

    SDL_Rect bg = { panelRect.x + 22, panelRect.y + panelRect.h - 54, panelRect.w - 44, 36 };
    SDL_Color b = { 0, 0, 0, 160 };
    SDL_Color br = { 255, 220, 160, 140 };
    fillRect(r, bg, b);
    strokeRect(r, bg, br);
    SDL_Rect tr = { bg.x + (bg.w - gToastSize.w) / 2, bg.y + (bg.h - gToastSize.h) / 2, gToastSize.w, gToastSize.h };
    SDL_RenderCopy(r, gToastTex, NULL, &tr);
}

static void respawnPlayerAwayFromEnemy(void)
{
    if (!gPlayerReady) return;
    const float safeDist = 520.0f;
    float enemyCenter = (float)gEnemy.pos.x + (float)gEnemy.pos.w * 0.5f;
    float newX = gPlayer.x;

    if (gEnemySpawned && gEnemy.health != NEUTRALISE)
    {
        if (enemyCenter - safeDist >= 0.0f) newX = enemyCenter - safeDist;
        else newX = enemyCenter + safeDist;
    }
    else
    {
        newX = (gPlayer.x >= safeDist) ? (gPlayer.x - safeDist) : 0.0f;
    }

    if (newX < 0.0f) newX = 0.0f;
    gPlayer.x = newX;
    gPlayer.vx = 0.0f;
    gPlayer.vy = 0.0f;
    gPlayer.onGround = 1;
    gPlayer.y = gPlayer.groundY;

    float targetScreenX = (float)SCREEN_WIDTH * 0.45f;
    gCameraX = gPlayer.x - targetScreenX;
    if (gCameraX < 0.0f) gCameraX = 0.0f;
    gSupportLastCamX = gCameraX;
}

static int insideRect(SDL_Rect rect, int x, int y)
{
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

static void fillCircle(SDL_Renderer* r, int cx, int cy, int radius, SDL_Color c)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    for (int dy = -radius; dy <= radius; dy++)
    {
        int y = cy + dy;
        int dx = (int)sqrtf((float)(radius * radius - dy * dy));
        SDL_RenderDrawLine(r, cx - dx, y, cx + dx, y);
    }
}

static void fillTriangle(SDL_Renderer* r, SDL_Point p0, SDL_Point p1, SDL_Point p2, SDL_Color c)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);

    SDL_Point pts[3] = { p0, p1, p2 };
    for (int i = 0; i < 3; i++)
        for (int j = i + 1; j < 3; j++)
            if (pts[j].y < pts[i].y) { SDL_Point t = pts[i]; pts[i] = pts[j]; pts[j] = t; }

    SDL_Point a = pts[0], b = pts[1], d = pts[2];
    int totalH = d.y - a.y;
    if (totalH == 0) return;

    for (int i = 0; i <= totalH; i++)
    {
        int y = a.y + i;
        int secondHalf = i > (b.y - a.y) || b.y == a.y;
        int segH = secondHalf ? (d.y - b.y) : (b.y - a.y);
        if (segH == 0) continue;

        float alpha = (float)i / (float)totalH;
        float beta = (float)(i - (secondHalf ? (b.y - a.y) : 0)) / (float)segH;

        int ax = (int)lroundf((float)a.x + (float)(d.x - a.x) * alpha);
        int bx = secondHalf
            ? (int)lroundf((float)b.x + (float)(d.x - b.x) * beta)
            : (int)lroundf((float)a.x + (float)(b.x - a.x) * beta);

        if (ax > bx) { int t = ax; ax = bx; bx = t; }
        SDL_RenderDrawLine(r, ax, y, bx, y);
    }
}

static void drawHeart(SDL_Renderer* r, int x, int y, int size, int filled)
{
    SDL_Color fill = { 220, 60, 70, (Uint8)(filled ? 220 : 80) };
    SDL_Color stroke = { 20, 10, 12, 200 };

    int radius = size / 4;
    int leftCx = x + size / 3;
    int rightCx = x + (2 * size) / 3;
    int cy = y + radius + 1;

    fillCircle(r, leftCx, cy, radius, fill);
    fillCircle(r, rightCx, cy, radius, fill);

    SDL_Point p0 = { x + size / 2, y + size };
    SDL_Point p1 = { x + size / 10, y + size / 2 };
    SDL_Point p2 = { x + size - size / 10, y + size / 2 };
    fillTriangle(r, p0, p1, p2, fill);

    SDL_Rect bounds = { x, y, size, size };
    strokeRect(r, bounds, stroke);
}

static void renderPauseButton(SDL_Renderer* r)
{
    SDL_Color base = { 0, 0, 0, 140 };
    SDL_Color hover = { 255, 180, 100, 130 };
    SDL_Color border = { 255, 220, 160, (Uint8)(gPauseHovered ? 220 : 160) };

    fillRect(r, gPauseRect, base);
    if (gPauseHovered) fillRect(r, gPauseRect, hover);
    strokeRect(r, gPauseRect, border);

    SDL_Color bars = { 255, 255, 255, 220 };
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, bars.r, bars.g, bars.b, bars.a);

    int pad = 18;
    int barW = 8;
    int barH = gPauseRect.h - 2 * 12;
    SDL_Rect b1 = { gPauseRect.x + pad, gPauseRect.y + 12, barW, barH };
    SDL_Rect b2 = { gPauseRect.x + pad + 16, gPauseRect.y + 12, barW, barH };

    SDL_RenderFillRect(r, &b1);
    SDL_RenderFillRect(r, &b2);
}

static void renderHealthAndSouls(SDL_Renderer* r)
{
    int barX = 18;
    int barY = 18;
    int barW = 260;
    int barH = 22;

    SDL_Rect frame = { barX, barY, barW, barH };
    SDL_Rect inner = { barX + 3, barY + 3, barW - 6, barH - 6 };

    SDL_Color frameCol = { 255, 220, 160, 200 };
    SDL_Color bgCol = { 0, 0, 0, 140 };
    SDL_Color hpCol = { 120, 220, 90, 220 };

    fillRect(r, frame, bgCol);
    strokeRect(r, frame, frameCol);

    int maxHp = (gPlayer.maxHealth <= 0) ? 1 : gPlayer.maxHealth;
    float dispHp = clampf(gDisplayHealth, 0.0f, (float)maxHp);
    int fillW = (int)((inner.w * dispHp) / (float)maxHp);
    SDL_Rect fill = inner;
    fill.w = fillW;
    fillRect(r, fill, hpCol);

    int heartSize = 26;
    int heartGap = 8;
    int startX = barX + barW + 18;
    int y = barY - 2;
    for (int i = 0; i < 3; i++)
    {
        int filled = (i < clampi(gSouls, 0, 3));
        drawHeart(r, startX + i * (heartSize + heartGap), y, heartSize, filled);
    }
}

static void renderPauseOverlay(SDL_Renderer* r)
{
    SDL_Color shade = { 0, 0, 0, 110 };
    SDL_Rect full = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    fillRect(r, full, shade);

    SDL_Rect panelRect = gPauseMainPanelRect;
    if (gPauseMode == PAUSE_MODE_SKIN) panelRect = gSkinPanelRect;
    else if (gPauseMode == PAUSE_MODE_SETTINGS) panelRect = gSettingsPanelRect;
    else if (gPauseMode == PAUSE_MODE_QUIT) panelRect = gQuitPanelRect;

    SDL_Color panelCol = { 0, 0, 0, 150 };
    SDL_Color border = { 255, 220, 160, 220 };
    fillRect(r, panelRect, panelCol);
    strokeRectThick(r, panelRect, border, 2);

    SDL_Color btnBase = { 10, 15, 25, 190 };
    SDL_Color btnHover = { 255, 180, 100, 120 };
    SDL_Color btnBorder = { 255, 220, 160, 180 };
    SDL_Color txt = { 255, 255, 255, 235 };

    if (gPauseMode == PAUSE_MODE_MAIN)
    {
        for (int i = 0; i < 5; i++)
        {
            PauseBtn* b = &gPauseBtns[i];
            fillRect(r, b->rect, btnBase);
            if (b->hovered) fillRect(r, b->rect, btnHover);
            strokeRect(r, b->rect, btnBorder);
            if (b->textTex)
            {
                SDL_Rect tr = { b->rect.x + (b->rect.w - b->textSize.w) / 2,
                                b->rect.y + (b->rect.h - b->textSize.h) / 2,
                                b->textSize.w, b->textSize.h };
                SDL_RenderCopy(r, b->textTex, NULL, &tr);
            }
            else
            {
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(r, txt.r, txt.g, txt.b, txt.a);
                SDL_Rect inner = { b->rect.x + 18, b->rect.y + b->rect.h / 2 - 2, b->rect.w - 36, 4 };
                SDL_RenderFillRect(r, &inner);
            }
        }
        renderToast(r, panelRect);
        return;
    }

    if (gPauseMode == PAUSE_MODE_SETTINGS)
    {
        if (gSettingsTitleTex)
        {
            SDL_Rect tr = {
                panelRect.x + (panelRect.w - gSettingsTitleSize.w) / 2,
                panelRect.y + 18,
                gSettingsTitleSize.w,
                gSettingsTitleSize.h
            };
            SDL_RenderCopy(r, gSettingsTitleTex, NULL, &tr);
        }
        if (gSettingsHintTex)
        {
            SDL_Rect tr = {
                panelRect.x + (panelRect.w - gSettingsHintSize.w) / 2,
                panelRect.y + 18 + gSettingsTitleSize.h + 6,
                gSettingsHintSize.w,
                gSettingsHintSize.h
            };
            SDL_RenderCopy(r, gSettingsHintTex, NULL, &tr);
        }

        SDL_Color rowBg = { 0, 0, 0, 90 };
        SDL_Color rowBorder = { 255, 220, 160, 110 };

        SDL_Rect row1 = { panelRect.x + 40, panelRect.y + 120, panelRect.w - 80, 86 };
        SDL_Rect row2 = { panelRect.x + 40, panelRect.y + 220, panelRect.w - 80, 86 };
        SDL_Rect row3 = { panelRect.x + 40, panelRect.y + 320, panelRect.w - 80, 86 };
        fillRect(r, row1, rowBg);
        strokeRect(r, row1, rowBorder);
        fillRect(r, row2, rowBg);
        strokeRect(r, row2, rowBorder);
        fillRect(r, row3, rowBg);
        strokeRect(r, row3, rowBorder);

        if (gSettingsMusicLabelTex)
        {
            SDL_Rect tr = { row1.x + 20, row1.y + (row1.h - gSettingsMusicLabelSize.h) / 2, gSettingsMusicLabelSize.w, gSettingsMusicLabelSize.h };
            SDL_RenderCopy(r, gSettingsMusicLabelTex, NULL, &tr);
        }
        if (gSettingsSfxLabelTex)
        {
            SDL_Rect tr = { row2.x + 20, row2.y + (row2.h - gSettingsSfxLabelSize.h) / 2, gSettingsSfxLabelSize.w, gSettingsSfxLabelSize.h };
            SDL_RenderCopy(r, gSettingsSfxLabelTex, NULL, &tr);
        }
        if (gSettingsMusicValTex)
        {
            SDL_Rect tr = { row1.x + row1.w / 2 - gSettingsMusicValSize.w / 2, row1.y + (row1.h - gSettingsMusicValSize.h) / 2, gSettingsMusicValSize.w, gSettingsMusicValSize.h };
            SDL_RenderCopy(r, gSettingsMusicValTex, NULL, &tr);
        }
        if (gSettingsSfxValTex)
        {
            SDL_Rect tr = { row2.x + row2.w / 2 - gSettingsSfxValSize.w / 2, row2.y + (row2.h - gSettingsSfxValSize.h) / 2, gSettingsSfxValSize.w, gSettingsSfxValSize.h };
            SDL_RenderCopy(r, gSettingsSfxValTex, NULL, &tr);
        }

        if (gSettingsScreenLabelTex)
        {
            SDL_Rect tr = { row3.x + 20, row3.y + (row3.h - gSettingsScreenLabelSize.h) / 2, gSettingsScreenLabelSize.w, gSettingsScreenLabelSize.h };
            SDL_RenderCopy(r, gSettingsScreenLabelTex, NULL, &tr);
        }
        if (gSettingsScreenValTex)
        {
            SDL_Rect tr = { row3.x + row3.w / 2 - gSettingsScreenValSize.w / 2, row3.y + (row3.h - gSettingsScreenValSize.h) / 2, gSettingsScreenValSize.w, gSettingsScreenValSize.h };
            SDL_RenderCopy(r, gSettingsScreenValTex, NULL, &tr);
        }

        SDL_Window* w = gLastWindowID ? SDL_GetWindowFromID(gLastWindowID) : NULL;
        int flags = w ? (int)SDL_GetWindowFlags(w) : 0;
        int fullscreen = (flags & SDL_WINDOW_FULLSCREEN) || (flags & SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_Color selectedBorder = { 120, 220, 90, 220 };

        for (int i = 0; i < 7; i++)
        {
            PauseBtn* b = &gSettingsBtns[i];
            fillRect(r, b->rect, btnBase);
            if (b->hovered) fillRect(r, b->rect, btnHover);
            int selected = (i == 4 && fullscreen) || (i == 5 && !fullscreen);
            if (selected) strokeRectThick(r, b->rect, selectedBorder, 2);
            else strokeRect(r, b->rect, btnBorder);
            if (b->textTex)
            {
                SDL_Rect tr = { b->rect.x + (b->rect.w - b->textSize.w) / 2,
                                b->rect.y + (b->rect.h - b->textSize.h) / 2,
                                b->textSize.w, b->textSize.h };
                SDL_RenderCopy(r, b->textTex, NULL, &tr);
            }
        }
        renderToast(r, panelRect);
        return;
    }

    if (gPauseMode == PAUSE_MODE_QUIT)
    {
        if (gQuitTitleTex)
        {
            SDL_Rect tr = {
                panelRect.x + (panelRect.w - gQuitTitleSize.w) / 2,
                panelRect.y + 20,
                gQuitTitleSize.w,
                gQuitTitleSize.h
            };
            SDL_RenderCopy(r, gQuitTitleTex, NULL, &tr);
        }

        for (int i = 0; i < 3; i++)
        {
            PauseBtn* b = &gQuitBtns[i];
            fillRect(r, b->rect, btnBase);
            if (b->hovered) fillRect(r, b->rect, btnHover);
            strokeRect(r, b->rect, btnBorder);
            if (b->textTex)
            {
                SDL_Rect tr = { b->rect.x + (b->rect.w - b->textSize.w) / 2,
                                b->rect.y + (b->rect.h - b->textSize.h) / 2,
                                b->textSize.w, b->textSize.h };
                SDL_RenderCopy(r, b->textTex, NULL, &tr);
            }
        }
        renderToast(r, panelRect);
        return;
    }

    if (gPauseMode == PAUSE_MODE_SKIN)
    {
        SDL_Color cardBorder = { 255, 220, 160, 190 };
        SDL_Color selectedBorder = { 120, 220, 90, 220 };

        if (gSkinTitleTex)
        {
            SDL_Rect tr = {
                panelRect.x + (panelRect.w - gSkinTitleSize.w) / 2,
                panelRect.y + 18,
                gSkinTitleSize.w,
                gSkinTitleSize.h
            };
            SDL_RenderCopy(r, gSkinTitleTex, NULL, &tr);
        }
        if (gSkinHintTex)
        {
            SDL_Rect tr = {
                panelRect.x + (panelRect.w - gSkinHintSize.w) / 2,
                panelRect.y + 18 + gSkinTitleSize.h + 6,
                gSkinHintSize.w,
                gSkinHintSize.h
            };
            SDL_RenderCopy(r, gSkinHintTex, NULL, &tr);
        }

        for (int i = 0; i < 3; i++)
        {
            SkinCard* c = &gSkinCards[i];
            fillRect(r, c->rect, btnBase);
            if (c->hovered) fillRect(r, c->rect, btnHover);

            SDL_Color bcol = (c->skinIndex == gSkinIndex) ? selectedBorder : cardBorder;
            strokeRectThick(r, c->rect, bcol, (c->skinIndex == gSkinIndex) ? 3 : 2);

            if (c->previewTex && c->previewW > 0 && c->previewH > 0)
            {
                int pad = 18;
                int maxW = c->rect.w - 2 * pad;
                int labelH = c->labelTex ? c->labelSize.h : 0;
                int maxH = c->rect.h - 2 * pad - labelH - 18;
                SDL_Rect src = c->hasPreviewSrc ? c->previewSrc : (SDL_Rect){ 0, 0, c->previewW, c->previewH };
                int srcW = (src.w > 0) ? src.w : c->previewW;
                int srcH = (src.h > 0) ? src.h : c->previewH;
                int targetW = (int)lroundf((float)maxW * 0.86f);
                int targetH = (int)lroundf((float)maxH * 0.86f);
                if (targetW < 1) targetW = 1;
                if (targetH < 1) targetH = 1;
                float sx = (float)targetW / (float)srcW;
                float sy = (float)targetH / (float)srcH;
                float s = sx < sy ? sx : sy;
                if (s > 3.0f) s = 3.0f;
                int dw = (int)lroundf((float)srcW * s);
                int dh = (int)lroundf((float)srcH * s);
                SDL_Rect dst = {
                    c->rect.x + (c->rect.w - dw) / 2,
                    c->rect.y + pad + (maxH - dh) / 2,
                    dw, dh
                };
                SDL_RenderCopy(r, c->previewTex, &src, &dst);
            }

            if (c->labelTex)
            {
                SDL_Rect tr = {
                    c->rect.x + (c->rect.w - c->labelSize.w) / 2,
                    c->rect.y + c->rect.h - 16 - c->labelSize.h,
                    c->labelSize.w,
                    c->labelSize.h
                };
                SDL_RenderCopy(r, c->labelTex, NULL, &tr);
            }
        }
        renderToast(r, panelRect);
    }
}

static void rebuildButtonsTextures(SDL_Renderer* r, PauseBtn* btns, int count)
{
    SDL_Color gold = { 220, 180, 90, 255 };
    for (int i = 0; i < count; i++)
    {
        destroyBtnText(&btns[i]);
        if (!font || !btns[i].label) continue;
        SDL_Surface* s = TTF_RenderText_Blended(font, btns[i].label, gold);
        if (!s) continue;
        btns[i].textTex = SDL_CreateTextureFromSurface(r, s);
        btns[i].textSize.w = s->w;
        btns[i].textSize.h = s->h;
        SDL_FreeSurface(s);
    }
}

static void rebuildPauseButtonTextures(SDL_Renderer* r)
{
    rebuildButtonsTextures(r, gPauseBtns, 5);
}

static SDL_Texture* loadUiTexture(SDL_Renderer* r, const char* path, int* outW, int* outH, SDL_Rect* outSrcRect)
{
    if (outW) *outW = 0;
    if (outH) *outH = 0;
    if (outSrcRect) *outSrcRect = (SDL_Rect){0,0,0,0};

    SDL_Surface* s = IMG_Load(path);
    if (!s) return NULL;

    SDL_Rect crop = { 0, 0, s->w, s->h };
    if (s->format && s->format->Amask != 0)
    {
        SDL_Surface* conv = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_RGBA32, 0);
        if (conv)
        {
            int minX = conv->w, minY = conv->h, maxX = -1, maxY = -1;
            SDL_LockSurface(conv);
            for (int y = 0; y < conv->h; y++)
            {
                Uint8* row = (Uint8*)conv->pixels + y * conv->pitch;
                for (int x = 0; x < conv->w; x++)
                {
                    Uint32 px = *(Uint32*)(row + x * 4);
                    Uint8 r8, g8, b8, a8;
                    SDL_GetRGBA(px, conv->format, &r8, &g8, &b8, &a8);
                    if (a8 != 0)
                    {
                        if (x < minX) minX = x;
                        if (y < minY) minY = y;
                        if (x > maxX) maxX = x;
                        if (y > maxY) maxY = y;
                    }
                }
            }
            SDL_UnlockSurface(conv);

            if (maxX >= minX && maxY >= minY)
            {
                int pad = 2;
                minX = clampi(minX - pad, 0, conv->w - 1);
                minY = clampi(minY - pad, 0, conv->h - 1);
                maxX = clampi(maxX + pad, 0, conv->w - 1);
                maxY = clampi(maxY + pad, 0, conv->h - 1);
                crop.x = minX;
                crop.y = minY;
                crop.w = (maxX - minX) + 1;
                crop.h = (maxY - minY) + 1;
            }

            SDL_FreeSurface(conv);
        }
    }

    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
    if (outW) *outW = s->w;
    if (outH) *outH = s->h;
    if (outSrcRect) *outSrcRect = crop;
    SDL_FreeSurface(s);
    return t;
}

static void destroySkinMenuTextures()
{
    if (gSkinTitleTex) SDL_DestroyTexture(gSkinTitleTex);
    if (gSkinHintTex) SDL_DestroyTexture(gSkinHintTex);
    gSkinTitleTex = NULL;
    gSkinHintTex = NULL;
    gSkinTitleSize = (SDL_Rect){0,0,0,0};
    gSkinHintSize = (SDL_Rect){0,0,0,0};

    for (int i = 0; i < 3; i++)
    {
        if (gSkinCards[i].previewTex) SDL_DestroyTexture(gSkinCards[i].previewTex);
        if (gSkinCards[i].labelTex) SDL_DestroyTexture(gSkinCards[i].labelTex);
        gSkinCards[i].previewTex = NULL;
        gSkinCards[i].labelTex = NULL;
        gSkinCards[i].previewW = 0;
        gSkinCards[i].previewH = 0;
        gSkinCards[i].previewSrc = (SDL_Rect){0,0,0,0};
        gSkinCards[i].hasPreviewSrc = 0;
        gSkinCards[i].labelSize = (SDL_Rect){0,0,0,0};
        gSkinCards[i].hovered = 0;
        gSkinCards[i].skinIndex = 0;
    }
}

static void destroySettingsMenuTextures()
{
    if (gSettingsTitleTex) SDL_DestroyTexture(gSettingsTitleTex);
    if (gSettingsHintTex) SDL_DestroyTexture(gSettingsHintTex);
    gSettingsTitleTex = NULL;
    gSettingsHintTex = NULL;
    gSettingsTitleSize = (SDL_Rect){0,0,0,0};
    gSettingsHintSize = (SDL_Rect){0,0,0,0};
    if (gSettingsMusicLabelTex) SDL_DestroyTexture(gSettingsMusicLabelTex);
    if (gSettingsSfxLabelTex) SDL_DestroyTexture(gSettingsSfxLabelTex);
    if (gSettingsMusicValTex) SDL_DestroyTexture(gSettingsMusicValTex);
    if (gSettingsSfxValTex) SDL_DestroyTexture(gSettingsSfxValTex);
    if (gSettingsScreenLabelTex) SDL_DestroyTexture(gSettingsScreenLabelTex);
    if (gSettingsScreenValTex) SDL_DestroyTexture(gSettingsScreenValTex);
    gSettingsMusicLabelTex = NULL;
    gSettingsSfxLabelTex = NULL;
    gSettingsMusicValTex = NULL;
    gSettingsSfxValTex = NULL;
    gSettingsScreenLabelTex = NULL;
    gSettingsScreenValTex = NULL;
    gSettingsMusicLabelSize = (SDL_Rect){0,0,0,0};
    gSettingsSfxLabelSize = (SDL_Rect){0,0,0,0};
    gSettingsMusicValSize = (SDL_Rect){0,0,0,0};
    gSettingsSfxValSize = (SDL_Rect){0,0,0,0};
    gSettingsScreenLabelSize = (SDL_Rect){0,0,0,0};
    gSettingsScreenValSize = (SDL_Rect){0,0,0,0};
    for (int i = 0; i < 7; i++)
        destroyBtnText(&gSettingsBtns[i]);
}

static void destroyQuitMenuTextures()
{
    if (gQuitTitleTex) SDL_DestroyTexture(gQuitTitleTex);
    gQuitTitleTex = NULL;
    gQuitTitleSize = (SDL_Rect){0,0,0,0};
    for (int i = 0; i < 3; i++)
        destroyBtnText(&gQuitBtns[i]);
}

static void updateSettingsValueTextures(SDL_Renderer* r);

static void setPauseSelection(int idx, int playSound)
{
    idx = clampi(idx, 0, 4);
    for (int i = 0; i < 5; i++) gPauseBtns[i].hovered = (i == idx);
    if (playSound && idx != gLastPauseHovered && hoverSound)
        Mix_PlayChannel(-1, hoverSound, 0);
    gLastPauseHovered = idx;
    gPauseSelected = idx;
}

static void setSkinSelection(int idx, int playSound)
{
    idx = clampi(idx, 0, 2);
    for (int i = 0; i < 3; i++) gSkinCards[i].hovered = (i == idx);
    if (playSound && idx != gLastSkinHovered && hoverSound)
        Mix_PlayChannel(-1, hoverSound, 0);
    gLastSkinHovered = idx;
    gSkinSelected = idx;
}

static void setSettingsSelection(int idx, int playSound)
{
    idx = clampi(idx, 0, 6);
    for (int i = 0; i < 7; i++) gSettingsBtns[i].hovered = (i == idx);
    if (playSound && idx != gLastSettingsHovered && hoverSound)
        Mix_PlayChannel(-1, hoverSound, 0);
    gLastSettingsHovered = idx;
    gSettingsSelected = idx;
}

static void setQuitSelection(int idx, int playSound)
{
    idx = clampi(idx, 0, 2);
    for (int i = 0; i < 3; i++) gQuitBtns[i].hovered = (i == idx);
    if (playSound && idx != gLastQuitHovered && hoverSound)
        Mix_PlayChannel(-1, hoverSound, 0);
    gLastQuitHovered = idx;
    gQuitSelected = idx;
}

static void layoutSkinMenu(SDL_Renderer* r)
{
    destroySkinMenuTextures();

    gSkinPanelRect.w = 900;
    gSkinPanelRect.h = 500;
    gSkinPanelRect.x = (SCREEN_WIDTH - gSkinPanelRect.w) / 2;
    gSkinPanelRect.y = (SCREEN_HEIGHT - gSkinPanelRect.h) / 2;

    int cardW = 220;
    int cardH = 340;
    int gap = 36;
    int totalW = 3 * cardW + 2 * gap;
    int startX = gSkinPanelRect.x + (gSkinPanelRect.w - totalW) / 2;
    int startY = gSkinPanelRect.y + 122;

    SDL_Color gold = { 220, 180, 90, 255 };

    if (font)
    {
        SDL_Surface* t = TTF_RenderText_Blended(font, "SELECT SKIN", gold);
        if (t)
        {
            gSkinTitleTex = SDL_CreateTextureFromSurface(r, t);
            gSkinTitleSize.w = t->w;
            gSkinTitleSize.h = t->h;
            SDL_FreeSurface(t);
        }
        SDL_Surface* h = TTF_RenderText_Blended(font, "ESC - BACK", gold);
        if (h)
        {
            gSkinHintTex = SDL_CreateTextureFromSurface(r, h);
            gSkinHintSize.w = h->w;
            gSkinHintSize.h = h->h;
            SDL_FreeSurface(h);
        }
    }

    for (int i = 0; i < 3; i++)
    {
        SkinCard* c = &gSkinCards[i];
        c->skinIndex = i + 1;
        c->rect = (SDL_Rect){ startX + i * (cardW + gap), startY, cardW, cardH };
        c->hovered = 0;

        char previewPath[256];
        snprintf(previewPath, sizeof(previewPath),
                 "assets/images/characters/main_character/%d/idle/Asassin_%02d__IDLE_000.png",
                 c->skinIndex, c->skinIndex);
        c->previewTex = loadUiTexture(r, previewPath, &c->previewW, &c->previewH, &c->previewSrc);
        c->hasPreviewSrc = (c->previewSrc.w > 0 && c->previewSrc.h > 0);

        c->labelTex = NULL;
        c->labelSize = (SDL_Rect){0,0,0,0};
        if (font)
        {
            char label[32];
            snprintf(label, sizeof(label), "SKIN %d", c->skinIndex);
            SDL_Surface* s = TTF_RenderText_Blended(font, label, gold);
            if (s)
            {
                c->labelTex = SDL_CreateTextureFromSurface(r, s);
                c->labelSize.w = s->w;
                c->labelSize.h = s->h;
                SDL_FreeSurface(s);
            }
        }
    }

    gLastSkinHovered = -1;
}

static void layoutSettingsMenu(SDL_Renderer* r)
{
    destroySettingsMenuTextures();

    gSettingsPanelRect.w = 660;
    gSettingsPanelRect.h = 520;
    gSettingsPanelRect.x = (SCREEN_WIDTH - gSettingsPanelRect.w) / 2;
    gSettingsPanelRect.y = (SCREEN_HEIGHT - gSettingsPanelRect.h) / 2;

    gSettingsBtns[0].label = "-";
    gSettingsBtns[1].label = "+";
    gSettingsBtns[2].label = "-";
    gSettingsBtns[3].label = "+";
    gSettingsBtns[4].label = "FULL SCREEN";
    gSettingsBtns[5].label = "NORMAL";
    gSettingsBtns[6].label = "BACK";
    for (int i = 0; i < 7; i++) gSettingsBtns[i].hovered = 0;

    int padX = 40;
    SDL_Rect row1 = { gSettingsPanelRect.x + padX, gSettingsPanelRect.y + 120, gSettingsPanelRect.w - 2 * padX, 86 };
    SDL_Rect row2 = { gSettingsPanelRect.x + padX, gSettingsPanelRect.y + 220, gSettingsPanelRect.w - 2 * padX, 86 };
    SDL_Rect row3 = { gSettingsPanelRect.x + padX, gSettingsPanelRect.y + 320, gSettingsPanelRect.w - 2 * padX, 86 };
    int btnW = 64;
    int btnH = 52;
    int gap = 12;
    int btnY1 = row1.y + (row1.h - btnH) / 2;
    int btnY2 = row2.y + (row2.h - btnH) / 2;
    int btnXPlus = row1.x + row1.w - 18 - btnW;
    int btnXMinus = btnXPlus - gap - btnW;

    gSettingsBtns[0].rect = (SDL_Rect){ btnXMinus, btnY1, btnW, btnH }; // MUSIC -
    gSettingsBtns[1].rect = (SDL_Rect){ btnXPlus,  btnY1, btnW, btnH }; // MUSIC +
    gSettingsBtns[2].rect = (SDL_Rect){ btnXMinus, btnY2, btnW, btnH }; // SFX -
    gSettingsBtns[3].rect = (SDL_Rect){ btnXPlus,  btnY2, btnW, btnH }; // SFX +

    int modeBtnH = 56;
    int modeBtnW = 200;
    int modeGap = 18;
    int modeY = row3.y + (row3.h - modeBtnH) / 2;
    int modeStartX = row3.x + row3.w - 18 - (2 * modeBtnW + modeGap);
    gSettingsBtns[4].rect = (SDL_Rect){ modeStartX, modeY, modeBtnW, modeBtnH };                 // FULL SCREEN
    gSettingsBtns[5].rect = (SDL_Rect){ modeStartX + modeBtnW + modeGap, modeY, modeBtnW, modeBtnH }; // NORMAL

    int backW = 360;
    int backH = 56;
    gSettingsBtns[6].rect = (SDL_Rect){
        gSettingsPanelRect.x + (gSettingsPanelRect.w - backW) / 2,
        gSettingsPanelRect.y + gSettingsPanelRect.h - 78,
        backW,
        backH
    };

    SDL_Color gold = { 220, 180, 90, 255 };
    if (font)
    {
        SDL_Surface* t = TTF_RenderText_Blended(font, "SETTINGS", gold);
        if (t)
        {
            gSettingsTitleTex = SDL_CreateTextureFromSurface(r, t);
            gSettingsTitleSize.w = t->w;
            gSettingsTitleSize.h = t->h;
            SDL_FreeSurface(t);
        }
        SDL_Surface* h = TTF_RenderText_Blended(font, "ESC - BACK", gold);
        if (h)
        {
            gSettingsHintTex = SDL_CreateTextureFromSurface(r, h);
            gSettingsHintSize.w = h->w;
            gSettingsHintSize.h = h->h;
            SDL_FreeSurface(h);
        }
    }

    if (font)
    {
        SDL_Surface* s = TTF_RenderText_Blended(font, "MUSIC", gold);
        if (s)
        {
            gSettingsMusicLabelTex = SDL_CreateTextureFromSurface(r, s);
            gSettingsMusicLabelSize.w = s->w;
            gSettingsMusicLabelSize.h = s->h;
            SDL_FreeSurface(s);
        }
        s = TTF_RenderText_Blended(font, "SFX", gold);
        if (s)
        {
            gSettingsSfxLabelTex = SDL_CreateTextureFromSurface(r, s);
            gSettingsSfxLabelSize.w = s->w;
            gSettingsSfxLabelSize.h = s->h;
            SDL_FreeSurface(s);
        }

        s = TTF_RenderText_Blended(font, "SCREEN", gold);
        if (s)
        {
            gSettingsScreenLabelTex = SDL_CreateTextureFromSurface(r, s);
            gSettingsScreenLabelSize.w = s->w;
            gSettingsScreenLabelSize.h = s->h;
            SDL_FreeSurface(s);
        }
    }

    updateSettingsValueTextures(r);

    gLastSettingsHovered = -1;
    rebuildButtonsTextures(r, gSettingsBtns, 7);
}

static void layoutQuitMenu(SDL_Renderer* r)
{
    destroyQuitMenuTextures();

    gQuitPanelRect.w = 640;
    gQuitPanelRect.h = 320;
    gQuitPanelRect.x = (SCREEN_WIDTH - gQuitPanelRect.w) / 2;
    gQuitPanelRect.y = (SCREEN_HEIGHT - gQuitPanelRect.h) / 2;

    const char* labels[3] = { "SAVE & QUIT", "QUIT", "CANCEL" };
    int btnW = 360;
    int btnH = 56;
    int gap = 14;
    int startY = gQuitPanelRect.y + 104;
    int startX = gQuitPanelRect.x + (gQuitPanelRect.w - btnW) / 2;

    for (int i = 0; i < 3; i++)
    {
        gQuitBtns[i].label = labels[i];
        gQuitBtns[i].rect = (SDL_Rect){ startX, startY + i * (btnH + gap), btnW, btnH };
        gQuitBtns[i].hovered = 0;
    }

    SDL_Color gold = { 220, 180, 90, 255 };
    if (font)
    {
        SDL_Surface* t = TTF_RenderText_Blended(font, "SAVE BEFORE QUIT?", gold);
        if (t)
        {
            gQuitTitleTex = SDL_CreateTextureFromSurface(r, t);
            gQuitTitleSize.w = t->w;
            gQuitTitleSize.h = t->h;
            SDL_FreeSurface(t);
        }
    }

    gLastQuitHovered = -1;
    rebuildButtonsTextures(r, gQuitBtns, 3);
}

static void layoutPauseMenu(SDL_Renderer* r)
{
    gPauseMainPanelRect.w = 520;
    gPauseMainPanelRect.h = 420;
    gPauseMainPanelRect.x = (SCREEN_WIDTH - gPauseMainPanelRect.w) / 2;
    gPauseMainPanelRect.y = (SCREEN_HEIGHT - gPauseMainPanelRect.h) / 2;

    int btnW = 320;
    int btnH = 56;
    int gap = 14;
    int startY = gPauseMainPanelRect.y + 60;
    int startX = gPauseMainPanelRect.x + (gPauseMainPanelRect.w - btnW) / 2;

    const char* labels[5] = { "CONTINUE", "SAVE", "SKIN", "SETTINGS", "QUIT" };
    for (int i = 0; i < 5; i++)
    {
        gPauseBtns[i].label = labels[i];
        gPauseBtns[i].rect = (SDL_Rect){ startX, startY + i * (btnH + gap), btnW, btnH };
        gPauseBtns[i].hovered = 0;
    }

    rebuildPauseButtonTextures(r);
    layoutSkinMenu(r);
    layoutSettingsMenu(r);
    layoutQuitMenu(r);
}

static void setPaused(int paused)
{
    gPaused = paused ? 1 : 0;
    if (gPaused)
    {
        gLeftDown = 0;
        gRightDown = 0;
        gJumpQueued = 0;
        gAttackQueued = 0;
        gPauseMode = PAUSE_MODE_MAIN;
        setPauseSelection(0, 0);
    }
}

static void updatePauseHover(int mx, int my)
{
    int hovered = -1;
    for (int i = 0; i < 5; i++)
    {
        gPauseBtns[i].hovered = insideRect(gPauseBtns[i].rect, mx, my);
        if (gPauseBtns[i].hovered) hovered = i;
    }
    if (hovered != -1 && hovered != gLastPauseHovered && hoverSound)
        Mix_PlayChannel(-1, hoverSound, 0);
    if (hovered != -1) { gLastPauseHovered = hovered; gPauseSelected = hovered; }
}

static void updateSkinHover(int mx, int my)
{
    int hovered = -1;
    for (int i = 0; i < 3; i++)
    {
        gSkinCards[i].hovered = insideRect(gSkinCards[i].rect, mx, my);
        if (gSkinCards[i].hovered) hovered = i;
    }
    if (hovered != -1 && hovered != gLastSkinHovered && hoverSound)
        Mix_PlayChannel(-1, hoverSound, 0);
    if (hovered != -1) { gLastSkinHovered = hovered; gSkinSelected = hovered; }
}

static void updateSettingsHover(int mx, int my)
{
    int hovered = -1;
    for (int i = 0; i < 7; i++)
    {
        gSettingsBtns[i].hovered = insideRect(gSettingsBtns[i].rect, mx, my);
        if (gSettingsBtns[i].hovered) hovered = i;
    }
    if (hovered != -1 && hovered != gLastSettingsHovered && hoverSound)
        Mix_PlayChannel(-1, hoverSound, 0);
    if (hovered != -1) { gLastSettingsHovered = hovered; gSettingsSelected = hovered; }
}

static void updateQuitHover(int mx, int my)
{
    int hovered = -1;
    for (int i = 0; i < 3; i++)
    {
        gQuitBtns[i].hovered = insideRect(gQuitBtns[i].rect, mx, my);
        if (gQuitBtns[i].hovered) hovered = i;
    }
    if (hovered != -1 && hovered != gLastQuitHovered && hoverSound)
        Mix_PlayChannel(-1, hoverSound, 0);
    if (hovered != -1) { gLastQuitHovered = hovered; gQuitSelected = hovered; }
}

static void updateSettingsValueTextures(SDL_Renderer* r)
{
    if (!r || !font) return;

    if (gSettingsMusicValTex) SDL_DestroyTexture(gSettingsMusicValTex);
    if (gSettingsSfxValTex) SDL_DestroyTexture(gSettingsSfxValTex);
    if (gSettingsScreenValTex) SDL_DestroyTexture(gSettingsScreenValTex);
    gSettingsMusicValTex = NULL;
    gSettingsSfxValTex = NULL;
    gSettingsScreenValTex = NULL;
    gSettingsMusicValSize = (SDL_Rect){0,0,0,0};
    gSettingsSfxValSize = (SDL_Rect){0,0,0,0};
    gSettingsScreenValSize = (SDL_Rect){0,0,0,0};

    SDL_Color gold = { 220, 180, 90, 255 };
    int mv = clampi(Mix_VolumeMusic(-1), 0, 128);
    int sv = clampi(Mix_Volume(-1, -1), 0, 128);
    int mp = (mv * 100) / 128;
    int sp = (sv * 100) / 128;
    char buf[32];
    SDL_Surface* s;

    snprintf(buf, sizeof(buf), "%d%%", mp);
    s = TTF_RenderText_Blended(font, buf, gold);
    if (s)
    {
        gSettingsMusicValTex = SDL_CreateTextureFromSurface(r, s);
        gSettingsMusicValSize.w = s->w;
        gSettingsMusicValSize.h = s->h;
        SDL_FreeSurface(s);
    }

    snprintf(buf, sizeof(buf), "%d%%", sp);
    s = TTF_RenderText_Blended(font, buf, gold);
    if (s)
    {
        gSettingsSfxValTex = SDL_CreateTextureFromSurface(r, s);
        gSettingsSfxValSize.w = s->w;
        gSettingsSfxValSize.h = s->h;
        SDL_FreeSurface(s);
    }

    SDL_Window* w = gLastWindowID ? SDL_GetWindowFromID(gLastWindowID) : NULL;
    int flags = w ? (int)SDL_GetWindowFlags(w) : 0;
    int fullscreen = (flags & SDL_WINDOW_FULLSCREEN) || (flags & SDL_WINDOW_FULLSCREEN_DESKTOP);
    snprintf(buf, sizeof(buf), "%s", fullscreen ? "FULL SCREEN" : "NORMAL");
    s = TTF_RenderText_Blended(font, buf, gold);
    if (s)
    {
        gSettingsScreenValTex = SDL_CreateTextureFromSurface(r, s);
        gSettingsScreenValSize.w = s->w;
        gSettingsScreenValSize.h = s->h;
        SDL_FreeSurface(s);
    }
}

static void selectSkin(int skin)
{
    if (skin < 1) skin = 1;
    if (skin > 3) skin = 3;
    if (gSkinIndex == skin) return;
    gSkinIndex = skin;
    if (gPlayerReady)
    {
        characterDestroy(&gPlayer);
        gPlayerReady = characterInit(&gPlayer, gRenderer, gSkinIndex);
    }
}

static void activateSettingsButton(int idx)
{
    if (clickSound) Mix_PlayChannel(-1, clickSound, 0);

    if (idx == 0) { int v = clampi(Mix_VolumeMusic(-1) - 16, 0, 128); Mix_VolumeMusic(v); updateSettingsValueTextures(gRenderer); return; } // MUSIC -
    if (idx == 1) { int v = clampi(Mix_VolumeMusic(-1) + 16, 0, 128); Mix_VolumeMusic(v); updateSettingsValueTextures(gRenderer); return; } // MUSIC +
    if (idx == 2) { int v = clampi(Mix_Volume(-1, -1) - 16, 0, 128); Mix_Volume(-1, v); updateSettingsValueTextures(gRenderer); return; } // SFX -
    if (idx == 3) { int v = clampi(Mix_Volume(-1, -1) + 16, 0, 128); Mix_Volume(-1, v); updateSettingsValueTextures(gRenderer); return; } // SFX +
    if (idx == 4)
    {
        SDL_Window* w = gLastWindowID ? SDL_GetWindowFromID(gLastWindowID) : NULL;
        if (w) applyDisplayMode(w, gRenderer, 1);
        updateSettingsValueTextures(gRenderer);
        showToast(gRenderer, "FULL SCREEN");
        return;
    }
    if (idx == 5)
    {
        SDL_Window* w = gLastWindowID ? SDL_GetWindowFromID(gLastWindowID) : NULL;
        if (w) applyDisplayMode(w, gRenderer, 0);
        updateSettingsValueTextures(gRenderer);
        showToast(gRenderer, "NORMAL");
        return;
    }
    if (idx == 6) { gPauseMode = PAUSE_MODE_MAIN; return; }
}

static void activateQuitButton(int idx, MenuState* currentMenu)
{
    if (!currentMenu) return;
    if (clickSound) Mix_PlayChannel(-1, clickSound, 0);

    if (idx == 0)
    {
        saveGame();
        setPaused(0);
        *currentMenu = MENU_MAIN;
        if (menuMusic) Mix_PlayMusic(menuMusic, -1);
        return;
    }
    if (idx == 1)
    {
        setPaused(0);
        *currentMenu = MENU_MAIN;
        if (menuMusic) Mix_PlayMusic(menuMusic, -1);
        return;
    }
    if (idx == 2)
    {
        gPauseMode = PAUSE_MODE_MAIN;
        return;
    }
}

static void activatePauseButton(int idx, MenuState* currentMenu)
{
    if (!currentMenu) return;
    if (clickSound) Mix_PlayChannel(-1, clickSound, 0);

    if (idx == 0)
    {
        setPaused(0);
        return;
    }
    if (idx == 1)
    {
        saveGame();
        showToast(gRenderer, "SAVED");
        gPauseMode = PAUSE_MODE_MAIN;
        return;
    }
    if (idx == 2)
    {
        gPauseMode = PAUSE_MODE_SKIN;
        setSkinSelection(0, 0);
        return;
    }
    if (idx == 3)
    {
        gPauseMode = PAUSE_MODE_SETTINGS;
        updateSettingsValueTextures(gRenderer);
        setSettingsSelection(0, 0);
        return;
    }
    if (idx == 4)
    {
        gPauseMode = PAUSE_MODE_QUIT;
        setQuitSelection(0, 0);
        return;
    }
}

static void destroyLayer(Layer* l)
{
    if (l->tex)
    {
        SDL_DestroyTexture(l->tex);
        l->tex = NULL;
    }
    l->texW = 0;
    l->texH = 0;
    l->renderW = 0;
    l->renderH = 0;
}

static void loadLayer(SDL_Renderer* renderer, Layer* l)
{
    destroyLayer(l);

    SDL_Surface* s = IMG_Load(l->path);
    if (!s)
    {
        printf("Failed to load background layer %s: %s\n", l->path, IMG_GetError());
        return;
    }

    l->tex = SDL_CreateTextureFromSurface(renderer, s);
    SDL_FreeSurface(s);

    if (!l->tex)
    {
        printf("Failed to create texture for background layer %s: %s\n", l->path, SDL_GetError());
        return;
    }

    SDL_QueryTexture(l->tex, NULL, NULL, &l->texW, &l->texH);
    if (l->texW <= 0) l->texW = SCREEN_WIDTH;
    if (l->texH <= 0) l->texH = SCREEN_HEIGHT;

    float scale = 1.0f;
    if (l->texH > SCREEN_HEIGHT)
        scale = (float)SCREEN_HEIGHT / (float)l->texH;

    l->renderW = (int)lroundf((float)l->texW * scale);
    l->renderH = (int)lroundf((float)l->texH * scale);
    if (l->renderW < 1) l->renderW = 1;
    if (l->renderH < 1) l->renderH = 1;
}

void initBackground(SDL_Renderer* renderer)
{
    gRenderer = renderer;
    for (int i = 0; i < gLayerCount; i++)
        loadLayer(renderer, &gLayers[i]);
    gLastTick = SDL_GetTicks();

    if (!gPlayerReady)
        gPlayerReady = characterInit(&gPlayer, renderer, gSkinIndex);
    gCameraX = 0.0f;
    gSupportScrollAccum = 0.0f;
    gSupportLastCamX = gCameraX;
    gSupportEncounterDone = 0;
    gEnemySpawned = 0;
    initEnnemi(&gEnemy, renderer);
    gLeftDown = 0;
    gRightDown = 0;
    gJumpQueued = 0;
    gAttackQueued = 0;
    setPaused(0);
    gPauseHovered = 0;
    gSouls = 3;
    gHurtCooldown = 0.0f;
    gScreenFlashTimer = 0.0f;
    gLastPauseHovered = -1;
    gPauseMode = PAUSE_MODE_MAIN;
    layoutPauseMenu(renderer);
    initMiniMap();
    gGameOverPrompt = 0;
    gGameOverSelected = 0;
    destroyGameOverPromptTextures();
    layoutGameOverPrompt();

    gSupportLastCamX = gCameraX;
}

void handleBackgroundEvent(SDL_Event* e, MenuState* currentMenu)
{
    if (!e || !currentMenu) return;

    Uint32 wid = 0;
    if (e->type == SDL_MOUSEMOTION) wid = e->motion.windowID;
    else if (e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP) wid = e->button.windowID;
    else if (e->type == SDL_KEYDOWN || e->type == SDL_KEYUP) wid = e->key.windowID;
    if (wid) gLastWindowID = wid;

    if (e->type == SDL_MOUSEMOTION)
    {
        gPauseHovered = insideRect(gPauseRect, e->motion.x, e->motion.y);
        if (gPaused)
        {
            if (gPauseMode == PAUSE_MODE_MAIN) updatePauseHover(e->motion.x, e->motion.y);
            else if (gPauseMode == PAUSE_MODE_SKIN) updateSkinHover(e->motion.x, e->motion.y);
            else if (gPauseMode == PAUSE_MODE_SETTINGS) updateSettingsHover(e->motion.x, e->motion.y);
            else if (gPauseMode == PAUSE_MODE_QUIT) updateQuitHover(e->motion.x, e->motion.y);
        }
    }
    
    handleMiniMapEvent(e);

    if (gGameOverPrompt)
    {
        if (e->type == SDL_MOUSEMOTION)
        {
            int mx = e->motion.x;
            int my = e->motion.y;
            if (insideRect(gGameOverBtnRects[0], mx, my)) gGameOverSelected = 0;
            else if (insideRect(gGameOverBtnRects[1], mx, my)) gGameOverSelected = 1;
        }
        else if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT)
        {
            int mx = e->button.x;
            int my = e->button.y;
            if (insideRect(gGameOverBtnRects[0], mx, my)) gGameOverSelected = 0;
            else if (insideRect(gGameOverBtnRects[1], mx, my)) gGameOverSelected = 1;
            else return;

            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
            if (gGameOverSelected == 0)
            {
                gGameOverPrompt = 0;
                backgroundClearGameplayInput();
                *currentMenu = MENU_ENIGM;
            }
            else
            {
                backgroundClearGameplayInput();
                gGameOverPrompt = 0;
                *currentMenu = MENU_MAIN;
            }
        }
        else if (e->type == SDL_KEYDOWN)
        {
            SDL_Keycode k = e->key.keysym.sym;
            if (k == SDLK_LEFT || k == SDLK_a || k == SDLK_UP || k == SDLK_w) gGameOverSelected = 0;
            else if (k == SDLK_RIGHT || k == SDLK_d || k == SDLK_DOWN || k == SDLK_s) gGameOverSelected = 1;
            else if (k == SDLK_ESCAPE) gGameOverSelected = 1;
            else if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE)
            {
                if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                if (gGameOverSelected == 0)
                {
                    gGameOverPrompt = 0;
                    backgroundClearGameplayInput();
                    *currentMenu = MENU_ENIGM;
                }
                else
                {
                    backgroundClearGameplayInput();
                    gGameOverPrompt = 0;
                    *currentMenu = MENU_MAIN;
                }
            }
        }
        return;
    }

    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT)
    {
        int mx = e->button.x;
        int my = e->button.y;
        if (insideRect(gPauseRect, mx, my))
        {
            setPaused(!gPaused);
            return;
        }
        if (gPaused)
        {
            if (gPauseMode == PAUSE_MODE_MAIN)
            {
                for (int i = 0; i < 5; i++)
                {
                    if (insideRect(gPauseBtns[i].rect, mx, my))
                    {
                        activatePauseButton(i, currentMenu);
                        return;
                    }
                }
            }
            else if (gPauseMode == PAUSE_MODE_SKIN)
            {
                for (int i = 0; i < 3; i++)
                {
                    if (insideRect(gSkinCards[i].rect, mx, my))
                    {
                        if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                        selectSkin(gSkinCards[i].skinIndex);
                        return;
                    }
                }
            }
            else if (gPauseMode == PAUSE_MODE_SETTINGS)
            {
                for (int i = 0; i < 7; i++)
                {
                    if (insideRect(gSettingsBtns[i].rect, mx, my))
                    {
                        activateSettingsButton(i);
                        return;
                    }
                }
            }
            else if (gPauseMode == PAUSE_MODE_QUIT)
            {
                for (int i = 0; i < 3; i++)
                {
                    if (insideRect(gQuitBtns[i].rect, mx, my))
                    {
                        activateQuitButton(i, currentMenu);
                        return;
                    }
                }
            }
        }
    }

    if (e->type == SDL_KEYDOWN || e->type == SDL_KEYUP)
    {
        int down = (e->type == SDL_KEYDOWN);
        SDL_Keycode k = e->key.keysym.sym;

        if (down && k == SDLK_ESCAPE)
        {
            if (gPaused && gPauseMode != PAUSE_MODE_MAIN)
                gPauseMode = PAUSE_MODE_MAIN;
            else
                setPaused(!gPaused);
            return;
        }
        if (down && k == SDLK_p)
        {
            setPaused(!gPaused);
            return;
        }

        if (down && gPaused)
        {
            int navUp = (k == SDLK_UP || k == SDLK_w);
            int navDown = (k == SDLK_DOWN || k == SDLK_s);
            int navLeft = (k == SDLK_LEFT || k == SDLK_a);
            int navRight = (k == SDLK_RIGHT || k == SDLK_d);
            int activate = (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE);

            if (navUp || navDown || navLeft || navRight)
            {
                if (gPauseMode == PAUSE_MODE_MAIN)
                {
                    int idx = gPauseSelected;
                    if (navUp || navLeft) idx = (idx + 4) % 5;
                    else if (navDown || navRight) idx = (idx + 1) % 5;
                    setPauseSelection(idx, 1);
                    return;
                }
                if (gPauseMode == PAUSE_MODE_SKIN)
                {
                    int idx = gSkinSelected;
                    if (navUp || navLeft) idx = (idx + 2) % 3;
                    else if (navDown || navRight) idx = (idx + 1) % 3;
                    setSkinSelection(idx, 1);
                    return;
                }
                if (gPauseMode == PAUSE_MODE_QUIT)
                {
                    int idx = gQuitSelected;
                    if (navUp || navLeft) idx = (idx + 2) % 3;
                    else if (navDown || navRight) idx = (idx + 1) % 3;
                    setQuitSelection(idx, 1);
                    return;
                }
                if (gPauseMode == PAUSE_MODE_SETTINGS)
                {
                    int idx = gSettingsSelected;
                    int side = (idx == 1 || idx == 3 || idx == 5) ? 1 : 0;
                    int group = 0;
                    if (idx <= 1) group = 0;
                    else if (idx <= 3) group = 1;
                    else if (idx <= 5) group = 2;
                    else group = 3;

                    if (navLeft || navRight)
                    {
                        if (group == 0 || group == 1 || group == 2)
                        {
                            side = 1 - side;
                        }
                        else
                        {
                            group = 2;
                            side = navLeft ? 0 : 1;
                        }
                    }
                    else if (navUp)
                    {
                        group = (group + 3) % 4;
                    }
                    else if (navDown)
                    {
                        group = (group + 1) % 4;
                    }

                    if (group == 0) idx = side;
                    else if (group == 1) idx = 2 + side;
                    else if (group == 2) idx = 4 + side;
                    else idx = 6;

                    setSettingsSelection(idx, 1);
                    return;
                }
            }

            if (activate)
            {
                if (gPauseMode == PAUSE_MODE_MAIN)
                {
                    activatePauseButton(gPauseSelected, currentMenu);
                    return;
                }
                if (gPauseMode == PAUSE_MODE_SKIN)
                {
                    if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
                    selectSkin(gSkinCards[gSkinSelected].skinIndex);
                    return;
                }
                if (gPauseMode == PAUSE_MODE_SETTINGS)
                {
                    activateSettingsButton(gSettingsSelected);
                    if (gPauseMode == PAUSE_MODE_MAIN) setPauseSelection(gPauseSelected, 0);
                    return;
                }
                if (gPauseMode == PAUSE_MODE_QUIT)
                {
                    activateQuitButton(gQuitSelected, currentMenu);
                    return;
                }
            }
        }

        if (!gPaused)
        {
            if (k == SDLK_q) gLeftDown = down;
            if (k == SDLK_d) gRightDown = down;
            if (down && k == SDLK_SPACE) gJumpQueued = 1;
            if (down && k == SDLK_a) gAttackQueued = 1;
        }
    }
}

void updateBackground(MenuState* currentMenu)
{
    Uint32 now = SDL_GetTicks();
    float dt = (now - gLastTick) / 1000.0f;
    gLastTick = now;

    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.05f) dt = 0.05f;

    if (gGameOverPrompt) return;
    if (gPaused) return;

        if (gPlayerReady)
        {
            int dir = 0;
            if (!gPlayer.isDead && gPlayer.anim != CHAR_ANIM_HURT) {
                if (gLeftDown && !gRightDown) dir = -1;
                else if (gRightDown && !gLeftDown) dir = 1;

                characterSetMove(&gPlayer, dir);
                if (gJumpQueued) { characterJump(&gPlayer); gJumpQueued = 0; }
                if (gAttackQueued) { characterAttack(&gPlayer); gAttackQueued = 0; }
            } else {
                characterSetMove(&gPlayer, 0);
                gJumpQueued = 0;
                gAttackQueued = 0;
            }
        
        characterUpdate(&gPlayer, dt);

        float targetScreenX = (float)SCREEN_WIDTH * 0.45f;
        gCameraX = gPlayer.x - targetScreenX;
        if (gCameraX < 0.0f) gCameraX = 0.0f;
    }

        if (currentMenu && *currentMenu == MENU_GAME && gPlayerReady && gRenderer)
    {
        // Smooth health lerp for player
        if (gDisplayHealth > (float)gPlayer.currentHealth) {
            gDisplayHealth -= 50.0f * dt; // Lerp speed
            if (gDisplayHealth < (float)gPlayer.currentHealth) gDisplayHealth = (float)gPlayer.currentHealth;
        } else if (gDisplayHealth < (float)gPlayer.currentHealth) {
            gDisplayHealth += 50.0f * dt;
            if (gDisplayHealth > (float)gPlayer.currentHealth) gDisplayHealth = (float)gPlayer.currentHealth;
        }

        Layer* land = &gLayers[gLayerCount - 1];
        float period = (land->speedMul > 0.001f && land->renderW > 0)
            ? (float)land->renderW / land->speedMul
            : (float)SCREEN_WIDTH;

        float delta = gCameraX - gSupportLastCamX;
        gSupportLastCamX = gCameraX;
        if (delta > 0.0f)
        {
            gSupportScrollAccum += delta;
        }

        /* Enemy spawn: after support dialogue finished. */
        if (!gEnemySpawned && supportIsDialogueFinished())
        {
            gEnemySpawned = 1;
            gEnemy.pos.x = (int)(gPlayer.x + SCREEN_WIDTH * 0.8f); // Spawn ahead
            // Positioned slightly lower (but still flying): groundY minus height minus small offset
            gEnemy.pos.y = (int)(gPlayer.groundY - gEnemy.pos.h - 40); 
            gEnemy.health = VIVANT;
            gEnemy.state = EN_ANIM_IDLE;
        }

        /* Update Enemy if spawned */
        if (gEnemySpawned && gEnemy.health != NEUTRALISE)
        {
            // Player body for collision: centered on player X, extending up from Y
            // Since player is drawn from groundY - dh, we adjust collision box
            SDL_Rect playerBody = {(int)gPlayer.x - 40, (int)gPlayer.y - 180, 80, 180};
            
            // Debug collision boxes if needed (visual only)
            // SDL_SetRenderDrawColor(gRenderer, 255, 255, 0, 255);
            // SDL_RenderDrawRect(gRenderer, &playerBody);
            // SDL_RenderDrawRect(gRenderer, &gEnemy.pos);

            moveIA(&gEnemy, playerBody, dt);
            animerEnnemi(&gEnemy);

            // Simple distance check - hurt enemy if player attacks at specific frames
            if (gPlayer.anim == CHAR_ANIM_ATTACK && !gPlayer.hasDealtDamageInCurrentAttack)
            {
                int playerFrame = (int)(gPlayer.animTime / gPlayer.clips[CHAR_ANIM_ATTACK].frameDuration);
                if (playerFrame >= 4 && playerFrame <= 6)
                {
                    int pCenterX = playerBody.x + playerBody.w / 2;
                    int pCenterY = playerBody.y + playerBody.h / 2;
                    int attackOffset = 60;
                    int attackX = pCenterX + (gPlayer.facing >= 0 ? attackOffset : -attackOffset);
                    int attackY = pCenterY;

                    int ex1 = gEnemy.pos.x;
                    int ex2 = gEnemy.pos.x + gEnemy.pos.w;
                    int ey1 = gEnemy.pos.y;
                    int ey2 = gEnemy.pos.y + gEnemy.pos.h;

                    int facingOk = 1;
                    if (gPlayer.facing >= 0 && ex2 < pCenterX - 10) facingOk = 0;
                    if (gPlayer.facing < 0 && ex1 > pCenterX + 10) facingOk = 0;

                    int dx = 0;
                    if (attackX < ex1) dx = ex1 - attackX;
                    else if (attackX > ex2) dx = attackX - ex2;

                    int dy = 0;
                    if (attackY < ey1) dy = ey1 - attackY;
                    else if (attackY > ey2) dy = attackY - ey2;

                    float dist = sqrtf((float)(dx * dx + dy * dy));

                    printf("[DEBUG] PlayerAttackCheck dist=%.1f range=%.1f facing=%d origin=(%d,%d) dxy=(%d,%d)\n",
                           dist, gPlayer.attackRange, gPlayer.facing, attackX, attackY, dx, dy);

                    if (facingOk && dist <= gPlayer.attackRange)
                    {
                        int playerDamage = 25;
                        ennemiTakeDamage(&gEnemy, playerDamage);
                        gPlayer.hasDealtDamageInCurrentAttack = 1;
                        printf("[DEBUG] Player hit Enemy for %d damage. Enemy health: %d/%d\n", 
                               playerDamage, gEnemy.currentHealth, gEnemy.maxHealth);
                    }
                }
            }
            
            // Enemy attacks player: check distance during attack frames (e.g., frames 6-8)
            if (gEnemy.state == EN_ANIM_ATTACK && !gEnemy.hasDealtDamageInCurrentAttack && gHurtCooldown <= 0.0f)
            {
                if (gEnemy.currentFrame >= 6 && gEnemy.currentFrame <= 8)
                {
                    int dx = (int)gEnemy.pos.x - (int)playerBody.x;
                    int abs_dx = (dx < 0) ? -dx : dx;
                    
                    if (abs_dx <= gEnemy.attackRange + 40) // +40 for some buffer
                    {
                        printf("[DEBUG] Damage Applied! Enemy distance: %d, attackRange: %d\n", abs_dx, gEnemy.attackRange);
                        characterTakeDamage(&gPlayer, 20);
                        gHurtCooldown = 1.1f;
                        gScreenFlashTimer = 0.25f; 
                        gEnemy.hasDealtDamageInCurrentAttack = 1; // Prevent multi-hit in one swing
                        
                        if (gPlayer.currentHealth == 0)
                        {
                            gSouls--;
                            if (gSouls > 0)
                            {
                                characterRevive(&gPlayer);
                                respawnPlayerAwayFromEnemy();
                                gDisplayHealth = (float)gPlayer.currentHealth;
                                gHurtCooldown = 1.6f;
                            }
                            else
                            {
                                printf("[DEBUG] GameOverPrompt\n");
                                backgroundClearGameplayInput();
                                gGameOverPrompt = 1;
                                gGameOverSelected = 0;
                                ensureGameOverPromptTextures(gRenderer);
                                return;
                            }
                        }
                    }
                }
            }
        }

        if (gHurtCooldown > 0.0f)
            gHurtCooldown -= dt;
        
        if (gScreenFlashTimer > 0.0f)
            gScreenFlashTimer -= dt;

        /* Wizard milestone: half of one ground-layer parallax period. */
        if (!gSupportEncounterDone && gSupportScrollAccum >= period * 0.5f)
        {
            gSupportEncounterDone = 1;
            supportSpawnWizardAhead(gRenderer, gPlayer.x);
        }
    }

    if (currentMenu && *currentMenu == MENU_GAME && gPlayerReady && gRenderer && supportIsAwaitingApproach())
        supportUpdateApproach(gRenderer, currentMenu, gPlayer.x);

    if (currentMenu && *currentMenu == MENU_GAME && gPlayerReady && gRenderer)
        supportUpdateFarewell(gPlayer.x);
}

float backgroundGetCameraX(void)
{
    return gCameraX;
}

float backgroundGetPlayerWorldX(void)
{
    return gPlayerReady ? gPlayer.x : 0.0f;
}

float backgroundGetPlayerGroundY(void)
{
    return gPlayerReady ? gPlayer.groundY : (float)SCREEN_HEIGHT - 120.0f;
}

void backgroundOnEnterGameplay(void)
{
    gSupportScrollAccum = 0.0f;
    gSupportEncounterDone = 0;
    gSupportLastCamX = gCameraX;
    supportResetForNewRun();
}

void backgroundClearGameplayInput(void)
{
    gLeftDown = 0;
    gRightDown = 0;
    gJumpQueued = 0;
    gAttackQueued = 0;
    if (gPlayerReady)
        characterSetMove(&gPlayer, 0);
}

void backgroundGrantOneMoreLife(void)
{
    gSouls = 1;
    if (gPlayerReady)
        characterRevive(&gPlayer);
    gDisplayHealth = gPlayerReady ? (float)gPlayer.currentHealth : 100.0f;
    gHurtCooldown = 0.0f;
    gScreenFlashTimer = 0.0f;
}

static void renderLayer(SDL_Renderer* renderer, const Layer* l)
{
    if (!l->tex || l->renderW <= 0 || l->renderH <= 0) return;

    int y = l->anchorBottom ? (SCREEN_HEIGHT - l->renderH) : 0;
    if (y < 0) y = 0;

    float scroll = 0.0f;
    if (l->speedMul != 0.0f && l->renderW > 0)
    {
        scroll = fmodf(gCameraX * l->speedMul, (float)l->renderW);
        if (scroll < 0.0f) scroll += (float)l->renderW;
    }

    int xStart = -(int)scroll;
    while (xStart > 0) xStart -= l->renderW;

    for (int x = xStart; x < SCREEN_WIDTH; x += l->renderW)
    {
        SDL_Rect dst = { x, y, l->renderW, l->renderH };
        SDL_RenderCopy(renderer, l->tex, NULL, &dst);
    }
}

void renderBackground(SDL_Renderer* renderer)
{
    if (!renderer) renderer = gRenderer;
    if (!renderer) return;

    for (int i = 0; i < gLayerCount; i++)
        renderLayer(renderer, &gLayers[i]);

    if (gPlayerReady)
        characterRender(&gPlayer, renderer, gCameraX, (gPlayer.hurtTimer > 0.0f));

    if (gPlayerReady && supportShouldRenderFieldWizard())
        supportRenderFieldWizard(renderer, gCameraX, gPlayer.x, gPlayer.groundY);

    if (gEnemySpawned && gEnemy.health != NEUTRALISE)
        afficherEnnemi(gEnemy, renderer, gCameraX);

    // Render Red Flash Effect if hurt
    if (gScreenFlashTimer > 0.0f)
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100); // Semi-transparent red
        SDL_RenderFillRect(renderer, NULL);
    }

    renderHealthAndSouls(renderer);
    renderMiniMap(renderer, &gPlayer, &gEnemy, gEnemySpawned);
    renderPauseButton(renderer);
    if (gPaused)
        renderPauseOverlay(renderer);
    renderGameOverPrompt(renderer);
}

void destroyBackground()
{
    for (int i = 0; i < gLayerCount; i++)
        destroyLayer(&gLayers[i]);
    if (gPlayerReady)
    {
        characterDestroy(&gPlayer);
        gPlayerReady = 0;
    }
    for (int i = 0; i < 5; i++)
    {
        if (gPauseBtns[i].textTex)
            SDL_DestroyTexture(gPauseBtns[i].textTex);
        gPauseBtns[i].textTex = NULL;
    }
    destroySkinMenuTextures();
    destroySettingsMenuTextures();
    destroyQuitMenuTextures();
    destroyToast();
    destroyGameOverPromptTextures();
    gRenderer = NULL;
}
