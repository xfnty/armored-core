#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>

#define INI_IMPLEMENTATION
#include "ini.h"
#include "libretro.h"

#define OPENGL_EXT_API_LIST \
    _X(PFNGLGENVERTEXARRAYSPROC,         glGenVertexArrays) \
    _X(PFNGLBINDVERTEXARRAYPROC,         glBindVertexArray) \
    _X(PFNGLGENBUFFERSPROC,              glGenBuffers) \
    _X(PFNGLBINDBUFFERPROC,              glBindBuffer) \
    _X(PFNGLBUFFERDATAPROC,              glBufferData) \
    _X(PFNGLVERTEXATTRIBPOINTERPROC,     glVertexAttribPointer) \
    _X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
    _X(PFNGLCREATESHADERPROC,            glCreateShader) \
    _X(PFNGLSHADERSOURCEPROC,            glShaderSource) \
    _X(PFNGLCOMPILESHADERPROC,           glCompileShader) \
    _X(PFNGLGETSHADERIVPROC,             glGetShaderiv) \
    _X(PFNGLCREATEPROGRAMPROC,           glCreateProgram) \
    _X(PFNGLATTACHSHADERPROC,            glAttachShader) \
    _X(PFNGLLINKPROGRAMPROC,             glLinkProgram) \
    _X(PFNGLUSEPROGRAMPROC,              glUseProgram) \
    _X(PFNGLGENFRAMEBUFFERSPROC,         glGenFramebuffers) \
    _X(PFNGLBINDFRAMEBUFFERPROC,         glBindFramebuffer) \
    _X(PFNGLFRAMEBUFFERTEXTURE2DPROC,    glFramebufferTexture2D) \
    _X(PFNGLCHECKFRAMEBUFFERSTATUSPROC,  glCheckFramebufferStatus) \

typedef struct JoypadInputBinding JoypadInputBinding;
struct JoypadInputBinding {
    SDL_Keycode keyboard_key;
    int joypad_key;
};

static struct {
    SDL_Window *window;
    bool is_fullscreen;
} g_app;

static struct {
    bool initialized;
    SDL_GLContext ctx;
    GLuint vbo;
    GLuint vao;
    GLuint shader;
    GLuint tex;
    GLuint fbo;
} g_gfx;

static struct {
    struct retro_system_info    info;
    struct retro_system_av_info avinfo;
    int current_width, current_height;
} g_core;

static struct {
    bool ok;
    ini_t ini;
} g_profile;

#define _X(_T, _n) _T _n;
OPENGL_EXT_API_LIST
#undef _X

bool ConfigureOpenGL(SDL_GLProfile profile, int version_major, int version_minor, int max_width, int max_height);
void UpdateVboLetterboxed(int render_width, int render_height, int max_render_width, int max_render_height, int window_width, int window_height);
bool LoadProfile(const char *path);

