#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_opengl.h>

static struct {
    SDL_Window *window;
    SDL_GLContext opengl;
} g_app;

SDL_AppResult SDL_AppInit(void **userdata, int argc, char **argv)
{
    SDL_SetAppMetadata("Emulator", "0.1.0", "com.xfnty.libretro-frontend");

    SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);

    g_app.window = SDL_CreateWindow("Emulator", 500, 500, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    SDL_assert_release(g_app.window);

    SDL_assert_release(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3));
    SDL_assert_release(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3));
    SDL_assert_release(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE));
    g_app.opengl = SDL_GL_CreateContext(g_app.window);
    SDL_assert_release(g_app.opengl);
    SDL_assert_release(SDL_GL_MakeCurrent(g_app.window, g_app.opengl));

    SDL_ShowWindow(g_app.window);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *userdata)
{
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(g_app.window);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *userdata, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *userdata, SDL_AppResult result)
{
    SDL_GL_DestroyContext(g_app.opengl);
}
