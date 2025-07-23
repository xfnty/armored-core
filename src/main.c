#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>

#include "gl.h"
#include "profile.h"

static struct {
    SDL_Window *window;
    bool is_fullscreen;
} g_app;

SDL_AppResult SDL_AppInit(void **userdata, int argc, char **argv)
{
    SDL_SetAppMetadata("Emulator", "0.1.0", "com.xfnty.libretro-frontend");

    SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);
    g_app.window = SDL_CreateWindow("Emulator", 640, 360, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    SDL_assert_release(g_app.window);
    SDL_ShowWindow(g_app.window);

    Gl_Init(g_app.window);
    Gl_Configure(SDL_GL_CONTEXT_PROFILE_CORE, 3, 0, 1024, 512);

    if (argc == 2 && !Profile_Load(argv[1]))
    {
        SDL_Log("Failed to load profile \"%s\"", argv[1]);
        return SDL_APP_FAILURE;
    }

    SDL_Log("core=\"%s\"", Profile_GetCorePath());

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *userdata)
{
    Gl_Present(1024, 512);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *userdata, SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;

    case SDL_EVENT_KEY_DOWN:
        if (!event->key.repeat && event->key.key == SDLK_F11)
        {
            g_app.is_fullscreen = !g_app.is_fullscreen;
            SDL_SetWindowFullscreen(g_app.window, g_app.is_fullscreen);
        }
        break;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *userdata, SDL_AppResult result) {}
