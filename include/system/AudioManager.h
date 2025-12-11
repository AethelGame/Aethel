#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <bass.h>

enum class AudioType {
    SOUND,
    MUSIC,
    STREAM
};

struct AudioHandle {
    HSTREAM stream = 0;
    HSAMPLE sample = 0;
    AudioType type = AudioType::SOUND;
    std::string path;
    float baseVolume = 1.0f;
    bool isLooping = false;
};

class AudioManager {
public:
    static AudioManager& getInstance() {
        static AudioManager instance;
        return instance;
    }
    
    bool initialize(int frequency = 44100, int device = -1);
    void shutdown();
    
    bool loadSound(const std::string& name, const std::string& filepath);
    bool loadMusic(const std::string& name, const std::string& filepath);
    bool loadStream(const std::string& name, const std::string& filepath);
    
    void unloadSound(const std::string& name);
    void unloadMusic(const std::string& name);
    void unloadAll();
    
    bool playSound(const std::string& name, float volume = 1.0f, bool loop = false);
    bool playMusic(const std::string& name, float volume = 1.0f, bool loop = true);
    bool playStream(const std::string& name, float volume = 1.0f, bool loop = false);
    
    void pauseMusic();
    void resumeMusic();
    void stopMusic();
    void stopSound(const std::string& name);
    void stopAllSounds();
    void stopAll();
    
    void setMasterVolume(float volume);
    void setMusicVolume(float volume);
    void setSoundVolume(float volume);
    void setVolume(const std::string& name, float volume);
    
    float getMasterVolume() const { return masterVolume_; }
    float getMusicVolume() const { return musicVolume_; }
    float getSoundVolume() const { return soundVolume_; }
    
    bool isMusicPlaying() const;
    bool isSoundPlaying(const std::string& name) const;
    
    void setMusicPosition(double seconds);
    double getMusicPosition() const;
    double getMusicLength() const;
    
    void fadeMusicIn(float duration = 2.0f);
    void fadeMusicOut(float duration = 2.0f);
    void crossfadeMusic(const std::string& newMusic, float duration = 2.0f);
    
    void update(float deltaTime);
    
private:
    AudioManager() = default;
    ~AudioManager() { shutdown(); }
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    
    void updateFade(float deltaTime);
    void applyVolumes();
    
    std::map<std::string, AudioHandle> audioHandles_;
    std::string currentMusic_;
    
    float masterVolume_ = 0.1f;
    float musicVolume_ = 1.0f;
    float soundVolume_ = 1.0f;
    
    bool isFading_ = false;
    bool isFadingIn_ = false;
    float fadeTimer_ = 0.0f;
    float fadeDuration_ = 0.0f;
    float fadeStartVolume_ = 0.0f;
    float fadeTargetVolume_ = 0.0f;
    
    bool isCrossfading_ = false;
    std::string crossfadeTarget_;
    
    bool initialized_ = false;
};

#endif