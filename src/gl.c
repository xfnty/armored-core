#include "gl.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
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
    _X(PFNGLCHECKFRAMEBUFFERSTATUSPROC,  glCheckFramebufferStatus)

static struct {
    bool initialized;
    SDL_GLContext ctx;
    GLuint vbo;
    GLuint vao;
    GLuint shader;
    GLuint tex;
    GLuint fbo;
    float max_width, max_height;
} g_gl;

static SDL_Window *g_gl_window;

#define _X(_T, _n) _T _n;
OPENGL_EXT_API_LIST
#undef _X

static void Gl_Free(void)
{
    SDL_GL_DestroyContext(g_gl.ctx);
    SDL_memset(&g_gl, 0, sizeof(g_gl));
} 

void Gl_Init(SDL_Window *window)
{
    SDL_assert_release(window);
    Gl_Free();
    g_gl_window = window;
}

bool Gl_Configure(SDL_GLProfile profile, int version_major, int version_minor, int max_width, int max_height)
{
    Gl_Free();

    int error_line = 0;

    g_gl.max_width = max_width;
    g_gl.max_height = max_height;

    SDL_assert_release(g_gl_window);
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile)) { error_line = __LINE__; { error_line = __LINE__; goto Failure; } }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, version_major)) { error_line = __LINE__; goto Failure; }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, version_minor)) { error_line = __LINE__; goto Failure; }
    if (!(g_gl.ctx = SDL_GL_CreateContext(g_gl_window))) { error_line = __LINE__; goto Failure; }
    if (!SDL_GL_MakeCurrent(g_gl_window, g_gl.ctx)) { error_line = __LINE__; goto Failure; }

    #define _X(_T, _n) if (!(_n = (_T)SDL_GL_GetProcAddress(#_n))) { error_line = __LINE__; goto Failure; }
    OPENGL_EXT_API_LIST
    #undef _X

    glGenVertexArrays(1, &g_gl.vao);
    glBindVertexArray(g_gl.vao);
    glGenBuffers(1, &g_gl.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_gl.vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, 0, sizeof(float) * 4, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, 0, sizeof(float) * 4, (void*)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);
    if (glGetError() != 0) { error_line = __LINE__; goto Failure; }

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, (const char*[]){
        "#version 300 core\n"
        "layout (location = 0) in vec2 xy;\n"
        "layout (location = 1) in vec2 uv;\n"
        "out vec2 uv_out;\n"
        "void main() {\n"
        "    uv_out = uv;\n"
        "    gl_Position = vec4(xy.x, xy.y, 0.0, 1.0); \n"
        "}"
    }, 0);
    glCompileShader(vs);
    if (glGetError() != 0) { error_line = __LINE__; goto Failure; }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, (const char*[]){
        "#version 300 core\n"
        "in vec2 uv_out;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D tex;\n"
        "void main() {\n"
        "    FragColor = texture(tex, uv_out);\n"
        "} "
    }, 0);
    glCompileShader(fs);
    if (glGetError() != 0) { error_line = __LINE__; goto Failure; }

    g_gl.shader = glCreateProgram();
    glAttachShader(g_gl.shader, vs);
    glAttachShader(g_gl.shader, fs);
    glLinkProgram(g_gl.shader);
    if (glGetError() != 0) { error_line = __LINE__; goto Failure; }

    glGenTextures(1, &g_gl.tex);
    glBindTexture(GL_TEXTURE_2D, g_gl.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_gl.max_width, g_gl.max_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    if (glGetError() != 0) { error_line = __LINE__; goto Failure; }

    glGenFramebuffers(1, &g_gl.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_gl.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_gl.tex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { error_line = __LINE__; goto Failure; }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    SDL_Log(
        "configured OpenGL %s on %s (%.0fx%.0f)",
        glGetString(GL_VERSION),
        glGetString(GL_RENDERER),
        g_gl.max_width,
        g_gl.max_height
    );
    return true;

    Failure:
    Gl_Free();
    const char *profile_names[] = {
        [SDL_GL_CONTEXT_PROFILE_CORE] = "Core",
        [SDL_GL_CONTEXT_PROFILE_COMPATIBILITY] = "Compatibility",
        [SDL_GL_CONTEXT_PROFILE_ES] = "ES",
    };
    SDL_Log("failed to configure OpenGL %d.%d %s (%dx%d) (line %d)", version_major, version_minor, profile_names[profile], max_width, max_height, error_line);
    return false;
}

GLuint Gl_GetFramebuffer(void)
{
    return g_gl.fbo;
}

void Gl_Present(float rw, float rh)
{
    glBindFramebuffer(GL_FRAMEBUFFER, g_gl.fbo);
    glViewport(0, 0, rw, rh);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int ww, wh;
    SDL_GetWindowSize(g_gl_window, &ww, &wh);
    glViewport(0, 0, ww, wh);

    float wr = ww / (float)wh;
    float rr = rw / (float)rh;

    float x, y, w, h;
    if (wr > rr)
    {
        w = wh * rr;
        h = wh;
        x = (ww - w) / 2;
        y = 0;
    }
    else
    {
        w = ww;
        h = ww / rr;
        x = 0;
        y = (wh - h) / 2;
    }

    float u = rw / (float)g_gl.max_width;
    float v = rh / (float)g_gl.max_height;

    float l = x / ww * 2 - 1;
    float t = 1 - y / wh * 2;
    float r = l + w / ww * 2;
    float b = t - h / wh * 2;

    float verts[] = {
        l, t, 0, v,
        r, t, u, v,
        r, b, u, 0, 
        l, t, 0, v,
        r, b, u, 0,
        l, b, u, 0,
    };
    glBindBuffer(GL_ARRAY_BUFFER, g_gl.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindBuffer(GL_ARRAY_BUFFER, g_gl.vbo);
    glBindVertexArray(g_gl.vao);
    glBindTexture(GL_TEXTURE_2D, g_gl.tex);
    glUseProgram(g_gl.shader);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    SDL_GL_SwapWindow(g_gl_window);
}
