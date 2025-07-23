#include "core.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_stdinc.h>

#include "gl.h"
#include "profile.h"
#include "libretro.h"

#define RETRO_API_DECL_LIST \
    _X(void,     retro_get_system_info,            struct retro_system_info* info) \
    _X(void,     retro_set_environment,            retro_environment_t callback) \
    _X(void,     retro_set_video_refresh,          retro_video_refresh_t callback) \
    _X(void,     retro_set_audio_sample,           retro_audio_sample_t callback) \
    _X(void,     retro_set_audio_sample_batch,     retro_audio_sample_batch_t callback) \
    _X(void,     retro_set_input_poll,             retro_input_poll_t callback) \
    _X(void,     retro_set_input_state,            retro_input_state_t callback) \
    _X(void,     retro_init,                       void) \
    _X(void,     retro_deinit,                     void) \
    _X(unsigned, retro_api_version,                void) \
    _X(void,     retro_get_system_av_info,         struct retro_system_av_info* avinfo) \
    _X(void,     retro_set_controller_port_device, unsigned port, unsigned device) \
    _X(void,     retro_reset,                      void) \
    _X(void,     retro_run,                        void) \
    _X(size_t,   retro_serialize_size,             void) \
    _X(bool,     retro_serialize,                  void *data, size_t len) \
    _X(bool,     retro_unserialize,                const void *data, size_t len) \
    _X(void,     retro_cheat_reset,                void) \
    _X(void,     retro_cheat_set,                  unsigned index, bool enabled, const char *code) \
    _X(bool,     retro_load_game,                  const struct retro_game_info *info) \
    _X(bool,     retro_load_game_special,          unsigned type, const struct retro_game_info *info, size_t count) \
    _X(void,     retro_unload_game,                void) \
    _X(unsigned, retro_get_region,                 void) \
    _X(void*,    retro_get_memory_data,            unsigned type) \
    _X(size_t,   retro_get_memory_size,            unsigned type)

static struct {
    bool initialized;
    SDL_SharedObject *so;
    struct retro_system_info info;
    struct retro_system_av_info avinfo;
    struct retro_hw_render_callback *hw;
    struct {
        #define _X(_ret, _name, _arg1, ...) _ret (*_name)(_arg1, ##__VA_ARGS__);
        RETRO_API_DECL_LIST
        #undef _X
    } api;
    struct {
        char save[256];
        char system[256];
    } paths;
    float current_width, current_height;
} g_core;

static retro_proc_address_t Core_GlGetProcAddress(const char *sym);

static void    Core_LogCb(enum retro_log_level level, const char *format, ...);
static bool    Core_EnvCb(unsigned cmd, void *data);
static void    Core_VideoCb(const void *data, unsigned width, unsigned height, size_t pitch);
static void    Core_AudioSampleCb(int16_t left, int16_t right);
static size_t  Core_AudioBatchCb(const int16_t *data, size_t frames);
static void    Core_InputPollCb(void);
static int16_t Core_InputStateCb(unsigned port, unsigned device, unsigned index, unsigned id);

bool Core_Load(const char *path)
{
    Core_Free();

    if (!(g_core.so = SDL_LoadObject(path))) return false;

    #define _X(_ret, _name, _arg1, ...) if (!(g_core.api._name = (void*)SDL_LoadFunction(g_core.so, #_name))) return SDL_SetError("core does not implement " #_name "() function");
    RETRO_API_DECL_LIST
    #undef _X

    g_core.initialized = true;

    g_core.api.retro_get_system_info(&g_core.info);

    g_core.api.retro_set_environment(Core_EnvCb);
    g_core.api.retro_set_video_refresh(Core_VideoCb);
    g_core.api.retro_set_audio_sample(Core_AudioSampleCb);
    g_core.api.retro_set_audio_sample_batch(Core_AudioBatchCb);
    g_core.api.retro_set_input_poll(Core_InputPollCb);
    g_core.api.retro_set_input_state(Core_InputStateCb);
    g_core.api.retro_init();

    g_core.api.retro_get_system_av_info(&g_core.avinfo);

    SDL_Log(
        "loaded core %s (%.0f FPS, %dx%d)",
        g_core.info.library_name,
        g_core.avinfo.timing.fps,
        g_core.avinfo.geometry.max_width,
        g_core.avinfo.geometry.max_height
    );
    return SDL_ClearError();
}

