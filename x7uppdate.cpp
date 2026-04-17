
#include <GL/glut.h>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <string>
#include <vector>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

struct Vec2 {
    float x;
    float z;
};

struct Obstacle {
    float x;
    float z;
    float sx;
    float sz;
    float dx = 0.0f; // Velocity X
    float dz = 0.0f; // Velocity Z
};

enum LevelType {
    LEVEL_ROOM,
    LEVEL_GROUND
};

enum GameState {
    STATE_MENU,
    STATE_RULES,
    STATE_PLAYING,
    STATE_HIGHSCORES,
    STATE_NAME_ENTRY,
    STATE_GAME_OVER
};

struct HighScoreEntry {
    string name;
    int score;
    int livesLeft;
    int bonusTime;
};

const float ROOM_HALF_SIZE = 5.0f;
const float GROUND_HALF_SIZE = 16.0f;
const float WALL_THICKNESS = 0.3f;
const float PLAYER_RADIUS = 0.25f;
const float BASE_MOVE_SPEED = 0.07f;
const float ROT_SPEED = 0.03f;
const float PITCH_SPEED = 0.02f;
const float MOUSE_SENSITIVITY = 0.0035f;
const float PITCH_LIMIT = 1.45f;
const float PLAYER_EYE_HEIGHT = 1.0f;
const float JUMP_VELOCITY = 4.2f;
const float GRAVITY_ACCEL = 9.2f;
const float FALL_DAMAGE_THRESHOLD = 1.15f;
const int MAX_LIVES = 3;
const char* HIGHSCORE_FILE = "highscores.txt";

const int TOTAL_LEVELS = 4;
const int MAX_KEYS = 3;
const int MAX_OBSTACLES = 6;

const char* levelNames[TOTAL_LEVELS] = {"Easy", "Medium", "Hard", "Ground"};
const LevelType levelTypes[TOTAL_LEVELS] = {LEVEL_ROOM, LEVEL_ROOM, LEVEL_ROOM, LEVEL_GROUND};
const float levelHalfSizes[TOTAL_LEVELS] = {ROOM_HALF_SIZE, ROOM_HALF_SIZE, ROOM_HALF_SIZE, GROUND_HALF_SIZE};
const int keysRequiredByLevel[TOTAL_LEVELS] = {1, 2, 3, 0};
const int obstacleCountByLevel[TOTAL_LEVELS] = {3, 4, 6, 3};
const float moveSpeedScale[TOTAL_LEVELS] = {1.00f, 0.92f, 0.84f, 1.00f};
const float pickupRadiusByLevel[TOTAL_LEVELS] = {0.95f, 0.82f, 0.72f, 0.90f};
const float levelTimeLimitByLevel[TOTAL_LEVELS] = {10.0f, 15.0f, 25.0f, 20.0f};
const float doorWidthByLevel[TOTAL_LEVELS] = {1.6f, 1.6f, 1.6f, 2.6f};
const float doorHeightByLevel[TOTAL_LEVELS] = {2.6f, 2.6f, 2.6f, 3.6f};
const float doorThicknessByLevel[TOTAL_LEVELS] = {0.16f, 0.16f, 0.16f, 0.20f};

