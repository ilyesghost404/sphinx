#include "enemy.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ANIM_SPEED 0.1f // Animation frame duration in seconds

void initEnnemi(Ennemi *e, SDL_Renderer *renderer) {
    char path[256];
    // Skeleton 3_2 folder structure
    const char* states[] = {"idle", "walk", "run", "attack", "hurt", "die"};
    const char* prefix = "Skeleton_03__";
    const char* suffixes[] = {"IDLE", "WALK", "RUN", "ATTACK", "HURT", "DIE"};

    for (int s = 0; s < 6; s++) {
        e->frameCount[s] = 10;
        for (int f = 0; f < 10; f++) {
            sprintf(path, "assets/images/enemies/3_2/%s/%s%s_%03d.png", states[s], prefix, suffixes[s], f);
            e->animFrames[s][f] = IMG_LoadTexture(renderer, path);
            if (!e->animFrames[s][f]) {
                printf("Failed to load %s\n", path);
            }
        }
    }

    e->pos = (SDL_Rect){1000, 440, 280, 280}; // Reduced size slightly to 280x280
    e->targetHeight = 280.0f; // Scale to 280 pixels height
    e->currentFrame = 0;
    e->vitesse = 200.0f; // Pixels per second
    e->direction = 0; // 0 = Left, 1 = Right
    e->health = VIVANT;
    e->state = EN_ANIM_IDLE;
    e->animTimer = 0;
    e->level = 1;
    e->maxHealth = 100;
    e->currentHealth = 100; // 100 health for the Skeleton
    e->displayHealth = 100.0f;
    e->attackRange = 160; // Reasonable distance for a 280px sprite
    e->attackCooldown = 0.0f;
    e->hasDealtDamageInCurrentAttack = 0;
}

void renderEnemyHealthBar(Ennemi e, SDL_Renderer *renderer, float cameraX) {
    if (e.health == NEUTRALISE || e.state == EN_ANIM_DIE) return;

    int screenX = (int)(e.pos.x - cameraX);
    int screenY = (int)(e.pos.y);

    int barW = 100;
    int barH = 8;
    SDL_Rect bgRect = { screenX - barW / 2, screenY - 20, barW, barH };
    
    // Background (Black)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &bgRect);

    // Health (Red)
    float healthPct = e.displayHealth / (float)e.maxHealth;
    if (healthPct < 0) healthPct = 0;
    SDL_Rect healthRect = { bgRect.x + 1, bgRect.y + 1, (int)((barW - 2) * healthPct), barH - 2 };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &healthRect);
}

void afficherEnnemi(Ennemi e, SDL_Renderer *renderer, float cameraX) {
    if (e.health == NEUTRALISE) return;

    SDL_Texture* t = e.animFrames[e.state][e.currentFrame];
    if (!t) return;

    int w = 0, h = 0;
    SDL_QueryTexture(t, NULL, NULL, &w, &h);
    if (w <= 0 || h <= 0) return;

    float scale = e.targetHeight / (float)h;
    int dw = (int)(w * scale);
    int dh = (int)(h * scale);

    int screenX = (int)(e.pos.x - cameraX);
    int screenY = (int)(e.pos.y + e.pos.h - dh); // Align to ground

    SDL_Rect dst = { screenX - dw / 2, screenY, dw, dh };
    // Convention: 0 = Left (Flip), 1 = Right (None)
    SDL_RendererFlip flip = (e.direction == 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, t, NULL, &dst, 0.0, NULL, flip);

    // Render health bar above enemy
    renderEnemyHealthBar(e, renderer, cameraX);
}

void animerEnnemi(Ennemi *e) {
    float dt = 0.016f; // Assuming 60 FPS
    e->animTimer += dt;

    // Smooth health lerp
    if (e->displayHealth > (float)e->currentHealth) {
        e->displayHealth -= 60.0f * dt; // Increased lerp speed for larger health values
        if (e->displayHealth < (float)e->currentHealth) e->displayHealth = (float)e->currentHealth;
    }

    if (e->animTimer >= ANIM_SPEED) {
        e->animTimer = 0;
        e->currentFrame = (e->currentFrame + 1) % e->frameCount[e->state];

        // Handle end of one-shot animations
        if (e->state == EN_ANIM_HURT && e->currentFrame == e->frameCount[EN_ANIM_HURT] - 1) {
            e->state = EN_ANIM_IDLE;
        }
        if (e->state == EN_ANIM_ATTACK && e->currentFrame == e->frameCount[EN_ANIM_ATTACK] - 1) {
            e->state = EN_ANIM_IDLE;
        }
        if (e->state == EN_ANIM_DIE && e->currentFrame == e->frameCount[EN_ANIM_DIE] - 1) {
            e->health = NEUTRALISE;
        }
    }
}

