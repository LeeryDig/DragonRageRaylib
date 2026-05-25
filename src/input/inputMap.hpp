#ifndef INPUT_INPUT_MAP_HPP
#define INPUT_INPUT_MAP_HPP

#include <string>

enum class GameAction {
    MoveForward,
    MoveBack,
    MoveLeft,
    MoveRight,
    Interact,
    OpenMenu,
    LightCigarette,
    TakePuff,
};

struct InputMap {
    int moveForward;
    int moveBack;
    int moveLeft;
    int moveRight;
    int interact;
    int openMenu;
    int lightCigarette;
    int takePuff;
};

InputMap DefaultInputMap();
InputMap LoadInputMap(const std::string& filePath, const InputMap& fallback);

bool IsActionDown(const InputMap& map, GameAction action);
bool IsActionPressed(const InputMap& map, GameAction action);

#endif
