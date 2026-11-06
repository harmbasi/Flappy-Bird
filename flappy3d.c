#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define SCREEN_W 1024
#define SCREEN_H 768
#define GRAVITY -0.6f
#define JUMP_FORCE 8.0f
#define PIPE_SPEED 0.4f
#define MAX_PIPES 3

typedef struct { float x, y, z; } Vec3;
typedef struct { int x, y; float scale; } Vec2;

float cam_z = -15.0f;
float focal_length = 400.0f;

// 3x5 Pixel Font for Score
const int font[10][15] = {
    {1,1,1, 1,0,1, 1,0,1, 1,0,1, 1,1,1}, {0,1,0, 1,1,0, 0,1,0, 0,1,0, 1,1,1},
    {1,1,1, 0,0,1, 1,1,1, 1,0,0, 1,1,1}, {1,1,1, 0,0,1, 1,1,1, 0,0,1, 1,1,1},
    {1,0,1, 1,0,1, 1,1,1, 0,0,1, 0,0,1}, {1,1,1, 1,0,0, 1,1,1, 0,0,1, 1,1,1},
    {1,1,1, 1,0,0, 1,1,1, 1,0,1, 1,1,1}, {1,1,1, 0,0,1, 0,1,0, 1,0,0, 1,0,0},
    {1,1,1, 1,0,1, 1,1,1, 1,0,1, 1,1,1}, {1,1,1, 1,0,1, 1,1,1, 0,0,1, 1,1,1}
};

Vec2 project(Vec3 p) {
    Vec2 res = {-1000, -1000, 0};
    if (p.z <= cam_z) return res;
    res.scale = focal_length / (p.z - cam_z);
    res.x = (int)(p.x * res.scale + SCREEN_W / 2.0f);
    res.y = (int)(-p.y * res.scale + SCREEN_H / 2.0f);
    return res;
}

void draw_box(SDL_Renderer* r, float cx, float cy, float cz, float w, float h, float d, Uint8 red, Uint8 green, Uint8 blue) {
    Vec3 p3d = {cx, cy, cz};
    Vec2 p = project(p3d);
    if (p.scale > 0) {
        int rw = (int)(w * p.scale);
        int rh = (int)(h * p.scale);
        SDL_Rect rect = {p.x - rw/2, p.y - rh/2, rw, rh};
        
        // Draw Face
        SDL_SetRenderDrawColor(r, red, green, blue, 255);
        SDL_RenderFillRect(r, &rect);
        // Draw Outline
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderDrawRect(r, &rect);
    }
}

void draw_score(SDL_Renderer* r, int score, int x, int y, int size) {
    char buf[10];
    sprintf(buf, "%d", score);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    for (int c = 0; buf[c]; c++) {
        int n = buf[c] - '0';
        for (int i = 0; i < 15; i++) {
            if (font[n][i]) {
                SDL_Rect rect = {x + (c * 4 + (i % 3)) * size, y + (i / 3) * size, size, size};
                SDL_RenderFillRect(r, &rect);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    SDL_Window* window = SDL_CreateWindow("Flappy3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (SDL_NumJoysticks() > 0) SDL_JoystickOpen(0);
    srand(time(NULL));

    bool running = true, game_over = false;
    float bird_y = 5.0f, bird_v = 0.0f;
    int score = 0;
    
    struct { float x, gap_y; bool passed; } pipes[MAX_PIPES];
    for (int i = 0; i < MAX_PIPES; i++) {
        pipes[i].x = 20.0f + (i * 15.0f);
        pipes[i].gap_y = (rand() % 10) - 2.0f;
        pipes[i].passed = false;
    }

    while (running) {
        SDL_Event event;
        bool jump = false;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (event.key.keysym.sym == SDLK_SPACE || event.key.keysym.sym == SDLK_RETURN) jump = true;
            }
            if (event.type == SDL_JOYBUTTONDOWN) {
                if (event.jbutton.button == 0 || event.jbutton.button == 1) jump = true; // A/B
                if (event.jbutton.button == 11 || event.jbutton.button == 7) running = false; // Menu/Start
            }
        }

        if (jump) {
            if (game_over) {
                bird_y = 5.0f; bird_v = 0.0f; score = 0; game_over = false;
                for (int i = 0; i < MAX_PIPES; i++) {
                    pipes[i].x = 20.0f + (i * 15.0f);
                    pipes[i].gap_y = (rand() % 10) - 2.0f;
                    pipes[i].passed = false;
                }
            } else {
                bird_v = JUMP_FORCE;
            }
        }

        if (!game_over) {
            bird_v += GRAVITY;
            bird_y += bird_v * 0.05f;

            for (int i = 0; i < MAX_PIPES; i++) {
                pipes[i].x -= PIPE_SPEED;
                
                if (!pipes[i].passed && pipes[i].x < 0.0f) {
                    score++;
                    pipes[i].passed = true;
                }
                
                if (pipes[i].x < -10.0f) {
                    pipes[i].x = pipes[(i + MAX_PIPES - 1) % MAX_PIPES].x + 15.0f;
                    pipes[i].gap_y = (rand() % 10) - 2.0f;
                    pipes[i].passed = false;
                }

                // Collision
                if (pipes[i].x > -2.0f && pipes[i].x < 2.0f) {
                    if (bird_y > pipes[i].gap_y + 3.0f || bird_y < pipes[i].gap_y - 3.0f) game_over = true;
                }
            }
            if (bird_y < -10.0f || bird_y > 20.0f) game_over = true;
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255); // Sky blue
        SDL_RenderClear(renderer);

        // Draw Ground
        draw_box(renderer, 0, -12, 0, 50, 2, 50, 139, 69, 19);

        // Draw Pipes (Back to front)
        for (int i = MAX_PIPES - 1; i >= 0; i--) {
            float px = pipes[i].x;
            if (px > 50.0f) continue;
            // Bottom pipe
            draw_box(renderer, px, pipes[i].gap_y - 12.0f, 0, 4.0f, 18.0f, 4.0f, 50, 205, 50);
            // Top pipe
            draw_box(renderer, px, pipes[i].gap_y + 12.0f, 0, 4.0f, 18.0f, 4.0f, 50, 205, 50);
        }

        // Draw Bird
        draw_box(renderer, 0.0f, bird_y, 0.0f, 1.5f, 1.5f, 1.5f, 255, 69, 0);

        // Draw Score
        draw_score(renderer, score, 30, 30, 8);

        if (game_over) draw_score(renderer, score, SCREEN_W/2 - 20, SCREEN_H/2 - 20, 10);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
