#ifndef DEBUG_INFO_H
#define DEBUG_INFO_H

#include <string>
#include <objects/debug/FPSCounter.h>
#include "system/TextRenderer.h"
#include "system/Renderer2D.h"

class DebugInfo : public FPSCounter {
public:
    DebugInfo(TextRenderer* textRenderer, const std::string& fontPath, int fontSize, float yPos);
    ~DebugInfo() = default;

    void update() override;
    void render(Renderer2D* renderer) override;
};

#endif