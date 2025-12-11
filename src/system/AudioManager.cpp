#include "system/AudioManager.h"
#include "system/Logger.h"
#include <bass.h>
#include <algorithm>

bool AudioManager::initialize(int frequency, int device) {
    if (initialized_) {
        GAME_LOG_WARN("AudioManager already initialized");
        return true;
    }
    
    if (!BASS_Init(device, frequency, 0, 0, NULL)) {
        GAME_LOG_ERROR("Failed to initialize BASS: " + std::to_string(BASS_ErrorGetCode()));
        return false;
    }
    
    initialized_ = true;
    GAME_LOG_INFO("AudioManager initialized successfully");
    return true;
}

void AudioManager::shutdown() {
    if (!initialized_) return;
    
    stopAll();
    unloadAll();
    BASS_Free();
    
    initialized_ = false;
    GAME_LOG_INFO("AudioManager shut down");
}

bool AudioManager::loadSound(const std::string& name, const std::string& filepath) {
    if (audioHandles_.find(name) != audioHandles_.end()) {
        GAME_LOG_WARN("Sound already loaded: " + name);
        return true;
    }
    
    HSAMPLE sample = BASS_SampleLoad(FALSE, filepath.c_str(), 0, 0, 3, BASS_SAMPLE_OVER_POS);
    if (!sample) {
        GAME_LOG_ERROR("Failed to load sound: " + filepath + " (Error: " + std::to_string(BASS_ErrorGetCode()) + ")");
        return false;
    }
    
    AudioHandle handle;
    handle.sample = sample;
    handle.type = AudioType::SOUND;
    handle.path = filepath;
    handle.baseVolume = 1.0f;
    
    audioHandles_[name] = handle;
    GAME_LOG_INFO("Loaded sound: " + name + " from " + filepath);
    return true;
}

bool AudioManager::loadMusic(const std::string& name, const std::string& filepath) {
    if (audioHandles_.find(name) != audioHandles_.end()) {
        GAME_LOG_WARN("Music already loaded: " + name);
        return true;
    }
    
    HSTREAM stream = BASS_StreamCreateFile(FALSE, filepath.c_str(), 0, 0, BASS_STREAM_PRESCAN);
    if (!stream) {
        GAME_LOG_ERROR("Failed to load music: " + filepath + " (Error: " + std::to_string(BASS_ErrorGetCode()) + ")");
        return false;
    }
    
    AudioHandle handle;
    handle.stream = stream;
    handle.type = AudioType::MUSIC;
    handle.path = filepath;
    handle.baseVolume = 1.0f;
    
    audioHandles_[name] = handle;
    GAME_LOG_INFO("Loaded music: " + name + " from " + filepath);
    return true;
}

bool AudioManager::loadStream(const std::string& name, const std::string& filepath) {
    if (audioHandles_.find(name) != audioHandles_.end()) {
        GAME_LOG_WARN("Stream already loaded: " + name);
        return true;
    }
    
    HSTREAM stream = BASS_StreamCreateFile(FALSE, filepath.c_str(), 0, 0, 0);
    if (!stream) {
        GAME_LOG_ERROR("Failed to load stream: " + filepath + " (Error: " + std::to_string(BASS_ErrorGetCode()) + ")");
        return false;
    }
    
    AudioHandle handle;
    handle.stream = stream;
    handle.type = AudioType::STREAM;
    handle.path = filepath;
    handle.baseVolume = 1.0f;
    
    audioHandles_[name] = handle;
    GAME_LOG_INFO("Loaded stream: " + name + " from " + filepath);
    return true;
}

void AudioManager::unloadSound(const std::string& name) {
    auto it = audioHandles_.find(name);
    if (it == audioHandles_.end()) return;
    
    if (it->second.type == AudioType::SOUND && it->second.sample) {
        BASS_SampleFree(it->second.sample);
    } else if (it->second.stream) {
        BASS_StreamFree(it->second.stream);
    }
    
    audioHandles_.erase(it);
}

void AudioManager::unloadMusic(const std::string& name) {
    unloadSound(name);
}

void AudioManager::unloadAll() {
    for (auto& pair : audioHandles_) {
        if (pair.second.type == AudioType::SOUND && pair.second.sample) {
            BASS_SampleFree(pair.second.sample);
        } else if (pair.second.stream) {
            BASS_StreamFree(pair.second.stream);
        }
    }
    audioHandles_.clear();
    currentMusic_.clear();
}

