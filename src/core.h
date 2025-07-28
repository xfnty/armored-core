#pragma once

#include <SDL3/SDL_stdinc.h>

typedef enum core_mouse_hack_t core_mouse_hack_t;
enum core_mouse_hack_t {
    CORE_MOUSE_HACK_AC1,
    CORE_MOUSE_HACK_AC_PROJECT_PHANTASMA,
};

bool Core_Load(const char *path);
bool Core_LoadGame(const char *path);
bool Core_LoadState(const char *path);
bool Core_SaveState(const char *path);
void Core_Free(void);

void Core_RunFrame(void);

float Core_GetRenderWidth(void);
float Core_GetRenderHeight(void);
float Core_GetTargetFPS(void);

void Core_SetJoypadAxis(uint8_t axis, int16_t value);
void Core_SetMouseHackProfile(core_mouse_hack_t profile);
void Core_SetMouseMove(float rx, float ry);
