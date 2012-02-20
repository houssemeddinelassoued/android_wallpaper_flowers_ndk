#ifndef GL_THREAD_H__
#define GL_THREAD_H__

#include <android/native_window.h>
#include <EGL/egl.h>
#include "gl_context.h"

typedef ANativeWindow* (*gl_GetNativeWindow_t)(void);
typedef void (*gl_OnRenderFrame_t)(void);
typedef void (*gl_OnContextCreated_t)(void);
typedef void (*gl_OnSurfaceChanged_t)(int width, int height);

typedef struct {
	gl_ChooseConfig_t      gl_ChooseConfig;
	gl_OnRenderFrame_t     gl_OnRenderFrame;
	gl_OnContextCreated_t  gl_OnContextCreated;
	gl_OnSurfaceChanged_t  gl_OnSurfaceChanged;
} gl_thread_params_t;

void gl_ThreadStart(gl_thread_params_t *params);
void gl_ThreadStop();
void gl_ThreadPause();
void gl_ThreadResume();
void gl_ThreadSetSurface(ANativeWindow *nativeWindow);
void gl_ThreadSetSize(int width, int height);

#endif
