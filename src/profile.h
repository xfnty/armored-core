#pragma once

#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_keycode.h>

#include "core.h"

bool Profile_Load(const char *path);

const char       *Profile_GetCorePath(void);
const char       *Profile_GetGamePath(void);
const char       *Profile_GetSavePath(void);
const char       *Profile_GetSystemPath(void);
const char       *Profile_GetAutosavePath(void);
bool              Profile_IsFullscreen(void);
float             Profile_GetMouseSensitivityX(void);
float             Profile_GetMouseSensitivityY(void);
core_mouse_hack_t Profile_GetMouseHackProfile(void);
float             Profile_GetAutosavePeriod(void);

unsigned int Profile_GetVarCount(void);
const char  *Profile_GetVarName(unsigned int idx);
const char  *Profile_GetVarValue(unsigned int idx);
unsigned int Profile_GetVarIdx(const char *name);
const char  *Profile_GetVarValueByName(const char *name);
