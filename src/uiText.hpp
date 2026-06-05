#ifndef UI_TEXT_HPP
#define UI_TEXT_HPP

#include "raylib.h"

void LoadUiFont();
void UnloadUiFont();
void DrawUiText(const char* text, int posX, int posY, int fontSize, Color color);
int MeasureUiText(const char* text, int fontSize);

#endif
