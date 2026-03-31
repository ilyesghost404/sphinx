#include "support.h"
#include "../background/background.h"
#include "../common.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern TTF_Font* font;
extern Mix_Chunk* hoverSound;
extern Mix_Chunk* clickSound;

typedef struct {
    const char* speaker;
    const char* text;
} DlgLine;

static const DlgLine kDlg[] = {
    { "Archmage", "Hold a moment, traveler. The dunes favor those who listen before they leap." },
    { "You", "A wizard? Out here?" },
    { "Archmage", "A guide. Three truths for your road — heed them, and you may keep your souls." },
    { "Archmage", "First: save your leaps for blades and broken ground, not for idle steps." },
    { "Archmage", "Second: when quiet falls, sharpen your timing; panic is the deadliest foe." },
    { "Archmage", "Third: what was lost can be found — but only if you still draw breath." },
    { "You", "I'll remember. Thank you." },
    { "Archmage", "Then walk with care. The desert remembers every footfall." },
};

#define DLG_COUNT ((int)(sizeof(kDlg) / sizeof(kDlg[0])))
#define WIZ_FRAME_COUNT 10

/* World distance at spawn (ahead of player, along the path). */
#define WIZ_SPAWN_OFFSET 620.0f
/* World scale: ~130 units ≈ 1 m (matches walk-away distance comment). */
#define WIZ_METERS_TO_WORLD 130.0f
/* Start dialogue when within this distance (center-to-center). */
#define WIZ_TALK_RADIUS (1.0f * WIZ_METERS_TO_WORLD)
/* Same band as approach: tiny on horizon -> full size when near (used for arrival & leaving). */
#define WIZ_FAR_DIST 520.0f
#define WIZ_NEAR_DIST 130.0f

static SDL_Renderer* gRenderer = NULL;
static SDL_Texture* gWizardIdleFrames[WIZ_FRAME_COUNT];
static int gWizardIdleLoaded = 0;

static float gWizardWorldX = 0.0f;
static float gWizardAnimTime = 0.0f;
static int gAwaitingApproach = 0;
static int gWizardIdleAfterDialogue = 0;

static int gLineIndex = 0;
static SDL_Texture* gSpeakerTex = NULL;
static SDL_Texture* gBodyTex = NULL;
static SDL_Rect gSpeakerRect;
static SDL_Rect gBodyRect;

static SDL_Rect gPanelRect;

static Uint32 gWizardTickLast = 0;
static SDL_Rect gNextRect;
static int gNextHovered = 0;
static SDL_Texture* gNextLabelTex = NULL;
static SDL_Rect gNextLabelRect;

static void destroyDlgTextures(void)
{
    if (gSpeakerTex) SDL_DestroyTexture(gSpeakerTex);
    if (gBodyTex) SDL_DestroyTexture(gBodyTex);
    gSpeakerTex = NULL;
    gBodyTex = NULL;
    gSpeakerRect = (SDL_Rect){ 0, 0, 0, 0 };
    gBodyRect = (SDL_Rect){ 0, 0, 0, 0 };
}

static void destroyNextLabel(void)
{
    if (gNextLabelTex) SDL_DestroyTexture(gNextLabelTex);
    gNextLabelTex = NULL;
    gNextLabelRect = (SDL_Rect){ 0, 0, 0, 0 };
}

static void layoutPanel(void)
{
    gPanelRect.w = SCREEN_WIDTH - 80;
    gPanelRect.h = 236;
    gPanelRect.x = 40;
    gPanelRect.y = SCREEN_HEIGHT - gPanelRect.h - 36;

    gNextRect.w = 140;
    gNextRect.h = 48;
    gNextRect.x = gPanelRect.x + gPanelRect.w - gNextRect.w - 20;
    gNextRect.y = gPanelRect.y + gPanelRect.h - gNextRect.h - 16;
}

