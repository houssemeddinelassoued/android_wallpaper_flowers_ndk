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

#include <stdlib.h>
#include <pthread.h>
#include "gl_thread.h"
#include "log.h"

// Global variables for communicating with rendering thread.
#define GLOBALS gl_thread_globals
typedef struct {
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int mutexCounter;

	gl_thread_bool_t threadCreated;
	gl_thread_bool_t threadPause;
	gl_thread_bool_t threadExit;
	gl_thread_bool_t windowChanged;
	gl_thread_bool_t windowSizeChanged;

	ANativeWindow *window;
	int32_t windowWidth;
	int32_t windowHeight;
} gl_thread_global_t;
gl_thread_global_t GLOBALS;

// EGL structure for storing EGL related variables.
typedef struct {
	EGLDisplay display;
	EGLContext context;
	EGLConfig config;
	EGLSurface surface;
} gl_thread_egl_t;

// Initializes EGL context. Uses chooseConfig callback
// for choosing appropriate EGL configuration.
gl_thread_bool_t gl_ContextCreate(gl_thread_egl_t *egl,
		gl_ChooseConfig_t chooseConfig) {

	// First make sure we got proper attributes.
	if (chooseConfig == NULL || egl->display != EGL_NO_DISPLAY
			|| egl->context != EGL_NO_CONTEXT || egl->surface != EGL_NO_SURFACE) {
		return GL_THREAD_FALSE;
	}

	// Get default display.
	egl->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (egl->display == EGL_NO_DISPLAY) {
		goto error;
	}
	// Try to initialize display.
	if (eglInitialize(egl->display, NULL, NULL) != EGL_TRUE) {
		goto error;
	}

	EGLint numConfigs;
	EGLint configAttrs[] = { EGL_RED_SIZE, 4, EGL_GREEN_SIZE, 4, EGL_BLUE_SIZE,
			4, EGL_ALPHA_SIZE, 0, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
			EGL_NONE };

	// Try to get available configuration count.
	if (eglChooseConfig(egl->display, configAttrs, NULL, 0,
			&numConfigs) != EGL_TRUE) {
		goto error;
	}
	// If configuration count <= 0.
	if (numConfigs <= 0) {
		goto error;
	}

	// Allocate array for holding all configurations.
	EGLint configsSize = numConfigs;
	EGLConfig* configs = (EGLConfig*) malloc(configsSize * sizeof(EGLConfig));

	// Try to get available configurations.
	if (eglChooseConfig(egl->display, configAttrs, configs, configsSize,
			&numConfigs) == EGL_FALSE) {
		free(configs);
		goto error;
	}

	// Ask chooseConfig to pick desired one.
	egl->config = chooseConfig(egl->display, configs, numConfigs);
	free(configs);

	// If chooseConfig returned NULL.
	if (egl->config == NULL) {
		goto error;
	}

	// Finally try to create EGL context for OpenGL ES 2.
	EGLint contextAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	egl->context = eglCreateContext(egl->display, egl->config, EGL_NO_CONTEXT,
			contextAttrs);
	if (egl->context == EGL_NO_CONTEXT) {
		goto error;
	}

	// On success return true.
	return GL_THREAD_TRUE;

	error:
	// On error release allocated resources.
	if (egl->context != EGL_NO_CONTEXT) {
		eglDestroyContext(egl->display, egl->context);
	}
	if (egl->display != EGL_NO_DISPLAY) {
		eglTerminate(egl->display);
	}
	egl->display = EGL_NO_DISPLAY;
	egl->context = EGL_NO_CONTEXT;

	return GL_THREAD_FALSE;
}

// Destroy EGL context.
void gl_ContextDestroy(gl_thread_egl_t *egl) {
	// If we have a display.
	if (egl->display != EGL_NO_DISPLAY) {
		// Release surface and context from current thread.
		eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
				EGL_NO_CONTEXT);
		// If we have a context, destroy it.
		if (egl->context != EGL_NO_CONTEXT) {
			eglDestroyContext(egl->display, egl->context);
		}
		// Release display connection.
		eglTerminate(egl->display);
	}
	egl->display = EGL_NO_DISPLAY;
	egl->context = EGL_NO_CONTEXT;
	egl->surface = EGL_NO_SURFACE;
}

// Create EGL surface.
gl_thread_bool_t gl_SurfaceCreate(gl_thread_egl_t *egl, ANativeWindow *window) {

	// Check we have valid values.
	if (egl->display == EGL_NO_DISPLAY || egl->context == EGL_NO_CONTEXT
			|| egl->surface != EGL_NO_SURFACE || egl->config == NULL
			|| window == NULL) {
		return GL_THREAD_FALSE;
	}

	// Try to create new surface.
	egl->surface = eglCreateWindowSurface(egl->display, egl->config, window,
			NULL);

	// If creation failed.
	if (egl->surface == EGL_NO_SURFACE) {
		return GL_THREAD_FALSE;
	}

	if (eglMakeCurrent(egl->display, egl->surface, egl->surface,
			egl->context) != EGL_TRUE) {
		// If setting up current thread failed.
		eglDestroySurface(egl->display, egl->surface);
		egl->surface = EGL_NO_SURFACE;
		return GL_THREAD_FALSE;
	}

	return GL_THREAD_TRUE;
}

