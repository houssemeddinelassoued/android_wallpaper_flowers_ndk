/*
 Copyright 2012 Harri Smått

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#ifndef GL_THREAD_H__
#define GL_THREAD_H__

#include <EGL/egl.h>
#include <android/native_window.h>

// Boolean values for gl thread functions.
typedef unsigned char gl_thread_bool_t;
#define GL_THREAD_TRUE   1
#define GL_THREAD_FALSE  0

// Callback function definitions.
typedef EGLConfig (*gl_ChooseConfig_t)(EGLDisplay display,
		EGLConfig* configArray, int configCount);
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

// Creates a new gl thread. If there is a thread running already it is always
// stopped before creating a new one. Meaning ultimately that there is exactly
// one thread running at all times. Thread is initially in paused state.
void gl_ThreadCreate(gl_thread_funcs_t *threadFuncs);

// Destroys current thread if there is one. Returns only after thread
// has exited its execution all resources are freed.
void gl_ThreadDestroy();

// Sets gl thread paused state. In paused state EGL context is released.
void gl_ThreadSetPaused(gl_thread_bool_t paused);

// Sets new native window for creating EGLSurface.
void gl_ThreadSetWindow(ANativeWindow* window);

// Sets new native window size.
void gl_ThreadSetWindowSize(int width, int height);

// Locks render thread for communicating with it safely. You
// have to call gl_ThreadUnlock after you're done with
// lock in order to let rendering thread continue.
// gl_ThreadLock returns only after it has received
// lock for rendering thread.
void gl_ThreadLock();

// Releases render thread lock.
void gl_ThreadUnlock();

#endif
