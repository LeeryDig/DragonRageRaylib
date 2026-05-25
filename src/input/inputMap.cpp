#include "inputMap.hpp"

#include <string>

#include <raylib.h>

namespace {

int ParseKeyName(const std::string& name) {
    if (name.length() == 1) {
        char c = name[0];
        if (c >= 'A' && c <= 'Z') return static_cast<int>(c);
        if (c >= '0' && c <= '9') return static_cast<int>(c);
    }
    if (name == "SPACE")         return KEY_SPACE;
    if (name == "ENTER")         return KEY_ENTER;
    if (name == "ESCAPE")        return KEY_ESCAPE;
    if (name == "TAB")           return KEY_TAB;
    if (name == "BACKSPACE")     return KEY_BACKSPACE;
    if (name == "UP")            return KEY_UP;
    if (name == "DOWN")          return KEY_DOWN;
    if (name == "LEFT")          return KEY_LEFT;
    if (name == "RIGHT")         return KEY_RIGHT;
    if (name == "LEFT_SHIFT")    return KEY_LEFT_SHIFT;
    if (name == "RIGHT_SHIFT")   return KEY_RIGHT_SHIFT;
    if (name == "LEFT_CTRL")     return KEY_LEFT_CONTROL;
    if (name == "RIGHT_CTRL")    return KEY_RIGHT_CONTROL;
    if (name == "LEFT_ALT")      return KEY_LEFT_ALT;
    if (name == "RIGHT_ALT")     return KEY_RIGHT_ALT;
    if (name == "F1")            return KEY_F1;
    if (name == "F2")            return KEY_F2;
    if (name == "F3")            return KEY_F3;
    if (name == "F4")            return KEY_F4;
    if (name == "F5")            return KEY_F5;
    if (name == "F6")            return KEY_F6;
    if (name == "F7")            return KEY_F7;
    if (name == "F8")            return KEY_F8;
    if (name == "F9")            return KEY_F9;
    if (name == "F10")           return KEY_F10;
    if (name == "F11")           return KEY_F11;
    if (name == "F12")           return KEY_F12;
    return KEY_NULL;
}

std::string ExtractStringValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    std::size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";
    std::size_t colon = json.find(':', keyPos);
    if (colon == std::string::npos) return "";
    std::size_t openQuote = json.find('"', colon + 1);
    if (openQuote == std::string::npos) return "";
    std::size_t closeQuote = json.find('"', openQuote + 1);
    if (closeQuote == std::string::npos) return "";
    return json.substr(openQuote + 1, closeQuote - openQuote - 1);
}

}  // namespace

InputMap DefaultInputMap() {
    InputMap map = {};
    map.moveForward = KEY_W;
    map.moveBack    = KEY_S;
    map.moveLeft    = KEY_A;
    map.moveRight   = KEY_D;
    map.interact        = KEY_E;
    map.openMenu        = KEY_ESCAPE;
    map.lightCigarette  = KEY_F;
    map.takePuff        = KEY_T;
    return map;
}

InputMap LoadInputMap(const std::string& filePath, const InputMap& fallback) {
    InputMap map = fallback;

    char* raw = LoadFileText(filePath.c_str());
    if (raw == nullptr) return map;
    std::string json = raw;
    UnloadFileText(raw);

    auto bind = [&](const std::string& key, int& field) {
        std::string name = ExtractStringValue(json, key);
        if (!name.empty()) {
            int parsed = ParseKeyName(name);
            if (parsed != KEY_NULL) field = parsed;
        }
    };

    bind("move_forward", map.moveForward);
    bind("move_back",    map.moveBack);
    bind("move_left",    map.moveLeft);
    bind("move_right",   map.moveRight);
    bind("interact",          map.interact);
    bind("open_menu",         map.openMenu);
    bind("light_cigarette",   map.lightCigarette);
    bind("take_puff",         map.takePuff);

    return map;
}

bool IsActionDown(const InputMap& map, GameAction action) {
    switch (action) {
        case GameAction::MoveForward: return IsKeyDown(map.moveForward);
        case GameAction::MoveBack:    return IsKeyDown(map.moveBack);
        case GameAction::MoveLeft:    return IsKeyDown(map.moveLeft);
        case GameAction::MoveRight:   return IsKeyDown(map.moveRight);
        case GameAction::Interact:        return IsKeyDown(map.interact);
        case GameAction::OpenMenu:        return IsKeyDown(map.openMenu);
        case GameAction::LightCigarette:  return IsKeyDown(map.lightCigarette);
        case GameAction::TakePuff:        return IsKeyDown(map.takePuff);
    }
    return false;
}

bool IsActionPressed(const InputMap& map, GameAction action) {
    switch (action) {
        case GameAction::MoveForward:     return IsKeyPressed(map.moveForward);
        case GameAction::MoveBack:        return IsKeyPressed(map.moveBack);
        case GameAction::MoveLeft:        return IsKeyPressed(map.moveLeft);
        case GameAction::MoveRight:       return IsKeyPressed(map.moveRight);
        case GameAction::Interact:        return IsKeyPressed(map.interact);
        case GameAction::OpenMenu:        return IsKeyPressed(map.openMenu);
        case GameAction::LightCigarette:  return IsKeyPressed(map.lightCigarette);
        case GameAction::TakePuff:        return IsKeyPressed(map.takePuff);
    }
    return false;
}
