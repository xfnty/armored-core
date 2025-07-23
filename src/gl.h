#pragma once

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_opengl.h>

void Gl_Init(SDL_Window *window);
bool Gl_Configure(SDL_GLProfile profile, int version_major, int version_minor, int max_width, int max_height);

GLuint Gl_GetFramebuffer(void);

void Gl_Present(float w, float h);