SDL_AppResult SDL_AppInit(void **userdata, int argc, char **argv)
{
    SDL_SetAppMetadata("Emulator", "0.1.0", "com.xfnty.libretro-frontend");

    SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);
    g_app.window = SDL_CreateWindow("Emulator", 640, 360, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    SDL_assert_release(g_app.window);
    SDL_ShowWindow(g_app.window);

    g_core.avinfo.geometry.max_width = 1024;
    g_core.avinfo.geometry.max_height = 512;
    g_core.current_width = 1024;
    g_core.current_height = 512;

    ConfigureOpenGL(
        SDL_GL_CONTEXT_PROFILE_CORE,
        3,
        3,
        g_core.avinfo.geometry.max_width,
        g_core.avinfo.geometry.max_height
    );

    if (argc == 2 && !LoadProfile(argv[1]))
    {
        // show message box
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *userdata)
{
    glBindFramebuffer(GL_FRAMEBUFFER, g_gfx.fbo);
    glViewport(0, 0, g_core.current_width, g_core.current_height);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    // retro_run();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int window_width, window_height;
    SDL_GetWindowSize(g_app.window, &window_width, &window_height);
    glViewport(0, 0, window_width, window_height);
    UpdateVboLetterboxed(
        g_core.current_width,
        g_core.current_height,
        g_core.avinfo.geometry.max_width,
        g_core.avinfo.geometry.max_height,
        window_width,
        window_height
    );

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindBuffer(GL_ARRAY_BUFFER, g_gfx.vbo);
    glBindVertexArray(g_gfx.vao);
    glBindTexture(GL_TEXTURE_2D, g_gfx.tex);
    glUseProgram(g_gfx.shader);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    SDL_GL_SwapWindow(g_app.window);
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

bool ConfigureOpenGL(SDL_GLProfile profile, int version_major, int version_minor, int max_width, int max_height)
{
    if (g_gfx.initialized)
    {
        SDL_GL_DestroyContext(g_gfx.ctx);
        SDL_memset(&g_gfx, 0, sizeof(g_gfx));
    }

    bool ok = true;

    ok &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile);
    ok &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, version_major);
    ok &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, version_minor);
    g_gfx.ctx = SDL_GL_CreateContext(g_app.window);
    ok &= g_gfx.ctx != 0;
    ok &= SDL_GL_MakeCurrent(g_app.window, g_gfx.ctx);
    #define _X(_T, _n) ok &= (bool)(_n = (_T)SDL_GL_GetProcAddress(#_n));
    OPENGL_EXT_API_LIST
    #undef _X

    glGenVertexArrays(1, &g_gfx.vao);
    glBindVertexArray(g_gfx.vao);
    glGenBuffers(1, &g_gfx.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_gfx.vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, 0, sizeof(float) * 4, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, 0, sizeof(float) * 4, (void*)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);
    ok &= !glGetError();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, (const char*[]){
        "#version 330 core\n"
        "layout (location = 0) in vec2 xy;\n"
        "layout (location = 1) in vec2 uv;\n"
        "out vec2 uv_out;\n"
        "void main() {\n"
        "    uv_out = uv;\n"
        "    gl_Position = vec4(xy.x, xy.y, 0.0, 1.0); \n"
        "}"
    }, 0);
    glCompileShader(vs);
    ok &= !glGetError();

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, (const char*[]){
        "#version 330 core\n"
        "in vec2 uv_out;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D tex;\n"
        "void main() {\n"
        "    FragColor = texture(tex, uv_out);\n"
        "} "
    }, 0);
    glCompileShader(fs);
    ok &= !glGetError();

    g_gfx.shader = glCreateProgram();
    glAttachShader(g_gfx.shader, vs);
    glAttachShader(g_gfx.shader, fs);
    glLinkProgram(g_gfx.shader);
    ok &= !glGetError();

    glGenTextures(1, &g_gfx.tex);
    glBindTexture(GL_TEXTURE_2D, g_gfx.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, max_width, max_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    ok &= !glGetError();

    glGenFramebuffers(1, &g_gfx.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_gfx.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_gfx.tex, 0);
    ok &= glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (!ok)
    {
        SDL_GL_DestroyContext(g_gfx.ctx);
        SDL_memset(&g_gfx, 0, sizeof(g_gfx));
    }
    else
    {
        SDL_Log(
            "Using OpenGL %s on %s (%dx%d)",
            glGetString(GL_VERSION),
            glGetString(GL_RENDERER),
            max_width,
            max_height
        );
    }

    g_gfx.initialized = ok;
    return ok;
}

void UpdateVboLetterboxed(int render_width, int render_height, int max_render_width, int max_render_height, int window_width, int window_height)
{
    SDL_assert_release(g_gfx.initialized);

    float wr = window_width / (float)window_height;
    float rr = render_width / (float)render_height;

    float x, y, w, h;
    if (wr > rr)
    {
        w = window_height * rr;
        h = window_height;
        x = (window_width - w) / 2;
        y = 0;
    }
    else
    {
        w = window_width;
        h = window_width / rr;
        x = 0;
        y = (window_height - h) / 2;
    }

    float u = render_width / (float)max_render_width;
    float v = render_height / (float)max_render_height;

    float l = x / window_width * 2 - 1;
    float t = 1 - y / window_height * 2;
    float r = l + w / window_width * 2;
    float b = t - h / window_height * 2;

    float verts[] = {
        l, t, 0, v,
        r, t, u, v,
        r, b, u, 0, 
        l, t, 0, v,
        r, b, u, 0,
        l, b, u, 0,
    };
    glBindBuffer(GL_ARRAY_BUFFER, g_gfx.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    SDL_assert_release(!glGetError());
}

bool LoadProfile(const char *path)
{
    ini_free(&g_profile.ini);
    SDL_memset(&g_profile, 0, sizeof(g_profile));

    g_profile.ini = ini_parse(path, &(iniopts_t){ .merge_duplicate_tables = true, .override_duplicate_keys = true });
    if (!ini_is_valid(&g_profile.ini))
    {
        SDL_Log("failed to load profile \"%s\"", path);
        return false;
    }

    SDL_Log("loaded profile \"%s\"", path);

    g_profile.ok = true;
    return g_profile.ok;

    Failure:
    ini_free(&g_profile.ini);
    SDL_memset(&g_profile, 0, sizeof(g_profile));
    return false;
}
