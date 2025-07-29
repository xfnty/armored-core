#include "profile.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>

#define INI_IMPLEMENTATION
#include "ini.h"
#include "libretro.h"

static struct {
    ini_t ini;
    char core[256];
    char game[256];
    char save[256];
    char autosave[256];
    char system[256];
    bool fullscreen;
    float mouse_sensitivity_x;
    float mouse_sensitivity_y;
    core_mouse_hack_t mouse_hack_profile;
    float autosave_period;
    struct {
        unsigned int count;
        char **names;
        char **values;
    } vars;
} g_profile;

bool Profile_Load(const char *path)
{
    ini_free(&g_profile.ini);
    for (unsigned int i = 0; i < g_profile.vars.count; i++)
    {
        SDL_free(g_profile.vars.names[i]);
        SDL_free(g_profile.vars.values[i]);
    }
    SDL_free(g_profile.vars.names);
    SDL_free(g_profile.vars.values);
    SDL_memset(&g_profile, 0, sizeof(g_profile));
    SDL_ClearError();

    g_profile.ini = ini_parse(path, NULL);
    if (!ini_is_valid(&g_profile.ini)) return SDL_SetError("failed to load profile \"%s\"", path);
    
    initable_t *general = ini_get_table(&g_profile.ini, "general");
    if (!ini_to_str(ini_get(general, "core"), g_profile.core, sizeof(g_profile.core), false)) return SDL_SetError("missing field \"general.core\" in profile \"%s\"", path);
    if (!ini_to_str(ini_get(general, "game"), g_profile.game, sizeof(g_profile.game), false)) return SDL_SetError("missing field \"general.game\" in profile \"%s\"", path);
    if (!(g_profile.autosave_period = ini_as_num(ini_get(general, "autosave_period")))) return SDL_SetError("missing or zeroed field \"general.autosave_period\" in profile \"%s\"", path);

    initable_t *paths = ini_get_table(&g_profile.ini, "paths");
    if (!ini_to_str(ini_get(paths, "save"), g_profile.save, sizeof(g_profile.save), false)) return SDL_SetError("missing field \"paths.save\" in profile \"%s\"", path);
    if (!ini_to_str(ini_get(paths, "system"), g_profile.system, sizeof(g_profile.system), false)) return SDL_SetError("missing field \"paths.system\" in profile \"%s\"", path);
    if (!ini_to_str(ini_get(paths, "autosave"), g_profile.autosave, sizeof(g_profile.autosave), false)) return SDL_SetError("missing field \"paths.autosave\" in profile \"%s\"", path);

    initable_t *input = ini_get_table(&g_profile.ini, "input");
    if (!(g_profile.mouse_sensitivity_x = ini_as_num(ini_get(input, "mouse_sensitivity_x")))) return SDL_SetError("missing or zeroed field \"input.mouse_sensitivity_x\" in profile \"%s\"", path);
    if (!(g_profile.mouse_sensitivity_y = ini_as_num(ini_get(input, "mouse_sensitivity_y")))) return SDL_SetError("missing or zeroed field \"input.mouse_sensitivity_y\" in profile \"%s\"", path);
    char profile_name[256];
    if (!ini_to_str(ini_get(input, "mouse_hack_for"), profile_name, sizeof(profile_name), false)) return SDL_SetError("missing field \"input.mouse_hack_for\" in profile \"%s\"", path);
    if      (SDL_strcmp(profile_name, "ac") == 0) g_profile.mouse_hack_profile = CORE_MOUSE_HACK_AC;
    else if (SDL_strcmp(profile_name, "acpp") == 0) g_profile.mouse_hack_profile = CORE_MOUSE_HACK_AC_PROJECT_PHANTASMA;
    else if (SDL_strcmp(profile_name, "acmoa") == 0) g_profile.mouse_hack_profile = CORE_MOUSE_HACK_AC_MASTER_OF_ARENA;
    else return SDL_SetError("field \"input.mouse_hack_for\" has invalid value of \"%s\" (only \"ac\", \"acpp\" and \"acmoa\" are allowed)", profile_name);

    g_profile.fullscreen = ini_as_bool(ini_get(general, "fullscreen"));

    initable_t *vars = ini_get_table(&g_profile.ini, "vars");
    if (vars)
    {
        g_profile.vars.count = ivec_len(vars->values);
        g_profile.vars.names = SDL_calloc(g_profile.vars.count, sizeof(g_profile.vars.names[0]));
        g_profile.vars.values = SDL_calloc(g_profile.vars.count, sizeof(g_profile.vars.values[0]));
        for (unsigned int i = 0; i < g_profile.vars.count; i++)
        {
            inivalue_t v = vars->values[i];
            g_profile.vars.names[i] = SDL_strndup(v.key.buf, v.key.len);
            g_profile.vars.values[i] = SDL_strndup(v.value.buf, v.value.len);
            // SDL_Log("%s=%s", g_profile.vars.names[i], g_profile.vars.values[i], i);
        }
    }

    SDL_Log("loaded profile \"%s\"", path);
    return SDL_ClearError();
}

const char *Profile_GetCorePath(void)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    return g_profile.core;
}

const char *Profile_GetGamePath(void)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    return g_profile.game;
}

const char *Profile_GetSavePath(void)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    return g_profile.save;
}

const char *Profile_GetSystemPath(void)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    return g_profile.system;
}

const char *Profile_GetAutosavePath(void)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    return g_profile.autosave;
}

bool Profile_IsFullscreen(void)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    return g_profile.fullscreen;
}

float Profile_GetMouseSensitivityX(void)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    return g_profile.mouse_sensitivity_x;
}

float Profile_GetMouseSensitivityY(void)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    return g_profile.mouse_sensitivity_y;
}

core_mouse_hack_t Profile_GetMouseHackProfile(void)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    return g_profile.mouse_hack_profile;
}

float Profile_GetAutosavePeriod(void)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    return g_profile.autosave_period;
}

unsigned int Profile_GetVarCount(void)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    return g_profile.vars.count;
}

const char *Profile_GetVarName(unsigned int idx)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    SDL_assert_release(idx < g_profile.vars.count);
    return g_profile.vars.names[idx];
}

const char *Profile_GetVarValue(unsigned int idx)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    SDL_assert_release(idx < g_profile.vars.count);
    return g_profile.vars.values[idx];
}

unsigned int Profile_GetVarIdx(const char *name)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    for (unsigned int i = 0; i < g_profile.vars.count; i++)
        if (SDL_strcmp(g_profile.vars.names[i], name) == 0)
            return i;
    return 0xFFFFFFFFu;
}

const char *Profile_GetVarValueByName(const char *name)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    unsigned int i = Profile_GetVarIdx(name);
    return (i < g_profile.vars.count) ? (g_profile.vars.values[i]) : (0);
}
