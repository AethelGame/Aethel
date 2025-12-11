#include <states/MainMenuState.h>
#include <system/AudioManager.h>

void MainMenuState::createButton(const std::string& label, int targetState, float y)
{
    TextObject* t = new TextObject(appContext->textRenderer, MAIN_FONT_PATH, 36);
    t->setText(label);
    t->setAlignment(ALIGN_CENTER, ALIGN_MIDDLE);
    t->setPosition(screenWidth_ / 2.0f, y);
    buttons_.push_back({t, targetState});
}

void MainMenuState::init(AppContext* appContext, void* payload)
{
    BaseState::init(appContext, payload);

    backgroundTexture_ = appContext->renderer2D->loadTexture("assets/songs/EGOIST - The Everlasting Guilty Crown/22627712_p0.jpg");
    if (backgroundTexture_ == 0) {
        GAME_LOG_WARN("Failed to load menu background, using solid color");
    }

    float centerY = screenHeight_ / 2.0f;
    createButton("Play", STATE_MAIN_MENU, centerY - 30.0f);
    createButton("Options", STATE_MAIN_MENU, centerY + 30.0f);

    AudioManager::getInstance().loadMusic("menu_theme", "assets/songs/EGOIST - The Everlasting Guilty Crown/audio.mp3");
    AudioManager::getInstance().playMusic("menu_theme", 0.7f, true);
    AudioManager::getInstance().fadeMusicIn(2.0f);
}

void MainMenuState::handleEvent(const TimedInputEvent& event)
{
    if (event.type == TimedInputEvent::Type::MOUSE_MOTION) {
        updateHover((float)event.mouseX, (float)event.mouseY);
    } else if (event.type == TimedInputEvent::Type::MOUSE_BUTTON_DOWN && event.button == GLFW_MOUSE_BUTTON_LEFT) {
        if (hoveredIndex_ >= 0 && hoveredIndex_ < (int)buttons_.size()) {
            activateButton(hoveredIndex_);
        }
    } else if (event.type == TimedInputEvent::Type::KEY_DOWN && event.key == GLFW_KEY_ESCAPE) {
        appContext->appQuit = true;
    }
}

void MainMenuState::update(float deltaTime)
{
}

void MainMenuState::render()
{
    if (backgroundTexture_ != 0) {
        appContext->renderer2D->drawTextureFullscreen(
            backgroundTexture_, 
            Color(0.2f, 0.2f, 0.2f, 1.0f)
        );
    }

    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (buttons_[i].text) {
            if ((int)i == hoveredIndex_) {
                buttons_[i].text->setColor(1.0f, 1.0f, 0.0f, 1.0f);
            } else {
                buttons_[i].text->setColor(1.0f, 1.0f, 1.0f, 1.0f);
            }
            buttons_[i].text->render();
        }
    }
}

void MainMenuState::destroy()
{
    AudioManager::getInstance().fadeMusicOut(1.0f);
    AudioManager::getInstance().unloadMusic("menu_theme");

    if (backgroundTexture_ != 0) {
        appContext->renderer2D->unloadTexture(backgroundTexture_);
        backgroundTexture_ = 0;
    }

    for (auto& b : buttons_) {
        delete b.text;
    }
    buttons_.clear();
}

void MainMenuState::updateHover(float x, float y)
{
    hoveredIndex_ = -1;
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (buttons_[i].text && buttons_[i].text->hitTest(x, y)) {
            hoveredIndex_ = (int)i;
            break;
        }
    }
}

void MainMenuState::activateButton(int index)
{
    if (index < 0 || index >= (int)buttons_.size()) return;
    int target = buttons_[index].targetState;
    if (target >= 0) {
        requestStateSwitch(target);
    }
}

