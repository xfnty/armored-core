#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_opengl_glext.h>

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

static struct {
    SDL_Window *window;
} g_app;

static struct {
    SDL_GLContext ctx;
    GLuint vbo;
    GLuint vao;
    GLuint shader;
    GLuint tex;
    GLuint fbo;
} g_gfx;

static struct {
    int width, height;
} g_core;

#define _X(_T, _n) _T _n;
OPENGL_EXT_API_LIST
#undef _X

bool InitOpenGL(SDL_GLProfile profile, int version_major, int version_minor, int max_width, int max_height);

SDL_AppResult SDL_AppInit(void **userdata, int argc, char **argv)
{
    SDL_SetAppMetadata("Emulator", "0.1.0", "com.xfnty.libretro-frontend");

    SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);
    g_app.window = SDL_CreateWindow("Emulator", 500, 500, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    SDL_assert_release(g_app.window);
    SDL_ShowWindow(g_app.window);

    InitOpenGL(SDL_GL_CONTEXT_PROFILE_CORE, 3, 3, 1024, 512);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *userdata)
{
    SDL_GetWindowSize(g_app.window, &g_core.width, &g_core.height);

    glBindFramebuffer(GL_FRAMEBUFFER, g_gfx.fbo);
    glViewport(0, 0, g_core.width, g_core.height);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    // retro_run();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *userdata, SDL_AppResult result)
{
}

bool InitOpenGL(SDL_GLProfile profile, int version_major, int version_minor, int max_width, int max_height)
{
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

    float verts[] = {
        -1,  1, 0, 1,
         1,  1, 1, 1, 
         1, -1, 1, 0, 
        -1,  1, 0, 1,
         1, -1, 1, 0, 
        -1, -1, 0, 0, 
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
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

    return ok;
}