bool AudioManager::playSound(const std::string& name, float volume, bool loop) {
    auto it = audioHandles_.find(name);
    if (it == audioHandles_.end()) {
        GAME_LOG_ERROR("Sound not loaded: " + name);
        return false;
    }
    
    if (it->second.type != AudioType::SOUND || !it->second.sample) {
        GAME_LOG_ERROR("Not a sound: " + name);
        return false;
    }
    
    HCHANNEL channel = BASS_SampleGetChannel(it->second.sample, FALSE);
    if (!channel) {
        GAME_LOG_ERROR("Failed to get sound channel: " + name);
        return false;
    }
    
    it->second.baseVolume = volume;
    it->second.isLooping = loop;
    
    float finalVolume = volume * soundVolume_ * masterVolume_;
    BASS_ChannelSetAttribute(channel, BASS_ATTRIB_VOL, finalVolume);
    
    if (loop) {
        BASS_ChannelFlags(channel, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
    }
    
    BASS_ChannelPlay(channel, TRUE);
    return true;
}

bool AudioManager::playMusic(const std::string& name, float volume, bool loop) {
    auto it = audioHandles_.find(name);
    if (it == audioHandles_.end()) {
        GAME_LOG_ERROR("Music not loaded: " + name);
        return false;
    }
    
    if (it->second.type != AudioType::MUSIC || !it->second.stream) {
        GAME_LOG_ERROR("Not music: " + name);
        return false;
    }
    
    if (!currentMusic_.empty() && currentMusic_ != name) {
        stopMusic();
    }
    
    currentMusic_ = name;
    it->second.baseVolume = volume;
    it->second.isLooping = loop;
    
    float finalVolume = volume * musicVolume_ * masterVolume_;
    BASS_ChannelSetAttribute(it->second.stream, BASS_ATTRIB_VOL, finalVolume);
    
    if (loop) {
        BASS_ChannelFlags(it->second.stream, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
    } else {
        BASS_ChannelFlags(it->second.stream, 0, BASS_SAMPLE_LOOP);
    }
    
    BASS_ChannelPlay(it->second.stream, TRUE);
    return true;
}

bool AudioManager::playStream(const std::string& name, float volume, bool loop) {
    auto it = audioHandles_.find(name);
    if (it == audioHandles_.end()) {
        GAME_LOG_ERROR("Stream not loaded: " + name);
        return false;
    }
    
    if (!it->second.stream) {
        GAME_LOG_ERROR("Invalid stream: " + name);
        return false;
    }
    
    it->second.baseVolume = volume;
    it->second.isLooping = loop;
    
    float finalVolume = volume * soundVolume_ * masterVolume_;
    BASS_ChannelSetAttribute(it->second.stream, BASS_ATTRIB_VOL, finalVolume);
    
    if (loop) {
        BASS_ChannelFlags(it->second.stream, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
    }
    
    BASS_ChannelPlay(it->second.stream, TRUE);
    return true;
}

void AudioManager::pauseMusic() {
    if (currentMusic_.empty()) return;
    
    auto it = audioHandles_.find(currentMusic_);
    if (it != audioHandles_.end() && it->second.stream) {
        BASS_ChannelPause(it->second.stream);
    }
}

void AudioManager::resumeMusic() {
    if (currentMusic_.empty()) return;
    
    auto it = audioHandles_.find(currentMusic_);
    if (it != audioHandles_.end() && it->second.stream) {
        BASS_ChannelPlay(it->second.stream, FALSE);
    }
}

void AudioManager::stopMusic() {
    if (currentMusic_.empty()) return;
    
    auto it = audioHandles_.find(currentMusic_);
    if (it != audioHandles_.end() && it->second.stream) {
        BASS_ChannelStop(it->second.stream);
    }
    
    currentMusic_.clear();
    isFading_ = false;
    isCrossfading_ = false;
}

void AudioManager::stopSound(const std::string& name) {
    auto it = audioHandles_.find(name);
    if (it == audioHandles_.end()) return;
    
    if (it->second.type == AudioType::SOUND && it->second.sample) {
        BASS_SampleStop(it->second.sample);
    }
}

void AudioManager::stopAllSounds() {
    for (auto& pair : audioHandles_) {
        if (pair.second.type == AudioType::SOUND && pair.second.sample) {
            BASS_SampleStop(pair.second.sample);
        }
    }
}

void AudioManager::stopAll() {
    stopMusic();
    stopAllSounds();
    
    for (auto& pair : audioHandles_) {
        if (pair.second.stream) {
            BASS_ChannelStop(pair.second.stream);
        }
    }
}

void AudioManager::setMasterVolume(float volume) {
    masterVolume_ = std::clamp(volume, 0.0f, 1.0f);
    applyVolumes();
}

void AudioManager::setMusicVolume(float volume) {
    musicVolume_ = std::clamp(volume, 0.0f, 1.0f);
    applyVolumes();
}

void AudioManager::setSoundVolume(float volume) {
    soundVolume_ = std::clamp(volume, 0.0f, 1.0f);
    applyVolumes();
}

void AudioManager::setVolume(const std::string& name, float volume) {
    auto it = audioHandles_.find(name);
    if (it == audioHandles_.end()) return;
    
    it->second.baseVolume = std::clamp(volume, 0.0f, 1.0f);
    
    if (it->second.stream) {
        float categoryVolume = (it->second.type == AudioType::MUSIC) ? musicVolume_ : soundVolume_;
        float finalVolume = it->second.baseVolume * categoryVolume * masterVolume_;
        BASS_ChannelSetAttribute(it->second.stream, BASS_ATTRIB_VOL, finalVolume);
    }
}

void AudioManager::applyVolumes() {
    for (auto& pair : audioHandles_) {
        if (pair.second.stream) {
            float categoryVolume = (pair.second.type == AudioType::MUSIC) ? musicVolume_ : soundVolume_;
            float finalVolume = pair.second.baseVolume * categoryVolume * masterVolume_;
            BASS_ChannelSetAttribute(pair.second.stream, BASS_ATTRIB_VOL, finalVolume);
        }
    }
}

bool AudioManager::isMusicPlaying() const {
    if (currentMusic_.empty()) return false;
    
    auto it = audioHandles_.find(currentMusic_);
    if (it == audioHandles_.end() || !it->second.stream) return false;
    
    DWORD state = BASS_ChannelIsActive(it->second.stream);
    return (state == BASS_ACTIVE_PLAYING);
}

bool AudioManager::isSoundPlaying(const std::string& name) const {
    auto it = audioHandles_.find(name);
    if (it == audioHandles_.end()) return false;
    
    if (it->second.type == AudioType::SOUND && it->second.sample) {
        HCHANNEL channel = BASS_SampleGetChannel(it->second.sample, FALSE);
        if (channel) {
            return BASS_ChannelIsActive(channel) == BASS_ACTIVE_PLAYING;
        }
    }
    
    return false;
}

void AudioManager::setMusicPosition(double seconds) {
    if (currentMusic_.empty()) return;
    
    auto it = audioHandles_.find(currentMusic_);
    if (it != audioHandles_.end() && it->second.stream) {
        QWORD pos = BASS_ChannelSeconds2Bytes(it->second.stream, seconds);
        BASS_ChannelSetPosition(it->second.stream, pos, BASS_POS_BYTE);
    }
}

double AudioManager::getMusicPosition() const {
    if (currentMusic_.empty()) return 0.0;
    
    auto it = audioHandles_.find(currentMusic_);
    if (it == audioHandles_.end() || !it->second.stream) return 0.0;
    
    QWORD pos = BASS_ChannelGetPosition(it->second.stream, BASS_POS_BYTE);
    return BASS_ChannelBytes2Seconds(it->second.stream, pos);
}

double AudioManager::getMusicLength() const {
    if (currentMusic_.empty()) return 0.0;
    
    auto it = audioHandles_.find(currentMusic_);
    if (it == audioHandles_.end() || !it->second.stream) return 0.0;
    
    QWORD len = BASS_ChannelGetLength(it->second.stream, BASS_POS_BYTE);
    return BASS_ChannelBytes2Seconds(it->second.stream, len);
}

void AudioManager::fadeMusicIn(float duration) {
    if (currentMusic_.empty()) return;
    
    isFading_ = true;
    isFadingIn_ = true;
    fadeTimer_ = 0.0f;
    fadeDuration_ = duration;
    fadeStartVolume_ = 0.0f;
    fadeTargetVolume_ = 1.0f;
    
    auto it = audioHandles_.find(currentMusic_);
    if (it != audioHandles_.end()) {
        it->second.baseVolume = 0.0f;
        applyVolumes();
    }
}

void AudioManager::fadeMusicOut(float duration) {
    if (currentMusic_.empty()) return;
    
    isFading_ = true;
    isFadingIn_ = false;
    fadeTimer_ = 0.0f;
    fadeDuration_ = duration;
    fadeStartVolume_ = 1.0f;
    fadeTargetVolume_ = 0.0f;
}

void AudioManager::crossfadeMusic(const std::string& newMusic, float duration) {
    if (audioHandles_.find(newMusic) == audioHandles_.end()) {
        GAME_LOG_ERROR("Music not loaded for crossfade: " + newMusic);
        return;
    }
    
    isCrossfading_ = true;
    crossfadeTarget_ = newMusic;
    fadeMusicOut(duration);
}

void AudioManager::update(float deltaTime) {
    updateFade(deltaTime);
}

void AudioManager::updateFade(float deltaTime) {
    if (!isFading_ && !isCrossfading_) return;
    
    fadeTimer_ += deltaTime;
    float progress = std::min(fadeTimer_ / fadeDuration_, 1.0f);
    
    float currentVolume = fadeStartVolume_ + (fadeTargetVolume_ - fadeStartVolume_) * progress;
    
    if (!currentMusic_.empty()) {
        auto it = audioHandles_.find(currentMusic_);
        if (it != audioHandles_.end()) {
            it->second.baseVolume = currentVolume;
            applyVolumes();
        }
    }
    
    if (progress >= 1.0f) {
        if (isCrossfading_) {
            stopMusic();
            playMusic(crossfadeTarget_, 1.0f, true);
            fadeMusicIn(fadeDuration_);
            isCrossfading_ = false;
            crossfadeTarget_.clear();
        } else if (!isFadingIn_) {
            stopMusic();
            isFading_ = false;
        } else {
            isFading_ = false;
        }
    }
}