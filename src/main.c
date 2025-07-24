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

#define FPS_DISPLAY_UPDATE_INTERVAL 0.5f

static struct {
    SDL_Window *window;
    Uint64 last_frame_tick;
    Uint64 frame_time_acc;
    Uint64 frame_acc_count;
    Uint64 last_fps_update_time;
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

    SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);

    SDL_WindowFlags wflags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
    if (Profile_IsFullscreen()) wflags |= SDL_WINDOW_FULLSCREEN;
    g_app.window = SDL_CreateWindow("Emulator", 640, 360, wflags);
    SDL_assert_release(g_app.window);
    Gl_Init(g_app.window);

    if (!Core_Load(Profile_GetCorePath()) || !Core_LoadGame(Profile_GetGamePath()))
        return SDL_APP_FAILURE;

    SDL_ShowWindow(g_app.window);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *userdata)
{
    Uint64 tick = SDL_GetTicks();
    if ((tick - g_app.last_frame_tick) / 1000.0 >= 1 / Core_GetTargetFPS())
    {
        Core_RunFrame();
        Gl_Present(Core_GetRenderWidth(), Core_GetRenderHeight());
        g_app.last_frame_tick = tick;
        g_app.frame_time_acc += SDL_GetTicks() - tick;
        g_app.frame_acc_count++;
    }

    if (tick - g_app.last_fps_update_time > FPS_DISPLAY_UPDATE_INTERVAL * 1000)
    {
        float ms = g_app.frame_time_acc / (float)g_app.frame_acc_count;
        char b[128];
        SDL_snprintf(b, sizeof(b), "%.0f ms (%d FPS)", ms, (int)(1.0f / (ms / 1000.0f)));
        SDL_SetWindowTitle(g_app.window, b);
        g_app.last_fps_update_time = tick;
        g_app.frame_time_acc = 0;
        g_app.frame_acc_count = 0;
    }

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
