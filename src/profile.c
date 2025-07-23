#include "profile.h"

#include <SDL3/SDL.h>

#define INI_IMPLEMENTATION
#include "ini.h"

static struct {
    ini_t ini;
    char core[256];
    char game[256];
    char save[256];
    char system[256];
    bool fullscreen;
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

    initable_t *dirs = ini_get_table(&g_profile.ini, "dirs");
    if (!ini_to_str(ini_get(dirs, "save"), g_profile.save, sizeof(g_profile.save), false)) return SDL_SetError("missing field \"dirs.save\" in profile \"%s\"", path);
    if (!ini_to_str(ini_get(dirs, "system"), g_profile.system, sizeof(g_profile.system), false)) return SDL_SetError("missing field \"dirs.system\" in profile \"%s\"", path);

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

bool Profile_IsFullscreen(void)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    return g_profile.fullscreen;
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
