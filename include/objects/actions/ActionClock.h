#ifndef ACTIONCLOCK_H
#define ACTIONCLOCK_H

#include <chrono>
#include <system/Renderer2D.h>
#include <system/TextRenderer.h>
#include <objects/actions/ActionAddon.h>

enum ClockMode {
    TIME_ONLY,
    UPTIME_ONLY,
    TIME_AND_UPTIME
}; 

class ActionClock : public ActionAddon
{
public:
    ActionClock(AppContext* appContext);
    ~ActionClock();

    void update(float deltaTime) override;
    void render(Rect& rect) override;
    void onClick() override;

    float getRequiredWidth() const override;
private:
    TextObject* clockText_ = nullptr;
    TextObject* uptimeText_ = nullptr;

    ClockMode mode_ = TIME_AND_UPTIME;
};

#endif