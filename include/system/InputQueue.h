#ifndef INPUT_QUEUE_H
#define INPUT_QUEUE_H

#include <queue>
#include <mutex>
#include <chrono>
#include <GLFW/glfw3.h>

struct TimedInputEvent {
    enum Type {
        KEY_DOWN,
        KEY_UP,
        MOUSE_BUTTON_DOWN,
        MOUSE_BUTTON_UP,
        MOUSE_MOTION
    };
    
    Type type;
    
    std::chrono::high_resolution_clock::time_point timestamp;
    double timeSeconds;
    
    int key;
    int scancode;
    int mods;
    
    double mouseX;
    double mouseY;
    int button;
};

class InputQueue {
public:
    InputQueue() = default;
    ~InputQueue() = default;
    
    void enqueue(const TimedInputEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(event);
    }
    
    bool dequeue(TimedInputEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        event = queue_.front();
        queue_.pop();
        return true;
    }
    
    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }
    
private:
    std::queue<TimedInputEvent> queue_;
    mutable std::mutex mutex_;
};

#endif