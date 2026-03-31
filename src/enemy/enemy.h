#ifndef ENEMY_H
#define ENEMY_H

#include <SDL2/SDL.h>

// Health states for enemy
typedef enum {
    VIVANT,
    BLESSE,
    NEUTRALISE
} EnemyHealthState;

// Animation states for enemy (Simplified for 3_2 Skeleton)
typedef enum {
    EN_ANIM_IDLE,
    EN_ANIM_WALK,
    EN_ANIM_RUN,
    EN_ANIM_ATTACK,
    EN_ANIM_HURT,
    EN_ANIM_DIE
} EnemyAnimState;

// Enemy Structure (3_2 Skeleton)
typedef struct {
    SDL_Texture* animFrames[6][10]; // 6 states, 10 frames each
    int frameCount[6];
    int currentFrame;
    SDL_Rect pos;
    float vitesse;
    int direction; // 0 for left, 1 for right
    EnemyHealthState health;
    EnemyAnimState state;
    float animTimer;
    int maxHealth;
    int currentHealth;
    float displayHealth; // For smooth lerp
    float targetHeight;
    int level;
    int attackRange;
    float attackCooldown;
    int hasDealtDamageInCurrentAttack;
} Ennemi;

// Functions for Enemy management
void initEnnemi(Ennemi *e, SDL_Renderer *renderer);
void afficherEnnemi(Ennemi e, SDL_Renderer *renderer, float cameraX);
void animerEnnemi(Ennemi *e);
void deplacementAleatoire(Ennemi *e, int level, float dt);
void gestionSanteEnnemi(Ennemi *e);
void ennemiTakeDamage(Ennemi *e, int damage);
int collisionBB(SDL_Rect r1, SDL_Rect r2);

// IA Move
void moveIA(Ennemi *e, SDL_Rect playerPos, float dt);

#endif
