#include "system/Variables.h"
#include "objects/ActionBar.h"
#include "objects/actions/ActionAddon.h"

static bool PointInRect(const glm::vec2& point, const Rect& rect) {
    return point.x >= rect.x && 
           point.x <= rect.x + rect.width && 
           point.y >= rect.y && 
           point.y <= rect.y + rect.height;
}

ActionAddon* ActionBar::findHoveredAddonAndRect(
    const glm::vec2& mousePoint, 
    const Rect& bar_rect, 
    Rect& outHitRect)
{
    ActionAddon* hovered = nullptr;

    auto checkHit = [&](ActionAddon* addon, const Rect& rect) {
        if (PointInRect(mousePoint, rect)) {
            outHitRect = rect;
            hovered = addon;
            return true;
        }
        return false;
    };

    std::vector<ActionAddon*> left_addons;
    std::vector<ActionAddon*> center_addons;
    std::vector<ActionAddon*> right_addons;

    for (auto& addon : actionAddons_)
    {
        if (!addon) continue;
        ActionAlignment alignment = addon->getAlignment();
        if (alignment == ACTION_ALIGN_LEFT) left_addons.push_back(addon);
        else if (alignment == ACTION_ALIGN_CENTER) center_addons.push_back(addon);
        else if (alignment == ACTION_ALIGN_RIGHT) right_addons.push_back(addon);
    }
    
    const float padding = 4.0f;
    const float addon_height = height_;
    const float addon_y = bar_rect.y;
    
    float current_x = bar_rect.x;

    for (auto& addon : left_addons)
    {
        float addon_width = addon->getRequiredWidth();
        current_x += padding;

        Rect full_slot_hit_rect = { current_x, addon_y, addon_width + padding, addon_height };

        if (checkHit(addon, full_slot_hit_rect)) {
            outHitRect = full_slot_hit_rect;
            return hovered;
        }
        current_x += addon_width;
    }

    float left_block_width = current_x - bar_rect.x;
    current_x = bar_rect.x + bar_rect.width;

    for (auto it = right_addons.rbegin(); it != right_addons.rend(); ++it)
    {
        ActionAddon* addon = *it;
        float addon_width = addon->getRequiredWidth();
        
        current_x -= padding;
        current_x -= addon_width;
        Rect hit_rect = { current_x, addon_y, addon_width + padding, addon_height };

        if (checkHit(addon, hit_rect)) {
            outHitRect = hit_rect;
            return hovered;
        }
    }

    float right_block_width = bar_rect.x + bar_rect.width - current_x;

    if (!center_addons.empty()) {
        float total_center_width = 0.0f;
        for (auto& addon : center_addons) total_center_width += addon->getRequiredWidth();
        total_center_width += (center_addons.size() - 1) * padding;
        
        float available_space = bar_rect.width - left_block_width - right_block_width;
        current_x = bar_rect.x + left_block_width + (available_space / 2.0f) - (total_center_width / 2.0f);

        for (auto& addon : center_addons)
        {
            float addon_width = addon->getRequiredWidth();
            float hit_w = addon_width + padding;
            
            if (addon == center_addons.back()) hit_w = addon_width;

            Rect hit_rect = { current_x, addon_y, hit_w, addon_height };
            if (checkHit(addon, hit_rect)) return hovered;
            
            current_x += hit_w;
        }
    }

    return nullptr;
}


ActionBar::ActionBar(AppContext* context, float height) 
    : appContext_(context), height_(height)
{
}

ActionBar::~ActionBar()
{
    for (auto& addon : actionAddons_)
    {
        delete addon;
        addon = nullptr;
    }

    actionAddons_.clear();
}

void ActionBar::update(float deltaTime)
{
    targetY_ = isVisible_ ? 0.0f : -height_;
    
    float lerpFactor = 1.0f - std::pow(0.001f, deltaTime);
    currentY_ = currentY_ + (targetY_ - currentY_) * lerpFactor;
    
    if (std::abs(currentY_ - targetY_) < 0.5f) {
        currentY_ = targetY_;
    }

    appContext_->renderY = currentY_ + height_;

    currentHoverRect_.x = currentHoverRect_.x + (targetHoverRect_.x - currentHoverRect_.x) * lerpFactor;
    currentHoverRect_.y = currentHoverRect_.y + (targetHoverRect_.y - currentHoverRect_.y) * lerpFactor;
    currentHoverRect_.width = currentHoverRect_.width + (targetHoverRect_.width - currentHoverRect_.width) * lerpFactor;
    currentHoverRect_.height = currentHoverRect_.height + (targetHoverRect_.height - currentHoverRect_.height) * lerpFactor;

    if (hoveredAddon_) {
        hoverAlpha_ += fadeSpeed_ * deltaTime;
        if (hoverAlpha_ > maxAlpha_) {
            hoverAlpha_ = maxAlpha_;
        }
    } else {
        hoverAlpha_ -= fadeSpeed_ * deltaTime;
        if (hoverAlpha_ < 0.0f) {
            hoverAlpha_ = 0.0f;
        }
    }

    for (auto& addon : actionAddons_)
    {
        if (addon) {
            addon->update(deltaTime);
        }
    }
}

