#ifndef INFO_STACK_MANAGER_H
#define INFO_STACK_MANAGER_H

#include <vector>
#include "system/Renderer2D.h"
#include "objects/debug/FPSCounter.h"

class InfoStackManager {
public:
    InfoStackManager() = default;
    ~InfoStackManager() {
        for (auto* info : infos_) {
            delete info;
        }
        infos_.clear();
    }
    
    void addInfo(FPSCounter* info) {
        infos_.push_back(info);
        updatePositions();
    }
    
    void removeInfo(FPSCounter* info) {
        auto it = std::find(infos_.begin(), infos_.end(), info);
        if (it != infos_.end()) {
            delete *it;
            infos_.erase(it);
            updatePositions();
        }
    }
    
    void updateAll() {
        for (auto* info : infos_) {
            info->update();
        }
    }
    
    void renderAll(Renderer2D* renderer) {
        for (auto* info : infos_) {
            info->render(renderer);
        }
    }
    
private:
    std::vector<FPSCounter*> infos_;
    
    void updatePositions() {
        float currentY = 8.0f;
        
        for (auto* info : infos_) {
            if (auto* fps = dynamic_cast<FPSCounter*>(info)) {
                fps->setYPosition(currentY);
                currentY += fps->getHeight() + 4.0f;
            }
        }
    }
};

#endif