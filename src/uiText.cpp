#include "uiText.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace {
Font gFont = {};
bool gLoaded = false;

std::string Lower(std::string value) {
    for (char& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

bool IsFontPath(const std::string& path) {
    std::string lower = Lower(path);
    return (lower.size() >= 4 && lower.rfind(".ttf") == lower.size() - 4) ||
           (lower.size() >= 4 && lower.rfind(".otf") == lower.size() - 4);
}
}

void LoadUiFont() {
    std::vector<std::string> paths;
    FilePathList files = LoadDirectoryFiles("resources/fonts");
    for (unsigned int i = 0; i < files.count; ++i) {
        std::string path = files.paths[i];
        if (IsFontPath(path)) paths.push_back(path);
    }
    UnloadDirectoryFiles(files);
    std::sort(paths.begin(), paths.end());
    if (!paths.empty()) {
        gFont = LoadFontEx(paths.front().c_str(), 64, nullptr, 0);
        if (gFont.texture.id > 0) {
            SetTextureFilter(gFont.texture, TEXTURE_FILTER_BILINEAR);
            gLoaded = true;
            TraceLog(LOG_INFO, "UI font loaded: %s", paths.front().c_str());
        }
    }
}

void UnloadUiFont() {
    if (gLoaded) UnloadFont(gFont);
    gFont = {};
    gLoaded = false;
}

void DrawUiText(const char* text, int posX, int posY, int fontSize, Color color) {
    Font font = gLoaded ? gFont : GetFontDefault();
    DrawTextEx(font, text, Vector2{static_cast<float>(posX), static_cast<float>(posY)}, static_cast<float>(fontSize), 1.0f, color);
}

int MeasureUiText(const char* text, int fontSize) {
    Font font = gLoaded ? gFont : GetFontDefault();
    Vector2 size = MeasureTextEx(font, text, static_cast<float>(fontSize), 1.0f);
    return static_cast<int>(size.x + 0.5f);
}
