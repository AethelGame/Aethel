#pragma once

#include <chrono>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <system/Logger.h>

#define GAME_NAME "Aethel"
#define GAME_VERSION "0.0.1"

enum AppStateID
{
    STATE_MAIN_MENU = 1,
    STATE_COUNT
};

extern int FRAMERATE_CAP;
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

inline const std::string MAIN_FONT_PATH = "assets/fonts/GoogleSansCode-Bold.ttf";

class InfoStackManager;
class FPSCounter;
class DebugInfo;
class BaseState;
class Renderer2D;
class TextRenderer;
class InputQueue;

struct RenderContext {
    GLuint framebuffer;
    GLuint colorTexture;
    GLuint shaderProgram;
    GLuint quadVAO, quadVBO;
    int width, height;
};

struct AppContext;
using StateSwitcher = void (*)(AppContext*, int, void*);

struct AppContext
{
    GLFWwindow* window;
    RenderContext* renderTarget;
    bool appQuit = false;

    Renderer2D* renderer2D = nullptr;
    TextRenderer* textRenderer = nullptr;

    InputQueue* inputQueue = nullptr;

    InfoStackManager* infoStack = nullptr;
    DebugInfo* debugInfo = nullptr;
    FPSCounter* fpsCounter = nullptr;

    StateSwitcher switchState = nullptr;
    BaseState* currentState = nullptr;

    std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();

    float renderY = 0.0f;
    float renderWidth = 1920.0f;
    float renderHeight = 1080.0f;

    int nextState = -1;
    void* nextStatePayload = nullptr;

    bool isTransitioning = false;
    bool transitioningOut = true;

    float transitionProgress = 0.0f;
    float transitionDuration = 0.5f;
};