static void rebuildLineTextures(void)
{
    if (!gRenderer) return;
    destroyDlgTextures();

    if (!font || gLineIndex < 0 || gLineIndex >= DLG_COUNT) return;

    const DlgLine* L = &kDlg[gLineIndex];
    SDL_Color nameGold = { 220, 180, 90, 255 };
    SDL_Color bodyCol = { 245, 245, 250, 255 };

    SDL_Surface* ns = TTF_RenderText_Blended(font, L->speaker, nameGold);
    if (ns)
    {
        gSpeakerTex = SDL_CreateTextureFromSurface(gRenderer, ns);
        gSpeakerRect.w = ns->w;
        gSpeakerRect.h = ns->h;
        SDL_FreeSurface(ns);
    }

    Uint32 wrap = (Uint32)(gPanelRect.w - 48);
    if (wrap < 200) wrap = 200;
    SDL_Surface* bs = TTF_RenderText_Blended_Wrapped(font, L->text, bodyCol, wrap);
    if (bs)
    {
        gBodyTex = SDL_CreateTextureFromSurface(gRenderer, bs);
        gBodyRect.w = bs->w;
        gBodyRect.h = bs->h;
        SDL_FreeSurface(bs);
    }

    gSpeakerRect.x = gPanelRect.x + 24;
    gSpeakerRect.y = gPanelRect.y + 16;

    gBodyRect.x = gPanelRect.x + 24;
    gBodyRect.y = gSpeakerRect.y + gSpeakerRect.h + 10;
}

static void rebuildNextLabel(void)
{
    if (!gRenderer || !font) return;
    destroyNextLabel();
    SDL_Color gold = { 220, 180, 90, 255 };
    SDL_Surface* s = TTF_RenderText_Blended(font, "NEXT", gold);
    if (!s) return;
    gNextLabelTex = SDL_CreateTextureFromSurface(gRenderer, s);
    gNextLabelRect.w = s->w;
    gNextLabelRect.h = s->h;
    SDL_FreeSurface(s);
}

static int loadWizardIdleFrames(SDL_Renderer* r)
{
    if (gWizardIdleLoaded) return 1;
    if (!r) return 0;

    for (int i = 0; i < WIZ_FRAME_COUNT; i++)
    {
        char path[256];
        snprintf(path, sizeof(path),
                 "assets/images/characters/WIZARD/Wizard_03__IDLE_%03d.png", i);
        SDL_Surface* s = IMG_Load(path);
        if (!s)
        {
            printf("support: failed to load %s: %s\n", path, IMG_GetError());
            for (int j = 0; j < i; j++)
            {
                if (gWizardIdleFrames[j]) SDL_DestroyTexture(gWizardIdleFrames[j]);
                gWizardIdleFrames[j] = NULL;
            }
            return 0;
        }
        gWizardIdleFrames[i] = SDL_CreateTextureFromSurface(r, s);
        SDL_FreeSurface(s);
        if (!gWizardIdleFrames[i])
        {
            for (int j = 0; j < i; j++)
            {
                SDL_DestroyTexture(gWizardIdleFrames[j]);
                gWizardIdleFrames[j] = NULL;
            }
            return 0;
        }
    }
    gWizardIdleLoaded = 1;
    return 1;
}

static void unloadWizardGfx(void)
{
    for (int i = 0; i < WIZ_FRAME_COUNT; i++)
    {
        if (gWizardIdleFrames[i]) SDL_DestroyTexture(gWizardIdleFrames[i]);
        gWizardIdleFrames[i] = NULL;
    }
    gWizardIdleLoaded = 0;
}

static int insideRect(SDL_Rect rect, int x, int y)
{
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
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

static float stepWizardClock(void)
{
    Uint32 now = SDL_GetTicks();
    if (gWizardTickLast == 0) gWizardTickLast = now;
    float dt = (now - gWizardTickLast) / 1000.0f;
    gWizardTickLast = now;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.08f) dt = 0.08f;
    return dt;
}

static void advanceWizardAnim(float dt)
{
    gWizardAnimTime += dt;
    const float frameDur = 0.11f;
    while (gWizardAnimTime >= frameDur * WIZ_FRAME_COUNT) gWizardAnimTime -= frameDur * WIZ_FRAME_COUNT;
}

static void tickWizardAnim(void)
{
    advanceWizardAnim(stepWizardClock());
}

static int currentWizardFrame(void)
{
    int idx = (int)(gWizardAnimTime / 0.11f);
    if (idx < 0) idx = 0;
    if (idx >= WIZ_FRAME_COUNT) idx %= WIZ_FRAME_COUNT;
    return idx;
}

