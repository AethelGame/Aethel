#include <iomanip>
#include <sstream>
#include <GLFW/glfw3.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <fstream>
#include <string>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

#include <system/Variables.h>
#include <utils/Utils.h>
#include <objects/debug/FPSCounter.h>
#include "system/TextRenderer.h"

FPSCounter::FPSCounter(TextRenderer* textRenderer, const std::string &fontPath, int fontSize, float yPos)
{
    textObject_ = new TextObject(textRenderer, fontPath, fontSize * 4.0f);
    
    textObject_->setScale(0.25f);
    textObject_->setPosition(8.0f, yPos);
    textObject_->setTextGap(4.0f);
    textObject_->setColor(1.0f, 1.0f, 1.0f, 1.0f);
    textObject_->setText("Init...");
    textObject_->updateDimensions();
}

FPSCounter::~FPSCounter()
{
    if (textObject_)
    {
        delete textObject_;
        textObject_ = nullptr;
    }
}

long FPSCounter::getAppMemoryUsageKb()
{
#if defined(_WIN32) || defined(_WIN64)
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc)))
    {
        return (long)(pmc.WorkingSetSize / 1024);
    }
    return 0;

#elif defined(__linux__)
    long residentSet = 0;
    std::ifstream stat_stream("/proc/self/status", std::ios_base::in);
    std::string line;

    while (std::getline(stat_stream, line))
    {
        if (line.compare(0, 8, "VmRSS:") == 0)
        {
            std::stringstream ss(line.substr(8));
            ss >> residentSet;
            break;
        }
    }
    return residentSet;

#else
    return 0;
#endif
}

void FPSCounter::update()
{
    if (!perfFrequency_)
    {
        perfFrequency_ = 1.0;
        lastTime_ = glfwGetTime();
    }

    if (!textObject_)
        return;

    double currentTime = glfwGetTime();
    double timeDifference = currentTime - lastTime_;
    lastTime_ = currentTime;

    float frameTimeMs = (float)(timeDifference * 1000.0);
    latestFrameTimeMs_ = frameTimeMs;

    frameAccumulator_ += timeDifference;
    frameCount_++;

    if (frameAccumulator_ >= 0.1)
    {
        float fps = (float)frameCount_ / (float)frameAccumulator_;
        memoryUsageKb_ = getAppMemoryUsageKb();

        std::stringstream ss;
        ss << std::fixed;

        ss << "FPS: " << std::setprecision(0) << fps
           << "\nFT: " << std::setprecision(2) << latestFrameTimeMs_ << "ms";

        if (memoryUsageKb_ >= 0)
            ss << "\nMEM: " << Utils::formatMemorySize(memoryUsageKb_ * 1024);

        textObject_->setText(ss.str());
        
        frameAccumulator_ = 0.0;
        frameCount_ = 0;
    }
}

void FPSCounter::render(Renderer2D* renderer) 
{
    if (!textObject_) {
        return;
    }
    
    float textX, textY;
    textObject_->getPosition(textX, textY);

    float textW = textObject_->getRenderedWidth();
    float textH = textObject_->getRenderedHeight();

    renderer->drawRect(
        textX - padding,
        textY - padding,
        textW + (2.0f * padding),
        textH + (2.0f * padding),
        Color(0.0f, 0.0f, 0.0f, 0.5f)
    );

    textObject_->render();
}