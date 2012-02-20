#ifndef GL_SURFACE_H__
#define GL_SURFACE_H__

#include <android/native_window.h>
#include "gl_defs.h"

bool_t gl_SurfaceCreate(gl_egl_vars_t* eglVars, ANativeWindow* nativeWin);
void gl_SurfaceRelease(gl_egl_vars_t* eglVars);

#endif
