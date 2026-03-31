#include "character.h"
#include "../common.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path)
{
    SDL_Surface* s = IMG_Load(path);
    if (!s)
    {
        printf("Failed to load character frame %s: %s\n", path, IMG_GetError());
        return NULL;
    }

    SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_FreeSurface(s);
    if (!t)
        printf("Failed to create texture for character frame %s: %s\n", path, SDL_GetError());
    return t;
}

static void destroyClip(CharacterClip* clip)
{
    if (!clip) return;
    if (clip->frames)
    {
        for (int i = 0; i < clip->frameCount; i++)
            if (clip->frames[i]) SDL_DestroyTexture(clip->frames[i]);
        free(clip->frames);
    }
    clip->frames = NULL;
    clip->frameCount = 0;
    clip->frameDuration = 0.0f;
    clip->loop = 1;
}

static int loadClip(CharacterClip* clip, SDL_Renderer* renderer, const char* pattern, int frameCount, float frameDuration, int loop)
{
    destroyClip(clip);
    clip->frames = (SDL_Texture**)calloc((size_t)frameCount, sizeof(SDL_Texture*));
    if (!clip->frames) return 0;

    clip->frameCount = frameCount;
    clip->frameDuration = frameDuration;
    clip->loop = loop;

    for (int i = 0; i < frameCount; i++)
    {
        char path[512];
        snprintf(path, sizeof(path), pattern, i);
        clip->frames[i] = loadTexture(renderer, path);
        if (!clip->frames[i]) return 0;
    }
    return 1;
}

static const CharacterClip* getClip(const Character* c, CharacterAnim anim)
{
    return &c->clips[(int)anim];
}

static int getFrameIndex(const Character* c)
{
    const CharacterClip* clip = getClip(c, c->anim);
    if (!clip->frames || clip->frameCount <= 0) return 0;

    int idx = (int)(c->animTime / clip->frameDuration);
    if (clip->loop)
    {
        idx %= clip->frameCount;
        if (idx < 0) idx += clip->frameCount;
        return idx;
    }

    if (idx >= clip->frameCount) idx = clip->frameCount - 1;
    if (idx < 0) idx = 0;
    return idx;
}

static void setAnim(Character* c, CharacterAnim anim, int restart)
{
    if (!c) return;
    if (!restart && c->anim == anim) return;
    c->anim = anim;
    c->animTime = 0.0f;
    c->animFinished = 0;
}

int characterInit(Character* c, SDL_Renderer* renderer, int skinIndex)
{
    if (!c || !renderer) return 0;
    memset(c, 0, sizeof(*c));

    if (skinIndex < 1) skinIndex = 1;
    if (skinIndex > 3) skinIndex = 1;

    char idlePattern[256];
    char walkPattern[256];
    char jumpPattern[256];
    char attackPattern[256];
    char hurtPattern[256];
    char diePattern[256];

    snprintf(idlePattern, sizeof(idlePattern),
             "assets/images/characters/main_character/%d/idle/Asassin_%02d__IDLE_%%03d.png",
             skinIndex, skinIndex);
    snprintf(walkPattern, sizeof(walkPattern),
             "assets/images/characters/main_character/%d/walk/Asassin_%02d__WALK_%%03d.png",
             skinIndex, skinIndex);
    snprintf(jumpPattern, sizeof(jumpPattern),
             "assets/images/characters/main_character/%d/jump/Asassin_%02d__JUMP_%%03d.png",
             skinIndex, skinIndex);
    snprintf(attackPattern, sizeof(attackPattern),
             "assets/images/characters/main_character/%d/attack/Asassin_%02d__ATTACK_01_%%03d.png",
             skinIndex, skinIndex);
    snprintf(hurtPattern, sizeof(hurtPattern),
             "assets/images/characters/main_character/%d/hurt/Asassin_%02d__HURT_%%03d.png",
             skinIndex, skinIndex);
    snprintf(diePattern, sizeof(diePattern),
             "assets/images/characters/main_character/%d/die/Asassin_%02d__DIE_%%03d.png",
             skinIndex, skinIndex);

    if (!loadClip(&c->clips[CHAR_ANIM_IDLE], renderer, idlePattern, 10, 0.10f, 1)) return 0;
    if (!loadClip(&c->clips[CHAR_ANIM_WALK], renderer, walkPattern, 10, 0.08f, 1)) return 0;
    if (!loadClip(&c->clips[CHAR_ANIM_JUMP], renderer, jumpPattern, 10, 0.09f, 1)) return 0;
    if (!loadClip(&c->clips[CHAR_ANIM_ATTACK], renderer, attackPattern, 10, 0.07f, 0)) return 0;
    if (!loadClip(&c->clips[CHAR_ANIM_HURT], renderer, hurtPattern, 10, 0.08f, 0)) return 0;
    if (!loadClip(&c->clips[CHAR_ANIM_DIE], renderer, diePattern, 10, 0.10f, 0)) return 0;

    c->x = 200.0f;
    c->groundY = (float)SCREEN_HEIGHT - 120.0f;
    c->y = c->groundY;
    c->vx = 0.0f;
    c->vy = 0.0f;
    c->facing = 1;
    c->onGround = 1;
    c->targetHeight = 240.0f;
    c->maxHealth = 100;
    c->currentHealth = 100;
    c->isDead = 0;
    c->hurtTimer = 0.0f;
    c->hasDealtDamageInCurrentAttack = 0;
    c->attackCooldown = 0.0f;
    c->attackRange = 180.0f;
    setAnim(c, CHAR_ANIM_IDLE, 1);

    return 1;
}