const Vec2 levelKeyPositions[TOTAL_LEVELS][MAX_KEYS] = {
    {{2.7f, -3.2f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
    {{3.1f, -3.3f}, {-2.5f, 3.5f}, {0.0f, 0.0f}},
    {{3.2f, -3.3f}, {-3.5f, -2.5f}, {2.5f, 2.5f}},
    {{-5.4f, 4.8f}, {0.0f, 0.0f}, {0.0f, 0.0f}}
};

const float levelKeyHeights[TOTAL_LEVELS][MAX_KEYS] = {
    {0.35f, 0.35f, 0.35f},
    {0.35f, 0.35f, 0.35f},
    {0.35f, 0.35f, 0.35f},
    {0.35f, 0.35f, 0.35f}
};

const float obstacleHeightByLevel[TOTAL_LEVELS][MAX_OBSTACLES] = {
    {1.05f, 1.20f, 1.05f, 0.0f, 0.0f, 0.0f},
    {1.2f, 1.6f, 1.15f, 1.15f, 0.0f, 0.0f},
    {1.9f, 1.9f, 2.1f, 1.4f, 1.7f, 1.7f},
    {1.2f, 1.2f, 1.2f, 0.0f, 0.0f, 0.0f}
};

const Obstacle levelObstacles[TOTAL_LEVELS][MAX_OBSTACLES] = {
    {
        {-2.0f, 1.4f, 0.75f, 0.75f},
        {0.0f, -1.6f, 0.90f, 0.90f},
        {2.0f, 1.2f, 0.75f, 0.75f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f}
    },
    {
        {-1.2f, 0.2f, 2.1f, 0.55f},
        {1.5f, -1.5f, 2.3f, 0.55f},
        {-2.7f, 2.4f, 1.2f, 0.60f},
        {2.4f, -2.6f, 1.1f, 0.60f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f}
    },
    {
        {-2.8f, 1.3f, 0.55f, 3.8f},
        {2.6f, -0.5f, 0.55f, 3.5f},
        {0.0f, 0.9f, 3.1f, 0.55f},
        {0.0f, -2.1f, 3.0f, 0.55f},
        {-3.7f, -0.9f, 0.70f, 2.8f},
        {3.5f, 1.8f, 0.70f, 2.8f}
    },
    {
        {-4.4f, 2.8f, 0.9f, 0.9f},
        {3.8f, 1.8f, 1.1f, 1.1f},
        {0.2f, -1.8f, 2.2f, 2.2f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f}
    }
};

GLuint startScreenTexture = 0;

void loadBackgroundTexture() {
    int width, height, channels;
    unsigned char* data = stbi_load("3d-escape.png", &width, &height, &channels, 4);
    if (!data) data = stbi_load("../3d-escape.png", &width, &height, &channels, 4);
    if (data) {
        glGenTextures(1, &startScreenTexture);
        glBindTexture(GL_TEXTURE_2D, startScreenTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
        cout << "Loaded 3d-escape.png successfully.\n";
    } else {
        cout << "Failed to load 3d-escape.png. Reason: " << stbi_failure_reason() << "\n";
    }
}

float camX = 0.0f;
float camY = 1.0f;
float camZ = 3.5f;
float yaw = 0.0f;
float pitch = 0.0f;

GameState gameState = STATE_MENU;
int menuSelection = 0;
const int MENU_ITEMS = 3;
const char* menuLabels[MENU_ITEMS] = {"Start", "Highscore List", "Quit"};

vector<HighScoreEntry> highScores;
char playerName[16] = {0};
int playerNameLength = 0;
int pendingScore = 0;
int bonusTimeScore = 0;

int currentLevel = 0;
int keysCollected = 0;
Obstacle activeObstacles[MAX_OBSTACLES];
int playerLives = MAX_LIVES;
bool keyTaken[MAX_KEYS] = {false, false, false};
bool doorOpening = false;
float doorAngle = 0.0f;
bool gameCompleted = false;
bool gameOver = false;
int levelBannerFrames = 0;
float levelTimeRemaining = 0.0f;
float damageCooldown = 0.0f;
float weaponAttackCooldown = 0.0f;
int lastTickMs = 0;
float verticalVelocity = 0.0f;
bool airborne = false;
float fallStartCamY = PLAYER_EYE_HEIGHT;

const int GHOST_MAX_HEALTH = 4;         // 4 hits each (6 ghosts x 4 = 24 total)
const int NUM_GHOSTS = 6;
const float GHOST_MOVE_SPEED = 1.10f;
const float GHOST_TOUCH_DAMAGE_RANGE = 1.05f;
const float GHOST_WEAPON_RANGE = 18.0f;
const float GHOST_BODY_RADIUS = 0.75f;
const float INTERACT_PICKUP_RANGE = 1.60f;
const float MEDKIT_HOLD_REQUIRED = 1.0f;

struct GhostEntity {
    bool  alive     = false;
    int   health    = GHOST_MAX_HEALTH;
    float x         = 0.0f;
    float z         = 0.0f;
    float hitFlash  = 0.0f;
    float phaseOff  = 0.0f;
    float swirlOff  = 0.0f;
};
GhostEntity ghosts[NUM_GHOSTS];

// Legacy scalar vars derived from the array (used by HUD / door logic)
bool  ghostAlive   = false;
int   ghostHealth  = GHOST_MAX_HEALTH * NUM_GHOSTS;
float ghostX       = 0.0f;
float ghostZ       = -5.8f;
float ghostHitFlashTimer = 0.0f;

Vec2 weaponPickupPos = {-7.0f, 10.5f};
Vec2 healthPickupPos = {7.0f,  9.8f};
bool weaponClaimed = false;
bool healthPickupClaimed = false;
bool groundDoorUnlockedByBoss = false;
bool medkitUsed = false;
float medkitHoldTimer = 0.0f;
bool groundIntroActive = false;
float swordSwingTimer = 0.0f;

// ---- Congratulation animation ----
bool  congratsActive = false;
float congratsTimer  = 0.0f;
const float CONGRATS_DURATION = 6.0f;

// ---- Gun / shooting variables ----
float gunRecoilTimer = 0.0f;
float muzzleFlashTimer = 0.0f;

// ---- Torch flicker variables ----
struct TorchFlicker {
    float rate;
    float phase;
    float intensity;
};
TorchFlicker torchFlickers[4];

// ---- Lava crack pulse ----
float lavaPulsePhase = 0.0f;

// ---- Bonus Clock Pickup (Level 3+) ----
bool bonusClockActive = false;
float bonusClockX = 0.0f;
float bonusClockZ = 0.0f;
float bonusClockSpawnTimer = 0.0f; // countdown to next spawn
float bonusClockNextInterval = 7.0f; // random 5-11
float bonusClockBannerTimer = 0.0f; // "+8 SEC!" display timer
const float BONUS_CLOCK_RADIUS = 0.9f;
const float BONUS_CLOCK_TIME_BONUS = 8.0f;

bool keyStates[256] = {false};
bool specialStates[256] = {false};
bool mouseInitialized = false;
int lastMouseX = 0;
int lastMouseY = 0;

void startLevel(int levelIndex);
void damagePlayer(int amount, bool bypassCooldown = false);
void tryGhostAttack();

float clampf(float value, float low, float high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

float distance2D(float x1, float z1, float x2, float z2) {
    float dx = x1 - x2;
    float dz = z1 - z2;
    return sqrt(dx * dx + dz * dz);
}

float distance3D(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    float dz = z1 - z2;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

bool isGroundLevel() {
    return levelTypes[currentLevel] == LEVEL_GROUND;
}

float worldHalfSize() {
    return levelHalfSizes[currentLevel];
}

float currentMoveSpeed() {
    return BASE_MOVE_SPEED * moveSpeedScale[currentLevel];
}

int keysRequired() {
    return keysRequiredByLevel[currentLevel];
}

int obstacleCount() {
    return obstacleCountByLevel[currentLevel];
}

float pickupRadius() {
    return pickupRadiusByLevel[currentLevel];
}

float currentDoorWidth() {
    return doorWidthByLevel[currentLevel];
}

float currentDoorHeight() {
    return doorHeightByLevel[currentLevel];
}

float currentDoorThickness() {
    return doorThicknessByLevel[currentLevel];
}

float currentDoorZ() {
    return -worldHalfSize() + WALL_THICKNESS * 0.5f;
}

float randFloat(float lo, float hi) {
    return lo + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (hi - lo);
}

void drawBox(float x, float y, float z, float sx, float sy, float sz) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawFilledRect2D(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawText2D(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* p = text; *p; ++p) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
}

int bitmapTextWidth(const char* text) {
    return glutBitmapLength(GLUT_BITMAP_HELVETICA_18, reinterpret_cast<const unsigned char*>(text));
}

void drawText2DRightAligned(float rightX, float y, const char* text) {
    drawText2D(rightX - static_cast<float>(bitmapTextWidth(text)), y, text);
}

void drawText2DCentered(float centerX, float y, const char* text) {
    drawText2D(centerX - static_cast<float>(bitmapTextWidth(text)) * 0.5f, y, text);
}

void resetInputStates() {
    for (int i = 0; i < 256; ++i) {
        keyStates[i] = false;
        specialStates[i] = false;
    }
}

void resetNameEntry() {
    playerName[0] = '\0';
    playerNameLength = 0;
}

void sortHighScores() {
    sort(highScores.begin(), highScores.end(), [](const HighScoreEntry& a, const HighScoreEntry& b) {
        if (a.score != b.score) return a.score > b.score;
        if (a.livesLeft != b.livesLeft) return a.livesLeft > b.livesLeft;
        return a.bonusTime > b.bonusTime;
    });

    if (highScores.size() > 10) {
        highScores.resize(10);
    }
}

void loadHighScores() {
    highScores.clear();

    ifstream file(HIGHSCORE_FILE);
    if (!file.is_open()) {
        return;
    }

    HighScoreEntry entry;
    while (file >> entry.name >> entry.score >> entry.livesLeft >> entry.bonusTime) {
        highScores.push_back(entry);
    }

    sortHighScores();
}

void saveHighScores() {
    sortHighScores();

    ofstream file(HIGHSCORE_FILE, ios::trunc);
    if (!file.is_open()) {
        cout << "Could not save high scores.\n";
        return;
    }

    for (const auto& entry : highScores) {
        file << entry.name << ' ' << entry.score << ' ' << entry.livesLeft << ' ' << entry.bonusTime << '\n';
    }
}

void addHighScore(const string& name, int score, int livesLeft, int bonusTime) {
    HighScoreEntry entry;
    entry.name = name.empty() ? "PLAYER" : name;
    entry.score = score;
    entry.livesLeft = livesLeft;
    entry.bonusTime = bonusTime;
    highScores.push_back(entry);
    sortHighScores();
    saveHighScores();
}

void startMenu() {
    gameState = STATE_MENU;
    menuSelection = 0;
    gameCompleted = false;
    gameOver = false;
    resetInputStates();
    resetNameEntry();
}

void startGame() {
    bonusTimeScore = 0;
    pendingScore = 0;
    playerLives = MAX_LIVES;
    gameCompleted = false;
    gameOver = false;
    gameState = STATE_PLAYING;
    startLevel(0);
}

void openHighScoreScreen() {
    gameState = STATE_HIGHSCORES;
    resetInputStates();
}

void beginNameEntry() {
    gameState = STATE_NAME_ENTRY;
    resetInputStates();
    resetNameEntry();
}

void finishGameToHighScore() {
    pendingScore = playerLives * 1000 + bonusTimeScore;
    beginNameEntry();
}

void returnToMenuAfterFailure() {
    gameState = STATE_GAME_OVER;
    resetInputStates();
}

void initTorchFlickers() {
    for (int i = 0; i < 4; ++i) {
        torchFlickers[i].rate = randFloat(3.5f, 7.0f);
        torchFlickers[i].phase = randFloat(0.0f, 6.28f);
        torchFlickers[i].intensity = randFloat(0.7f, 1.0f);
    }
}

void spawnBonusClock() {
    float hs = worldHalfSize();
    float margin = 1.5f;
    bonusClockX = randFloat(-hs + margin, hs - margin);
    bonusClockZ = randFloat(-hs + margin, hs - margin);
    bonusClockActive = true;
    cout << "Bonus clock spawned!\n";
}

void resetLevelCore() {
    keysCollected = 0;
    for (int i = 0; i < MAX_KEYS; ++i) {
        keyTaken[i] = false;
    }

    doorOpening = false;
    doorAngle = 0.0f;
    camX = 0.0f;
    camY = PLAYER_EYE_HEIGHT;
    camZ = isGroundLevel() ? (worldHalfSize() - 3.0f) : 3.6f;
    yaw = 0.0f;
    pitch = 0.0f;
    levelBannerFrames = 220;
    levelTimeRemaining = levelTimeLimitByLevel[currentLevel];
    damageCooldown = 0.0f;
    weaponAttackCooldown = 0.0f;
    verticalVelocity = 0.0f;
    airborne = false;
    fallStartCamY = camY;

    // Spawn 6 ghosts spread around the ground level
    {
        float spawnPositions[NUM_GHOSTS][2] = {
            {-8.0f, -6.0f}, { 8.0f, -6.0f},
            {-10.0f, 0.0f}, {10.0f,  0.0f},
            {-6.0f,  6.0f}, { 6.0f,  6.0f}
        };
        for (int g = 0; g < NUM_GHOSTS; ++g) {
            ghosts[g].alive    = isGroundLevel();
            ghosts[g].health   = GHOST_MAX_HEALTH;
            ghosts[g].x        = spawnPositions[g][0];
            ghosts[g].z        = spawnPositions[g][1];
            ghosts[g].hitFlash = 0.0f;
            ghosts[g].phaseOff = (float)g * 1.05f;
            ghosts[g].swirlOff = (float)g * 0.87f;
        }
    }
    ghostAlive   = isGroundLevel();
    ghostHealth  = GHOST_MAX_HEALTH * NUM_GHOSTS;
    ghostX       = 0.0f;
    ghostZ       = -5.8f;
    ghostHitFlashTimer = 0.0f;
    weaponClaimed = false;
    healthPickupClaimed = false;
    groundDoorUnlockedByBoss = false;
    medkitUsed = false;
    medkitHoldTimer = 0.0f;
    groundIntroActive = isGroundLevel();
    swordSwingTimer = 0.0f;
    congratsActive = false;
    congratsTimer  = 0.0f;
    gunRecoilTimer = 0.0f;
    muzzleFlashTimer = 0.0f;

    // Bonus clock reset
    bonusClockActive = false;
    bonusClockBannerTimer = 0.0f;
    bonusClockSpawnTimer = randFloat(5.0f, 11.0f);

    mouseInitialized = false;
    resetInputStates();
    initTorchFlickers();
}

void startLevel(int levelIndex) {
    currentLevel = levelIndex;
    gameState = STATE_PLAYING;
    gameOver = false;
    for (int i = 0; i < obstacleCount(); ++i) {
        activeObstacles[i] = levelObstacles[currentLevel][i];
        activeObstacles[i].dx = 0.0f;
        activeObstacles[i].dz = 0.0f;

        if (currentLevel == 1 || currentLevel == 2) {
            // Level 3 (Hard) now has much slower obstacle movement
            float speedMult = (currentLevel == 2) ? 0.35f : 0.95f;
            float vx = (static_cast<float>(rand() % 200) / 100.0f - 1.0f) * speedMult;
            float vz = (static_cast<float>(rand() % 200) / 100.0f - 1.0f) * speedMult;
            if (fabs(vx) < 0.15f) vx = (vx < 0.0f) ? -0.15f : 0.15f;
            if (fabs(vz) < 0.15f) vz = (vz < 0.0f) ? -0.15f : 0.15f;
            activeObstacles[i].dx = vx;
            activeObstacles[i].dz = vz;
        }
    }
    resetLevelCore();
    lastTickMs = glutGet(GLUT_ELAPSED_TIME);
    cout << "Entered Level " << (currentLevel + 1) << " (" << levelNames[currentLevel] << ")\n";
}

bool doorBlocksPassage() {
    return doorAngle < 80.0f;
}

float obstacleTopAtIndex(int obstacleIndex) {
    return obstacleHeightByLevel[currentLevel][obstacleIndex];
}

bool isInsideObstacleFootprint(const Obstacle& obstacle, float x, float z, float padding = PLAYER_RADIUS) {
    float minX = obstacle.x - obstacle.sx * 0.5f - padding;
    float maxX = obstacle.x + obstacle.sx * 0.5f + padding;
    float minZ = obstacle.z - obstacle.sz * 0.5f - padding;
    float maxZ = obstacle.z + obstacle.sz * 0.5f + padding;
    return x >= minX && x <= maxX && z >= minZ && z <= maxZ;
}

float supportSurfaceHeightAt(float x, float z) {
    float supportHeight = 0.0f;

    if (isGroundLevel()) {
        return supportHeight;
    }

    for (int i = 0; i < obstacleCount(); ++i) {
        const Obstacle& obstacle = activeObstacles[i];
        float top = obstacleTopAtIndex(i);
        if (top <= 0.0f) {
            continue;
        }

        if (isInsideObstacleFootprint(obstacle, x, z, PLAYER_RADIUS * 0.6f) && top > supportHeight) {
            supportHeight = top;
        }
    }

    return supportHeight;
}

bool collidesWithObstacle(float x, float z, float feetHeight) {
    for (int i = 0; i < obstacleCount(); ++i) {
        const Obstacle& obstacle = activeObstacles[i];
        float top = obstacleTopAtIndex(i);
        if (top <= 0.0f) {
            continue;
        }

        if (isInsideObstacleFootprint(obstacle, x, z) && feetHeight < (top - 0.05f)) {
            return true;
        }
    }
    return false;
}

bool canMoveTo(float newX, float newZ) {
    float halfSize = worldHalfSize();
    float minBound = -halfSize + WALL_THICKNESS + PLAYER_RADIUS + 0.15f;
    float maxBound = halfSize - WALL_THICKNESS - PLAYER_RADIUS - 0.15f;

    if (newX < minBound || newX > maxBound || newZ > maxBound) {
        return false;
    }

    if (newZ < minBound) {
        bool withinDoorOpening = fabs(newX) < (currentDoorWidth() * 0.5f - PLAYER_RADIUS * 0.5f);
        if (!(withinDoorOpening && !doorBlocksPassage())) {
            return false;
        }
    }

    float feetHeight = camY - PLAYER_EYE_HEIGHT;
    if (collidesWithObstacle(newX, newZ, feetHeight)) {
        damagePlayer(1, false);
        return false;
    }

    if (isGroundLevel() && ghostAlive) {
        float ghostDistance = distance2D(newX, newZ, ghostX, ghostZ);
        if (ghostDistance < GHOST_BODY_RADIUS) {
            damagePlayer(1, false);
            return false;
        }
    }

    return true;
}

void damagePlayer(int amount, bool bypassCooldown) {
    if (gameState != STATE_PLAYING) {
        return;
    }

    if (!bypassCooldown && damageCooldown > 0.0f) {
        return;
    }

    damageCooldown = 1.5f;
    playerLives -= amount;
    if (playerLives <= 0) {
        playerLives = 0;
        gameOver = true;
        gameState = STATE_GAME_OVER;
        resetInputStates();
        cout << "Game over. Health depleted or hit obstacle.\n";
        return;
    }

    cout << "Health lost. Health left: " << playerLives << "\n";
}

// =============================================
// ENVIRONMENT DECORATION RENDERING
// =============================================

void renderTorch(float x, float y, float z, int torchIndex) {
    float time = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;
    TorchFlicker& tf = torchFlickers[torchIndex % 4];
    float flicker = 0.6f + 0.4f * tf.intensity * (0.5f + 0.5f * sin(time * tf.rate + tf.phase));
    float flicker2 = 0.5f + 0.5f * sin(time * tf.rate * 1.7f + tf.phase + 1.0f);

    // Torch bracket (metal)
    glColor3f(0.25f, 0.22f, 0.20f);
    drawBox(x, y - 0.3f, z, 0.08f, 0.6f, 0.08f);

    // Torch top holder
    glColor3f(0.30f, 0.25f, 0.18f);
    drawBox(x, y, z, 0.14f, 0.12f, 0.14f);

    // Fire glow (animated)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Inner flame (bright yellow-orange)
    glColor4f(1.0f, 0.85f * flicker, 0.15f, 0.9f * flicker);
    glPushMatrix();
    glTranslatef(x, y + 0.18f + flicker2 * 0.05f, z);
    glutSolidSphere(0.08f, 8, 8);
    glPopMatrix();

    // Outer flame (orange-red)
    glColor4f(1.0f, 0.45f * flicker, 0.05f, 0.6f * flicker);
    glPushMatrix();
    glTranslatef(x, y + 0.22f + flicker2 * 0.08f, z);
    glutSolidSphere(0.14f, 8, 8);
    glPopMatrix();

    // Flame halo
    glColor4f(1.0f, 0.6f, 0.1f, 0.15f * flicker);
    glPushMatrix();
    glTranslatef(x, y + 0.15f, z);
    glutSolidSphere(0.35f, 10, 10);
    glPopMatrix();

    glDisable(GL_BLEND);
}

void renderLavaCracks() {
    float time = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;
    float pulse = 0.5f + 0.5f * sin(time * 1.8f + lavaPulsePhase);
    float pulse2 = 0.5f + 0.5f * sin(time * 2.5f + lavaPulsePhase + 2.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Multiple lava cracks across the floor
    float crackY = 0.02f;

    // Crack 1 - diagonal
    glColor4f(0.95f, 0.3f * pulse, 0.02f, 0.7f * pulse);
    drawBox(-1.5f, crackY, -1.0f, 2.2f, 0.02f, 0.06f);
    drawBox(-0.5f, crackY, -0.5f, 0.06f, 0.02f, 1.2f);

    // Crack 2
    glColor4f(1.0f, 0.35f * pulse2, 0.05f, 0.65f * pulse2);
    drawBox(1.8f, crackY, 2.0f, 1.8f, 0.02f, 0.05f);
    drawBox(2.5f, crackY, 1.5f, 0.05f, 0.02f, 1.5f);

    // Crack 3
    glColor4f(0.9f, 0.25f * pulse, 0.0f, 0.6f * pulse);
    drawBox(-2.2f, crackY, 2.5f, 1.5f, 0.02f, 0.07f);
    drawBox(-3.0f, crackY, 2.0f, 0.07f, 0.02f, 1.8f);

    // Crack 4
    glColor4f(1.0f, 0.4f * pulse2, 0.08f, 0.5f * pulse2);
    drawBox(0.5f, crackY, -3.0f, 2.0f, 0.02f, 0.05f);
    drawBox(1.3f, crackY, -2.5f, 0.05f, 0.02f, 1.3f);

    // Glow spots at crack intersections
    glColor4f(1.0f, 0.5f, 0.0f, 0.2f * pulse);
    drawBox(-0.5f, crackY + 0.01f, -0.5f, 0.3f, 0.01f, 0.3f);
    drawBox(2.5f, crackY + 0.01f, 1.5f, 0.25f, 0.01f, 0.25f);
    drawBox(-3.0f, crackY + 0.01f, 2.0f, 0.2f, 0.01f, 0.2f);

    glDisable(GL_BLEND);
}

void renderWallChains() {
    float halfSize = worldHalfSize();
    float wallZ = halfSize - WALL_THICKNESS * 0.5f - 0.05f;
    float time = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;

    // Two chain sets on the far (back) wall
    float chainPositions[2] = {-2.0f, 2.0f};

    for (int c = 0; c < 2; ++c) {
        float cx = chainPositions[c];
        float sway = sin(time * 0.8f + c * 1.5f) * 0.03f;

        // Mounting ring
        glColor3f(0.35f, 0.33f, 0.30f);
        glPushMatrix();
        glTranslatef(cx, 2.8f, wallZ);
        glutSolidTorus(0.03f, 0.10f, 8, 16);
        glPopMatrix();

        // Chain links (hanging down)
        for (int i = 0; i < 6; ++i) {
            float linkY = 2.5f - i * 0.22f;
            float linkSway = sway * (i + 1) * 0.5f;
            glColor3f(0.32f - i * 0.02f, 0.30f - i * 0.02f, 0.28f - i * 0.01f);

            glPushMatrix();
            glTranslatef(cx + linkSway, linkY, wallZ);
            if (i % 2 == 0) {
                glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
            }
            glutSolidTorus(0.015f, 0.04f, 6, 12);
            glPopMatrix();
        }

        // Hanging ring at bottom
        glColor3f(0.28f, 0.26f, 0.24f);
        glPushMatrix();
        glTranslatef(cx + sway * 3.5f, 1.1f, wallZ);
        glutSolidTorus(0.025f, 0.08f, 8, 16);
        glPopMatrix();
    }
}

void renderCornerPillars() {
    float halfSize = worldHalfSize();
    float pillarSize = 0.35f;
    float pillarHeight = 3.8f;

    // Four corner positions near the entrance wall (back wall Z+)
    float positions[4][2] = {
        {-halfSize + 0.5f, halfSize - 0.5f},
        { halfSize - 0.5f, halfSize - 0.5f},
        {-halfSize + 0.5f, -halfSize + 0.8f},
        { halfSize - 0.5f, -halfSize + 0.8f}
    };

    for (int i = 0; i < 4; ++i) {
        float px = positions[i][0];
        float pz = positions[i][1];

        // Stone pillar body
        glColor3f(0.38f, 0.36f, 0.33f);
        drawBox(px, pillarHeight * 0.5f, pz, pillarSize, pillarHeight, pillarSize);

        // Base
        glColor3f(0.32f, 0.30f, 0.28f);
        drawBox(px, 0.15f, pz, pillarSize + 0.12f, 0.30f, pillarSize + 0.12f);

        // Carved cap
        glColor3f(0.42f, 0.40f, 0.36f);
        drawBox(px, pillarHeight + 0.08f, pz, pillarSize + 0.10f, 0.16f, pillarSize + 0.10f);

        // Cap detail
        glColor3f(0.35f, 0.32f, 0.28f);
        drawBox(px, pillarHeight + 0.20f, pz, pillarSize - 0.05f, 0.08f, pillarSize - 0.05f);
    }
}

void renderSkullOrnaments() {
    float halfSize = worldHalfSize();
    float time = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;

    // Skulls on left and right walls
    struct SkullPos { float x; float y; float z; float rotY; };
    SkullPos skulls[4] = {
        {-halfSize + WALL_THICKNESS * 0.5f + 0.12f, 2.2f, -1.5f, 90.0f},
        {-halfSize + WALL_THICKNESS * 0.5f + 0.12f, 2.2f,  1.5f, 90.0f},
        { halfSize - WALL_THICKNESS * 0.5f - 0.12f, 2.2f, -1.5f, -90.0f},
        { halfSize - WALL_THICKNESS * 0.5f - 0.12f, 2.2f,  1.5f, -90.0f}
    };

    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
        glTranslatef(skulls[i].x, skulls[i].y, skulls[i].z);
        glRotatef(skulls[i].rotY, 0.0f, 1.0f, 0.0f);

        // Skull cranium
        glColor3f(0.85f, 0.82f, 0.72f);
        glPushMatrix();
        glScalef(1.0f, 1.15f, 0.9f);
        glutSolidSphere(0.14f, 12, 12);
        glPopMatrix();

        // Eye sockets
        float eyeGlow = 0.3f + 0.7f * (0.5f + 0.5f * sin(time * 2.0f + i * 1.5f));
        glColor3f(0.8f * eyeGlow, 0.15f * eyeGlow, 0.0f);
        drawBox(-0.05f, 0.02f, 0.11f, 0.04f, 0.04f, 0.03f);
        drawBox( 0.05f, 0.02f, 0.11f, 0.04f, 0.04f, 0.03f);

        // Nose
        glColor3f(0.25f, 0.22f, 0.18f);
        drawBox(0.0f, -0.03f, 0.12f, 0.025f, 0.03f, 0.02f);

        // Jaw
        glColor3f(0.80f, 0.77f, 0.68f);
        drawBox(0.0f, -0.10f, 0.06f, 0.10f, 0.04f, 0.08f);

        // Mounting plate
        glColor3f(0.30f, 0.28f, 0.25f);
        drawBox(0.0f, 0.0f, -0.06f, 0.18f, 0.22f, 0.03f);

        glPopMatrix();
    }
}

void renderStalactites() {
    float halfSize = worldHalfSize();
    float ceilingY = 4.0f;

    struct StalactiteInfo { float x; float z; float length; float width; };
    StalactiteInfo stalactites[] = {
        {-3.0f,  2.5f, 0.8f, 0.10f},
        {-1.5f, -3.0f, 1.2f, 0.12f},
        { 0.5f,  1.8f, 0.6f, 0.08f},
        { 2.5f, -1.0f, 1.0f, 0.11f},
        {-2.0f, -0.5f, 0.5f, 0.07f},
        { 3.2f,  3.0f, 0.7f, 0.09f},
        { 1.0f, -2.5f, 0.9f, 0.10f},
        {-3.5f,  0.5f, 0.55f, 0.08f},
        { 0.0f,  3.5f, 0.65f, 0.09f},
        { 2.0f,  2.0f, 0.45f, 0.07f}
    };
    int numStalactites = 10;

    for (int i = 0; i < numStalactites; ++i) {
        float sx = stalactites[i].x;
        float sz = stalactites[i].z;
        float len = stalactites[i].length;
        float w = stalactites[i].width;

        if (fabs(sx) > halfSize - 0.5f || fabs(sz) > halfSize - 0.5f) continue;

        // Main body (cone shape approximated with tapered boxes)
        glColor3f(0.35f, 0.33f, 0.30f);
        drawBox(sx, ceilingY - len * 0.25f, sz, w * 1.5f, len * 0.5f, w * 1.5f);

        glColor3f(0.30f, 0.28f, 0.26f);
        drawBox(sx, ceilingY - len * 0.55f, sz, w * 1.0f, len * 0.3f, w * 1.0f);

        // Tip
        glColor3f(0.25f, 0.24f, 0.22f);
        drawBox(sx, ceilingY - len * 0.8f, sz, w * 0.5f, len * 0.2f, w * 0.5f);

        // Drip effect (tiny sphere at tip, subtle)
        float time = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;
        float dripPhase = fmod(time * 0.3f + i * 1.1f, 3.0f);
        if (dripPhase < 0.5f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0.4f, 0.5f, 0.6f, 0.5f * (1.0f - dripPhase * 2.0f));
            glPushMatrix();
            glTranslatef(sx, ceilingY - len - dripPhase * 0.3f, sz);
            glutSolidSphere(0.02f, 6, 6);
            glPopMatrix();
            glDisable(GL_BLEND);
        }
    }
}

// =============================================
// BONUS CLOCK RENDERING
// =============================================

void renderBonusClock() {
    if (!bonusClockActive) return;

    float time   = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;
    // Bob up/down gently – no Y-axis spin so face stays visible
    float hover  = 0.48f + 0.10f * sin(time * 2.2f);
    // Slight tilt toward player (face-forward) – tiny rock side to side
    float rock   = 4.0f * sin(time * 1.6f);

    glPushMatrix();
    glTranslatef(bonusClockX, hover, bonusClockZ);
    // Make clock face the player (rotate around Y so face points +Z)
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);   // face toward +X world = toward cam roughly
    glRotatef(rock,  1.0f, 0.0f, 0.0f);   // gentle rocking tilt

    // ── SIZES (match reference: thick green ring → gold body → white face) ──
    const float R_GREEN  = 0.310f;  // green outer ring radius
    const float R_GOLD   = 0.265f;  // gold body radius
    const float R_WHITE  = 0.210f;  // white face radius
    const float DEPTH    = 0.22f;   // disc thickness scale along Z

    // ── PULSING GREEN OUTER GLOW ─────────────────────────────────────────────
    float glow = 0.5f + 0.5f * sin(time * 4.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(0.05f, 0.85f, 0.15f, 0.10f + 0.08f * glow);
    glPushMatrix(); glScalef(1.0f, 1.0f, DEPTH * 0.6f);
    glutSolidSphere(R_GREEN + 0.06f, 20, 20);
    glPopMatrix();
    glDisable(GL_BLEND);

    // ── GREEN OUTER RING ─────────────────────────────────────────────────────
    // Solid dark-green ring (the thick border seen in the image)
    glColor3f(0.10f, 0.55f, 0.12f);
    glPushMatrix();
    glScalef(1.0f, 1.0f, DEPTH);
    glutSolidSphere(R_GREEN, 28, 28);
    glPopMatrix();

    // Slightly lighter green highlight on top-left (lighting feel)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.25f, 0.80f, 0.28f, 0.35f);
    glPushMatrix();
    glTranslatef(-0.06f, 0.08f, R_GREEN * DEPTH * 0.55f);
    glScalef(0.55f, 0.45f, 0.08f);
    glutSolidSphere(R_GREEN, 16, 16);
    glPopMatrix();
    glDisable(GL_BLEND);

    // ── GOLD / YELLOW BODY DISC ───────────────────────────────────────────────
    glColor3f(0.92f, 0.78f, 0.08f);   // bright gold – matches image
    glPushMatrix();
    glScalef(1.0f, 1.0f, DEPTH);
    glutSolidSphere(R_GOLD, 26, 26);
    glPopMatrix();

    // Shadow rim between green and gold (darker gold band at edge)
    glColor3f(0.62f, 0.50f, 0.04f);
    glPushMatrix();
    glScalef(1.0f, 1.0f, DEPTH * 0.85f);
    glutSolidSphere(R_GOLD + 0.010f, 26, 26);
    glPopMatrix();

    // ── WHITE / CREAM CLOCK FACE ─────────────────────────────────────────────
    glColor3f(0.97f, 0.96f, 0.90f);
    glPushMatrix();
    glScalef(1.0f, 1.0f, DEPTH * 0.60f);
    glutSolidSphere(R_WHITE, 26, 26);
    glPopMatrix();

    // Inner shadow ring on face edge (gives depth)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.55f, 0.50f, 0.30f, 0.22f);
    glPushMatrix();
    glScalef(1.0f, 1.0f, DEPTH * 0.38f);
    glutSolidSphere(R_WHITE + 0.005f, 24, 24);
    glPopMatrix();
    glDisable(GL_BLEND);

    // ── HOUR TICK MARKS (4 thick marks at 12/3/6/9, 8 thin for others) ───────
    float faceZ = R_WHITE * DEPTH * 0.62f + 0.004f;
    const float PI2 = 6.28318f;
    for (int i = 0; i < 12; ++i) {
        float ang  = (float)i / 12.0f * PI2;
        float r    = R_WHITE * 0.84f;
        float mx   = sin(ang) * r;
        float my   = cos(ang) * r;
        bool major = (i % 3 == 0);
        glColor3f(0.12f, 0.10f, 0.10f);
        glPushMatrix();
        glTranslatef(mx, my, faceZ);
        glRotatef(-(float)i / 12.0f * 360.0f, 0.0f, 0.0f, 1.0f);
        float tw = major ? 0.022f : 0.011f;
        float th = major ? 0.042f : 0.025f;
        drawBox(0.0f, 0.0f, 0.0f, tw, th, 0.006f);
        glPopMatrix();
    }

    // ── HOUR HAND  (short, chunky, black – like in image) ────────────────────
    // Image shows ~10:10 style – we animate it slowly
    float hourAng = fmod(time * 8.0f, 360.0f);   // 8 deg/sec looks like a fast demo clock
    glColor3f(0.08f, 0.07f, 0.08f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, faceZ + 0.006f);
    glRotatef(-hourAng, 0.0f, 0.0f, 1.0f);
    // Wider base tapering to tip
    drawBox(0.0f,  0.028f, 0.0f, 0.024f, 0.010f, 0.007f);
    drawBox(0.0f,  0.068f, 0.0f, 0.020f, 0.062f, 0.007f);
    drawBox(0.0f,  0.108f, 0.0f, 0.013f, 0.018f, 0.006f);
    glPopMatrix();

    // ── MINUTE HAND (long, slender, black) ───────────────────────────────────
    float minAng  = fmod(time * 48.0f, 360.0f);  // 48 deg/sec = full lap in 7.5 s (visible)
    glColor3f(0.08f, 0.07f, 0.08f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, faceZ + 0.011f);
    glRotatef(-minAng, 0.0f, 0.0f, 1.0f);
    drawBox(0.0f,  0.015f, 0.0f, 0.014f, 0.008f, 0.006f);
    drawBox(0.0f,  0.070f, 0.0f, 0.011f, 0.100f, 0.006f);
    drawBox(0.0f,  0.148f, 0.0f, 0.007f, 0.014f, 0.005f);
    glPopMatrix();

    // ── CENTER BOSS (gold dot over both hands) ────────────────────────────────
    glColor3f(0.85f, 0.70f, 0.10f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, faceZ + 0.016f);
    glutSolidSphere(0.020f, 10, 10);
    glPopMatrix();

    // ── CROWN / STEM on top of clock ─────────────────────────────────────────
    glColor3f(0.82f, 0.70f, 0.10f);
    drawBox(0.0f, R_GREEN * 1.05f, 0.0f, 0.038f, 0.055f, 0.038f);
    glColor3f(0.65f, 0.52f, 0.06f);
    drawBox(0.0f, R_GREEN * 1.05f + 0.030f, 0.0f, 0.050f, 0.018f, 0.050f);

    glPopMatrix();
}

void renderKeyModel(float scale) {
    glPushMatrix();
    glScalef(scale, scale, scale);

    glColor3f(1.0f, 0.88f, 0.08f);

    glPushMatrix();
    glTranslatef(-0.42f, 0.08f, 0.0f);
    glScalef(0.70f, 0.70f, 0.70f);
    glutSolidTorus(0.10f, 0.24f, 14, 24);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.23f, 0.0f, 0.0f);
    glScalef(1.20f, 0.12f, 0.16f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.66f, -0.12f, 0.0f);
    glScalef(0.11f, 0.26f, 0.16f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.82f, 0.10f, 0.0f);
    glScalef(0.11f, 0.22f, 0.16f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix();
}

void drawRoomLights() {
    glColor3f(0.95f, 0.92f, 0.72f);
    drawBox(-2.4f, 3.85f, 0.0f, 0.45f, 0.10f, 0.80f);
    drawBox(0.0f, 3.85f, 0.0f, 0.45f, 0.10f, 0.80f);
    drawBox(2.4f, 3.85f, 0.0f, 0.45f, 0.10f, 0.80f);
}

void renderDoorOrGate() {
    float doorWidth = currentDoorWidth();
    float doorHeight = currentDoorHeight();
    float doorThickness = currentDoorThickness();
    float doorZ = currentDoorZ();
    float hingeX = -doorWidth * 0.5f;

    glPushMatrix();
    glTranslatef(hingeX, 0.0f, doorZ);
    glRotatef(-doorAngle, 0.0f, 1.0f, 0.0f);
    glTranslatef(doorWidth * 0.5f, doorHeight * 0.5f, 0.0f);

    if (isGroundLevel()) {
        glColor3f(0.45f, 0.48f, 0.52f);
        drawBox(0.0f, 0.0f, 0.0f, doorWidth, doorHeight, doorThickness);

        glColor3f(0.25f, 0.28f, 0.32f);
        for (int i = -2; i <= 2; ++i) {
            drawBox(i * 0.42f, 0.0f, 0.03f, 0.08f, doorHeight * 0.88f, doorThickness * 0.7f);
        }

        glColor3f(0.18f, 0.18f, 0.20f);
        drawBox(0.0f, doorHeight * 0.38f, 0.05f, doorWidth * 0.95f, 0.06f, doorThickness * 0.8f);
        drawBox(0.18f, -0.25f, 0.05f, 0.12f, 0.12f, doorThickness * 0.9f);
    } else {
        glColor3f(0.63f, 0.34f, 0.12f);
        drawBox(0.0f, 0.0f, 0.0f, doorWidth, doorHeight, doorThickness);

        glColor3f(0.45f, 0.22f, 0.08f);
        drawBox(0.0f, 0.6f, 0.06f, doorWidth * 0.84f, 0.08f, doorThickness * 0.8f);
        drawBox(0.0f, -0.2f, 0.06f, doorWidth * 0.84f, 0.08f, doorThickness * 0.8f);
        drawBox(-0.22f, -0.1f, 0.06f, 0.08f, doorHeight * 0.78f, doorThickness * 0.8f);

        glColor3f(0.78f, 0.68f, 0.15f);
        drawBox(0.35f, -0.05f, 0.08f, 0.10f, 0.18f, doorThickness * 0.8f);
    }

    glPopMatrix();
}

void renderObstacles() {
    for (int i = 0; i < obstacleCount(); ++i) {
        const Obstacle& obstacle = activeObstacles[i];

        if (isGroundLevel()) {
            if (i == 0 || i == 2) {
                glPushMatrix();
                glTranslatef(obstacle.x, 0.0f, obstacle.z);
                glColor3f(0.42f, 0.28f, 0.14f);
                drawBox(0.0f, 0.45f, 0.0f, obstacle.sx * 0.45f, 0.9f, obstacle.sz * 0.45f);
                glColor3f(0.16f, 0.50f, 0.24f);
                glTranslatef(0.0f, 1.20f, 0.0f);
                glutSolidSphere(obstacle.sx * 0.85f, 18, 18);
                glPopMatrix();
            } else {
                glPushMatrix();
                glTranslatef(obstacle.x, 0.24f, obstacle.z);
                glColor3f(0.56f, 0.56f, 0.58f);
                glScalef(obstacle.sx * 0.8f, 0.45f, obstacle.sz * 0.8f);
                glutSolidSphere(0.5f, 12, 12);
                glPopMatrix();
            }
        } else {
            glColor3f(0.36f, 0.20f, 0.22f);
            float height = obstacleTopAtIndex(i);
            drawBox(obstacle.x, height * 0.5f, obstacle.z, obstacle.sx, height, obstacle.sz);
        }
    }
}

void renderGroundPickups() {
    if (!isGroundLevel()) {
        return;
    }

    float spin = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.12f;

    // Gun pickup instead of sword
    if (!weaponClaimed) {
        glPushMatrix();
        glTranslatef(weaponPickupPos.x, 0.48f, weaponPickupPos.z);
        glRotatef(spin, 0.0f, 1.0f, 0.0f);

        // Gun body
        glColor3f(0.18f, 0.18f, 0.20f);
        drawBox(0.0f, 0.0f, 0.0f, 0.12f, 0.14f, 0.50f);
        // Gun barrel
        glColor3f(0.25f, 0.25f, 0.28f);
        drawBox(0.0f, 0.04f, 0.35f, 0.06f, 0.06f, 0.30f);
        // Gun grip
        glColor3f(0.30f, 0.20f, 0.10f);
        drawBox(0.0f, -0.14f, -0.05f, 0.10f, 0.20f, 0.12f);
        // Gun trigger guard
        glColor3f(0.22f, 0.22f, 0.24f);
        drawBox(0.0f, -0.06f, 0.08f, 0.04f, 0.06f, 0.10f);

        glPopMatrix();
    }

    if (!healthPickupClaimed) {
        glPushMatrix();
        glTranslatef(healthPickupPos.x, 0.42f, healthPickupPos.z);
        glRotatef(-spin * 0.8f, 0.0f, 1.0f, 0.0f);
        glColor3f(0.90f, 0.20f, 0.22f);
        drawBox(0.0f, 0.0f, 0.0f, 0.62f, 0.34f, 0.42f);
        glColor3f(0.96f, 0.96f, 0.96f);
        drawBox(0.0f, 0.10f, 0.0f, 0.16f, 0.26f, 0.32f);
        drawBox(0.0f, 0.10f, 0.0f, 0.46f, 0.26f, 0.12f);
        glPopMatrix();
    }
}

// Draw a single ghost at local-space origin (caller provides the world transform)
// Style: clean white/lavender 3D ghost with dark oval eyes, wavy skirt, purple underglow
void drawOneGhost(float phaseOff, float swirlOff, float hurtFlash, int health) {
    float timeSec = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f + phaseOff;
    float pulse   = 0.5f + 0.5f * sin(timeSec * 2.5f + phaseOff);
    float healthR = (float)health / (float)GHOST_MAX_HEALTH;
    float breathe = 1.0f + 0.018f * sin(timeSec * 2.0f + phaseOff);

    // Ghost body base colour: clean white/lavender, flashes red on hit
    float bR = 0.93f - 0.10f * hurtFlash;
    float bG = 0.91f - 0.55f * hurtFlash;
    float bB = 0.97f - 0.60f * hurtFlash;

    // ── SETUP: lighting + blending ───────────────────────────────────────────
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT2);

    // Bright key light from upper-front (warm white) — gives that 3D toy look
    float light2Pos[]  = {-0.5f, 1.5f, 1.2f, 0.0f};
    float light2Diff[] = {0.95f, 0.93f, 0.98f, 1.0f};
    float light2Amb[]  = {0.55f, 0.53f, 0.60f, 1.0f};
    float light2Spec[] = {1.00f, 1.00f, 1.00f, 1.0f};
    glLightfv(GL_LIGHT2, GL_POSITION, light2Pos);
    glLightfv(GL_LIGHT2, GL_DIFFUSE,  light2Diff);
    glLightfv(GL_LIGHT2, GL_AMBIENT,  light2Amb);
    glLightfv(GL_LIGHT2, GL_SPECULAR, light2Spec);

    // Soft purple fill from below — matches the underglow colour
    glEnable(GL_LIGHT3);
    float light3Pos[]  = {0.0f, -1.0f, 0.0f, 0.0f};
    float light3Diff[] = {0.28f, 0.22f, 0.55f, 1.0f};
    float light3Amb[]  = {0.12f, 0.10f, 0.25f, 1.0f};
    float light3Spec[] = {0.10f, 0.10f, 0.20f, 1.0f};
    glLightfv(GL_LIGHT3, GL_POSITION, light3Pos);
    glLightfv(GL_LIGHT3, GL_DIFFUSE,  light3Diff);
    glLightfv(GL_LIGHT3, GL_AMBIENT,  light3Amb);
    glLightfv(GL_LIGHT3, GL_SPECULAR, light3Spec);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Shiny material — gives that smooth 3D render look
    float matSpec[] = {0.80f, 0.78f, 0.90f, 1.0f};
    float matShin[] = {64.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  matSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShin);

    // ── SUBTLE BOTTOM PURPLE/BLUE UNDERGLOW ──────────────────────────────────
    // Small, stays near the bottom edge — like the reference image
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(0.45f, 0.35f, 0.90f, 0.18f + 0.08f * pulse);
    glPushMatrix();
    glTranslatef(0.0f, -0.70f, 0.0f);
    glScalef(1.10f * breathe, 0.35f, 1.10f * breathe);
    glutSolidSphere(0.55f, 18, 12); glPopMatrix();
    // Tighter inner glow
    glColor4f(0.55f, 0.45f, 1.00f, 0.12f + 0.06f * pulse);
    glPushMatrix();
    glTranslatef(0.0f, -0.60f, 0.0f);
    glScalef(0.80f * breathe, 0.22f, 0.80f * breathe);
    glutSolidSphere(0.52f, 16, 10); glPopMatrix();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHTING);

    // ── DOME HEAD ────────────────────────────────────────────────────────────
    glColor4f(bR, bG, bB, 1.0f);
    glPushMatrix();
    glScalef(1.0f * breathe, 1.18f, 1.0f * breathe);
    glutSolidSphere(0.54f, 32, 32);
    glPopMatrix();

    // Bright specular highlight on top-left — the "shiny toy" catch light
    glDisable(GL_LIGHTING);
    glColor4f(1.0f, 1.0f, 1.0f, 0.60f);
    glPushMatrix(); glTranslatef(-0.10f, 0.34f, 0.22f);
    glScalef(0.45f, 0.28f, 0.35f);
    glutSolidSphere(0.22f, 12, 12); glPopMatrix();
    // Smaller secondary catch light
    glColor4f(1.0f, 1.0f, 1.0f, 0.30f);
    glPushMatrix(); glTranslatef(0.10f, 0.28f, 0.24f);
    glScalef(0.25f, 0.16f, 0.20f);
    glutSolidSphere(0.18f, 10, 10); glPopMatrix();
    glEnable(GL_LIGHTING);

    // ── BODY: smooth bell-curve rings (white, shaded darker toward bottom) ───
    for (int ring = 0; ring < 8; ++ring) {
        float t     = (float)ring / 7.0f;
        float ringY = -0.12f - t * 0.68f;
        float ringW = (0.94f + t * 0.22f) * breathe;
        float ringH = 0.20f;
        // Slight lavender tint toward the bottom (like reference)
        float shade = 1.0f - t * 0.10f;
        float rV    = (bR * shade);
        float gV    = (bG * shade);
        float bV    = (bB * shade + t * 0.03f);
        glColor4f(rV, gV, bV, 1.0f);
        glPushMatrix();
        glTranslatef(0.0f, ringY, 0.0f);
        glScalef(ringW, ringH + 0.14f, ringW);
        glutSolidSphere(0.50f, 24, 24);
        glPopMatrix();
    }

    // ── WAVY SKIRT BOTTOM (white curtain folds, like the reference) ──────────
    {
        int   numScallops = 5;
        float baseY  = -0.86f;
        float scallR = 0.42f * breathe;
        for (int d = 0; d < numScallops; ++d) {
            float ang      = (float)d / numScallops * 6.28318f + swirlOff * 0.15f;
            float dxS      = sin(ang) * scallR;
            float dzS      = cos(ang) * scallR;
            float wave     = 0.06f * sin(ang * 2.0f + timeSec * 1.8f + phaseOff);
            float scShade  = 0.88f + 0.06f * sin(ang + 1.0f);

            // Main rounded lobe — same white as body
            glColor4f(bR * scShade, bG * scShade, bB * (scShade + 0.02f), 1.0f);
            glPushMatrix();
            glTranslatef(dxS, baseY - 0.08f + wave, dzS);
            glScalef(0.90f, 1.55f, 0.90f);
            glutSolidSphere(0.14f, 14, 14);
            glPopMatrix();

            // Drip tip
            glColor4f(bR * (scShade - 0.06f), bG * (scShade - 0.06f), bB * (scShade - 0.02f), 0.95f);
            glPushMatrix();
            glTranslatef(dxS, baseY - 0.28f + wave, dzS);
            glScalef(0.62f, 1.05f, 0.62f);
            glutSolidSphere(0.11f, 12, 12);
            glPopMatrix();

            // In-between bump
            float ang2 = ((float)d + 0.5f) / numScallops * 6.28318f + swirlOff * 0.15f;
            float dx2  = sin(ang2) * (scallR * 0.94f);
            float dz2  = cos(ang2) * (scallR * 0.94f);
            glColor4f(bR * 0.92f, bG * 0.92f, bB * 0.94f, 1.0f);
            glPushMatrix();
            glTranslatef(dx2, baseY + 0.04f, dz2);
            glScalef(0.80f, 0.85f, 0.80f);
            glutSolidSphere(0.13f, 12, 12);
            glPopMatrix();
        }
        // Central skirt disc
        glColor4f(bR * 0.86f, bG * 0.86f, bB * 0.90f, 1.0f);
        glPushMatrix();
        glTranslatef(0.0f, baseY - 0.04f, 0.0f);
        glScalef(0.76f * breathe, 0.28f, 0.76f * breathe);
        glutSolidSphere(0.46f, 20, 20);
        glPopMatrix();
    }

    // ── TWO STUBBY ARMS (same white as body, slightly raised) ────────────────
    {
        float armY    = -0.18f;
        float armWave = 8.0f * sin(timeSec * 2.0f + phaseOff);
        for (int side = -1; side <= 1; side += 2) {
            glPushMatrix();
            glTranslatef(side * 0.54f * breathe, armY, 0.04f);
            glRotatef(side * (25.0f - armWave), 0.0f, 0.0f, 1.0f);
            glRotatef(side * -10.0f, 0.0f, 1.0f, 0.0f);

            glColor4f(bR * 0.95f, bG * 0.94f, bB * 0.97f, 1.0f);
            glPushMatrix(); glScalef(1.25f, 0.72f, 0.88f);
            glutSolidSphere(0.14f, 16, 16); glPopMatrix();

            glColor4f(bR * 0.93f, bG * 0.92f, bB * 0.96f, 1.0f);
            glPushMatrix(); glTranslatef(side * 0.12f, -0.04f, 0.0f);
            glScalef(1.45f, 0.68f, 0.78f);
            glutSolidSphere(0.13f, 14, 14); glPopMatrix();

            glColor4f(bR * 0.96f, bG * 0.95f, bB * 0.98f, 1.0f);
            glPushMatrix(); glTranslatef(side * 0.24f, -0.06f, 0.0f);
            glScalef(0.95f, 0.88f, 0.72f);
            glutSolidSphere(0.14f, 14, 14); glPopMatrix();

            // Arm catch light
            glDisable(GL_LIGHTING);
            glColor4f(1.0f, 1.0f, 1.0f, 0.22f);
            glPushMatrix(); glTranslatef(side * 0.18f, 0.03f, 0.09f);
            glScalef(0.50f, 0.38f, 0.32f);
            glutSolidSphere(0.09f, 8, 8); glPopMatrix();
            glEnable(GL_LIGHTING);

            glPopMatrix();
        }
    }

    // ── TWO LARGE DARK OVAL EYES (solid black, prominent) ────────────────────
    {
        float eZ       = 0.48f;
        float eY       = 0.07f;
        float eyeSpace = 0.18f;

        for (int side = -1; side <= 1; side += 2) {
            float ex = side * eyeSpace;
            glDisable(GL_LIGHTING);

            // Soft white shadow/socket behind eye
            glColor4f(0.75f, 0.73f, 0.82f, 0.50f);
            glPushMatrix(); glTranslatef(ex, eY, eZ - 0.03f);
            glScalef(0.92f, 1.20f, 0.32f);
            glutSolidSphere(0.16f, 14, 14); glPopMatrix();

            // Main eye — deep black oval
            glColor4f(0.04f, 0.03f, 0.05f, 1.0f);
            glPushMatrix(); glTranslatef(ex, eY, eZ);
            glScalef(0.80f, 1.10f, 0.52f);
            glutSolidSphere(0.13f, 18, 18); glPopMatrix();

            // Inner void
            glColor4f(0.01f, 0.00f, 0.02f, 1.0f);
            glPushMatrix(); glTranslatef(ex, eY - 0.01f, eZ - 0.01f);
            glScalef(0.58f, 0.88f, 0.28f);
            glutSolidSphere(0.10f, 12, 12); glPopMatrix();

            // Tiny white eye gleam (top-right of each eye)
            glColor4f(1.0f, 1.0f, 1.0f, 0.85f);
            glPushMatrix(); glTranslatef(ex + side * 0.03f, eY + 0.05f, eZ + 0.04f);
            glScalef(0.28f, 0.22f, 0.18f);
            glutSolidSphere(0.07f, 8, 8); glPopMatrix();

            glEnable(GL_LIGHTING);
        }
    }

    // ── SOFT BACK-RIM GLOW (subtle purple, NOT a giant blob) ─────────────────
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(0.60f, 0.50f, 1.0f, 0.07f + 0.04f * pulse);
    glPushMatrix(); glTranslatef(0.0f, 0.0f, -0.20f);
    glScalef(1.08f * breathe, 1.18f, 0.55f);
    glutSolidSphere(0.58f, 16, 16); glPopMatrix();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ── HIT FLASH: red tint ring ─────────────────────────────────────────────
    if (hurtFlash > 0.0f) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 0.1f, 0.1f, 0.55f * hurtFlash);
        glPushMatrix(); glScalef(1.12f * breathe, 1.20f, 1.12f * breathe);
        glutSolidSphere(0.56f, 18, 18); glPopMatrix();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // ── HEALTH DAMAGE CRACKS ─────────────────────────────────────────────────
    if (healthR < 0.75f) {
        int cracks = (int)((1.0f - healthR) * 5.0f);
        for (int c = 0; c < cracks; ++c) {
            float ca   = (float)c / 5.0f * 6.28318f + phaseOff;
            float cx   = sin(ca) * 0.30f;
            float cy   = cos(ca) * 0.20f + 0.05f;
            float czVal = 0.54f * 0.54f - cx * cx - cy * cy;
            float cz   = czVal > 0.0f ? sqrt(czVal) * 0.90f : 0.0f;
            glColor4f(0.30f, 0.15f, 0.40f, 0.65f * (1.0f - healthR));
            glPushMatrix(); glTranslatef(cx, cy, cz);
            glScalef(2.2f, 0.45f, 0.28f);
            glutSolidSphere(0.06f, 6, 6); glPopMatrix();
        }
    }

    // ── CLEANUP ──────────────────────────────────────────────────────────────
    glDisable(GL_LIGHT2);
    glDisable(GL_LIGHT3);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
}

void renderGhostBoss() {
    if (!isGroundLevel()) return;

    float timeSec = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;

    for (int g = 0; g < NUM_GHOSTS; ++g) {
        if (!ghosts[g].alive) continue;

        float ph   = ghosts[g].phaseOff;
        float sw   = ghosts[g].swirlOff;
        float hf   = ghosts[g].hitFlash > 0.0f ? (ghosts[g].hitFlash / 0.25f) : 0.0f;

        float floatY  = 1.30f
                      + 0.22f * sin(timeSec * 1.8f + ph)
                      + 0.08f * sin(timeSec * 4.3f + ph + 0.8f);
        float shake   = hf * 0.14f * sin(timeSec * 40.0f + ph);
        float tiltFwd = 4.5f * sin(timeSec * 1.8f + ph);
        float tiltSide= 3.0f * sin(timeSec * 1.2f + ph + 0.5f);

        // Face ghost toward player
        float toPlayerX = camX - ghosts[g].x;
        float toPlayerZ = camZ - ghosts[g].z;
        float faceAngle = atan2(toPlayerX, -toPlayerZ) * 57.29578f;

        glPushMatrix();
        glTranslatef(ghosts[g].x + shake, floatY, ghosts[g].z);
        glRotatef(faceAngle, 0.0f, 1.0f, 0.0f);
        glRotatef(tiltFwd,  1.0f, 0.0f, 0.0f);
        glRotatef(tiltSide, 0.0f, 0.0f, 1.0f);
        glScalef(1.8f, 1.8f, 1.8f);  // scale up for visibility

        // Ground shadow (larger, softer)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float invS = 1.0f / 1.8f;
        glPushMatrix();
        glScalef(invS, invS, invS);
        glRotatef(-tiltSide, 0.0f, 0.0f, 1.0f);
        glRotatef(-tiltFwd,  1.0f, 0.0f, 0.0f);
        glRotatef(-faceAngle, 0.0f, 1.0f, 0.0f);
        glTranslatef(0.0f, -floatY + 0.03f, 0.0f);
        glColor4f(0.0f, 0.0f, 0.0f, 0.28f);
        glScalef(0.85f, 0.04f, 0.70f);
        glutSolidSphere(0.60f, 18, 8);
        glPopMatrix();
        // Outer soft shadow ring
        glPushMatrix();
        glScalef(invS, invS, invS);
        glRotatef(-tiltSide, 0.0f, 0.0f, 1.0f);
        glRotatef(-tiltFwd,  1.0f, 0.0f, 0.0f);
        glRotatef(-faceAngle, 0.0f, 1.0f, 0.0f);
        glTranslatef(0.0f, -floatY + 0.02f, 0.0f);
        glColor4f(0.0f, 0.0f, 0.0f, 0.10f);
        glScalef(1.20f, 0.03f, 1.0f);
        glutSolidSphere(0.60f, 16, 6);
        glPopMatrix();
        glDisable(GL_BLEND);

        drawOneGhost(ph, sw, hf, ghosts[g].health);

        glPopMatrix();
    }
}

// =============================================
// HELD GUN RENDERING (replaces sword for Level 4)
// =============================================

void renderHeldGun() {
    if (!(gameState == STATE_PLAYING && isGroundLevel() && weaponClaimed)) {
        return;
    }

    float forwardX = cos(pitch) * sin(yaw);
    float forwardY = sin(pitch);
    float forwardZ = -cos(pitch) * cos(yaw);

    float rightX = cos(yaw);
    float rightZ = sin(yaw);

    float upX = -sin(pitch) * sin(yaw);
    float upY = cos(pitch);
    float upZ = sin(pitch) * cos(yaw);

    float recoil = gunRecoilTimer > 0.0f ? (gunRecoilTimer / 0.15f) : 0.0f;
    float recoilKick = sin(recoil * 3.14159f) * 0.06f;

    float baseX = camX + forwardX * 0.55f + rightX * 0.28f + upX * -0.22f;
    float baseY = camY + forwardY * 0.55f + upY * -0.22f;
    float baseZ = camZ + forwardZ * 0.55f + rightZ * 0.28f + upZ * -0.22f;

    // Pull back on recoil
    baseX -= forwardX * recoilKick;
    baseY -= forwardY * recoilKick;
    baseZ -= forwardZ * recoilKick;

    glPushMatrix();
    glTranslatef(baseX, baseY, baseZ);
    glRotatef((-yaw * 57.29578f) + 10.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(pitch * 57.29578f - 8.0f, 1.0f, 0.0f, 0.0f);

    // Gun body (main receiver)
    glColor3f(0.15f, 0.15f, 0.17f);
    drawBox(0.0f, 0.0f, 0.0f, 0.06f, 0.06f, 0.28f);

    // Gun barrel
    glColor3f(0.20f, 0.20f, 0.22f);
    drawBox(0.0f, 0.015f, 0.22f, 0.035f, 0.035f, 0.20f);

    // Gun slide top
    glColor3f(0.18f, 0.18f, 0.20f);
    drawBox(0.0f, 0.04f, 0.02f, 0.055f, 0.025f, 0.22f);

    // Grip
    glColor3f(0.28f, 0.18f, 0.10f);
    drawBox(0.0f, -0.08f, -0.06f, 0.05f, 0.12f, 0.06f);

    // Trigger
    glColor3f(0.12f, 0.12f, 0.14f);
    drawBox(0.0f, -0.02f, 0.04f, 0.015f, 0.04f, 0.02f);

    // Muzzle flash
    if (muzzleFlashTimer > 0.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        float flashIntensity = muzzleFlashTimer / 0.08f;
        glColor4f(1.0f, 0.9f, 0.3f, 0.8f * flashIntensity);
        glPushMatrix();
        glTranslatef(0.0f, 0.015f, 0.35f);
        glutSolidSphere(0.04f * flashIntensity, 8, 8);
        glPopMatrix();
        glColor4f(1.0f, 0.5f, 0.1f, 0.4f * flashIntensity);
        glPushMatrix();
        glTranslatef(0.0f, 0.015f, 0.38f);
        glutSolidSphere(0.07f * flashIntensity, 8, 8);
        glPopMatrix();
        glDisable(GL_BLEND);
    }

    glPopMatrix();
}

void renderKeys() {
    float spin = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.08f;
    for (int i = 0; i < keysRequired(); ++i) {
        if (keyTaken[i]) continue;

        const Vec2& keyPos = levelKeyPositions[currentLevel][i];
        float keyY = levelKeyHeights[currentLevel][i];
        glPushMatrix();
        glTranslatef(keyPos.x, keyY, keyPos.z);
        glRotatef(spin + i * 25.0f, 0.0f, 1.0f, 0.0f);
        renderKeyModel(isGroundLevel() ? 0.60f : 0.50f);
        glPopMatrix();
    }
}

void renderRoomLevel() {
    float halfSize = worldHalfSize();

    // Dark stone floor
    glColor3f(0.25f, 0.23f, 0.22f);
    drawBox(0.0f, -0.10f, 0.0f, halfSize * 2.0f, 0.20f, halfSize * 2.0f);

    // Floor tiles pattern
    glColor3f(0.22f, 0.20f, 0.19f);
    drawBox(0.0f, -0.06f, 0.0f, halfSize * 1.95f, 0.03f, halfSize * 1.95f);

    // Dark ceiling
    glColor3f(0.08f, 0.08f, 0.10f);
    drawBox(0.0f, 4.0f, 0.0f, halfSize * 2.0f, 0.20f, halfSize * 2.0f);

    // Dungeon walls - darker, more stone-like
    float wallR = 0.22f - 0.02f * static_cast<float>(currentLevel);
    float wallG = 0.20f - 0.02f * static_cast<float>(currentLevel);
    float wallB = 0.25f - 0.01f * static_cast<float>(currentLevel);
    if (wallR < 0.15f) wallR = 0.15f;
    if (wallG < 0.14f) wallG = 0.14f;
    if (wallB < 0.18f) wallB = 0.18f;
    glColor3f(wallR, wallG, wallB);

    drawBox(-halfSize + WALL_THICKNESS * 0.5f, 2.0f, 0.0f, WALL_THICKNESS, 4.0f, halfSize * 2.0f);
    drawBox(halfSize - WALL_THICKNESS * 0.5f, 2.0f, 0.0f, WALL_THICKNESS, 4.0f, halfSize * 2.0f);
    drawBox(0.0f, 2.0f, halfSize - WALL_THICKNESS * 0.5f, halfSize * 2.0f, 4.0f, WALL_THICKNESS);

    float doorWidth = currentDoorWidth();
    float doorZ = currentDoorZ();
    float sideWidth = (halfSize * 2.0f - doorWidth) * 0.5f;
    float leftCenterX = -halfSize + sideWidth * 0.5f;
    float rightCenterX = halfSize - sideWidth * 0.5f;

    drawBox(leftCenterX, 2.0f, doorZ, sideWidth, 4.0f, WALL_THICKNESS);
    drawBox(rightCenterX, 2.0f, doorZ, sideWidth, 4.0f, WALL_THICKNESS);
    drawBox(0.0f, 3.5f, doorZ, doorWidth, 1.0f, WALL_THICKNESS);

    drawRoomLights();

    // ---- ENVIRONMENT DECORATIONS ----

    // 🔥 Torches at four corners
    renderTorch(-halfSize + 0.6f, 2.5f, -halfSize + 0.6f, 0);
    renderTorch( halfSize - 0.6f, 2.5f, -halfSize + 0.6f, 1);
    renderTorch(-halfSize + 0.6f, 2.5f,  halfSize - 0.6f, 2);
    renderTorch( halfSize - 0.6f, 2.5f,  halfSize - 0.6f, 3);

    // 🌋 Lava cracks in the floor
    renderLavaCracks();

    // ⛓️ Wall chains on far wall
    renderWallChains();

    // 🪨 Stone corner pillars
    renderCornerPillars();

    // 💀 Skull ornaments on side walls
    renderSkullOrnaments();

    // 🏔️ Ceiling stalactites
    renderStalactites();
}

void renderGroundLevel() {
    float halfSize = worldHalfSize();

    glColor3f(0.18f, 0.50f, 0.18f);
    drawBox(0.0f, -0.12f, 0.0f, halfSize * 2.0f, 0.24f, halfSize * 2.0f);

    glColor3f(0.14f, 0.40f, 0.15f);
    drawBox(0.0f, -0.07f, 0.0f, halfSize * 1.70f, 0.05f, halfSize * 1.70f);

    glColor3f(0.52f, 0.41f, 0.24f);
    drawBox(0.0f, -0.03f, -1.0f, 2.35f, 0.04f, halfSize * 1.78f);
    drawBox(0.0f, -0.03f, 8.9f, 1.55f, 0.04f, 5.2f);
    drawBox(0.0f, -0.03f, -11.9f, 1.75f, 0.04f, 5.8f);

    glColor3f(0.88f, 0.86f, 0.55f);
    drawBox(6.7f, 5.9f, -6.8f, 0.9f, 0.9f, 0.9f);

    glColor3f(0.33f, 0.26f, 0.15f);
    drawBox(-halfSize + 0.15f, 1.0f, 0.0f, 0.25f, 2.0f, halfSize * 2.0f);
    drawBox(halfSize - 0.15f, 1.0f, 0.0f, 0.25f, 2.0f, halfSize * 2.0f);
    drawBox(0.0f, 1.0f, halfSize - 0.15f, halfSize * 2.0f, 2.0f, 0.25f);

    float doorWidth = currentDoorWidth();
    float doorZ = currentDoorZ();
    float sideWidth = (halfSize * 2.0f - doorWidth) * 0.5f;

    glColor3f(0.40f, 0.32f, 0.22f);
    drawBox(-halfSize + sideWidth * 0.5f, 1.0f, doorZ, sideWidth, 2.0f, 0.25f);
    drawBox(halfSize - sideWidth * 0.5f, 1.0f, doorZ, sideWidth, 2.0f, 0.25f);
    drawBox(0.0f, 2.0f, doorZ, doorWidth, 0.65f, 0.25f);

    glColor3f(0.48f, 0.36f, 0.20f);
    drawBox(0.0f, -0.02f, doorZ + 0.85f, 2.2f, 0.04f, 1.4f);
    drawBox(0.0f, -0.02f, doorZ + 3.20f, 2.5f, 0.04f, 1.5f);

    glColor3f(0.85f, 0.86f, 0.90f);
    drawBox(-5.8f, 1.0f, 2.2f, 0.9f, 2.0f, 0.9f);
    drawBox(5.4f, 1.0f, -2.0f, 1.1f, 2.0f, 1.1f);
    drawBox(0.5f, 1.0f, -4.8f, 1.5f, 2.0f, 1.5f);
}

void drawHUDBar(float x, float y, float w, float h, float ratio, float r, float g, float b) {
    ratio = clampf(ratio, 0.0f, 1.0f);
    glColor3f(0.15f, 0.15f, 0.15f);
    drawFilledRect2D(x, y, w, h);
    glColor3f(r, g, b);
    drawFilledRect2D(x + 2.0f, y + 2.0f, (w - 4.0f) * ratio, h - 4.0f);
}

void drawCongratsAnimation() {
    if (!congratsActive) return;

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    float t = congratsTimer;
    float prog = t / CONGRATS_DURATION;  // 0 -> 1 over full duration

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    // ── Dark semi-transparent overlay ────────────────────────────────────────
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float overlayAlpha = clampf(t * 1.5f, 0.0f, 0.72f);
    glColor4f(0.01f, 0.02f, 0.06f, overlayAlpha);
    drawFilledRect2D(0, 0, (float)w, (float)h);

    // ── FIREWORKS (20 bursts at random positions, each with coloured rays) ───
    srand(42);  // fixed seed so positions are consistent every frame
    for (int fw = 0; fw < 20; ++fw) {
        float fwX  = ((float)(rand() % 1000) / 1000.0f) * w;
        float fwY  = ((float)(rand() % 800) / 800.0f) * h * 0.85f + h * 0.10f;
        float fwDelay = ((float)(rand() % 100)) / 100.0f * CONGRATS_DURATION * 0.7f;
        float fwT  = t - fwDelay;
        if (fwT < 0.0f) { rand(); rand(); rand(); continue; }
        float fwLife = fmod(fwT, 1.2f);  // each burst repeats every 1.2 s
        float fwProg = fwLife / 1.2f;
        float radius = fwProg * 90.0f;
        float alpha  = (1.0f - fwProg) * 0.9f;

        // Random colour per firework
        float cr = 0.4f + ((float)(rand() % 60)) / 100.0f;
        float cg = 0.4f + ((float)(rand() % 60)) / 100.0f;
        float cb = 0.4f + ((float)(rand() % 60)) / 100.0f;

        int rays = 12;
        for (int r = 0; r < rays; ++r) {
            float ang  = (float)r / rays * 6.28318f;
            float rx   = fwX + sin(ang) * radius;
            float ry   = fwY + cos(ang) * radius;
            glColor4f(cr, cg, cb, alpha);
            // Draw a small dot at ray tip
            drawFilledRect2D(rx - 3.0f, ry - 3.0f, 6.0f, 6.0f);
            // Draw line from centre to tip (as thin rect)
            glColor4f(cr * 0.7f, cg * 0.7f, cb * 0.7f, alpha * 0.5f);
            glBegin(GL_LINES);
            glVertex2f(fwX, fwY);
            glVertex2f(rx, ry);
            glEnd();
        }
        // Bright centre flash
        float flashAlpha = clampf((0.15f - fwProg) * 8.0f, 0.0f, 1.0f);
        glColor4f(1.0f, 1.0f, 0.9f, flashAlpha);
        drawFilledRect2D(fwX - 8.0f, fwY - 8.0f, 16.0f, 16.0f);
    }
    srand(static_cast<unsigned int>(time(nullptr)));  // restore random seed

    // ── SPINNING STARS around the text ────────────────────────────────────────
    float cx = w * 0.5f;
    float cy = h * 0.54f;
    for (int s = 0; s < 8; ++s) {
        float sang  = (float)s / 8.0f * 6.28318f + t * 2.5f;
        float srad  = 160.0f + 20.0f * sin(t * 3.0f + s);
        float sx    = cx + sin(sang) * srad;
        float sy    = cy + cos(sang) * srad * 0.55f;  // ellipse
        float salpha= 0.7f + 0.3f * sin(t * 4.0f + s);
        // Star colour cycles
        float sc = fmod(t * 0.8f + (float)s / 8.0f, 1.0f);
        glColor4f(sc, 1.0f - sc * 0.5f, 0.2f, salpha);
        // Draw a small 4-point star shape
        drawFilledRect2D(sx - 6.0f, sy - 2.0f, 12.0f, 4.0f);
        drawFilledRect2D(sx - 2.0f, sy - 6.0f, 4.0f, 12.0f);
    }

    // ── BIG GOLDEN GLOW behind text ───────────────────────────────────────────
    float textAlpha = clampf(t * 2.0f, 0.0f, 1.0f);
    float glowPulse = 0.6f + 0.4f * sin(t * 4.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(1.0f, 0.85f, 0.1f, 0.12f * glowPulse * textAlpha);
    drawFilledRect2D(cx - 280.0f, cy - 10.0f, 560.0f, 90.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ── CONGRATULATIONS text (scale up from 0 over first 0.5s) ───────────────
    float scale = clampf(t / 0.5f, 0.0f, 1.0f);
    // Simulate scale via raster pos offset (GLUT bitmap text can't scale, so we
    // stack 3 shifted copies for a bold/shadow look)
    if (scale > 0.0f) {
        // Shadow
        glColor4f(0.0f, 0.0f, 0.0f, 0.8f * textAlpha);
        drawText2DCentered(cx + 3.0f, cy + 3.0f, "CONGRATULATIONS!");
        // Gold text
        glColor4f(1.0f, 0.88f, 0.10f, textAlpha);
        drawText2DCentered(cx,       cy,       "CONGRATULATIONS!");
        drawText2DCentered(cx - 1.0f, cy + 1.0f, "CONGRATULATIONS!");
    }

    // ── Sub-text lines ────────────────────────────────────────────────────────
    if (t > 0.7f) {
        float subAlpha = clampf((t - 0.7f) * 2.5f, 0.0f, 1.0f);
        glColor4f(0.85f, 1.0f, 0.85f, subAlpha);
        drawText2DCentered(cx, cy - 38.0f, "You escaped all 4 levels and defeated all 6 ghosts!");
        glColor4f(0.7f, 0.9f, 1.0f, subAlpha);
        drawText2DCentered(cx, cy - 68.0f, "A true survivor. Your name will be remembered.");
    }

    // ── Countdown bar at bottom ───────────────────────────────────────────────
    if (t > 1.0f) {
        float barAlpha = clampf((t - 1.0f) * 2.0f, 0.0f, 0.9f);
        glColor4f(0.2f, 0.2f, 0.3f, barAlpha * 0.7f);
        drawFilledRect2D(cx - 150.0f, 28.0f, 300.0f, 14.0f);
        float remaining = 1.0f - prog;
        glColor4f(0.4f, 1.0f, 0.5f, barAlpha);
        drawFilledRect2D(cx - 148.0f, 30.0f, 296.0f * remaining, 10.0f);
        glColor4f(0.7f, 0.8f, 0.7f, barAlpha);
        drawText2DCentered(cx, 50.0f, "Saving score...");
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawHUD() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    char line1[180];
    char line2[180];
    char timerLine[64];
    int timeSeconds = static_cast<int>(levelTimeRemaining);
    if (timeSeconds < 0) timeSeconds = 0;
    int minutes = timeSeconds / 60;
    int seconds = timeSeconds % 60;
    snprintf(timerLine, sizeof(timerLine), "Time: %02d:%02d", minutes, seconds);

    snprintf(line1, sizeof(line1), "Story: Escape the rooms, then survive the ground. Level %d (%s)", currentLevel + 1, levelNames[currentLevel]);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText2D(15.0f, static_cast<float>(h - 25), line1);

    if (gameState == STATE_PLAYING) {
        if (isGroundLevel()) {
            snprintf(line2, sizeof(line2), "Ghosts: %d/%d alive   Gun: %s   Medkit: %s   Health: %d/%d",
                ghostHealth, GHOST_MAX_HEALTH * NUM_GHOSTS,
                weaponClaimed ? "Claimed" : "Not Claimed",
                (!healthPickupClaimed ? "Not Claimed" : (medkitUsed ? "Used" : "Ready")),
                playerLives, MAX_LIVES);
        } else {
            snprintf(line2, sizeof(line2), "Keys: %d / %d   Obstacles: %d   Health: %d/%d   Difficulty: %s", keysCollected, keysRequired(), obstacleCount(), playerLives, MAX_LIVES, levelNames[currentLevel]);
        }
        glColor3f(0.95f, 0.95f, 0.7f);
        drawText2D(15.0f, static_cast<float>(h - 52), line2);

        if (isGroundLevel()) {
            if (groundIntroActive) {
                glColor3f(0.95f, 0.95f, 0.7f);
                drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Read all rules, then press ENTER to start this ground fight.");
            } else if (!weaponClaimed) {
                glColor3f(1.0f, 0.95f, 0.2f);
                drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Claim the gun first (Press E near gun). Then left-click to shoot ghosts.");
            } else if (ghostAlive) {
                int aliveCount = 0;
                for (int g = 0; g < NUM_GHOSTS; ++g) if (ghosts[g].alive) aliveCount++;
                char objMsg[96];
                snprintf(objMsg, sizeof(objMsg), "Objective: Shoot all 6 ghosts with LEFT CLICK! (%d remaining)", aliveCount);
                glColor3f(1.0f, 0.78f, 0.35f);
                drawText2D(15.0f, static_cast<float>(h - 80), objMsg);
            } else if (!doorOpening) {
                glColor3f(0.7f, 1.0f, 0.7f);
                drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Ghost defeated. Escape door is unlocking.");
            } else {
                glColor3f(0.7f, 1.0f, 1.0f);
                drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Run through the opened escape door to complete the game.");
            }
        } else if (keysCollected < keysRequired()) {
            glColor3f(1.0f, 0.95f, 0.2f);
            drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Find keys (some are above obstacles). Press E near key. Space = jump.");
        } else if (!doorOpening) {
            glColor3f(0.7f, 1.0f, 0.7f);
            drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Go to the door and press E to unlock this level.");
        } else {
            glColor3f(0.7f, 1.0f, 1.0f);
            drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Walk through the opened exit.");
        }

        glColor3f(1.0f, 1.0f, 1.0f);
        drawText2DRightAligned(static_cast<float>(w - 15), static_cast<float>(h - 25), timerLine);
        drawHUDBar(static_cast<float>(w - 250), static_cast<float>(h - 55), 220.0f, 18.0f, static_cast<float>(playerLives) / static_cast<float>(MAX_LIVES), 0.25f, 0.95f, 0.35f);
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText2D(static_cast<float>(w - 250), static_cast<float>(h - 75), "Health");

        if (isGroundLevel() && healthPickupClaimed && !medkitUsed && !groundIntroActive) {
            float medkitRatio = medkitHoldTimer / MEDKIT_HOLD_REQUIRED;
            if (medkitRatio < 0.0f) medkitRatio = 0.0f;
            if (medkitRatio > 1.0f) medkitRatio = 1.0f;
            drawHUDBar(static_cast<float>(w - 250), static_cast<float>(h - 102), 220.0f, 14.0f, medkitRatio, 0.95f, 0.25f, 0.25f);
            glColor3f(1.0f, 1.0f, 1.0f);
            drawText2D(static_cast<float>(w - 250), static_cast<float>(h - 118), "Hold 4 to use medkit");
        }

        if (isGroundLevel() && !groundIntroActive) {
            // Count alive ghosts
            int aliveCount = 0;
            for (int g = 0; g < NUM_GHOSTS; ++g) if (ghosts[g].alive) aliveCount++;

            if (aliveCount > 0) {
                float ghostRatio = (float)aliveCount / (float)NUM_GHOSTS;
                float barW = 320.0f;
                float barX = static_cast<float>(w) * 0.5f - barW * 0.5f;
                drawHUDBar(barX, static_cast<float>(h - 55), barW, 16.0f, ghostRatio, 0.88f, 0.20f, 0.22f);
                char ghostMsg[64];
                snprintf(ghostMsg, sizeof(ghostMsg), "GHOSTS: %d / %d REMAINING", aliveCount, NUM_GHOSTS);
                glColor3f(1.0f, 0.82f, 0.82f);
                drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h - 74), ghostMsg);

                // Individual ghost HP dots
                float dotSpacing = barW / NUM_GHOSTS;
                for (int g = 0; g < NUM_GHOSTS; ++g) {
                    float dotX = barX + dotSpacing * g + dotSpacing * 0.5f;
                    float dotY = static_cast<float>(h - 88);
                    if (ghosts[g].alive) {
                        float hpRatio = (float)ghosts[g].health / (float)GHOST_MAX_HEALTH;
                        glColor3f(1.0f - hpRatio, hpRatio * 0.8f, 0.2f);
                    } else {
                        glColor3f(0.25f, 0.25f, 0.25f);
                    }
                    drawFilledRect2D(dotX - 6.0f, dotY, 12.0f, 8.0f);
                }
            }
        }

        // ---- Bonus Clock HUD (Level 3+) ----
        if (currentLevel >= 2 && !isGroundLevel()) {
            if (bonusClockActive) {
                float dist = distance2D(camX, camZ, bonusClockX, bonusClockZ);
                char clockMsg[80];
                snprintf(clockMsg, sizeof(clockMsg), ">> BONUS CLOCK nearby (%.1fm) <<", dist);
                glColor3f(0.2f, 1.0f, 0.4f);
                drawText2DCentered(static_cast<float>(w) * 0.5f, 40.0f, clockMsg);
            } else {
                char clockMsg[80];
                snprintf(clockMsg, sizeof(clockMsg), "Next clock in: %.1fs", bonusClockSpawnTimer);
                glColor3f(0.6f, 0.8f, 0.6f);
                drawText2DCentered(static_cast<float>(w) * 0.5f, 40.0f, clockMsg);
            }
        }

        // "+8 SEC!" floating banner
        if (bonusClockBannerTimer > 0.0f) {
            float alpha = bonusClockBannerTimer / 1.5f;
            float yOffset = (1.5f - bonusClockBannerTimer) * 40.0f;
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0.1f, 1.0f, 0.3f, alpha);
            drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.35f + yOffset, "+8 SEC!");
            glDisable(GL_BLEND);
        }

        // Crosshair for gun (ground level with weapon)
        if (isGroundLevel() && weaponClaimed) {
            glColor3f(1.0f, 0.3f, 0.3f);
            float cx = static_cast<float>(w) * 0.5f;
            float cy = static_cast<float>(h) * 0.5f;
            drawFilledRect2D(cx - 8.0f, cy - 1.0f, 16.0f, 2.0f);
            drawFilledRect2D(cx - 1.0f, cy - 8.0f, 2.0f, 16.0f);
        }
    }

    if (gameState == STATE_PLAYING) {
        float doorDist = distance2D(camX, camZ, 0.0f, currentDoorZ());
        if (doorDist < 3.0f) {
            if (!doorOpening) {
                if (isGroundLevel()) {
                    glColor3f(1.0f, 0.3f, 0.3f);
                    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.45f, "ESCAPE DOOR SEALED");
                    if (ghostAlive) {
                        int aliveCount = 0;
                        for (int g = 0; g < NUM_GHOSTS; ++g) if (ghosts[g].alive) aliveCount++;
                        char ghostMsg[64];
                        snprintf(ghostMsg, sizeof(ghostMsg), "Defeat all %d ghosts to unlock the door. (%d remain)", NUM_GHOSTS, aliveCount);
                        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, ghostMsg);
                    } else {
                        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, "Ghost down. Door mechanism is unlocking...");
                    }
                } else if (keysCollected < keysRequired()) {
                    glColor3f(1.0f, 0.3f, 0.3f);
                    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.45f, "DOOR LOCKED");
                    char msg[64];
                    snprintf(msg, sizeof(msg), "Need %d more key(s)!", keysRequired() - keysCollected);
                    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, msg);
                } else {
                    glColor3f(1.0f, 0.8f, 0.2f);
                    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.45f, "DOOR LOCKED (Keys Acquired)");

                    if (doorDist < 1.65f) {
                        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, "Press E to Insert Keys & Unlock");
                    } else {
                        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, "Walk closer to unlock");
                    }
                }
            } else if (doorAngle < 80.0f) {
                glColor3f(0.3f, 1.0f, 0.3f);
                drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.45f, isGroundLevel() ? "ESCAPE DOOR UNLOCKED!" : "DOOR UNLOCKED!");
                drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, "Opening...");
            }
        }
    }

    if (levelBannerFrames > 0 && gameState == STATE_PLAYING) {
        char levelBanner[128];
        snprintf(levelBanner, sizeof(levelBanner), "LEVEL %d - %s", currentLevel + 1, levelNames[currentLevel]);
        glColor3f(0.4f, 1.0f, 0.9f);
        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.54f, levelBanner);
    }

    if (damageCooldown > 0.0f && gameState == STATE_PLAYING) {
        // Red flashing effect for getting hurt
        float alpha = (damageCooldown / 1.5f) * 0.5f;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0f, 0.0f, 0.0f, alpha);

        drawFilledRect2D(0, 0, static_cast<float>(w), static_cast<float>(h));

        glColor3f(1.0f, 1.0f, 1.0f);
        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.40f, "HEALTH COMPROMISED!");
        glDisable(GL_BLEND);
    }

    if (gameCompleted && gameState == STATE_NAME_ENTRY) {
        glColor3f(0.4f, 1.0f, 0.4f);
        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.52f, "YOU COMPLETED EASY + MEDIUM + HARD + GROUND.");
        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.47f, "A NEW CHAPTER IS FINISHED. PRESS ESC TO QUIT.");
    }

    if (isGroundLevel() && gameState == STATE_PLAYING && groundIntroActive) {
        float panelW = static_cast<float>(w) * 0.72f;
        float panelH = static_cast<float>(h) * 0.72f;
        float panelX = (static_cast<float>(w) - panelW) * 0.5f;
        float panelY = (static_cast<float>(h) - panelH) * 0.5f;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.02f, 0.04f, 0.06f, 0.80f);
        drawFilledRect2D(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));

        glColor4f(0.08f, 0.12f, 0.18f, 0.95f);
        drawFilledRect2D(panelX, panelY, panelW, panelH);
        glColor4f(0.18f, 0.44f, 0.58f, 1.0f);
        drawFilledRect2D(panelX + 4.0f, panelY + panelH - 6.0f, panelW - 8.0f, 2.5f);

        glDisable(GL_BLEND);

        float y = panelY + panelH - 48.0f;
        glColor3f(0.64f, 0.94f, 1.0f);
        drawText2DCentered(static_cast<float>(w) * 0.5f, y, "GROUND LEVEL RULES - FINAL FIGHT");

        y -= 55.0f;
        glColor3f(0.95f, 0.95f, 0.90f);
        drawText2D(panelX + 28.0f, y, "1) Claim the gun first (Press E near the gun pickup)."); y -= 34.0f;
        drawText2D(panelX + 28.0f, y, "2) Shoot ghosts with LEFT MOUSE CLICK (aim at each ghost)."); y -= 34.0f;
        drawText2D(panelX + 28.0f, y, "3) 6 GHOSTS must be defeated - each takes 4 hits!"); y -= 34.0f;
        drawText2D(panelX + 28.0f, y, "4) Claim medkit (Press E near medkit), then HOLD 4 to use it."); y -= 34.0f;
        drawText2D(panelX + 28.0f, y, "5) Ghost contact damages you - keep moving and shoot!"); y -= 34.0f;
        drawText2D(panelX + 28.0f, y, "6) After ALL ghosts die, door opens. Run through to WIN!"); y -= 46.0f;

        glColor3f(0.45f, 1.0f, 0.65f);
        drawText2DCentered(static_cast<float>(w) * 0.5f, y, "PRESS ENTER TO START GROUND BATTLE");
    }

    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawRulesScreen() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    float panelX = static_cast<float>(w) * 0.12f;
    float panelY = static_cast<float>(h) * 0.08f;
    float panelW = static_cast<float>(w) * 0.76f;
    float panelH = static_cast<float>(h) * 0.84f;

    glColor3f(0.12f, 0.05f, 0.05f);
    drawFilledRect2D(panelX, panelY, panelW, panelH);
    glColor3f(0.35f, 0.15f, 0.15f);
    drawFilledRect2D(panelX + 4.0f, panelY + panelH - 6.0f, panelW - 8.0f, 2.5f);

    glColor3f(1.0f, 0.45f, 0.45f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + panelH - 42.0f, "GAME RULES - SURVIVAL PROTOCOL");

    float y = panelY + panelH - 82.0f;
    glColor3f(0.95f, 0.95f, 0.88f);
    drawText2D(panelX + 30.0f, y, "1) Each level has a timer. Collect all keys before time runs out!"); y -= 30.0f;
    drawText2D(panelX + 30.0f, y, "2) Find and collect all keys, then unlock the door."); y -= 30.0f;
    glColor3f(1.0f, 0.35f, 0.35f);
    drawText2D(panelX + 30.0f, y, "3) DO NOT TOUCH OBSTACLES - they cost you health!"); y -= 22.0f;
    drawText2D(panelX + 45.0f, y, "Walls move in Medium and Hard, but at manageable speed."); y -= 30.0f;
    glColor3f(0.95f, 0.95f, 0.88f);
    drawText2D(panelX + 30.0f, y, "4) Press E to interact (Pick up keys, unlock doors)."); y -= 30.0f;
    drawText2D(panelX + 30.0f, y, "5) Press SPACE to jump over gaps and obstacles."); y -= 30.0f;
    glColor3f(0.3f, 1.0f, 0.5f);
    drawText2D(panelX + 30.0f, y, "6) [Level 3+] Bonus clocks spawn! Walk over for +8 seconds."); y -= 30.0f;
    glColor3f(0.95f, 0.95f, 0.88f);
    drawText2D(panelX + 30.0f, y, "7) Level 4 (Ground): Pick up the GUN and SHOOT the ghost!"); y -= 30.0f;
    drawText2D(panelX + 30.0f, y, "8) Dungeon rooms have torches, lava cracks, and skulls."); y -= 30.0f;
    drawText2D(panelX + 30.0f, y, "9) Mouse to look around. WASD to move."); y -= 40.0f;

    glColor3f(0.45f, 1.0f, 0.45f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, y, "PRESS ENTER TO START YOUR ESCAPE");

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawMenuScreen() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    float t = static_cast<float>(glutGet(GLUT_ELAPSED_TIME));

    if (startScreenTexture == 0) {
        glBegin(GL_QUADS);
        glColor3f(0.03f, 0.07f, 0.11f); glVertex2f(0.0f, 0.0f);
        glColor3f(0.03f, 0.07f, 0.11f); glVertex2f(static_cast<float>(w), 0.0f);
        glColor3f(0.02f, 0.03f, 0.08f); glVertex2f(static_cast<float>(w), static_cast<float>(h));
        glColor3f(0.02f, 0.03f, 0.08f); glVertex2f(0.0f, static_cast<float>(h));
        glEnd();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float glowX = static_cast<float>(w) * 0.5f + sin(t * 0.0011f) * static_cast<float>(w) * 0.25f;
        float glowY = static_cast<float>(h) * 0.72f + cos(t * 0.0013f) * 36.0f;
        glColor4f(0.10f, 0.60f, 0.85f, 0.12f);
        drawFilledRect2D(glowX - 220.0f, glowY - 100.0f, 440.0f, 200.0f);
        glColor4f(0.05f, 0.35f, 0.60f, 0.10f);
        drawFilledRect2D(glowX - 330.0f, glowY - 170.0f, 660.0f, 340.0f);
        glDisable(GL_BLEND);
    }

    if (startScreenTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, startScreenTexture);
        glColor3f(1.0f, 1.0f, 1.0f);

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, h);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(w, h);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(w, 0.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 0.0f);
        glEnd();

        glDisable(GL_TEXTURE_2D);

        // Add a dark semi-transparent overlay over the image so it looks slightly blurry/dim
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.05f, 0.05f, 0.1f, 0.70f);
        drawFilledRect2D(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
        glDisable(GL_BLEND);
    }

    float panelX = static_cast<float>(w) * 0.22f;
    float panelY = static_cast<float>(h) * 0.15f;
    float panelW = static_cast<float>(w) * 0.56f;
    float panelH = static_cast<float>(h) * 0.72f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.04f, 0.09f, 0.13f, 0.88f);
    drawFilledRect2D(panelX - 8.0f, panelY - 8.0f, panelW + 16.0f, panelH + 16.0f);
    glDisable(GL_BLEND);

    glColor3f(0.08f, 0.14f, 0.18f);
    drawFilledRect2D(panelX, panelY, panelW, panelH);
    glColor3f(0.15f, 0.28f, 0.34f);
    drawFilledRect2D(panelX + 4.0f, panelY + panelH - 6.0f, panelW - 8.0f, 2.5f);

    glColor3f(0.52f, 1.0f, 0.80f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + panelH - 72.0f, "3D ESCAPE ROOM");
    glColor3f(0.85f, 0.93f, 0.98f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + panelH - 106.0f, "Rooms, climbing, timer pressure, and final survival.");

    float buttonW = panelW * 0.60f;
    float buttonH = 46.0f;
    float buttonX = panelX + (panelW - buttonW) * 0.5f;
    float firstButtonY = panelY + panelH * 0.52f;

    for (int i = 0; i < MENU_ITEMS; ++i) {
        float y = firstButtonY - i * 72.0f;
        bool selected = (i == menuSelection);

        glColor3f(selected ? 0.11f : 0.08f, selected ? 0.42f : 0.18f, selected ? 0.36f : 0.23f);
        drawFilledRect2D(buttonX, y, buttonW, buttonH);

        glColor3f(selected ? 0.55f : 0.24f, selected ? 0.98f : 0.45f, selected ? 0.85f : 0.52f);
        drawFilledRect2D(buttonX + 2.0f, y + buttonH - 5.0f, buttonW - 4.0f, 2.0f);

        if (selected) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0.20f, 0.92f, 0.72f, 0.18f);
            drawFilledRect2D(buttonX - 3.0f, y - 3.0f, buttonW + 6.0f, buttonH + 6.0f);
            glDisable(GL_BLEND);
        }

        if (selected) {
            glColor3f(0.95f, 1.0f, 0.96f);
            drawText2D(buttonX + 18.0f, y + 29.0f, "> ");
        }

        glColor3f(selected ? 0.95f : 0.82f, selected ? 1.0f : 0.90f, selected ? 0.95f : 0.95f);
        drawText2DCentered(buttonX + buttonW * 0.5f + 12.0f, y + 29.0f, menuLabels[i]);
    }

    glColor3f(0.70f, 0.78f, 0.82f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + 38.0f, "Use W/S or Up/Down to navigate. Press Enter to select.");

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawHighScoreScreen() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    float panelX = static_cast<float>(w) * 0.14f;
    float panelY = static_cast<float>(h) * 0.12f;
    float panelW = static_cast<float>(w) * 0.72f;
    float panelH = static_cast<float>(h) * 0.76f;

    glColor3f(0.07f, 0.12f, 0.19f);
    drawFilledRect2D(panelX, panelY, panelW, panelH);
    glColor3f(0.14f, 0.34f, 0.45f);
    drawFilledRect2D(panelX + 4.0f, panelY + panelH - 6.0f, panelW - 8.0f, 2.5f);

    glColor3f(0.4f, 1.0f, 0.9f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + panelH - 48.0f, "HIGH SCORES");

    float colRank = panelX + 44.0f;
    float colName = panelX + 110.0f;
    float colScore = panelX + panelW * 0.52f;
    float colLives = panelX + panelW * 0.70f;
    float colBonus = panelX + panelW * 0.84f;

    glColor3f(0.78f, 0.9f, 0.98f);
    drawText2D(colRank, panelY + panelH - 92.0f, "#");
    drawText2D(colName, panelY + panelH - 92.0f, "NAME");
    drawText2D(colScore, panelY + panelH - 92.0f, "SCORE");
    drawText2D(colLives, panelY + panelH - 92.0f, "HEALTH");
    drawText2D(colBonus, panelY + panelH - 92.0f, "BONUS");

    glColor3f(0.22f, 0.42f, 0.55f);
    drawFilledRect2D(panelX + 24.0f, panelY + panelH - 107.0f, panelW - 48.0f, 2.0f);

    float y = panelY + panelH - 140.0f;
    if (highScores.empty()) {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText2DCentered(static_cast<float>(w) * 0.5f, y, "No scores yet.");
    } else {
        int count = static_cast<int>(highScores.size());
        if (count > 10) count = 10;
        for (int i = 0; i < count; ++i) {
            if (i % 2 == 0) {
                glColor3f(0.10f, 0.17f, 0.26f);
                drawFilledRect2D(panelX + 18.0f, y - 19.0f, panelW - 36.0f, 28.0f);
            }

            char rankText[8];
            char scoreText[20];
            char livesText[12];
            char bonusText[20];

            snprintf(rankText, sizeof(rankText), "%d", i + 1);
            snprintf(scoreText, sizeof(scoreText), "%d", highScores[i].score);
            snprintf(livesText, sizeof(livesText), "%d", highScores[i].livesLeft);
            snprintf(bonusText, sizeof(bonusText), "%d", highScores[i].bonusTime);

            glColor3f(0.95f, 0.95f, 0.88f);
            drawText2D(colRank, y, rankText);
            drawText2D(colName, y, highScores[i].name.c_str());
            drawText2D(colScore, y, scoreText);
            drawText2D(colLives, y, livesText);
            drawText2D(colBonus, y, bonusText);

            y -= 32.0f;
        }
    }

    glColor3f(0.75f, 0.8f, 0.85f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + 24.0f, "Press Esc to return.");

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawNameEntryScreen() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    glColor3f(0.45f, 1.0f, 0.55f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.66f, "YOU ESCAPED!");

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.59f, "Enter your name for the high score list:");

    char nameLine[48];
    snprintf(nameLine, sizeof(nameLine), "%s_", playerNameLength == 0 ? "" : playerName);
    glColor3f(0.9f, 0.95f, 0.45f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.50f, nameLine);

    char scoreLine[120];
    snprintf(scoreLine, sizeof(scoreLine), "Current Score: %d", pendingScore);
    glColor3f(0.85f, 0.85f, 0.85f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, scoreLine);

    glColor3f(0.75f, 0.75f, 0.75f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.28f, "Type letters, Backspace deletes, Enter saves.");

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawGameOverScreen() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    glColor3f(1.0f, 0.35f, 0.35f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.60f, "GAME OVER");
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.52f, "All health is gone.");
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.45f, "Press M for menu or Esc to quit.");

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void renderWorld() {
    if (isGroundLevel()) {
        renderGroundLevel();
    } else {
        renderRoomLevel();
    }

    renderObstacles();
    renderGroundPickups();
    renderGhostBoss();
    renderKeys();
    renderDoorOrGate();
    renderHeldGun();

    // Render bonus clock if active
    if (currentLevel >= 2 && !isGroundLevel()) {
        renderBonusClock();
    }
}

