#ifndef FLOWERS_CONFIGCHOOSER_H__
#define FLOWERS_CONFIGCHOOSER_H__

#include <EGL/egl.h>

EGLConfig flowers_ChooseConfig(EGLDisplay eglDisplay, EGLConfig* configArray, int configCount);

#endif
