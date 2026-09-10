#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <deque>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>

const int CELL = 50;
const int COLS = 15;
const int ROWS = 11;
const int W = CELL * COLS;
const int H = CELL * ROWS;

struct V { int x, y; bool operator==(const V& o) const { return x==o.x && y==o.y; } };

enum State { MENU, WARNING, PLAYING, GAMEOVER };

int main(int, char**) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    srand(time(nullptr));

    SDL_Window* win = SDL_CreateWindow("SNAKE UP",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_Texture* screamTex = IMG_LoadTexture(ren, "scream.png");
    Mix_Chunk* screamSnd = Mix_LoadWAV("scream.wav");
    if (!screamTex) SDL_Log("Нет scream.png");
    if (!screamSnd) SDL_Log("Нет scream.wav");

    State state = MENU;

    std::deque<V> snake;
    std::vector<V> walls;
    std::vector<V> foods;

    int score = 0;
    int dirX = 0, dirY = -1;
    int nextDirX = 0, nextDirY = -1;

    Uint32 lastMove = SDL_GetTicks();
    const Uint32 MOVE_DELAY = 200;
    const Uint32 WALL_SPAWN_MS = 400;
    Uint32 lastWallSpawn = SDL_GetTicks();

    auto isOccupied = [&](int x, int y) {
        for (auto& s : snake) if (s.x == x && s.y == y) return true;
        for (auto& w : walls) if (w.x == x && w.y == y) return true;
        for (auto& f : foods) if (f.x == x && f.y == y) return true;
        return false;
    };

    auto spawnFood = [&]() {
        for (int tries = 0; tries < 60; tries++) {
            int x = rand() % COLS;
            int y = rand() % (ROWS / 2);
            if (isOccupied(x, y)) continue;
            foods.push_back({x, y});
            return;
        }
    };

    auto buildWorld = [&]() {
        snake.clear();
        walls.clear();
        foods.clear();

        snake.push_back({COLS/2, ROWS - 2});
        snake.push_back({COLS/2, ROWS - 1});

        int headY = ROWS - 2;
        int headX = COLS/2;

        for (int y = 0; y < ROWS - 3; y++) {
            int count = rand() % 3;
            for (int i = 0; i < count; i++) {
                for (int tries = 0; tries < 40; tries++) {
                    int x = rand() % COLS;
                    if (isOccupied(x, y)) continue;
                    if (y >= headY - 3 && x == headX) continue;
                    walls.push_back({x, y});
                    break;
                }
            }
        }

        for (int i = 0; i < 4; i++) spawnFood();

        score = 0;
        dirX = 0; dirY = -1;
        nextDirX = 0; nextDirY = -1;
        lastMove = SDL_GetTicks();
        lastWallSpawn = SDL_GetTicks();
    };

    buildWorld();

    auto pressScream = [&](Uint32 ms) {
        if (screamSnd) Mix_PlayChannel(-1, screamSnd, 0);
        if (screamTex) {
            SDL_Rect dst = {0,0,W,H};
            SDL_RenderCopy(ren, screamTex, nullptr, &dst);
            SDL_RenderPresent(ren);
        }
        SDL_Delay(ms);
    };

    SDL_Rect startBtn = { W/2 - 120, H/2 - 30, 240, 60 };
    SDL_Rect warnBtn  = { W/2 - 120, H/2 + 20, 240, 60 };

    auto mouseInRect = [](int mx, int my, SDL_Rect r) {
        return mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h;
    };

    auto drawText = [&](const std::string& text, int cx, int cy, int scale, SDL_Color col) {
        static const std::vector<std::vector<std::string>> font = {
            {"010","101","111","101","101"},
            {"110","101","110","101","110"},
            {"011","100","100","100","011"},
            {"110","101","101","101","110"},
            {"111","100","110","100","111"},
            {"111","100","110","100","100"},
            {"011","100","101","101","011"},
            {"101","101","111","101","101"},
            {"111","010","010","010","111"},
            {"001","001","001","101","010"},
            {"101","101","110","101","101"},
            {"100","100","100","100","111"},
            {"101","111","111","101","101"},
            {"101","111","111","111","101"},
            {"010","101","101","101","010"},
            {"110","101","110","100","100"},
            {"010","101","101","111","011"},
            {"110","101","110","101","101"},
            {"011","100","010","001","110"},
            {"111","010","010","010","010"},
            {"101","101","101","101","111"},
            {"101","101","101","101","010"},
            {"101","101","111","111","101"},
            {"101","101","010","101","101"},
            {"101","101","010","010","010"},
            {"111","001","010","100","111"},
            {"000","000","000","000","000"},
            {"010","010","010","000","010"},
            {"000","000","000","000","010"},
            {"000","000","111","000","000"},
        };

        auto idxOf = [&](char c) -> int {
            if (c >= 'A' && c <= 'Z') return c - 'A';
            if (c >= 'a' && c <= 'z') return c - 'a';
            if (c == ' ') return 26;
            if (c == '!') return 27;
            if (c == '.') return 28;
            if (c == '-') return 29;
            return 26;
        };

        int cw = 3 * scale + scale;
        int totalW = (int)text.size() * cw - scale;
        int startX = cx - totalW / 2;
        int startY = cy - (5 * scale) / 2;

        SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a);
        for (size_t i = 0; i < text.size(); i++) {
            int f = idxOf(text[i]);
            const auto& glyph = font[f];
            for (int gy = 0; gy < 5; gy++) {
                for (int gx = 0; gx < 3; gx++) {
                    if (glyph[gy][gx] == '1') {
                        SDL_Rect r = { startX + (int)i * cw + gx * scale,
                                       startY + gy * scale, scale, scale };
                        SDL_RenderFillRect(ren, &r);
                    }
                }
            }
        }
    };

    bool running = true;
    SDL_Event e;

    while (running) {
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;

            if (e.type == SDL_KEYDOWN) {
                auto k = e.key.keysym.sym;
                if (k == SDLK_ESCAPE) running = false;

                if (state == MENU) {
                    if (k == SDLK_RETURN || k == SDLK_SPACE) state = WARNING;
                } else if (state == WARNING) {
                    if (k == SDLK_RETURN || k == SDLK_SPACE) {
                        buildWorld();
                        state = PLAYING;
                    }
                } else if (state == PLAYING) {
                    // управление змейкой
                    if (k == SDLK_UP    || k == SDLK_w) { if (dirY != 1)  { nextDirX = 0;  nextDirY = -1; } }
                    if (k == SDLK_DOWN  || k == SDLK_s) { if (dirY != -1) { nextDirX = 0;  nextDirY = 1; } }
                    if (k == SDLK_LEFT  || k == SDLK_a) { if (dirX != 1)  { nextDirX = -1; nextDirY = 0; } }
                    if (k == SDLK_RIGHT || k == SDLK_d) { if (dirX != -1) { nextDirX = 1;  nextDirY = 0; } }
                } else if (state == GAMEOVER) {
                    if (k == SDLK_r) { buildWorld(); state = PLAYING; }
                    if (k == SDLK_m) state = MENU;
                }
            }

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                if (state == MENU) {
                    if (mouseInRect(mx, my, startBtn)) state = WARNING;
                } else if (state == WARNING) {
                    if (mouseInRect(mx, my, warnBtn)) {
                        buildWorld();
                        state = PLAYING;
                    }
                }
            }
        }

        Uint32 now = SDL_GetTicks();

        if (state == PLAYING) {
            if (now - lastWallSpawn >= WALL_SPAWN_MS) {
                lastWallSpawn = now;
                for (int tries = 0; tries < 60; tries++) {
                    int x = rand() % COLS;
                    if (isOccupied(x, 0)) continue;
                    if (x == snake.front().x && 0 >= snake.front().y - 3) continue;
                    walls.push_back({x, 0});
                    break;
                }
            }

            if (now - lastMove >= MOVE_DELAY) {
                lastMove = now;
                dirX = nextDirX;
                dirY = nextDirY;

                V head = snake.front();
                head.x += dirX;
                head.y += dirY;

                bool dead = false;
                if (head.x < 0 || head.x >= COLS) dead = true;
                if (!dead) for (auto& w : walls) if (w == head) { dead = true; break; }
                if (!dead) for (size_t i = 0; i < snake.size(); i++)
                    if (snake[i] == head) { dead = true; break; }

                if (dead) {
                    state = GAMEOVER;
                    pressScream(900);
                } else {
                    snake.push_front(head);

                    bool ate = false;
                    for (size_t i = 0; i < foods.size(); i++) {
                        if (foods[i] == head) {
                            foods.erase(foods.begin() + i);
                            ate = true; score++; break;
                        }
                    }
                    if (!ate) snake.pop_back();

                    if (snake.front().y < ROWS / 3) {
                        for (auto& s : snake) s.y++;
                        for (auto& w : walls) w.y++;
                        for (auto& f : foods) f.y++;
                        std::vector<V> nw;
                        for (auto& w : walls) if (w.y < ROWS) nw.push_back(w);
                        walls = nw;
                        std::vector<V> nf;
                        for (auto& f : foods) if (f.y < ROWS) nf.push_back(f);
                        foods = nf;
                    }

                    while (foods.size() < 4) spawnFood();
                }
            }
        }

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        if (state == MENU) {
            drawText("SNAKE", W/2, 120, 8, {140, 255, 140, 255});
            drawText("UP", W/2, 220, 8, {140, 255, 140, 255});

            bool hov = mouseInRect(mx, my, startBtn);
            SDL_SetRenderDrawColor(ren, hov ? 80 : 40, hov ? 180 : 100, hov ? 100 : 60, 255);
            SDL_RenderFillRect(ren, &startBtn);
            SDL_SetRenderDrawColor(ren, 140, 255, 140, 255);
            SDL_RenderDrawRect(ren, &startBtn);
            drawText("START", startBtn.x + startBtn.w/2, startBtn.y + startBtn.h/2, 5,
                     {255, 255, 255, 255});

            drawText("ENTER OR CLICK", W/2, H - 60, 3, {120, 120, 120, 255});
        }
        else if (state == WARNING) {
            SDL_SetRenderDrawColor(ren, 255, 200, 0, 255);
            SDL_Rect frame = {20, 20, W - 40, H - 40};
            SDL_RenderDrawRect(ren, &frame);
            SDL_Rect frame2 = {22, 22, W - 44, H - 44};
            SDL_RenderDrawRect(ren, &frame2);

            drawText("WARNING", W/2, 90, 7, {255, 200, 0, 255});
            drawText("THIS GAME", W/2, 180, 4, {255, 255, 255, 255});
            drawText("CONTAINS", W/2, 220, 4, {255, 255, 255, 255});
            drawText("SCREAMERS", W/2, 260, 4, {255, 80, 80, 255});
            drawText("AND LOUD SOUND", W/2, 320, 3, {200, 200, 200, 255});
            drawText("IF YOU ARE SENSITIVE", W/2, 360, 3, {200, 200, 200, 255});
            drawText("DO NOT PLAY", W/2, 400, 3, {200, 200, 200, 255});

            bool hov = mouseInRect(mx, my, warnBtn);
            SDL_SetRenderDrawColor(ren, hov ? 80 : 40, hov ? 180 : 100, hov ? 100 : 60, 255);
            SDL_RenderFillRect(ren, &warnBtn);
            SDL_SetRenderDrawColor(ren, 140, 255, 140, 255);
            SDL_RenderDrawRect(ren, &warnBtn);
            drawText("I UNDERSTAND", warnBtn.x + warnBtn.w/2, warnBtn.y + warnBtn.h/2, 3,
                     {255, 255, 255, 255});
        }
        else if (state == PLAYING || state == GAMEOVER) {
            for (auto& w : walls) {
                SDL_Rect r = {w.x*CELL, w.y*CELL, CELL, CELL};
                SDL_SetRenderDrawColor(ren, 30, 60, 200, 255);
                SDL_RenderFillRect(ren, &r);
                SDL_SetRenderDrawColor(ren, 80, 130, 255, 255);
                SDL_RenderDrawRect(ren, &r);
            }
            for (auto& f : foods) {
                SDL_Rect fr = {f.x*CELL + 8, f.y*CELL + 8, CELL - 16, CELL - 16};
                SDL_SetRenderDrawColor(ren, 255, 220, 40, 255);
                SDL_RenderFillRect(ren, &fr);
            }
            for (size_t i = 0; i < snake.size(); i++) {
                SDL_Rect r = {snake[i].x*CELL + 3, snake[i].y*CELL + 3, CELL - 6, CELL - 6};
                if (i == 0) SDL_SetRenderDrawColor(ren, 140, 255, 140, 255);
                else        SDL_SetRenderDrawColor(ren, 60, 190, 80, 255);
                SDL_RenderFillRect(ren, &r);
            }

            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            for (int i = 0; i < score && i < 40; i++) {
                SDL_Rect d = {10 + i*14, 4, 10, 10};
                SDL_RenderFillRect(ren, &d);
            }

            if (state == GAMEOVER) {
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
                SDL_Rect full = {0, 0, W, H};
                SDL_RenderFillRect(ren, &full);
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);

                drawText("GAME OVER", W/2, H/2 - 60, 6, {255, 80, 80, 255});
                drawText("PRESS R TO RESTART", W/2, H/2 + 40, 3, {255, 255, 255, 255});
                drawText("PRESS M FOR MENU", W/2, H/2 + 80, 3, {200, 200, 200, 255});
            }
        }

        SDL_RenderPresent(ren);
    }

    if (screamTex) SDL_DestroyTexture(screamTex);
    if (screamSnd) Mix_FreeChunk(screamSnd);
    Mix_CloseAudio();
    IMG_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