bool Core_LoadGame(const char *path)
{
    if (!g_core.initialized) return SDL_SetError("core was not initialized");
    struct retro_game_info info = { .path = path };
    return g_core.api.retro_load_game(&info);
}

void Core_Free(void)
{
    if (!g_core.initialized)
        return;

    g_core.api.retro_unload_game();
    g_core.api.retro_deinit();
    SDL_UnloadObject(g_core.so);
    SDL_memset(&g_core, 0, sizeof(g_core));
    SDL_ClearError();
}

void Core_RunFrame(void)
{
    SDL_assert_release(g_core.initialized);
    g_core.api.retro_run();
}

float Core_GetRenderWidth(void)
{
    SDL_assert_release(g_core.initialized);
    return g_core.current_width;
}

float Core_GetRenderHeight(void)
{
    SDL_assert_release(g_core.initialized);
    return g_core.current_height;
}

retro_proc_address_t Core_GlGetProcAddress(const char *sym)
{
    SDL_assert_release(g_core.initialized);
    return (retro_proc_address_t)Gl_GetProcAddress(sym);
}

void Core_LogCb(enum retro_log_level level, const char *format, ...)
{
    SDL_assert_release(g_core.initialized);

    char b[1024];
    va_list args;
    va_start(args, format);
    SDL_vsnprintf(b, sizeof(b), format, args);
    va_end(args);

    for (int i = 0; i < sizeof(b); i++) if (b[i] == '\n') b[i] = ' ';

    SDL_Log("%s", b);
}

bool Core_EnvCb(unsigned cmd, void *data)
{
    SDL_assert_release(g_core.initialized);

    switch (cmd)
    {
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        ((struct retro_log_callback*)data)->log = Core_LogCb;
        return true;

    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        *(const char**)data = Profile_GetSystemPath();
        return true;

    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char**)data = Profile_GetSavePath();
        return true;

    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return *(enum retro_pixel_format*)data == RETRO_PIXEL_FORMAT_XRGB8888;

    case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER:
        *(unsigned*)data = RETRO_HW_CONTEXT_OPENGL_CORE;
        return true;

    case RETRO_ENVIRONMENT_SET_HW_RENDER:
        g_core.hw = data;

        if (!Gl_Configure(
            g_core.hw->version_major,
            g_core.hw->version_minor,
            g_core.avinfo.geometry.max_width,
            g_core.avinfo.geometry.max_height
        )) return false;

        g_core.hw->get_proc_address = Core_GlGetProcAddress;
        g_core.hw->get_current_framebuffer = Gl_GetFramebuffer;
        g_core.hw->context_reset();
        return true;

    case RETRO_ENVIRONMENT_GET_VARIABLE:
        struct retro_variable *v = data;
        return (v->value = Profile_GetVarValueByName(v->key)) != 0;

    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        return false;
    }

    SDL_Log("unhandled core command %u", cmd);
    return false;
}

void Core_VideoCb(const void *data, unsigned width, unsigned height, size_t pitch)
{
    SDL_assert_release(g_core.initialized);
    g_core.current_width = width;
    g_core.current_height = height;
}

void Core_AudioSampleCb(int16_t left, int16_t right)
{
    SDL_assert_release(g_core.initialized);
}

size_t Core_AudioBatchCb(const int16_t *data, size_t frames)
{
    SDL_assert_release(g_core.initialized);
    return frames;
}

void Core_InputPollCb(void)
{
    SDL_assert_release(g_core.initialized);
}

int16_t Core_InputStateCb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    SDL_assert_release(g_core.initialized);
    return 0;
}