void display() {
    if (gameState == STATE_GAME_OVER) {
        glClearColor(0.07f, 0.02f, 0.02f, 1.0f);
    } else if (gameState == STATE_NAME_ENTRY) {
        glClearColor(0.04f, 0.10f, 0.06f, 1.0f);
    } else if (gameState == STATE_HIGHSCORES) {
        glClearColor(0.05f, 0.05f, 0.12f, 1.0f);
    } else if (gameState == STATE_MENU) {
        glClearColor(0.06f, 0.08f, 0.09f, 1.0f);
    } else if (gameCompleted) {
        glClearColor(0.04f, 0.10f, 0.06f, 1.0f);
    } else if (isGroundLevel()) {
        glClearColor(0.52f, 0.74f, 0.95f, 1.0f);
    } else {
        // Darker dungeon atmosphere
        glClearColor(0.04f, 0.04f, 0.06f, 1.0f);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (gameState == STATE_PLAYING) {
        float lookX = cos(pitch) * sin(yaw);
        float lookY = sin(pitch);
        float lookZ = -cos(pitch) * cos(yaw);

        gluLookAt(camX, camY, camZ,
                  camX + lookX, camY + lookY, camZ + lookZ,
                  0.0f, 1.0f, 0.0f);

        // Set up atmospheric lighting for room levels
        if (!isGroundLevel()) {
            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);

            // Dim ambient for dungeon feel
            float ambient[] = {0.15f, 0.12f, 0.10f, 1.0f};
            float diffuse[] = {0.6f, 0.5f, 0.3f, 1.0f};
            float lightPos[] = {0.0f, 3.5f, 0.0f, 1.0f};
            glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
            glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

            glEnable(GL_COLOR_MATERIAL);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        }

        renderWorld();

        if (!isGroundLevel()) {
            glDisable(GL_LIGHTING);
            glDisable(GL_LIGHT0);
            glDisable(GL_COLOR_MATERIAL);
        }

        drawHUD();
        drawCongratsAnimation();
    } else {
        if (gameState == STATE_RULES) {
            drawRulesScreen();
        } else if (gameState == STATE_MENU) {
            drawMenuScreen();
        } else if (gameState == STATE_HIGHSCORES) {
            drawHighScoreScreen();
        } else if (gameState == STATE_NAME_ENTRY) {
            drawNameEntryScreen();
        } else if (gameState == STATE_GAME_OVER) {
            drawGameOverScreen();
        }
    }

    glutSwapBuffers();
}

