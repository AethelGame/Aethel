#include <iostream>
#include <thread>
#include <atomic>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>

#include <system/Variables.h>
#include <system/CrashHandler.h>
#include <BaseState.h>
#include <states/MainMenuState.h>
#include <utils/InfoStackManager.h>
#include <objects/debug/FPSCounter.h>
#include <objects/debug/DebugInfo.h>
#include <utils/Utils.h>
#include "system/InputQueue.h"
#include "system/Renderer2D.h"
#include "system/TextRenderer.h"
#include <system/AudioManager.h>

#include <objects/ActionBar.h>
#include <objects/actions/ActionTest.h>
#include <objects/actions/ActionClock.h>

BaseState *state = nullptr;
void* statePayload = nullptr;

int curState = -1;
int prevState = -1;

int WINDOW_WIDTH = 1600;
int WINDOW_HEIGHT = 900;
int FRAMERATE_CAP = -1;

double lastFrameTime = 0.0;
std::chrono::high_resolution_clock::time_point programStartTime;

std::atomic<bool> inputThreadRunning{true};
InputQueue globalInputQueue;

GLFWwindow* sharedWindow = nullptr;
std::mutex windowMutex;

void GLAPIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, 
                               GLenum severity, GLsizei length, 
                               const char *message, const void *userParam)
{
    if (type == GL_DEBUG_TYPE_ERROR) {
        GAME_LOG_ERROR("GL Error: " + std::string(message));
    }
}

void glfwErrorCallback(int error, const char* description)
{
    GAME_LOG_ERROR("GLFW Error " + std::to_string(error) + ": " + description);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;
}

void inputPollingThread() {
    GAME_LOG_DEBUG("Input thread started");
    
    while (inputThreadRunning.load()) {
        std::lock_guard<std::mutex> lock(windowMutex);
        
        if (!sharedWindow) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
    }
    
    GAME_LOG_DEBUG("Input thread stopped");
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    TimedInputEvent event;
    event.timestamp = std::chrono::high_resolution_clock::now();
    event.timeSeconds = std::chrono::duration<double>(event.timestamp - programStartTime).count();
    event.key = key;
    event.scancode = scancode;
    event.mods = mods;
    
    if (action == GLFW_PRESS) {
        event.type = TimedInputEvent::KEY_DOWN;
    } else if (action == GLFW_RELEASE) {
        event.type = TimedInputEvent::KEY_UP;
    } else {
        return;
    }
    
    globalInputQueue.enqueue(event);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    AppContext* app = (AppContext*)glfwGetWindowUserPointer(window);
    
    TimedInputEvent event;
    event.timestamp = std::chrono::high_resolution_clock::now();
    event.timeSeconds = std::chrono::duration<double>(event.timestamp - programStartTime).count();
    event.button = button;
    event.mods = mods;
    
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    
    float scaleX = (float)WINDOW_WIDTH / app->renderWidth;
    float scaleY = (float)WINDOW_HEIGHT / app->renderHeight;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;
    
    int scaledWidth = (int)(app->renderWidth * scale);
    int scaledHeight = (int)(app->renderHeight * scale);
    int offsetX = (WINDOW_WIDTH - scaledWidth) / 2;
    int offsetY = (WINDOW_HEIGHT - scaledHeight) / 2;
    
    event.mouseX = ((float)xpos - offsetX) / scale;
    event.mouseY = ((float)ypos - offsetY) / scale;
    
    if (action == GLFW_PRESS) {
        event.type = TimedInputEvent::MOUSE_BUTTON_DOWN;
    } else if (action == GLFW_RELEASE) {
        event.type = TimedInputEvent::MOUSE_BUTTON_UP;
    }
    
    globalInputQueue.enqueue(event);
}

