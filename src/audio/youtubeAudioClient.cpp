#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOUSER
#define NOUSER
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "youtubeAudioClient.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <sstream>

#include <raymath.h>

#ifdef _WIN32
using SocketHandle = SOCKET;
static const SocketHandle InvalidSocketHandle = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketHandle = int;
static const SocketHandle InvalidSocketHandle = -1;
#endif

namespace {
constexpr int SampleRate = 44100;
constexpr int SamplesPerUpdate = 2048;
constexpr std::size_t MaxBufferedSamples = SampleRate * 10;

void CloseSocketHandle(SocketHandle s) {
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

std::string UrlEncode(const std::string& value) {
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : value) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') out.push_back(static_cast<char>(c));
        else { out.push_back('%'); out.push_back(hex[c >> 4]); out.push_back(hex[c & 15]); }
    }
    return out;
}

bool HttpGetLocal(const std::string& path) {
    SocketHandle sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == InvalidSocketHandle) return false;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(3456);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) { CloseSocketHandle(sock); return false; }
    std::string req = "GET " + path + " HTTP/1.1\r\nHost: 127.0.0.1:3456\r\nConnection: close\r\n\r\n";
    send(sock, req.c_str(), static_cast<int>(req.size()), 0);
    char response[128] = {};
    int got = recv(sock, response, sizeof(response) - 1, 0);
    CloseSocketHandle(sock);
    return got > 0 && std::strstr(response, " 200 ") != nullptr;
}

void ReceiveLoop(YouTubeAudioClient* client) {
    SocketHandle sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == InvalidSocketHandle) return;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(static_cast<unsigned short>(client->udpPort));
    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        TraceLog(LOG_WARNING, "YouTubeAudioClient UDP bind failed on port %d", client->udpPort);
        CloseSocketHandle(sock);
        return;
    }
    TraceLog(LOG_INFO, "YouTubeAudioClient listening UDP on 127.0.0.1:%d", client->udpPort);

    bool loggedFirstPacket = false;
    std::array<char, 4096> packet{};
    while (client->running.load()) {
        int received = recv(sock, packet.data(), static_cast<int>(packet.size()), 0);
        if (received <= 0) continue;
        if (!loggedFirstPacket) {
            TraceLog(LOG_INFO, "YouTubeAudioClient received first UDP audio packet (%d bytes)", received);
            loggedFirstPacket = true;
        }
        std::size_t samples = static_cast<std::size_t>(received / 2);
        std::lock_guard<std::mutex> lock(client->bufferMutex);
        const int16_t* pcm = reinterpret_cast<const int16_t*>(packet.data());
        client->buffer.insert(client->buffer.end(), pcm, pcm + samples);
        if (client->buffer.size() > MaxBufferedSamples) {
            client->buffer.erase(client->buffer.begin(), client->buffer.begin() + static_cast<std::ptrdiff_t>(client->buffer.size() - MaxBufferedSamples));
        }
    }
    CloseSocketHandle(sock);
}
}

bool InitYouTubeAudioClient(YouTubeAudioClient& client, int udpPort) {
    if (client.initialized) return true;
#ifdef _WIN32
    WSADATA wsa{};
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    client.udpPort = udpPort;
    SetAudioStreamBufferSizeDefault(SamplesPerUpdate);
    client.stream = LoadAudioStream(SampleRate, 16, 1);
    PlayAudioStream(client.stream);
    client.initialized = true;
    return true;
}

void ShutdownYouTubeAudioClient(YouTubeAudioClient& client) {
    StopYouTubeStream(client);
    if (client.initialized) UnloadAudioStream(client.stream);
    client.initialized = false;
#ifdef _WIN32
    WSACleanup();
#endif
}

bool StartYouTubeStream(YouTubeAudioClient& client, const std::string& url) {
    if (!client.initialized) InitYouTubeAudioClient(client, client.udpPort);
    StopYouTubeStream(client);
    { std::lock_guard<std::mutex> lock(client.bufferMutex); client.buffer.clear(); }
    client.currentUrl = url;
    PlayAudioStream(client.stream);
    client.running = true;
    client.receiveThread = std::thread(ReceiveLoop, &client);
    client.connected = HttpGetLocal("/play?url=" + UrlEncode(url) + "&port=" + std::to_string(client.udpPort));
    if (!client.connected) StopYouTubeStream(client);
    return client.connected;
}

void StopYouTubeStream(YouTubeAudioClient& client) {
    if (client.running.load()) {
        client.running = false;
        HttpGetLocal("/stop");
        // Unblock UDP recv.
        SocketHandle poke = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (poke != InvalidSocketHandle) {
            sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(static_cast<unsigned short>(client.udpPort)); inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
            char b = 0; sendto(poke, &b, 1, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)); CloseSocketHandle(poke);
        }
        if (client.receiveThread.joinable()) client.receiveThread.join();
    }
    client.connected = false;
    client.currentUrl.clear();
}

void UpdateYouTubeAudioClient(YouTubeAudioClient& client) {
    if (!client.initialized) return;
    while (IsAudioStreamProcessed(client.stream)) {
        std::array<int16_t, SamplesPerUpdate> samples{};
        {
            std::lock_guard<std::mutex> lock(client.bufferMutex);
            std::size_t count = std::min(client.buffer.size(), samples.size());
            if (count > 0) {
                std::copy_n(client.buffer.begin(), count, samples.begin());
                client.buffer.erase(client.buffer.begin(), client.buffer.begin() + static_cast<std::ptrdiff_t>(count));
            }
        }
        UpdateAudioStream(client.stream, samples.data(), SamplesPerUpdate);
    }
}

void SetYouTubeAudioClientVolume(YouTubeAudioClient& client, float volume) {
    if (client.initialized) SetAudioStreamVolume(client.stream, Clamp(volume, 0.0f, 1.0f));
}