void tryInteraction() {
    if (gameState != STATE_PLAYING) return;

    if (isGroundLevel()) {
        bool interacted = false;

        if (!weaponClaimed && distance2D(camX, camZ, weaponPickupPos.x, weaponPickupPos.z) < INTERACT_PICKUP_RANGE) {
            weaponClaimed = true;
            interacted = true;
            cout << "Gun claimed. You can now shoot with left mouse click.\n";
        }

        if (!healthPickupClaimed && distance2D(camX, camZ, healthPickupPos.x, healthPickupPos.z) < INTERACT_PICKUP_RANGE) {
            healthPickupClaimed = true;
            interacted = true;
            cout << "Medkit claimed. Hold key 4 to use it when needed.\n";
        }

        if (!interacted) {
            float doorDistance = distance2D(camX, camZ, 0.0f, currentDoorZ());
            if (doorDistance < 1.8f && !doorOpening) {
                cout << "Escape door is sealed. Defeat the ghost first.\n";
            }
        }
        return;
    }

    bool foundKey = false;
    for (int i = 0; i < keysRequired(); ++i) {
        if (keyTaken[i]) continue;

        const Vec2& keyPos = levelKeyPositions[currentLevel][i];
        float keyY = levelKeyHeights[currentLevel][i];

        float dist2D_ = distance2D(camX, camZ, keyPos.x, keyPos.z);
        float distY = fabs(camY - PLAYER_EYE_HEIGHT - keyY);

        if (dist2D_ < 1.5f && distY < 2.0f) {
            keyTaken[i] = true;
            keysCollected++;
            foundKey = true;
            cout << "Key " << keysCollected << " collected in level " << (currentLevel + 1) << "\n";
        }
    }

    if (foundKey) return;

    float doorDistance = distance2D(camX, camZ, 0.0f, currentDoorZ());
    if (keysCollected >= keysRequired() && !doorOpening && doorDistance < 1.65f) {
        doorOpening = true;
        cout << "Exit unlocked for level " << (currentLevel + 1) << "\n";
    } else if (doorDistance < 1.65f && keysCollected < keysRequired()) {
        cout << "Exit locked. Need " << (keysRequired() - keysCollected) << " more key(s).\n";
    }
}