static void beginDialogue(SDL_Renderer* renderer, MenuState* currentMenu)
{
    if (!currentMenu || !renderer) return;
    gRenderer = renderer;
    loadWizardIdleFrames(renderer);

    gLineIndex = 0;
    gNextHovered = 0;

    layoutPanel();
    rebuildLineTextures();
    rebuildNextLabel();

    gWizardTickLast = SDL_GetTicks();

    *currentMenu = MENU_SUPPORT;
}

static void endDialogueReturnToGame(MenuState* currentMenu)
{
    if (!currentMenu) return;
    backgroundClearGameplayInput();
    
    // Instead of walking away, stay idle
    gWizardIdleAfterDialogue = 1;
    
    gNextHovered = 0;
    *currentMenu = MENU_GAME;
    gWizardTickLast = SDL_GetTicks();
}

void initSupport(SDL_Renderer* renderer)
{
    gRenderer = renderer;
    layoutPanel();
}

void supportResetForNewRun(void)
{
    gAwaitingApproach = 0;
    gWizardIdleAfterDialogue = 0;
}

void supportSpawnWizardAhead(SDL_Renderer* renderer, float playerWorldX)
{
    if (!renderer) return;
    gRenderer = renderer;
    loadWizardIdleFrames(renderer);

    gWizardWorldX = playerWorldX + WIZ_SPAWN_OFFSET;
    gWizardAnimTime = 0.0f;
    gWizardTickLast = SDL_GetTicks();
    gAwaitingApproach = 1;
}

int supportIsAwaitingApproach(void)
{
    return gAwaitingApproach;
}

int supportIsDialogueFinished(void)
{
    return gWizardIdleAfterDialogue;
}

void supportUpdateFarewell(float playerWorldX)
{
    (void)playerWorldX;
}

int supportShouldRenderFieldWizard(void)
{
    return gAwaitingApproach || gWizardIdleAfterDialogue;
}

void supportUpdateApproach(SDL_Renderer* renderer, MenuState* currentMenu, float playerWorldX)
{
    if (!gAwaitingApproach || !currentMenu || *currentMenu != MENU_GAME) return;

    gRenderer = renderer;
    tickWizardAnim();

    float d = fabsf(playerWorldX - gWizardWorldX);
    if (d <= WIZ_TALK_RADIUS)
    {
        gAwaitingApproach = 0;
        beginDialogue(renderer, currentMenu);
    }
}

static float approachScale(float worldDist)
{
    if (worldDist >= WIZ_FAR_DIST) return 0.30f;
    if (worldDist <= WIZ_NEAR_DIST) return 1.0f;
    float t = 1.0f - (worldDist - WIZ_NEAR_DIST) / (WIZ_FAR_DIST - WIZ_NEAR_DIST);
    return 0.30f + 0.70f * t;
}

void supportRenderFieldWizard(SDL_Renderer* renderer, float cameraX, float playerWorldX, float groundY)
{
    if (!renderer || !gWizardIdleLoaded) return;
    if (!gAwaitingApproach && !gWizardIdleAfterDialogue) return;

    int frame = currentWizardFrame();
    int screenX = (int)(gWizardWorldX - cameraX);
    int screenY;
    int dw, dh;
    SDL_Texture* t;
    SDL_RendererFlip flip;

    t = gWizardIdleFrames[frame];
    if (!t) return;
    int w = 0, h = 0;
    SDL_QueryTexture(t, NULL, NULL, &w, &h);
    if (w <= 0 || h <= 0) return;
    float d = fabsf(playerWorldX - gWizardWorldX);
    float mul = approachScale(d);
    float targetH = 280.0f * mul;
    float sc = targetH / (float)h;
    dw = (int)(w * sc);
    dh = (int)(h * sc);
    screenY = (int)(groundY - (float)dh - 30.0f); // Moved up by 30px
    screenY -= (int)((1.0f - mul) * 48.0f);
    flip = (gWizardWorldX > playerWorldX) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

    if (screenX + dw / 2 < -80 || screenX - dw / 2 > SCREEN_WIDTH + 80)
        return;

    SDL_Rect dst = { screenX - dw / 2, screenY, dw, dh };
    SDL_RenderCopyEx(renderer, t, NULL, &dst, 0.0, NULL, flip);
}