void deplacementAleatoire(Ennemi *e, int level, float dt) {
    if (e->health != VIVANT || e->state == EN_ANIM_ATTACK) return;

    e->level = level;
    float currentSpeed = (80.0f + (float)level * 40.0f);

    static float moveTimer = 0;
    if (moveTimer <= 0) {
        e->direction = rand() % 2;
        moveTimer = 1.0f + (float)(rand() % 200) / 100.0f;
    }
    moveTimer -= dt;

    if (e->direction == 0) { // Left
        e->pos.x -= (int)(currentSpeed * dt);
        if (e->pos.x < 0) e->direction = 1;
    } else { // Right
        e->pos.x += (int)(currentSpeed * dt);
        if (e->pos.x > 1280 - e->pos.w) e->direction = 0;
    }

    e->state = EN_ANIM_WALK;
}

void gestionSanteEnnemi(Ennemi *e) {
    if (e->health == BLESSE) {
        if (e->state == EN_ANIM_DIE) return; // Death has priority

        ennemiTakeDamage(e, 1);
        e->health = VIVANT; // Reset state after taking damage
    }
}

void ennemiTakeDamage(Ennemi *e, int damage) {
    if (!e || e->health == NEUTRALISE || e->state == EN_ANIM_DIE) return;

    e->currentHealth -= damage;
    if (e->currentHealth < 0) e->currentHealth = 0;

    printf("[DEBUG] Enemy took %d damage. Current Health: %d/%d\n", damage, e->currentHealth, e->maxHealth);

    if (e->currentHealth == 0) {
        e->state = EN_ANIM_DIE;
        e->currentFrame = 0;
        e->animTimer = 0;
        printf("[DEBUG] Enemy DIED.\n");
    } else {
        e->state = EN_ANIM_HURT;
        e->currentFrame = 0;
        e->animTimer = 0;
        e->hasDealtDamageInCurrentAttack = 0; // Interrupt attack
    }
}

int collisionBB(SDL_Rect r1, SDL_Rect r2) {
    return SDL_HasIntersection(&r1, &r2);
}

void moveIA(Ennemi *e, SDL_Rect playerPos, float dt) {
    if (e->health == NEUTRALISE || e->state == EN_ANIM_DIE) return;

    // Reduce attack cooldown regardless of state (except death)
    if (e->attackCooldown > 0.0f) {
        e->attackCooldown -= dt;
    }

    if (e->state == EN_ANIM_HURT || e->state == EN_ANIM_ATTACK) return;

    int dx = e->pos.x - playerPos.x;
    int abs_dx = (dx < 0) ? -dx : dx;

    // Orientation towards player (if not attacking)
    if (e->state != EN_ANIM_ATTACK) {
        e->direction = (dx > 0) ? 0 : 1;
    }

    const int RUN_RANGE = 600; // Increased range

    if (abs_dx > RUN_RANGE) {
        // Just idle if player is far
        e->state = EN_ANIM_IDLE;
    } else if (abs_dx > e->attackRange) {
        // Move towards player
        if (e->direction == 0) e->pos.x -= (int)(e->vitesse * dt);
        else e->pos.x += (int)(e->vitesse * dt);
        e->state = EN_ANIM_RUN;
    } else {
        // Attack range
        if (e->state != EN_ANIM_ATTACK && e->attackCooldown <= 0.0f) {
            printf("[DEBUG] Enemy within attackRange (%d). Distance: %d. Triggering Attack.\n", e->attackRange, abs_dx);
            e->state = EN_ANIM_ATTACK;
            e->currentFrame = 0;
            e->animTimer = 0;
            e->hasDealtDamageInCurrentAttack = 0; // Reset damage flag
            e->attackCooldown = 2.0f; // Add cooldown for next attack
        } else if (e->state != EN_ANIM_ATTACK) {
            e->state = EN_ANIM_IDLE;
        }
    }
}
