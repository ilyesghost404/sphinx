#ifndef CHARACTER_H
#define CHARACTER_H

#include <SDL2/SDL.h>

typedef enum {
    CHAR_ANIM_IDLE = 0,
    CHAR_ANIM_WALK = 1,
    CHAR_ANIM_JUMP = 2,
    CHAR_ANIM_ATTACK = 3,
    CHAR_ANIM_HURT = 4,
    CHAR_ANIM_DIE = 5
} CharacterAnim;

typedef struct {
    SDL_Texture** frames;
    int frameCount;
    float frameDuration;
    int loop;
} CharacterClip;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    int facing;
    int onGround;
    float groundY;

    CharacterAnim anim;
    float animTime;
    int animFinished;

    float targetHeight;

    int maxHealth;
    int currentHealth;
    int isDead;
    float hurtTimer;
    int hasDealtDamageInCurrentAttack;
    float attackCooldown;
    float attackRange;

    CharacterClip clips[6];
} Character;

int characterInit(Character* c, SDL_Renderer* renderer, int skinIndex);
void characterDestroy(Character* c);

void characterSetMove(Character* c, int dir);
void characterJump(Character* c);
void characterAttack(Character* c);
void characterTakeDamage(Character* c, int damage);
void characterDie(Character* c);
void characterRevive(Character* c);

void characterUpdate(Character* c, float dt);
void characterRender(const Character* c, SDL_Renderer* renderer, float cameraX, int isHurt);

#endif
