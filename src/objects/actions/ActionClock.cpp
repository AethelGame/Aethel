#include "system/Variables.h"
#include "objects/actions/ActionClock.h"

ActionClock::ActionClock(AppContext* appContext)
    : ActionAddon(appContext)
{
    clockText_ = new TextObject(appContext->textRenderer, MAIN_FONT_PATH, 16);
    clockText_->setPosition(0.0f, 0.0f);
    clockText_->setColor(1.0f, 1.0f, 1.0f, 1.0f);
    clockText_->setAlignment(ALIGN_CENTER, ALIGN_MIDDLE);

    uptimeText_ = new TextObject(appContext->textRenderer, MAIN_FONT_PATH, 14);
    uptimeText_->setColor(0.7f, 0.7f, 0.7f, 1.0f);
    uptimeText_->setAlignment(ALIGN_CENTER, ALIGN_MIDDLE);
}

ActionClock::~ActionClock()
{
}

float ActionClock::getRequiredWidth() const
{
    float uptimeWidth = uptimeText_ ? uptimeText_->getRenderedWidth() : 0.0f;
    float clockWidth = clockText_ ? clockText_->getRenderedWidth() : 0.0f;
    return std::max(uptimeWidth, clockWidth) + 16.0f;
}

void ActionClock::update(float deltaTime)
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&time);
    
    char timeBuffer[9];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", localTime);
    clockText_->setText(timeBuffer);

    auto duration = now - getAppContext()->startTime;
    auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();

    long hours = total_seconds / 3600;
    long minutes = (total_seconds % 3600) / 60;
    long seconds = total_seconds % 60;
    
    char uptimeBuffer[32];
    std::snprintf(uptimeBuffer, sizeof(uptimeBuffer), "%ldh %ldm %lds", hours, minutes, seconds);
    uptimeText_->setText(uptimeBuffer);
}

void ActionClock::render(Rect& rect)
{
    if (!clockText_) {
        return;
    }
    
    float center_x = rect.x + rect.width / 2.0f;

    float time_y = rect.y + rect.height * 0.35f; 
    float uptime_y = rect.y + rect.height * 0.75f;

    if (mode_ == TIME_ONLY) {
        time_y = rect.y + rect.height / 2.0f;
    } else if (mode_ == UPTIME_ONLY) {
        uptime_y = rect.y + rect.height / 2.0f;
    }

    if (mode_ != UPTIME_ONLY) {
        clockText_->setPosition(center_x, time_y);
        clockText_->render();
    }

    if (mode_ != TIME_ONLY) {
        uptimeText_->setPosition(center_x, uptime_y);
        uptimeText_->render();
    }
}

void ActionClock::onClick()
{
    if (mode_ == TIME_AND_UPTIME) {
        mode_ = TIME_ONLY;
    } else if (mode_ == TIME_ONLY) {
        mode_ = UPTIME_ONLY;
    } else {
        mode_ = TIME_AND_UPTIME;
    }
}