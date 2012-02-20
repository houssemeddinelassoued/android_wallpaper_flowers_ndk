#ifndef GL_CONTEXT_H__
#define GL_CONTEXT_H__

#include <EGL/egl.h>
#include <android/native_window.h>
#include "gl_defs.h"

typedef EGLConfig (*gl_ChooseConfig_t) (EGLDisplay, EGLConfig*, int);

bool_t gl_ContextCreate(gl_egl_vars_t* eglVars,
		gl_ChooseConfig_t chooseConfig);
void gl_ContextRelease(gl_egl_vars_t* eglVars);

#endif
