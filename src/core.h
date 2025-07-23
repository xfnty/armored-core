#pragma once

#include <SDL3/SDL_stdinc.h>

bool Core_Load(const char *path);
bool Core_LoadGame(const char *path);
void Core_Free(void);

void Core_RunFrame(void);

float Core_GetRenderWidth(void);
float Core_GetRenderHeight(void);
