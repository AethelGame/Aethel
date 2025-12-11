#pragma once

#include <vector>

#include <BaseState.h>
#include <system/Renderer2D.h>
#include <system/InputQueue.h>
#include <system/TextRenderer.h>

struct MenuButton {
    TextObject* text = nullptr;
    int targetState = -1;
};

class MainMenuState : public BaseState
{
public:
    void init(AppContext* appContext, void* payload = nullptr) override;
    void handleEvent(const TimedInputEvent& event) override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;
    std::string getStateName() const override { return "MainMenuState"; }

private:
    std::vector<MenuButton> buttons_;
    int hoveredIndex_ = -1;
    GLuint backgroundTexture_ = 0;

    void createButton(const std::string& label, int targetState, float y);
    void updateHover(float x, float y);
    void activateButton(int index);
};

