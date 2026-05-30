#include <SDL.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>

#define NUM 6
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define FISHTANK_WIDTH 100
#define FISHTANK_HEIGHT 200

// [필수 1] 물고기 종류 정의 (열거형)
typedef enum {
    NORMAL_FISH, // 기본 물고기
    FAST_FISH,   // 빠르게 자라는 물고기
    BIG_FISH     // 물을 많이 소비하는 물고기
} FishType;

// 게임 상태 구조체 정의
typedef struct {
    float fish;       // 물고기 크기/성장도 (부드러운 연산을 위해 float 유지)
    float water;      // 어항 물 높이 (float 유지)
    int isAlive;      // 1: 살아있음, 0: 죽음
    FishType type;    // [필수 1] 물고기 종류를 저장할 변수 추가
} FishTank;

FishTank fishTanks[NUM];    // 물고기 어항 배열
int level = 1;
int position = 0;
bool running = true;
bool gameOver = false;
bool gameWin = false;

unsigned int startTime = 0;
unsigned int lastUpdateTime = 0;

SDL_Window* window = NULL;          // SDL 창
SDL_Renderer* renderer = NULL;      // SDL 렌더러
TTF_Font* font = NULL;              // 폰트
SDL_Texture* fishTexture = NULL;    // 물고기 소스 텍스처

// 함수 프로토타입
bool engine_init();
void initGame();
void renderText(const char* text, int x, int y, SDL_Color color);
void renderFishTanks();
void updateGame();
void renderGame();
void cleanupGame();
void handleInput(SDL_Event* e);
SDL_Texture* loadTexture(const char* path);

int main(int argc, char* argv[]) {
    if (!engine_init()) {
        printf("엔진 초기화 실패: %s\n", SDL_GetError());
        return 1;
    }

    initGame();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
            handleInput(&event);
        }

        updateGame();
        renderGame();
        SDL_Delay(16); // 약 60 FPS 유지를 위해 16ms 대기
    }

    cleanupGame();
    return 0;
}

bool engine_init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;

    window = SDL_CreateWindow("Raising Fishes - SDL2 GUI (Custom Types)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) return false;

    // 수직 동기화(VSYNC) 적용 렌더러 생성
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return false;

    if (TTF_Init() != 0) return false;

    // Windows 기본 Arial 폰트 경로 사용 (텍스트 표시를 위해 크기를 16으로 조정)
    font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 16);
    if (!font) {
        printf("폰트 로드 실패! 기본 경로를 확인하세요.\n");
        return false;
    }

    // 비트맵 이미지 로드
    fishTexture = loadTexture("fish.bmp");
    if (!fishTexture) {
        printf("경고: fish.bmp를 찾을 수 없습니다. 임시 사각형 렌더링으로 대체합니다.\n");
    }

    return true;
}

// [필수 1] 각 어항에 서로 다른 물고기 종류 지정
void initGame() {
    for (int i = 0; i < NUM; i++) {
        fishTanks[i].fish = 10.0f;
        fishTanks[i].water = 100.0f;
        fishTanks[i].isAlive = 1;

        // 규칙적으로 종류 지정 (0,3: Normal / 1,4: Fast / 2,5: Big)
        if (i % 3 == 0) {
            fishTanks[i].type = NORMAL_FISH;
        }
        else if (i % 3 == 1) {
            fishTanks[i].type = FAST_FISH;
        }
        else {
            fishTanks[i].type = BIG_FISH;
        }
    }
    startTime = SDL_GetTicks();
    lastUpdateTime = startTime;
}

void renderGame() {
    // 배경: 짙은 네이비 색상
    SDL_SetRenderDrawColor(renderer, 10, 20, 40, 255);
    SDL_RenderClear(renderer);

    // 어항 및 물고기 렌더링
    renderFishTanks();

    // 상단 UI 정보 출력
    SDL_Color whiteText = { 255, 255, 255, 255 };
    char infoText[128];
    sprintf_s(infoText, sizeof(infoText), "LEVEL: %d   |   Time: %d sec", level, (SDL_GetTicks() - startTime) / 1000);
    renderText(infoText, 20, 20, whiteText);

    char helpText[] = "Controls: Move Left(J) / Move Right(L) / Give Water(K) / Exit(ESC)";
    renderText(helpText, 20, 45, whiteText);

    SDL_RenderPresent(renderer);
}