void tryGhostAttack() {
    if (!(gameState == STATE_PLAYING && isGroundLevel() && !groundIntroActive)) {
        return;
    }

    if (!weaponClaimed) {
        cout << "You need the gun first. Claim it before attacking.\n";
        return;
    }

    if (weaponAttackCooldown > 0.0f) {
        cout << "Gun is reloading. Wait a moment.\n";
        return;
    }

    // Always trigger gun effects
    gunRecoilTimer      = 0.15f;
    muzzleFlashTimer    = 0.08f;
    weaponAttackCooldown= 0.40f;

    float dirX = sin(yaw);
    float dirZ = -cos(yaw);

    // Find the best-aimed alive ghost within range
    int   bestGhost = -1;
    float bestDot   = 0.65f;   // minimum cos angle (~49 deg cone)

    for (int g = 0; g < NUM_GHOSTS; ++g) {
        if (!ghosts[g].alive) continue;

        float toGX = ghosts[g].x - camX;
        float toGZ = ghosts[g].z - camZ;
        float dist = sqrt(toGX * toGX + toGZ * toGZ);
        if (dist > GHOST_WEAPON_RANGE) continue;

        float invD = (dist > 0.0001f) ? (1.0f / dist) : 0.0f;
        float dot  = dirX * (toGX * invD) + dirZ * (toGZ * invD);
        if (dot > bestDot) {
            bestDot   = dot;
            bestGhost = g;
        }
    }

    if (bestGhost < 0) {
        cout << "Shot missed - aim at a ghost!\n";
        return;
    }

    ghosts[bestGhost].health--;
    ghosts[bestGhost].hitFlash = 0.25f;
    ghostHitFlashTimer = 0.25f;   // legacy var for any leftover HUD usage
    cout << "Hit ghost " << bestGhost << "! HP: " << ghosts[bestGhost].health << "\n";

    if (ghosts[bestGhost].health <= 0) {
        ghosts[bestGhost].alive  = false;
        ghosts[bestGhost].health = 0;
        cout << "Ghost " << bestGhost << " defeated!\n";
    }

    // Check if ALL ghosts are dead
    bool anyAlive = false;
    for (int g = 0; g < NUM_GHOSTS; ++g) {
        if (ghosts[g].alive) { anyAlive = true; break; }
    }
    if (!anyAlive) {
        ghostAlive = false;
        groundDoorUnlockedByBoss = true;
        doorOpening = true;
        cout << "All 6 ghosts defeated! Escape door unlocking...\n";
    }
}

