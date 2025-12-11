#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include <string>
#include "system/TextRenderer.h"
#include "system/Renderer2D.h"

struct AppContext;

class FPSCounter {
public:
    FPSCounter(TextRenderer* textRenderer, const std::string& fontPath, int fontSize, float yPos);
    ~FPSCounter();

    void setAppContext(AppContext* appContext) { appContext_ = appContext; }
    AppContext* getAppContext() const { return appContext_; }

    virtual void update();
    virtual void render(Renderer2D* renderer);

    void setYPosition(float yPos) {
        textObject_->setPosition(8.0f, yPos);
    }

    void setPosition(float x, float y) {
        textObject_->setPosition(x, y);
    }

    float getYPosition() const {
        return textObject_->getAnchorY();
    }

    float getHeight() const {
        return textObject_->getRenderedHeight();
    }
    
protected:
    AppContext* appContext_ = nullptr;

    TextObject* textObject_ = nullptr;
    
    double lastTime_ = 0.0;
    double perfFrequency_ = 0.0;
    
    int frameCount_ = 0;
    double frameAccumulator_ = 0.0;

    float latestFrameTimeMs_ = 0.0f;
    long memoryUsageKb_ = 0;

private:
    long getAppMemoryUsageKb();
};

#endif