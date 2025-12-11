#ifndef BASE_STATE_H
#define BASE_STATE_H

#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <system/Variables.h>
#include "system/InputQueue.h"

class BaseState
{
public:
    virtual ~BaseState() {}

    virtual void init(AppContext* appContext, void* payload = nullptr) {
        this->appContext = appContext;
        this->renderer2D = appContext->renderer2D;
        this->textRenderer = appContext->textRenderer;
        
        this->screenWidth_ = appContext->renderWidth;
        this->screenHeight_ = appContext->renderHeight;
    }
    
    virtual void handleEvent(const TimedInputEvent& e) {}
    
    virtual void update(float deltaTime) {}
    virtual void render() {}
    virtual void destroy() {}
    virtual void postBuffer() {}
    
    virtual std::string getName() const { return "BaseState"; }
    
    void requestStateSwitch(int stateID, void* payload = nullptr) {
        if (appContext && appContext->switchState) {
            appContext->switchState(appContext, stateID, payload);
        } else {
            GAME_LOG_ERROR("State switch requested but switchState function is null.");
        }
    }

    virtual std::string getStateName() const {
        return "BaseState";
    }
    
protected:
    AppContext* appContext = nullptr;
    Renderer2D* renderer2D = nullptr;
    TextRenderer* textRenderer = nullptr;

    float screenWidth_ = 0;
    float screenHeight_ = 0;
};

#endif