void keyboardDown(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    unsigned char lower = static_cast<unsigned char>(tolower(key));
    if (gameState == STATE_PLAYING) {
        if (isGroundLevel() && groundIntroActive) {
            if (lower == 13 || lower == 'e') {
                groundIntroActive = false;
                cout << "Ground battle started. Good luck.\n";
            } else if (lower == 27) {
                startMenu();
            }
            return;
        }

        keyStates[lower] = true;

        if (lower == 'e') {
            tryInteraction();
        }

        if (key == ' ' && !airborne) {
            airborne = true;
            verticalVelocity = JUMP_VELOCITY;
            fallStartCamY = camY;
        }

        if (lower == 27) {
            startMenu();
        }
        return;
    }

    if (gameState == STATE_MENU) {
        if (lower == 'w' || lower == 'k') {
            menuSelection = (menuSelection + MENU_ITEMS - 1) % MENU_ITEMS;
        } else if (lower == 's' || lower == 'j') {
            menuSelection = (menuSelection + 1) % MENU_ITEMS;
        } else if (lower == 13 || lower == 'e') {
            if (menuSelection == 0) {
                gameState = STATE_RULES;
                resetInputStates();
            } else if (menuSelection == 1) {
                openHighScoreScreen();
            } else {
                exit(0);
            }
        } else if (lower == 27) {
            exit(0);
        }
        return;
    }

    if (gameState == STATE_RULES) {
        if (lower == 13 || lower == 'e') {
            startGame();
        } else if (lower == 27) {
            startMenu();
        }
        return;
    }

    if (gameState == STATE_HIGHSCORES) {
        if (lower == 27 || lower == 'm') {
            startMenu();
        }
        return;
    }

    if (gameState == STATE_NAME_ENTRY) {
        if (lower == 13) {
            string name = playerNameLength == 0 ? string("PLAYER") : string(playerName);
            addHighScore(name, pendingScore, playerLives, bonusTimeScore);
            startMenu();
            return;
        }

        if (lower == 8) {
            if (playerNameLength > 0) {
                playerName[--playerNameLength] = '\0';
            }
            return;
        }

        if (isalpha(lower) && playerNameLength < 15) {
            playerName[playerNameLength++] = static_cast<char>(toupper(lower));
            playerName[playerNameLength] = '\0';
        }
        return;
    }

    if (gameState == STATE_GAME_OVER) {
        if (lower == 'm' || lower == 27) {
            startMenu();
        }
        return;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    unsigned char lower = static_cast<unsigned char>(tolower(key));
    if (gameState == STATE_PLAYING) {
        keyStates[lower] = false;
    }
}

void specialKeyDown(int key, int x, int y) {
    (void)x;
    (void)y;

    if (key >= 0 && key < 256) {
        specialStates[key] = true;
    }
}

void specialKeyUp(int key, int x, int y) {
    (void)x;
    (void)y;

    if (key >= 0 && key < 256) {
        specialStates[key] = false;
    }
}

void mouseLook(int x, int y) {
    if (gameState != STATE_PLAYING) {
        lastMouseX = x;
        lastMouseY = y;
        return;
    }

    if (!mouseInitialized) {
        lastMouseX = x;
        lastMouseY = y;
        mouseInitialized = true;
        return;
    }

    int deltaX = x - lastMouseX;
    int deltaY = y - lastMouseY;

    lastMouseX = x;
    lastMouseY = y;

    yaw += static_cast<float>(deltaX) * MOUSE_SENSITIVITY;
    pitch -= static_cast<float>(deltaY) * MOUSE_SENSITIVITY;
    pitch = clampf(pitch, -PITCH_LIMIT, PITCH_LIMIT);

    glutPostRedisplay();
}

void mouseButton(int button, int state, int x, int y) {
    (void)x;
    (void)y;

    if (gameState != STATE_PLAYING || groundIntroActive) {
        return;
    }

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && isGroundLevel()) {
        tryGhostAttack();
    }
}