void renderFishTanks() {
    for (int i = 0; i < NUM; i++) {
        // 각 어항 배치 좌표 계산 (하단 텍스트 공간 확보를 위해 y축을 250에서 230으로 조절)
        int x = 60 + i * (FISHTANK_WIDTH + 20);
        int y = 230;

        SDL_Rect bowl = { x, y, FISHTANK_WIDTH, FISHTANK_HEIGHT };

        // 1. 기본 어항 파란색 테두리 그리기
        SDL_SetRenderDrawColor(renderer, 0, 100, 255, 255);
        SDL_RenderDrawRect(renderer, &bowl);

        // 2. 어항 내부 채워진 물 그리기
        int waterHeight = (int)(fishTanks[i].water * FISHTANK_HEIGHT / 100.0f);
        SDL_Rect waterRect = { x + 2, y + FISHTANK_HEIGHT - waterHeight, FISHTANK_WIDTH - 4, waterHeight };

        // 물 수위에 따라 경고색 부여
        if (fishTanks[i].water < 30) {
            SDL_SetRenderDrawColor(renderer, 200, 50, 50, 200); // 경고: 빨간 물
        }
        else {
            SDL_SetRenderDrawColor(renderer, 0, 120, 220, 150); // 일반: 파란 물
        }
        SDL_RenderFillRect(renderer, &waterRect);

        // 3. 현재 커서 노란색 강조 테두리
        if (i == position) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDrawRect(renderer, &bowl);
            SDL_Rect innerBowl = { bowl.x + 1, bowl.y + 1, bowl.w - 2, bowl.h - 2 };
            SDL_RenderDrawRect(renderer, &innerBowl);
        }

        // 4. 상태 및 종류별 물고기 이미지 변경 렌더링
        if (fishTanks[i].isAlive) {
            int fishSizeW = 40;
            int fishSizeH = 20;

            // 종류별로 기본 고유 색상을 다르게 지정하여 직관성 부여 (선택적 시각 효과)
            if (fishTanks[i].fish >= 100.0f) {
                SDL_SetTextureColorMod(fishTexture, 255, 215, 0); // 만렙: 황금색
                fishSizeW = 70; fishSizeH = 35;
            }
            else {
                if (fishTanks[i].type == NORMAL_FISH) SDL_SetTextureColorMod(fishTexture, 255, 255, 255); // 기본: 흰색
                else if (fishTanks[i].type == FAST_FISH) SDL_SetTextureColorMod(fishTexture, 120, 255, 120); // Fast: 연두색
                else if (fishTanks[i].type == BIG_FISH) SDL_SetTextureColorMod(fishTexture, 120, 200, 255); // Big: 하늘색

                if (fishTanks[i].fish > 40.0f) {
                    fishSizeW = 60; fishSizeH = 30; // 어른 크기
                }
                else {
                    fishSizeW = 40; fishSizeH = 20; // 아기 크기
                }
            }

            int fishX = x + (FISHTANK_WIDTH - fishSizeW) / 2;
            int fishY = y + FISHTANK_HEIGHT - (waterHeight / 2) - (fishSizeH / 2);

            if (fishY > y + FISHTANK_HEIGHT - fishSizeH - 5) {
                fishY = y + FISHTANK_HEIGHT - fishSizeH - 5;
            }

            SDL_Rect fishRect = { fishX, fishY, fishSizeW, fishSizeH };

            if (fishTexture) {
                SDL_RenderCopy(renderer, fishTexture, NULL, &fishRect);
            }
            else {
                SDL_RenderFillRect(renderer, &fishRect);
            }
        }
        else {
            // 죽은 물고기 잿빛 및 뒤집기 표현
            SDL_SetTextureColorMod(fishTexture, 100, 100, 100);
            int fishX = x + (FISHTANK_WIDTH - 50) / 2;
            int fishY = y + FISHTANK_HEIGHT - 35;
            SDL_Rect fishRect = { fishX, fishY, 50, 25 };

            if (fishTexture) {
                SDL_RenderCopyEx(renderer, fishTexture, NULL, &fishRect, 0.0, NULL, SDL_FLIP_VERTICAL);
            }
        }

        // 5. [필수 1] 화면에 물고기 종류 문자열 및 상태 출력 정보 렌더링
        char typeStr[16] = "";
        if (fishTanks[i].type == NORMAL_FISH) sprintf_s(typeStr, sizeof(typeStr), "[Normal]");
        else if (fishTanks[i].type == FAST_FISH) sprintf_s(typeStr, sizeof(typeStr), "[Fast]");
        else if (fishTanks[i].type == BIG_FISH) sprintf_s(typeStr, sizeof(typeStr), "[Big]");

        char status[64];
        SDL_Color textColor = { 255, 255, 255, 255 };

        // 어항 바로 밑에 종류 이름 먼저 출력
        renderText(typeStr, x + 15, y + FISHTANK_HEIGHT + 10, textColor);

        // 종류 출력 아랫줄에 수치 정보 출력
        if (fishTanks[i].isAlive) {
            sprintf_s(status, sizeof(status), "F:%d W:%d", (int)fishTanks[i].fish, (int)fishTanks[i].water);
        }
        else {
            sprintf_s(status, sizeof(status), "DEAD");
            textColor.r = 255; textColor.g = 50; textColor.b = 50;
        }
        renderText(status, x + 12, y + FISHTANK_HEIGHT + 30, textColor);
    }
}

