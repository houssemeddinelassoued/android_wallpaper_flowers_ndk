#ifndef GL_THREAD_H__
#define GL_THREAD_H__

#include <EGL/egl.h>

// Callback function definitions.
typedef EGLConfig (*gl_ChooseConfig_t) (EGLDisplay, EGLConfig*, int);
typedef void (*gl_OnRenderFrame_t)(void);
typedef void (*gl_OnSurfaceCreated_t)(void);
typedef void (*gl_OnSurfaceChanged_t)(int width, int height);

// Callback functions struct definition.
typedef struct {
	gl_ChooseConfig_t chooseConfig;
	gl_OnRenderFrame_t onRenderFrame;
	gl_OnSurfaceCreated_t onSurfaceCreated;
	gl_OnSurfaceChanged_t onSurfaceChanged;
} gl_thread_funcs_t;

#endif
