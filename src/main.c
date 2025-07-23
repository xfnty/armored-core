#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>

#include "gl.h"
#include "core.h"
#include "profile.h"

static struct {
    SDL_Window *window;
    Uint64 prev_frame_time, target_frame_time;
} g_app;

static bool ApplyProfile(void);

SDL_AppResult SDL_AppInit(void **userdata, int argc, char **argv)
{
    SDL_SetAppMetadata("Emulator", "0.1.0", "com.xfnty.libretro-frontend");

    if (argc != 2)
    {
        SDL_SetError("Usage: emu <profile.cfg>");
        return SDL_APP_FAILURE;
    }

    if (!Profile_Load(argv[1]))
        return SDL_APP_FAILURE;

    SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_WindowFlags wflags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
    if (Profile_IsFullscreen()) wflags |= SDL_WINDOW_FULLSCREEN;
    g_app.window = SDL_CreateWindow("Emulator", 640, 360, wflags);
    SDL_assert_release(g_app.window);
    Gl_Init(g_app.window);

    if (!Core_Load(Profile_GetCorePath()) || !Core_LoadGame(Profile_GetGamePath()))
        return SDL_APP_FAILURE;

    g_app.target_frame_time = (1 / Core_GetTargetFPS()) * 1000000000;

    SDL_ShowWindow(g_app.window);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *userdata)
{
    Core_RunFrame();
    Gl_Present(Core_GetRenderWidth(), Core_GetRenderHeight());

    Uint64 time = SDL_GetTicksNS();
    Uint64 frame_time = time - g_app.prev_frame_time;
    g_app.prev_frame_time = time;
    if (frame_time < g_app.target_frame_time)
        SDL_DelayNS(g_app.target_frame_time - frame_time);

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
            SDL_SetWindowFullscreen(g_app.window, !(SDL_GetWindowFlags(g_app.window) & SDL_WINDOW_FULLSCREEN));
        break;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *userdata, SDL_AppResult result)
{
    if (result == SDL_APP_FAILURE)
        SDL_Log("%s", SDL_GetError());
    Core_Free();
}