// [필수 2] 물고기 종류에 따른 물 소비 속도 및 성장 메커니즘 차등화
void updateGame() {
    unsigned int currentTime = SDL_GetTicks();
    float dt = (currentTime - lastUpdateTime) / 1000.0f;
    lastUpdateTime = currentTime;

    if (gameOver || gameWin) return;

    int aliveCount = 0;
    for (int i = 0; i < NUM; i++) {
        if (fishTanks[i].isAlive == 1) {

            // --- [필수 2] 종류에 따른 가중치 계수 설정 ---
            float waterConsumption = 1.0f;  // 기본 물 소비량 배율
            float growthSpeed = 1.0f;       // 기본 성장 속도 배율

            if (fishTanks[i].type == FAST_FISH) {
                growthSpeed = 2.0f;         // Fast Fish는 자라는 속도가 2배!
            }
            else if (fishTanks[i].type == BIG_FISH) {
                waterConsumption = 2.0f;    // Big Fish는 물을 마시는 속도가 2배!
            }

            // 1. 물 소비 연산 (물고기 크기 공식에 소비 배율 적용)
            float waterDecreaseRate = level * (fishTanks[i].fish / 20.0f + 1.0f) * waterConsumption;
            fishTanks[i].water -= (waterDecreaseRate * dt * 2.2f); // 자연스러운 밸런스를 위한 고정 배율 조정

            if (fishTanks[i].water <= 0.0f) {
                fishTanks[i].water = 0.0f;
                fishTanks[i].isAlive = 0;
            }

            // 2. 성장 연산 (성장 배율 적용)
            if (fishTanks[i].water > 0.0f) {
                float growthRate = (fishTanks[i].water / 100.0f + 0.5f) * growthSpeed;
                fishTanks[i].fish += (growthRate * dt * 3.5f);
            }
            if (fishTanks[i].fish > 100.0f) fishTanks[i].fish = 100.0f;

            aliveCount++;
        }
    }

    if (aliveCount == 0) {
        gameOver = true;
        running = false;
    }

    // 3. 레벨업 시스템 (시간 경과 기준)
    unsigned int totalElapsed = (currentTime - startTime) / 1000;
    if (totalElapsed / 20 > (unsigned int)(level - 1)) {
        level++;
        if (level > 5) {
            level = 5;
            gameWin = true;
            running = false;
        }
    }
}

void handleInput(SDL_Event* e) {
    if (e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.sym) {
        case SDLK_j:
            if (position > 0) position--;
            break;
        case SDLK_l:
            if (position < NUM - 1) position++;
            break;
        case SDLK_k:
            if (fishTanks[position].isAlive && fishTanks[position].water < 100.0f) {
                fishTanks[position].water += 20.0f; // 밸런스를 위해 충전량 소폭 상향
                if (fishTanks[position].water > 100.0f) fishTanks[position].water = 100.0f;
            }
            break;
        case SDLK_ESCAPE:
            running = false;
            break;
        }
    }
}

void renderText(const char* text, int x, int y, SDL_Color color) {
    if (!font) return;
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_Rect dest = { x, y, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, NULL, &dest);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

SDL_Texture* loadTexture(const char* path) {
    SDL_Surface* surface = SDL_LoadBMP(path);
    if (!surface) return NULL;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void cleanupGame() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color goldColor = { 255, 215, 0, 255 };
    SDL_Color redColor = { 255, 50, 50, 255 };

    if (gameWin) {
        renderText("YOU WIN! Max level achieved successfully!", 180, 260, goldColor);
    }
    else if (gameOver) {
        renderText("GAME OVER... All your fishes are dead.", 200, 260, redColor);
    }
    else {
        renderText("GAME CLOSED", 320, 260, redColor);
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(3000);

    if (fishTexture) SDL_DestroyTexture(fishTexture);
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
}