#ifndef ACTIONTEST_H
#define ACTIONTEST_H

#include <chrono>

#include <system/Renderer2D.h>
#include <system/TextRenderer.h>
#include <objects/actions/ActionAddon.h>

class ActionTest : public ActionAddon
{
public:
    ActionTest(AppContext* appContext);
    ~ActionTest();

    void update(float deltaTime) override;
    void render(Rect& rect) override;

    float getRequiredWidth() const override;
private:
    TextObject* testText_ = nullptr;
};

#endif