// Destroy EGL surface.
void gl_SurfaceDestroy(gl_thread_egl_t *egl) {
	// If we have reasonable variables.
	if (egl->display != EGL_NO_DISPLAY && egl->surface != EGL_NO_SURFACE) {
		eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
				EGL_NO_CONTEXT);
		eglDestroySurface(egl->display, egl->surface);
	}
	// Mark surface as non existent.
	egl->surface = EGL_NO_SURFACE;
}

// Main rendering thread function.
void* gl_Thread(void *startParams) {

	LOGD("gl_Thread", "start");

	gl_thread_funcs_t *funcs = startParams;
	gl_thread_egl_t egl;

	int width = 0, height = 0;
	gl_thread_bool_t hasContext = GL_THREAD_FALSE;
	gl_thread_bool_t hasSurface = GL_THREAD_FALSE;
	gl_thread_bool_t notifySurfaceCreated = GL_THREAD_FALSE;
	gl_thread_bool_t notifySurfaceChanged = GL_THREAD_FALSE;

	// Acquire mutex lock for synchronization.
	pthread_mutex_lock(&GLOBALS.mutex);

	// Main rendering loop.
	while (!GLOBALS.threadExit) {

		// Inner event loop in which rendering context
		// changes are being handled.
		while (!GLOBALS.threadExit) {
			// If we're asked to pause, release whole EGL context.
			if (GLOBALS.threadPause && hasContext) {
				gl_SurfaceDestroy(&egl);
				gl_ContextDestroy(&egl);
				hasContext = GL_THREAD_FALSE;
				hasSurface = GL_THREAD_FALSE;
			}
			// If window changed release EGL surface.
			if (GLOBALS.windowChanged) {
				GLOBALS.windowChanged = GL_THREAD_FALSE;
				if (hasSurface) {
					gl_SurfaceDestroy(&egl);
					hasSurface = GL_THREAD_FALSE;
				}
			}
			// If we're asked to continue, recreate EGL context and surface.
			if (!GLOBALS.threadPause && !hasContext) {
				hasContext = gl_ContextCreate(&egl, funcs->chooseConfig);
				if (!hasContext) {
					LOGD("gl_Thread", "gl_ContextCreate failed");
				}
			}
			// NOTE: this handles also situations in which surface
			// only was deleted/changed (GLOBALS.windowChanged).
			if (!GLOBALS.threadPause && hasContext && !hasSurface) {
				hasSurface = gl_SurfaceCreate(&egl, GLOBALS.window);
				notifySurfaceCreated = hasSurface;
				if (!hasSurface) {
					LOGD("gl_Thread", "gl_SurfaceCreate failed");
				}
			}
			// If there's windowSizeChanged pending
			// update internal window size.
			// TODO: It's possible window size change
			// requires new EGL surface creation.
			if (GLOBALS.windowSizeChanged) {
				GLOBALS.windowSizeChanged = GL_THREAD_FALSE;
				width = GLOBALS.windowWidth;
				height = GLOBALS.windowHeight;
				notifySurfaceChanged = GL_THREAD_TRUE;
			}

			// If we have all the necessary for rendering
			// exit the wait loop. But only if there are no pending
			// mutex lock requests being made, in which case we try
			// to give them execution time first.
			if (hasContext && hasSurface && width > 0 && height > 0
					&& GLOBALS.mutexCounter == 0) {
				break;
			}

			LOGD("gl_Thread", "wait");

			// Releases mutex and waits for cond to be triggered.
			// If there are pending mutex lock requests they ought
			// to be handled before this function returns and
			// takes mutex ownership back to rendering thread.
			pthread_cond_wait(&GLOBALS.cond, &GLOBALS.mutex);
		}

		// If we exited wait loop for threadExit
		// exit outer main loop instantly.
		if (GLOBALS.threadExit) {
			break;
		}

		// If new surface was created do notifying.
		if (notifySurfaceCreated) {
			notifySurfaceCreated = GL_THREAD_FALSE;
			funcs->onSurfaceCreated();
		}
		// If surface changed do notifying.
		if (notifySurfaceChanged) {
			notifySurfaceChanged = GL_THREAD_FALSE;
			funcs->onSurfaceChanged(width, height);
		}

		// Finally do rendering and swap buffers.
		funcs->onRenderFrame();
		// TODO: In some cases underlying window is
		// destroyed while eglSwapBuffers is still
		// executing (it happens asynchronously after
		// function call has returned). In these
		// cases some error messages are being
		// printed on error console but haven't
		// found a way to prevent it from happening.
		eglSwapBuffers(egl.display, egl.surface);
	}

	// Release mutex.
	pthread_mutex_unlock(&GLOBALS.mutex);

	// Once we get out of rendering loop
	// release EGL surface and context.
	gl_SurfaceDestroy(&egl);
	gl_ContextDestroy(&egl);

	LOGD("gl_Thread", "exit");
	return NULL;
}

