#pragma once

struct GameState;

bool loadSaveFromFile(const char *const filePath, GameState *target);
bool saveGameToFile(const char *fileName, const GameState *state);