#include "profile.h"

#include <SDL3/SDL.h>

#define INI_IMPLEMENTATION
#include "ini.h"

static struct {
    ini_t ini;
    char core[256];
} g_profile;

static void Profile_Free(void)
{
    ini_free(&g_profile.ini);
    SDL_memset(&g_profile, 0, sizeof(g_profile));
}

bool Profile_Load(const char *path)
{
    Profile_Free();
    g_profile.ini = ini_parse(path, NULL);
    if (!ini_is_valid(&g_profile.ini)) return false;
    
    initable_t *general = ini_get_table(&g_profile.ini, "general");
    if (!ini_to_str(ini_get(general, "core"), g_profile.core, sizeof(g_profile.core), false)) return false;

    return true;
}

const char *Profile_GetCorePath(void)
{
    SDL_assert_release(ini_is_valid(&g_profile.ini));
    return g_profile.core;
}