void applyMovement(float dx, float dz) {
    float targetX = camX + dx;
    float targetZ = camZ + dz;

    if (canMoveTo(targetX, camZ)) {
        camX = targetX;
    }

    if (canMoveTo(camX, targetZ)) {
        camZ = targetZ;
    }
}

void updateVerticalMovement(float dt) {
    if (!airborne && fabs(verticalVelocity) < 0.0001f) {
        float supportHeight = supportSurfaceHeightAt(camX, camZ);
        float feetHeight = camY - PLAYER_EYE_HEIGHT;
        if (feetHeight > supportHeight + 0.03f) {
            airborne = true;
            verticalVelocity = 0.0f;
            fallStartCamY = camY;
        }
    }

    if (airborne || fabs(verticalVelocity) > 0.0001f) {
        verticalVelocity -= GRAVITY_ACCEL * dt;
        camY += verticalVelocity * dt;

        float supportHeight = supportSurfaceHeightAt(camX, camZ);
        float feetHeight = camY - PLAYER_EYE_HEIGHT;
        if (verticalVelocity <= 0.0f && feetHeight <= supportHeight) {
            camY = supportHeight + PLAYER_EYE_HEIGHT;
            float fallDrop = fallStartCamY - camY;
            airborne = false;
            verticalVelocity = 0.0f;

            if (fallDrop > FALL_DAMAGE_THRESHOLD) {
                cout << "Bad fall detected (drop: " << fallDrop << ").\n";
                damagePlayer(1, true);
            }
        }
    }
}

