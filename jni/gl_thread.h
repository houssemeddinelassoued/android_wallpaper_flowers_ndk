#ifndef GL_THREAD_H__
#define GL_THREAD_H__

#include <android/native_window.h>
#include <EGL/egl.h>
#include "gl_defs.h"
#include "gl_context.h"

typedef ANativeWindow* (*gl_GetNativeWindow_t)(void);
typedef void (*gl_OnRenderFrame_t)(void);
typedef void (*gl_OnContextCreated_t)(void);
typedef void (*gl_OnSurfaceChanged_t)(int width, int height);

typedef struct {
	gl_ChooseConfig_t gl_ChooseConfig;
	gl_OnRenderFrame_t gl_OnRenderFrame;
	gl_OnContextCreated_t gl_OnContextCreated;
	gl_OnSurfaceChanged_t gl_OnSurfaceChanged;
} gl_thread_params_t;

// Creates a new gl thread. If there is a thread running already it is always
// stopped before creating a new one. Meaning ultimately that there is exactly
// one thread running at all times. Thread is initially in paused state.
void gl_ThreadCreate(gl_thread_params_t *params);

// Destroys current thread if there is one. Returns only after thread
// has exited its execution all resources are freed.
void gl_ThreadDestroy();

// Sets gl thread paused state. In paused state EGL context is released.
void gl_ThreadSetPaused(bool_t paused);

// Sets new native window for creating EGLSurface.
void gl_ThreadSetSurface(ANativeWindow *nativeWindow);

// Sets new surface size.
void gl_ThreadSetSurfaceSize(int width, int height);

#endif
