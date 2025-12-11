#ifndef ACTIONBAR_H
#define ACTIONBAR_H

#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <system/InputQueue.h>
#include <system/Renderer2D.h>

class AppContext;
class ActionAddon;

class ActionBar
{
public:
    ActionBar(AppContext* appContext, float height = 50.0f);
    ~ActionBar();

    void update(float deltaTime);
    void render();
    void onEvent(const TimedInputEvent& e);

    void setBarVisibility(bool visible);
    void addAddon(ActionAddon* addon) {
        actionAddons_.push_back(addon);
    }
private:
    AppContext* appContext_;
    std::vector<ActionAddon*> actionAddons_;

    float targetY_ = 0.0f;
    float currentY_ = 0.0f;

    float height_ = 50.0f;
    bool isVisible_ = true;

    ActionAddon* hoveredAddon_ = nullptr;
    Rect targetHoverRect_ = {0.0f, 0.0f, 0.0f, 0.0f}; 
    Rect currentHoverRect_ = {0.0f, 0.0f, 0.0f, 0.0f}; 

    float fadeSpeed_ = 4.0f;
    float maxAlpha_ = 0.25f;
    float hoverAlpha_ = 0.0f;

    ActionAddon* findHoveredAddonAndRect(
        const glm::vec2& mousePoint, 
        const Rect& barRect, 
        Rect& outHitRect);
};

#endif