gl_thread_bool_t gl_ThreadRunning() {
	if (GLOBALS.threadCreated && !GLOBALS.threadExit) {
		return GL_THREAD_TRUE;
	}
	return GL_THREAD_FALSE;
}

void gl_ThreadCreate(gl_thread_funcs_t *threadFuncs) {
	// If there's thread running, stop it.
	if (GLOBALS.threadCreated) {
		gl_ThreadDestroy();
	}
	// Initialize new thread.
	// TODO: It might be a good idea to add some error checking.
	GLOBALS.threadCreated = GL_THREAD_TRUE;
	GLOBALS.threadPause = GL_THREAD_TRUE;
	pthread_cond_init(&GLOBALS.cond, NULL);
	pthread_mutex_init(&GLOBALS.mutex, NULL);
	pthread_create(&GLOBALS.thread, NULL, gl_Thread, threadFuncs);
}

void gl_ThreadDestroy() {
	// If there's thread running.
	if (GLOBALS.threadCreated) {
		// Mark exit flag.
		GLOBALS.threadExit = GL_THREAD_TRUE;
		// Notify thread.
		pthread_cond_signal(&GLOBALS.cond);
		// Wait until thread has exited.
		pthread_join(GLOBALS.thread, NULL);

		// If there are pending mutex locks let
		// them execute before destroying it.
		// This is needed because otherwise
		// pthread_mutex_destroy will fail.
		pthread_mutex_lock(&GLOBALS.mutex);
		while (GLOBALS.mutexCounter > 0) {
			pthread_cond_wait(&GLOBALS.cond, &GLOBALS.mutex);
		}
		pthread_mutex_unlock(&GLOBALS.mutex);

		// Release all GLOBALS data.
		pthread_cond_destroy(&GLOBALS.cond);
		pthread_mutex_destroy(&GLOBALS.mutex);
		// If we're holding a window release it.
		if (GLOBALS.window) {
			ANativeWindow_release(GLOBALS.window);
		}
		memset(&GLOBALS, 0, sizeof GLOBALS);
	}
}

void gl_ThreadSetPaused(gl_thread_bool_t paused) {
	if (gl_ThreadRunning()) {
		// Acquire thread lock.
		gl_ThreadLock();
		// Set thread paused flag.
		GLOBALS.threadPause = paused;
		// Release thread lock.
		gl_ThreadUnlock();
	}
}

// Sets new native window for creating EGLSurface.
void gl_ThreadSetWindow(ANativeWindow* window) {
	// We accept new window only when rendering
	// thread is active. Otherwise we can't promise
	// it gets released as expected.
	if (!gl_ThreadRunning()) {
		ANativeWindow_release(window);
		return;
	}
	// Acquire thread lock.
	gl_ThreadLock();
	// If we have new nativeWin.
	if (GLOBALS.window != window) {
		// If there is old one, release it first.
		if (GLOBALS.window) {
			ANativeWindow_release(GLOBALS.window);
		}
		// Store new nativeWin and mark it changed.
		GLOBALS.window = window;
		GLOBALS.windowChanged = GL_THREAD_TRUE;
		// Set window size to zero and mark it changed.
		GLOBALS.windowWidth = GLOBALS.windowHeight = 0;
		GLOBALS.windowSizeChanged = GL_THREAD_TRUE;
	}
	// Else if window != NULL.
	else if (window) {
		// Release window instantly.
		ANativeWindow_release(window);
	}
	// Release thread lock.
	gl_ThreadUnlock();
}

void gl_ThreadSetWindowSize(int32_t width, int32_t height) {
	// We accept new size only when rendering
	// thread is running.
	if (!gl_ThreadRunning()) {
		return;
	}
	// Acquire thread lock.
	gl_ThreadLock();
	// If we received new size.
	if (GLOBALS.windowWidth != width || GLOBALS.windowHeight != height) {
		// Store new dimensions.
		GLOBALS.windowWidth = width;
		GLOBALS.windowHeight = height;
		// Mark window size has changed.
		GLOBALS.windowSizeChanged = GL_THREAD_TRUE;
	}
	// Release thread lock.
	gl_ThreadUnlock();
}

void gl_ThreadLock() {
	if (gl_ThreadRunning()) {
		++GLOBALS.mutexCounter;
		pthread_mutex_lock(&GLOBALS.mutex);
	}
}

void gl_ThreadUnlock() {
	if (gl_ThreadRunning()) {
		--GLOBALS.mutexCounter;
		pthread_mutex_unlock(&GLOBALS.mutex);
		pthread_cond_signal(&GLOBALS.cond);
	}
}