void ActionBar::render()
{
    if (currentY_ <= -height_) {
        return;
    }
    
    Rect rect = { 
        0.0f, 
        currentY_,
        appContext_->renderWidth,
        height_
    };

    appContext_->renderer2D->drawRect(rect.x, rect.y, rect.width, rect.height, Color(0.12f, 0.12f, 0.12f, 1.0f));

    if (hoverAlpha_ > 0.0f) {
        Rect hoverRect = currentHoverRect_; 
        hoverRect.y += currentY_; 
        
        float alpha = hoverAlpha_ * 255.0f;
        appContext_->renderer2D->drawRect(hoverRect.x, hoverRect.y, hoverRect.width, hoverRect.height, Color(1.0f, 1.0f, 1.0f, alpha));
    }

    std::vector<ActionAddon*> left_addons;
    std::vector<ActionAddon*> center_addons;
    std::vector<ActionAddon*> right_addons;

    for (auto& addon : actionAddons_)
    {
        if (addon) {
            ActionAlignment alignment = addon->getAlignment();
            if (alignment == ACTION_ALIGN_LEFT) {
                left_addons.push_back(addon);
            } else if (alignment == ACTION_ALIGN_CENTER) {
                center_addons.push_back(addon);
            } else if (alignment == ACTION_ALIGN_RIGHT) {
                right_addons.push_back(addon);
            }
        }
    }

    const float padding = 4.0f;
    const float addon_height = height_;
    const float addon_y = rect.y;
    
    float current_x = rect.x;

    for (auto& addon : left_addons)
    {
        float addon_width = addon->getRequiredWidth();
        current_x += padding;

        Rect addon_rect = {
            current_x,
            addon_y,
            addon_width,
            addon_height
        };

        addon->render(addon_rect);
        current_x += addon_width;
    }

    float left_block_width = current_x - rect.x;
    current_x = rect.x + rect.width;

    for (auto it = right_addons.rbegin(); it != right_addons.rend(); ++it)
    {
        ActionAddon* addon = *it;
        float addon_width = addon->getRequiredWidth();
        
        current_x -= padding;
        current_x -= addon_width;

        Rect addon_rect = {
            current_x,
            addon_y,
            addon_width,
            addon_height
        };

        addon->render(addon_rect);
    }

    float right_block_width = rect.x + rect.width - current_x;

    if (!center_addons.empty()) {
        float total_center_width = 0.0f;
        for (auto& addon : center_addons) total_center_width += addon->getRequiredWidth();
        total_center_width += (center_addons.size() - 1) * padding;

        float available_space = rect.width - left_block_width - right_block_width;
        current_x = rect.x + left_block_width + (available_space / 2.0f) - (total_center_width / 2.0f);

        for (auto& addon : center_addons)
        {
            float addon_width = addon->getRequiredWidth();
            Rect addon_rect = {
                current_x,
                addon_y,
                addon_width,
                addon_height
            };

            addon->render(addon_rect);
            current_x += addon_width + padding;
        }
    }
}

void ActionBar::onEvent(const TimedInputEvent& e)
{
    if (currentY_ <= -height_) {
        return;
    }
    
    if (e.type != TimedInputEvent::Type::MOUSE_MOTION && 
        e.type != TimedInputEvent::Type::MOUSE_BUTTON_DOWN && 
        e.type != TimedInputEvent::Type::MOUSE_BUTTON_UP) {
        return;
    }
    
    float mouseX = e.mouseX;
    float mouseY = e.mouseY;

    Rect bar_rect = { 
        0.0f, 
        currentY_,
        appContext_->renderWidth,
        height_
    };

    glm::vec2 mousePoint = { mouseX, mouseY };
    Rect hitRect = {0.0f, 0.0f, 0.0f, 0.0f};

    ActionAddon* newHoveredAddon = findHoveredAddonAndRect(mousePoint, bar_rect, hitRect);
    ActionAddon* prevHoveredAddon = hoveredAddon_;

    if (newHoveredAddon) {
        Rect relativeHitRect = hitRect; 
        relativeHitRect.y -= currentY_; 
        
        if (newHoveredAddon != prevHoveredAddon) {
            if (prevHoveredAddon) {
                prevHoveredAddon->setHovered(false);
            }
            hoveredAddon_ = newHoveredAddon;
            hoveredAddon_->setHovered(true);

            targetHoverRect_ = relativeHitRect;
            
            if (!prevHoveredAddon || hoverAlpha_ == 0.0f) {
                currentHoverRect_ = relativeHitRect;
            } else {
                currentHoverRect_.y = targetHoverRect_.y;
                currentHoverRect_.height = targetHoverRect_.height;
            }
        }
        
        if (newHoveredAddon && e.type == TimedInputEvent::Type::MOUSE_BUTTON_DOWN) {
            newHoveredAddon->onClick();
        }
    } else {
        if (prevHoveredAddon) { 
            prevHoveredAddon->setHovered(false);
            hoveredAddon_ = nullptr;
        }
    }
}

void ActionBar::setBarVisibility(bool visible)
{
    isVisible_ = visible;
}