void handleSupportEvent(SDL_Event* e, MenuState* currentMenu)
{
    if (!e || !currentMenu || *currentMenu != MENU_SUPPORT) return;

    if (e->type == SDL_MOUSEMOTION)
    {
        int h = insideRect(gNextRect, e->motion.x, e->motion.y);
        if (h != gNextHovered && h && hoverSound)
            Mix_PlayChannel(-1, hoverSound, 0);
        gNextHovered = h;
    }

    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT)
    {
        if (insideRect(gNextRect, e->button.x, e->button.y))
        {
            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
            if (gLineIndex >= DLG_COUNT - 1)
            {
                endDialogueReturnToGame(currentMenu);
                return;
            }
            gLineIndex++;
            rebuildLineTextures();
        }
    }

    if (e->type == SDL_KEYDOWN)
    {
        SDL_Keycode k = e->key.keysym.sym;
        if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE)
        {
            if (clickSound) Mix_PlayChannel(-1, clickSound, 0);
            if (gLineIndex >= DLG_COUNT - 1)
            {
                endDialogueReturnToGame(currentMenu);
                return;
            }
            gLineIndex++;
            rebuildLineTextures();
        }
    }
}

void updateSupport(void)
{
    tickWizardAnim();
}

void renderSupport(SDL_Renderer* renderer)
{
    if (!renderer) renderer = gRenderer;
    if (!renderer) return;

    SDL_Color shade = { 0, 0, 0, 90 };
    SDL_Rect full = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    fillRect(renderer, full, shade);

    float cam = backgroundGetCameraX();
    float px = backgroundGetPlayerWorldX();
    float groundY = backgroundGetPlayerGroundY();

    int frame = currentWizardFrame();

    if (gWizardIdleLoaded && gWizardIdleFrames[frame])
    {
        SDL_Texture* t = gWizardIdleFrames[frame];
        int w = 0, h = 0;
        SDL_QueryTexture(t, NULL, NULL, &w, &h);
        if (w > 0 && h > 0)
        {
            float targetH = 260.0f;
            float scale = targetH / (float)h;
            int dw = (int)(w * scale);
            int dh = (int)(h * scale);
            int screenX = (int)(gWizardWorldX - cam);
            int screenY = (int)(groundY - (float)dh - 30.0f); // Moved up by 30px
            SDL_Rect dst = { screenX - dw / 2, screenY, dw, dh };
            SDL_RendererFlip flip = (gWizardWorldX > px) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            SDL_RenderCopyEx(renderer, t, NULL, &dst, 0.0, NULL, flip);
        }
    }

    SDL_Color panelBg = { 10, 12, 22, 220 };
    SDL_Color panelBr = { 255, 220, 160, 200 };
    fillRect(renderer, gPanelRect, panelBg);
    strokeRect(renderer, gPanelRect, panelBr);

    if (gSpeakerTex)
    {
        SDL_Rect dr = gSpeakerRect;
        SDL_RenderCopy(renderer, gSpeakerTex, NULL, &dr);
    }
    if (gBodyTex)
        SDL_RenderCopy(renderer, gBodyTex, NULL, &gBodyRect);

    SDL_Color btnBase = { 20, 24, 40, 230 };
    SDL_Color btnHi = { 255, 180, 100, 100 };
    SDL_Color btnBr = { 255, 220, 160, (Uint8)(gNextHovered ? 220 : 170) };
    fillRect(renderer, gNextRect, btnBase);
    if (gNextHovered) fillRect(renderer, gNextRect, btnHi);
    strokeRect(renderer, gNextRect, btnBr);

    if (gNextLabelTex)
    {
        SDL_Rect lr = {
            gNextRect.x + (gNextRect.w - gNextLabelRect.w) / 2,
            gNextRect.y + (gNextRect.h - gNextLabelRect.h) / 2,
            gNextLabelRect.w,
            gNextLabelRect.h
        };
        SDL_RenderCopy(renderer, gNextLabelTex, NULL, &lr);
    }
}

void destroySupport(void)
{
    destroyDlgTextures();
    destroyNextLabel();
    unloadWizardGfx();
    gRenderer = NULL;
    gLineIndex = 0;
    gNextHovered = 0;
    gAwaitingApproach = 0;
    gWizardIdleAfterDialogue = 0;
}
