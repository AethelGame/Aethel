#include <iomanip>
#include <sstream>

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
#include <BaseState.h>
#include <objects/debug/DebugInfo.h>

DebugInfo::DebugInfo(TextRenderer* renderer, const std::string& fontPath, int fontSize, float yPos)
    : FPSCounter(renderer, fontPath, fontSize, yPos)
{

}

void DebugInfo::update()
{
    if (!textObject_)
        return;

    AppContext* appContext = getAppContext();
    if (!appContext)
        return;

    if (!appContext->currentState)
        return;

    std::stringstream ss;
    ss.precision(2);
    ss << std::fixed;

    ss << "STATE: " << appContext->currentState->getStateName();

    textObject_->setText(ss.str());
}

void DebugInfo::render(Renderer2D* renderer)
{
    FPSCounter::render(renderer);
}