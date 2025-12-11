#include "system/Variables.h"
#include "objects/actions/ActionTest.h"

ActionTest::ActionTest(AppContext* appContext)
    : ActionAddon(appContext)
{
    testText_ = new TextObject(appContext->textRenderer, MAIN_FONT_PATH, 16);
    testText_->setPosition(0.0f, 0.0f);
    testText_->setColor(1.0f, 1.0f, 1.0f, 1.0f);
    testText_->setAlignment(ALIGN_CENTER, ALIGN_MIDDLE);

    testText_->setText("Fireable was here");
}

ActionTest::~ActionTest()
{
}

float ActionTest::getRequiredWidth() const
{
    float testWidth = testText_ ? testText_->getRenderedWidth() : 0.0f;
    return testWidth + 16.0f;
}

void ActionTest::update(float deltaTime)
{
    
}

void ActionTest::render(Rect& rect)
{
    if (!testText_) {
        return;
    }
    
    float center_x = rect.x + rect.width / 2.0f;
    float pos_y = rect.y + rect.height / 2.0f; 

    testText_->setPosition(center_x, pos_y);
    testText_->render();
}

