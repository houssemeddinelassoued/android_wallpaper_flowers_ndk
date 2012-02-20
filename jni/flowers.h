#ifndef FLOWERS_H__
#define FLOWERS_H__

#include <EGL/egl.h>

EGLConfig flowers_ChooseConfig(EGLDisplay eglDisplay, EGLConfig* configArray, int configCount);
void flowers_OnRenderFrame();
void flowers_OnSurfaceChanged(int width, int height);
void flowers_OnSurfaceCreated();

#endif
