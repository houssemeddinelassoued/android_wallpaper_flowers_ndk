#ifndef GL_CONTEXT_H__
#define GL_CONTEXT_H__

#include <EGL/egl.h>
#include "gl_defs.h"

bool_t gl_ContextCreate(gl_egl_vars_t* eglVars);

void gl_ContextRelease(gl_egl_vars_t* eglVars);

#endif