void handleLevelExit() {
    if (currentLevel < TOTAL_LEVELS - 1) {
        bonusTimeScore += static_cast<int>(levelTimeRemaining);
        cout << "Level " << (currentLevel + 1) << " complete. Moving to next level.\n";
        startLevel(currentLevel + 1);
    } else {
        bonusTimeScore += static_cast<int>(levelTimeRemaining);
        congratsActive = true;
        congratsTimer  = 0.0f;
        gameCompleted  = true;
        // Don't immediately go to name entry – let congratsTimer play for CONGRATS_DURATION
        cout << "All levels complete! Congratulations!\n";
    }
}

void update(int value) {
    (void)value;

    int now = glutGet(GLUT_ELAPSED_TIME);
    if (lastTickMs == 0) {
        lastTickMs = now;
    }
    float dt = static_cast<float>(now - lastTickMs) / 1000.0f;
    lastTickMs = now;

    if (gameState == STATE_PLAYING) {
        if (congratsActive) {
            congratsTimer += dt;
            if (congratsTimer >= CONGRATS_DURATION) {
                congratsActive = false;
                gameState = STATE_NAME_ENTRY;
                resetInputStates();
                finishGameToHighScore();
            }
            glutPostRedisplay();
            glutTimerFunc(16, update, 0);
            return;
        }

        if (swordSwingTimer > 0.0f) {
            swordSwingTimer -= dt;
            if (swordSwingTimer < 0.0f) {
                swordSwingTimer = 0.0f;
            }
        }

        if (gunRecoilTimer > 0.0f) {
            gunRecoilTimer -= dt;
            if (gunRecoilTimer < 0.0f) gunRecoilTimer = 0.0f;
        }

        if (muzzleFlashTimer > 0.0f) {
            muzzleFlashTimer -= dt;
            if (muzzleFlashTimer < 0.0f) muzzleFlashTimer = 0.0f;
        }

        if (ghostHitFlashTimer > 0.0f) {
            ghostHitFlashTimer -= dt;
            if (ghostHitFlashTimer < 0.0f) {
                ghostHitFlashTimer = 0.0f;
            }
        }

        if (damageCooldown > 0.0f) {
            damageCooldown -= dt;
            if (damageCooldown < 0.0f) {
                damageCooldown = 0.0f;
            }
        }

        if (weaponAttackCooldown > 0.0f) {
            weaponAttackCooldown -= dt;
            if (weaponAttackCooldown < 0.0f) {
                weaponAttackCooldown = 0.0f;
            }
        }

        // Bonus clock banner timer
        if (bonusClockBannerTimer > 0.0f) {
            bonusClockBannerTimer -= dt;
            if (bonusClockBannerTimer < 0.0f) bonusClockBannerTimer = 0.0f;
        }

        if (isGroundLevel() && groundIntroActive) {
            if (levelBannerFrames > 0) {
                levelBannerFrames--;
            }
            glutPostRedisplay();
            glutTimerFunc(16, update, 0);
            return;
        }

        levelTimeRemaining -= dt;
        if (levelTimeRemaining <= 0.0f) {
            damagePlayer(1, true);
            if (gameState == STATE_PLAYING) {
                cout << "Time expired. Resetting timer and reducing health.\n";
                levelTimeRemaining = levelTimeLimitByLevel[currentLevel];
            }
            glutPostRedisplay();
            glutTimerFunc(16, update, 0);
            return;
        }

        // ---- Bonus Clock Logic (Level 3+, room levels only) ----
        if (currentLevel >= 2 && !isGroundLevel()) {
            if (!bonusClockActive) {
                bonusClockSpawnTimer -= dt;
                if (bonusClockSpawnTimer <= 0.0f) {
                    spawnBonusClock();
                    bonusClockSpawnTimer = randFloat(5.0f, 11.0f);
                }
            } else {
                // Check if player walks over clock
                float clockDist = distance2D(camX, camZ, bonusClockX, bonusClockZ);
                if (clockDist < BONUS_CLOCK_RADIUS) {
                    levelTimeRemaining += BONUS_CLOCK_TIME_BONUS;
                    bonusClockActive = false;
                    bonusClockBannerTimer = 1.5f;
                    bonusClockSpawnTimer = randFloat(5.0f, 11.0f);
                    cout << "+8 seconds! Time remaining: " << levelTimeRemaining << "\n";
                }
            }
        }

        float halfSize = worldHalfSize();
        for (int i = 0; i < obstacleCount(); ++i) {
            Obstacle& obs = activeObstacles[i];
            obs.x += obs.dx * dt;
            obs.z += obs.dz * dt;

            // Bounce off walls
            if (obs.x + obs.sx * 0.5f > halfSize || obs.x - obs.sx * 0.5f < -halfSize) {
                obs.dx = -obs.dx;
                obs.x += obs.dx * dt;
            }
            if (obs.z + obs.sz * 0.5f > halfSize || obs.z - obs.sz * 0.5f < -halfSize) {
                obs.dz = -obs.dz;
                obs.z += obs.dz * dt;
            }
        }

        if (isGroundLevel() && ghostAlive) {
            // Update each individual ghost
            for (int g = 0; g < NUM_GHOSTS; ++g) {
                if (!ghosts[g].alive) continue;

                // Tick per-ghost hit flash
                if (ghosts[g].hitFlash > 0.0f) {
                    ghosts[g].hitFlash -= dt;
                    if (ghosts[g].hitFlash < 0.0f) ghosts[g].hitFlash = 0.0f;
                }

                float dxToPlayer = camX - ghosts[g].x;
                float dzToPlayer = camZ - ghosts[g].z;
                float distToPlayer = sqrt(dxToPlayer * dxToPlayer + dzToPlayer * dzToPlayer);

                if (distToPlayer > 0.01f) {
                    float invDist = 1.0f / distToPlayer;
                    float perpX = -dzToPlayer * invDist;
                    float perpZ =  dxToPlayer * invDist;
                    float strafeFactor = sin(static_cast<float>(now) * 0.004f + ghosts[g].phaseOff) * 0.80f;

                    float chaseX = dxToPlayer * invDist;
                    float chaseZ = dzToPlayer * invDist;
                    float desiredX = chaseX * 0.82f + perpX * strafeFactor * 0.55f;
                    float desiredZ = chaseZ * 0.82f + perpZ * strafeFactor * 0.55f;
                    float desiredLen = sqrt(desiredX * desiredX + desiredZ * desiredZ);
                    if (desiredLen > 0.0001f) { desiredX /= desiredLen; desiredZ /= desiredLen; }

                    float speedBoost = weaponClaimed ? 1.20f : 1.0f;
                    float step = GHOST_MOVE_SPEED * speedBoost * dt;
                    if (step > distToPlayer) step = distToPlayer;

                    ghosts[g].x += desiredX * step;
                    ghosts[g].z += desiredZ * step;

                    float minBound = -worldHalfSize() + WALL_THICKNESS + GHOST_BODY_RADIUS + 0.2f;
                    float maxBound =  worldHalfSize() - WALL_THICKNESS - GHOST_BODY_RADIUS - 0.2f;
                    ghosts[g].x = clampf(ghosts[g].x, minBound, maxBound);
                    ghosts[g].z = clampf(ghosts[g].z, minBound, maxBound);
                }

                // Touch damage
                if (distance2D(camX, camZ, ghosts[g].x, ghosts[g].z) < GHOST_TOUCH_DAMAGE_RANGE) {
                    damagePlayer(1, false);
                }
            }

            // Sync legacy scalar vars used by HUD / door logic
            ghostAlive  = false;
            ghostHealth = 0;
            for (int g = 0; g < NUM_GHOSTS; ++g) {
                if (ghosts[g].alive) {
                    ghostAlive = true;
                    ghostHealth += ghosts[g].health;
                }
            }
        }

        if (isGroundLevel() && groundDoorUnlockedByBoss && !doorOpening) {
            doorOpening = true;
        }

        if (isGroundLevel() && healthPickupClaimed && !medkitUsed) {
            if (keyStates['4']) {
                medkitHoldTimer += dt;
                if (medkitHoldTimer >= MEDKIT_HOLD_REQUIRED) {
                    medkitUsed = true;
                    medkitHoldTimer = 0.0f;
                    int oldHealth = playerLives;
                    playerLives += 2;
                    if (playerLives > MAX_LIVES) {
                        playerLives = MAX_LIVES;
                    }
                    cout << "Medkit used. Health: " << oldHealth << " -> " << playerLives << "\n";
                }
            } else {
                medkitHoldTimer = 0.0f;
            }
        }

        float forwardX = sin(yaw);
        float forwardZ = -cos(yaw);
        float rightX = cos(yaw);
        float rightZ = sin(yaw);

        float inputX = 0.0f;
        float inputZ = 0.0f;

        if (keyStates['w']) inputZ += 1.0f;
        if (keyStates['s']) inputZ -= 1.0f;
        if (keyStates['a']) inputX -= 1.0f;
        if (keyStates['d']) inputX += 1.0f;

        if (inputX != 0.0f || inputZ != 0.0f) {
            float len = sqrt(inputX * inputX + inputZ * inputZ);
            inputX /= len;
            inputZ /= len;

            float moveSpeed = currentMoveSpeed() * (dt / 0.016666f);

            float dx = (inputZ * forwardX + inputX * rightX) * moveSpeed;
            float dz = (inputZ * forwardZ + inputX * rightZ) * moveSpeed;

            applyMovement(dx, dz);
        }

        updateVerticalMovement(dt);

        if (gameState != STATE_PLAYING) {
            glutPostRedisplay();
            glutTimerFunc(16, update, 0);
            return;
        }

        if (specialStates[GLUT_KEY_LEFT]) yaw -= ROT_SPEED;
        if (specialStates[GLUT_KEY_RIGHT]) yaw += ROT_SPEED;
        if (specialStates[GLUT_KEY_UP]) pitch += PITCH_SPEED;
        if (specialStates[GLUT_KEY_DOWN]) pitch -= PITCH_SPEED;

        pitch = clampf(pitch, -PITCH_LIMIT, PITCH_LIMIT);

        if (doorOpening && doorAngle < 92.0f) {
            float openSpeed = 1.0f + static_cast<float>(currentLevel) * 0.7f;
            doorAngle += openSpeed;
            if (doorAngle > 92.0f) {
                doorAngle = 92.0f;
            }
        }

        if (doorAngle > 80.0f && camZ < (currentDoorZ() - 0.4f)) {
            handleLevelExit();
        }

        if (levelBannerFrames > 0) {
            levelBannerFrames--;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void reshape(int w, int h) {
    if (h == 0) {
        h = 1;
    }

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70.0, static_cast<double>(w) / static_cast<double>(h), 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    srand(static_cast<unsigned int>(time(nullptr)));
    loadHighScores();
    loadBackgroundTexture();
    playerLives = MAX_LIVES;
    gameOver = false;
    gameCompleted = false;
    initTorchFlickers();
    startMenu();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(900, 650);
    glutCreateWindow("3D Escape Room - Multi Level");

    init();
    reshape(900, 650);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    glutMouseFunc(mouseButton);
    glutPassiveMotionFunc(mouseLook);
    glutMotionFunc(mouseLook);
    glutTimerFunc(16, update, 0);

    cout << "Escape room ready. Use the menu to start.\n";
    cout << "Controls: WASD move, Mouse look, E interact, Left click shoot (ground), Hold 4 medkit, Space jump, Esc menu.\n";

    glutMainLoop();
    return 0;
}