void characterDestroy(Character* c)
{
    if (!c) return;
    for (int i = 0; i < 6; i++)
        destroyClip(&c->clips[i]);
}

void characterSetMove(Character* c, int dir)
{
    if (!c) return;
    const float speed = 280.0f;
    if (dir < 0) { c->vx = -speed; c->facing = -1; }
    else if (dir > 0) { c->vx = speed; c->facing = 1; }
    else c->vx = 0.0f;
}

void characterJump(Character* c)
{
    if (!c) return;
    if (!c->onGround) return;
    c->onGround = 0;
    c->vy = -820.0f;
    setAnim(c, CHAR_ANIM_JUMP, 1);
}

void characterTakeDamage(Character* c, int damage)
{
    if (!c || c->isDead) return;

    c->currentHealth -= damage;
    if (c->currentHealth < 0) c->currentHealth = 0;

    printf("[DEBUG] Character took %d damage. Current Health: %d/%d\n", damage, c->currentHealth, c->maxHealth);

    if (c->currentHealth == 0)
    {
        characterDie(c);
    }
    else
    {
        setAnim(c, CHAR_ANIM_HURT, 1);
        c->hurtTimer = 0.5f; // Hurt animation duration
    }
}

void characterDie(Character* c)
{
    if (!c || c->isDead) return;
    c->isDead = 1;
    c->currentHealth = 0;
    setAnim(c, CHAR_ANIM_DIE, 1);
    printf("[DEBUG] Character DIED.\n");
}

void characterRevive(Character* c)
{
    if (!c) return;
    c->isDead = 0;
    if (c->maxHealth <= 0) c->maxHealth = 100;
    c->currentHealth = c->maxHealth;
    c->vx = 0.0f;
    c->vy = 0.0f;
    c->onGround = 1;
    c->y = c->groundY;
    c->hurtTimer = 0.0f;
    c->attackCooldown = 0.0f;
    c->hasDealtDamageInCurrentAttack = 0;
    setAnim(c, CHAR_ANIM_IDLE, 1);
}

void characterAttack(Character* c)
{
    if (!c || c->isDead || c->anim == CHAR_ANIM_HURT || c->attackCooldown > 0.0f) return;
    setAnim(c, CHAR_ANIM_ATTACK, 1);
    c->hasDealtDamageInCurrentAttack = 0;
    c->attackCooldown = 0.5f;
}

void characterUpdate(Character* c, float dt)
{
    if (!c) return;

    if (c->hurtTimer > 0.0f) {
        c->hurtTimer -= dt;
        if (c->hurtTimer < 0.0f) c->hurtTimer = 0.0f;
    }

    if (c->attackCooldown > 0.0f) {
        c->attackCooldown -= dt;
        if (c->attackCooldown < 0.0f) c->attackCooldown = 0.0f;
    }

    const float gravity = 2200.0f;
    if (!c->onGround)
        c->vy += gravity * dt;

    c->x += c->vx * dt;
    if (c->x < 0.0f) c->x = 0.0f;

    c->y += c->vy * dt;
    if (c->y >= c->groundY)
    {
        c->y = c->groundY;
        c->vy = 0.0f;
        c->onGround = 1;
    }

    const CharacterClip* clip = getClip(c, c->anim);
    c->animTime += dt;
    if (!clip->loop && !c->animFinished)
    {
        float total = clip->frameDuration * (float)clip->frameCount;
        if (c->animTime >= total)
        {
            c->animFinished = 1;
            c->animTime = total;
        }
    }

    if (c->anim == CHAR_ANIM_ATTACK || c->anim == CHAR_ANIM_HURT)
    {
        if (c->animFinished)
        {
            if (!c->onGround) setAnim(c, CHAR_ANIM_JUMP, 1);
            else if (fabsf(c->vx) > 1.0f) setAnim(c, CHAR_ANIM_WALK, 1);
            else setAnim(c, CHAR_ANIM_IDLE, 1);
        }
        return;
    }

    if (c->anim == CHAR_ANIM_DIE)
    {
        // Don't transition out of die animation
        return;
    }

    if (!c->onGround)
        setAnim(c, CHAR_ANIM_JUMP, 0);
    else if (fabsf(c->vx) > 1.0f)
        setAnim(c, CHAR_ANIM_WALK, 0);
    else
        setAnim(c, CHAR_ANIM_IDLE, 0);
}

void characterRender(const Character* c, SDL_Renderer* renderer, float cameraX, int isHurt)
{
    if (!c || !renderer) return;

    const CharacterClip* clip = getClip(c, c->anim);
    if (!clip->frames || clip->frameCount <= 0) return;

    int idx = getFrameIndex(c);
    SDL_Texture* t = clip->frames[idx];
    if (!t) return;

    // Blinking effect if hurt: tint first then blink
    if (isHurt)
    {
        SDL_SetTextureColorMod(t, 255, 100, 100);
        // Faster blinking
        if ((SDL_GetTicks() / 80) % 2 == 0)
        {
            return;
        }
    }
    else
    { 
        SDL_SetTextureColorMod(t, 255, 255, 255);
    }

    int w = 0, h = 0;
    SDL_QueryTexture(t, NULL, NULL, &w, &h);
    if (w <= 0 || h <= 0) return;

    float scale = c->targetHeight / (float)h;
    int dw = (int)(w * scale);
    int dh = (int)(h * scale);

    int screenX = (int)(c->x - cameraX);
    int screenY = (int)(c->y - (float)dh);

    SDL_Rect dst = { screenX - dw / 2, screenY, dw, dh };
    SDL_RendererFlip flip = (c->facing < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, t, NULL, &dst, 0.0, NULL, flip);
}