void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
    AppContext* app = (AppContext*)glfwGetWindowUserPointer(window);
    
    TimedInputEvent event;
    event.timestamp = std::chrono::high_resolution_clock::now();
    event.timeSeconds = std::chrono::duration<double>(event.timestamp - programStartTime).count();
    event.type = TimedInputEvent::MOUSE_MOTION;
    
    float scaleX = (float)WINDOW_WIDTH / app->renderWidth;
    float scaleY = (float)WINDOW_HEIGHT / app->renderHeight;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;
    
    int scaledWidth = (int)(app->renderWidth * scale);
    int scaledHeight = (int)(app->renderHeight * scale);
    int offsetX = (WINDOW_WIDTH - scaledWidth) / 2;
    int offsetY = (WINDOW_HEIGHT - scaledHeight) / 2;
    
    event.mouseX = ((float)xpos - offsetX) / scale;
    event.mouseY = ((float)ypos - offsetY) / scale;
    
    globalInputQueue.enqueue(event);
}

BaseState *createState(int stateID)
{
    switch (stateID)
    {
    case STATE_MAIN_MENU:
        return new MainMenuState();
    default:
        return nullptr;
    }
}

void setState(AppContext* app, int stateID, void* payload)
{
    if (stateID >= 0 && stateID < STATE_COUNT && stateID != curState)
    {
        if (!app->isTransitioning) {
            app->isTransitioning = true;
            app->transitioningOut = true;
            app->transitionProgress = 0.0f;
            app->nextState = stateID;
            app->nextStatePayload = payload;
        }
    }
}

RenderContext* createRenderTarget(int width, int height)
{
    RenderContext* ctx = new RenderContext();
    ctx->width = width;
    ctx->height = height;
    
    glGenFramebuffers(1, &ctx->framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, ctx->framebuffer);
    
    glGenTextures(1, &ctx->colorTexture);
    glBindTexture(GL_TEXTURE_2D, ctx->colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctx->colorTexture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        GAME_LOG_ERROR("Framebuffer is not complete!");
        delete ctx;
        return nullptr;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &ctx->quadVAO);
    glGenBuffers(1, &ctx->quadVBO);
    glBindVertexArray(ctx->quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D screenTexture;
        void main() {
            FragColor = texture(screenTexture, TexCoord);
        }
    )";
    
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    ctx->shaderProgram = glCreateProgram();
    glAttachShader(ctx->shaderProgram, vertexShader);
    glAttachShader(ctx->shaderProgram, fragmentShader);
    glLinkProgram(ctx->shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return ctx;
}

bool setRenderResolution(AppContext *app, float width, float height)
{
    if (app->renderTarget) {
        glDeleteFramebuffers(1, &app->renderTarget->framebuffer);
        glDeleteTextures(1, &app->renderTarget->colorTexture);
        glDeleteVertexArrays(1, &app->renderTarget->quadVAO);
        glDeleteBuffers(1, &app->renderTarget->quadVBO);
        delete app->renderTarget;
    }
    
    app->renderTarget = createRenderTarget((int)width, (int)height);
    if (!app->renderTarget) {
        GAME_LOG_ERROR("Failed to create render target");
        return false;
    }
    
    app->renderWidth = width;
    app->renderHeight = height;
    
    if (app->renderer2D) {
        app->renderer2D->setViewport((int)width, (int)height);
    }
    
    if (app->textRenderer) {
        app->textRenderer->setViewport((int)width, (int)height);
    }
    
    return true;
}

int main(int argc, char* argv[])
{
    InstallCrashHandler("logs");
    Logger::getInstance().setLogLevel(LogLevel::GAME_DEBUG);
    
    programStartTime = std::chrono::high_resolution_clock::now();
    
    glfwSetErrorCallback(glfwErrorCallback);
    
    if (!glfwInit()) {
        GAME_LOG_ERROR("Failed to initialize GLFW");
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GAME_LOG_INFO("GLFW initialized successfully");
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    WINDOW_WIDTH = 1600;
    WINDOW_HEIGHT = 900;
    FRAMERATE_CAP = 999;
    
    std::string title = std::string(GAME_NAME) + " v" + GAME_VERSION;
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title.c_str(), nullptr, nullptr);
    
    if (!window) {
        GAME_LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();

        Logger::getInstance().shutdown();
        return -1;
    }

    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    int xPos = (mode->width - WINDOW_WIDTH) / 2;
    int yPos = (mode->height - WINDOW_HEIGHT) / 2;
    glfwSetWindowPos(window, xPos, yPos);

    Utils::loadWindowIcon(window, "assets/icon.png");
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);

    GAME_LOG_INFO("GLFW window created successfully");
    
    glfwSwapInterval(0);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        GAME_LOG_ERROR("Failed to initialize GLAD");

        Logger::getInstance().shutdown();
        return -1;
    }
    
