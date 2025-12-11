#ifndef ACTION_ADDON_H
#define ACTION_ADDON_H

#include <system/Renderer2D.h>

class AppContext;
enum ActionAlignment {
    ACTION_ALIGN_LEFT,
    ACTION_ALIGN_CENTER,
    ACTION_ALIGN_RIGHT
};

class ActionAddon {
public:
    ActionAddon(AppContext* appContext) : appContext_(appContext) {}
    virtual ~ActionAddon() = default;

    virtual void update(float deltaTime) {};
    virtual void render(Rect& rect) {};

    void setHovered(bool hovered) { isHovered_ = hovered; }
    bool isHovered() const { return isHovered_; }

    virtual void onHovered() {};
    virtual void onUnhovered() {};

    virtual void onClick() {};

    ActionAlignment getAlignment() const { return alignment_; }
    virtual float getRequiredWidth() const { return 0.0f; }

    AppContext* getAppContext() const {
        return appContext_;
    }
private:
    AppContext* appContext_ = nullptr;
    ActionAlignment alignment_ = ACTION_ALIGN_CENTER;

    bool isHovered_ = false;
};

#endif