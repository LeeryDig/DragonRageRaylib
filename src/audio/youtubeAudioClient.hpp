#ifndef AUDIO_YOUTUBE_AUDIO_CLIENT_HPP
#define AUDIO_YOUTUBE_AUDIO_CLIENT_HPP

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <raylib.h>

struct YouTubeAudioClient {
    bool initialized = false;
    bool connected = false;
    int udpPort = 45678;
    std::string currentUrl;
    AudioStream stream{};

    std::atomic<bool> running{false};
    std::thread receiveThread;
    std::mutex bufferMutex;
    std::vector<int16_t> buffer;
};

bool InitYouTubeAudioClient(YouTubeAudioClient& client, int udpPort = 45678);
void ShutdownYouTubeAudioClient(YouTubeAudioClient& client);
bool StartYouTubeStream(YouTubeAudioClient& client, const std::string& url);
void StopYouTubeStream(YouTubeAudioClient& client);
void UpdateYouTubeAudioClient(YouTubeAudioClient& client);
void SetYouTubeAudioClientVolume(YouTubeAudioClient& client, float volume);

#endif