#ifdef _DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
#endif

    GAME_LOG_INFO("OpenGL initialized successfully");
    
    auto *app = new AppContext();
    app->window = window;
    app->switchState = setState;
    app->inputQueue = &globalInputQueue;

    glfwSetWindowUserPointer(window, app);
    setRenderResolution(app, app->renderWidth, app->renderHeight);
    
    {
        std::lock_guard<std::mutex> lock(windowMutex);
        sharedWindow = window;
    }
    
    app->renderer2D = new Renderer2D();
    if (!app->renderer2D->initialize((int)app->renderWidth, (int)app->renderHeight)) {
        GAME_LOG_ERROR("Failed to initialize 2D renderer");

        Logger::getInstance().shutdown();
        return -1;
    }

    GAME_LOG_INFO("2D Renderer initialized successfully");
    
    app->textRenderer = new TextRenderer((int)app->renderWidth, (int)app->renderHeight);
    if (!app->textRenderer) {
        GAME_LOG_ERROR("Failed to initialize text renderer");
        Logger::getInstance().shutdown();
        return -1;
    }

    GAME_LOG_INFO("Text renderer initialized successfully");

    app->actionBar = new ActionBar(app);
    app->actionBar->addAddon(new ActionTest(app));
    app->actionBar->addAddon(new ActionClock(app));

    app->infoStack = new InfoStackManager(app);
    
    FPSCounter* fpsCounter = new FPSCounter(app->textRenderer, MAIN_FONT_PATH, 16, 8.0f);
    fpsCounter->setAppContext(app);
    app->infoStack->addInfo(fpsCounter);
    app->fpsCounter = fpsCounter;
    
    DebugInfo* debugInfo = new DebugInfo(app->textRenderer, MAIN_FONT_PATH, 16, 8.0f);
    debugInfo->setAppContext(app);
    app->infoStack->addInfo(debugInfo);
    app->debugInfo = debugInfo;

    GAME_LOG_INFO("App context and subsystems initialized successfully");

    if (!AudioManager::getInstance().initialize()) {
        GAME_LOG_ERROR("Failed to initialize audio system");
        return -1;
    }

    GAME_LOG_INFO("Audio system initialized successfully");
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    std::thread inputThread(inputPollingThread);
    GAME_LOG_DEBUG("Initialization successful. Input thread running on separate thread.");
    
    curState = STATE_MAIN_MENU;
    lastFrameTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window) && !app->appQuit)
    {
        glfwPollEvents();

        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastFrameTime);
        lastFrameTime = currentTime;

        AudioManager::getInstance().update(deltaTime);
        
        TimedInputEvent inputEvent;
        while (globalInputQueue.dequeue(inputEvent)) {
            auto now = std::chrono::high_resolution_clock::now();
            double latencyMs = std::chrono::duration<double, std::milli>(now - inputEvent.timestamp).count();
            
            if (latencyMs > 5.0) {
                GAME_LOG_DEBUG("High input latency: " + std::to_string(latencyMs) + "ms");
            }
            
            if (inputEvent.key == GLFW_KEY_ESCAPE && inputEvent.type == TimedInputEvent::KEY_DOWN) {
                app->appQuit = true;
            }
            
            if (state != nullptr) {
                state->handleEvent(inputEvent);
            }
        }
        
        if (app->isTransitioning)
        {
            app->transitionProgress += deltaTime / app->transitionDuration;
            
            if (app->transitioningOut && app->transitionProgress >= 1.0f)
            {
                if (state)
                {
                    state->destroy();
                    delete state;
                    state = nullptr;
                }
                
                curState = app->nextState;
                state = createState(curState);
                prevState = curState;
                
                if (state != nullptr)
                {
                    state->init(app, app->nextStatePayload);
                    app->nextStatePayload = nullptr;
                    app->currentState = state;
                }
                
                app->transitioningOut = false;
                app->transitionProgress = 0.0f;
                
                lastFrameTime = glfwGetTime();
            }
            else if (!app->transitioningOut && app->transitionProgress >= 1.0f)
            {
                app->isTransitioning = false;
                app->transitionProgress = 0.0f;
            }
        }
        
        if (!app->isTransitioning && curState != prevState)
        {
            if (state)
            {
                state->destroy();
                delete state;
                state = nullptr;
            }
            
            state = createState(curState);
            prevState = curState;
            
            if (state != nullptr)
            {
                state->init(app, statePayload);
                statePayload = nullptr;
                app->currentState = state;
            }
        }
        
        if (app->infoStack) {
            app->infoStack->updateAll();
        }

        if (app->actionBar) {
            app->actionBar->update(deltaTime);
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, app->renderTarget->framebuffer);
        glViewport(0, 0, (int)app->renderWidth, (int)app->renderHeight);
        app->renderer2D->clear(Color(0.0f, 0.0f, 0.0f, 1.0f));
        
        if (state != nullptr)
        {
            state->update(deltaTime);
            state->render();
        }

        if (app->actionBar) {
            app->actionBar->render();
        }
        
        if (app->infoStack) {
            app->infoStack->renderAll(app->renderer2D);
        }
        
        if (app->isTransitioning)
        {
            float fadeAlpha;
            if (app->transitioningOut) {
                fadeAlpha = app->transitionProgress;
            } else {
                fadeAlpha = 1.0f - app->transitionProgress;
            }
            
            app->renderer2D->drawRect(0, 0, app->renderWidth, app->renderHeight, 
                                     Color(0, 0, 0, (uint8_t)(fadeAlpha * 255)));
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(app->renderTarget->shaderProgram);
        glBindVertexArray(app->renderTarget->quadVAO);
        glBindTexture(GL_TEXTURE_2D, app->renderTarget->colorTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        if (state != nullptr)
        {
            state->postBuffer();
        }
        
        glfwSwapBuffers(window);
    }
    
    inputThreadRunning.store(false);
    
    {
        std::lock_guard<std::mutex> lock(windowMutex);
        sharedWindow = nullptr;
    }
    
    inputThread.join();
    
    if (state != nullptr)
    {
        state->destroy();
        delete state;
        state = nullptr;
    }
    
    Logger::getInstance().shutdown();
    AudioManager::getInstance().shutdown();
    
    if (app->infoStack) {
        delete app->infoStack;
        app->infoStack = nullptr;
    }

    if (app->actionBar) {
        delete app->actionBar;
        app->actionBar = nullptr;
    }
    
    app->fpsCounter = nullptr;
    app->debugInfo = nullptr;
    
    if (app->renderTarget) {
        glDeleteFramebuffers(1, &app->renderTarget->framebuffer);
        glDeleteTextures(1, &app->renderTarget->colorTexture);
        delete app->renderTarget;
    }
    
    if (app->renderer2D) {
        delete app->renderer2D;
    }
    
    if (app->textRenderer) {
        delete app->textRenderer;
    }
    
    delete app;
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}