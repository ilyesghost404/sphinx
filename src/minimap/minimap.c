#include "minimap.h"
#include <math.h>

static int gShowMiniMap = 1;
static SDL_Rect gMiniMapRect = { 20, 100, 200, 200 }; // Square minimap
static float gLerpPlayerX = 0.0f;
static float gLerpEnemyX = 0.0f;

void initMiniMap(void)
{
    gShowMiniMap = 1;
    gMiniMapRect = (SDL_Rect){ 24, 110, 180, 180 };
    gLerpPlayerX = 0.0f;
    gLerpEnemyX = 0.0f;
}

void handleMiniMapEvent(SDL_Event* e)
{
    if (e->type == SDL_KEYDOWN)
    {
        if (e->key.keysym.sym == SDLK_m)
        {
            gShowMiniMap = !gShowMiniMap;
        }
    }
}

static void drawPlayerIcon(SDL_Renderer* r, int cx, int cy, int size, int facing)
{
    SDL_Point points[4];
    if (facing > 0) {
        points[0] = (SDL_Point){ cx + size, cy };
        points[1] = (SDL_Point){ cx - size, cy - size / 2 };
        points[2] = (SDL_Point){ cx - size, cy + size / 2 };
        points[3] = points[0];
    } else {
        points[0] = (SDL_Point){ cx - size, cy };
        points[1] = (SDL_Point){ cx + size, cy - size / 2 };
        points[2] = (SDL_Point){ cx + size, cy + size / 2 };
        points[3] = points[0];
    }
    SDL_SetRenderDrawColor(r, 0, 255, 255, 255); // Cyan player
    SDL_RenderDrawLines(r, points, 4);
    // Fill slightly
    SDL_RenderDrawLine(r, points[0].x, points[0].y, points[1].x, points[1].y);
    SDL_RenderDrawLine(r, points[0].x, points[0].y, points[2].x, points[2].y);
}

void renderMiniMap(SDL_Renderer* r, const Character* player, const Ennemi* enemy, int enemySpawned)
{
    if (!gShowMiniMap || !player) return;

    // Smooth movement (lerp)
    float dt = 0.1f; // Approx dt for smooth lerp on minimap
    gLerpPlayerX += (player->x - gLerpPlayerX) * dt;
    if (enemySpawned && enemy) {
        gLerpEnemyX += (enemy->pos.x - gLerpEnemyX) * dt;
    }

    // 1. Draw Background (Rounded Square)
    SDL_Color bgCol = { 15, 20, 30, 180 }; // Modern dark slate
    uiFillRounded(r, gMiniMapRect, 12, bgCol);
    
    SDL_Color borderCol = { 100, 110, 130, 200 }; // Modern gray border
    uiStrokeRounded(r, gMiniMapRect, 12, borderCol);

    // 2. Define world-to-minimap scale (Local view)
    // We show roughly 1000 units across the 180px minimap
    float worldViewWidth = 1200.0f;
    float scale = (float)gMiniMapRect.w / worldViewWidth;
    int midX = gMiniMapRect.x + gMiniMapRect.w / 2;
    int midY = gMiniMapRect.y + gMiniMapRect.h / 2;

    // 3. Draw World simplified (Floor line relative to player)
    SDL_SetRenderDrawColor(r, 60, 70, 90, 255);
    int floorY = midY + 40;
    SDL_RenderDrawLine(r, gMiniMapRect.x + 5, floorY, gMiniMapRect.x + gMiniMapRect.w - 5, floorY);

    // 4. Draw Player (Centered modern icon)
    drawPlayerIcon(r, midX, floorY - 12, 8, player->facing);

    // 5. Draw Enemy (Red sleek dot relative to player)
    if (enemySpawned && enemy && enemy->health != NEUTRALISE)
    {
        float relX = (gLerpEnemyX - gLerpPlayerX) * scale;
        int ex = midX + (int)relX;
        
        if (ex >= gMiniMapRect.x + 8 && ex <= gMiniMapRect.x + gMiniMapRect.w - 8)
        {
            SDL_Rect eDot = { ex - 4, floorY - 12, 8, 8 };
            SDL_Color enemyCol = { 255, 60, 60, 255 };
            uiFillRounded(r, eDot, 4, enemyCol);
        }
    }

    // 6. Optional: N/S indicator
    // Since it's a 2D side-scroller, we can just show "LEFT" and "RIGHT" or "W" and "E"
}
