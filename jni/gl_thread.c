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

	gl_thread_bool_t threadCreated;
	gl_thread_bool_t threadPause;
	gl_thread_bool_t threadExit;
	gl_thread_bool_t windowChanged;
	gl_thread_bool_t windowSizeChanged;

	ANativeWindow *window;
	int windowWidth;
	int windowHeight;
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

	// First make sure we got a callback.
	if (chooseConfig == NULL) {
		return GL_THREAD_FALSE;
	}

	// Fetch default display.
	egl->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (egl->display == EGL_NO_DISPLAY) {
		return GL_THREAD_FALSE;
	}
	// Try to initialize EGL.
	if (eglInitialize(egl->display, NULL, NULL) != EGL_TRUE) {
		return GL_THREAD_FALSE;
	}

	EGLint numConfigs;
	EGLint configAttrs[] = { EGL_RED_SIZE, 4, EGL_GREEN_SIZE, 4, EGL_BLUE_SIZE,
			4, EGL_ALPHA_SIZE, 0, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
			EGL_NONE };

	// Try to fetch available configuration count.
	if (eglChooseConfig(egl->display, configAttrs, NULL, 0,
			&numConfigs) != EGL_TRUE) {
		return GL_THREAD_FALSE;
	}
	// If configuration count <= 0.
	if (numConfigs <= 0) {
		return GL_THREAD_FALSE;
	}

	// Allocate array for holding all configurations.
	EGLint configsSize = numConfigs;
	EGLConfig* configs = (EGLConfig*) malloc(configsSize * sizeof(EGLConfig));
	// Try to get available configurations.
	if (eglChooseConfig(egl->display, configAttrs, configs, configsSize,
			&numConfigs) == EGL_FALSE) {
		free(configs);
		return GL_THREAD_FALSE;
	}

	// Ask chooseConfig to pick desired one.
	egl->config = chooseConfig(egl->display, configs, numConfigs);
	free(configs);

	// If chooseConfig returned NULL.
	if (egl->config == NULL) {
		return GL_THREAD_FALSE;
	}

	// Finally try to create EGL context for OpenGL ES 2.
	EGLint contextAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	egl->context = eglCreateContext(egl->display, egl->config, EGL_NO_CONTEXT,
			contextAttrs);
	if (egl->context == EGL_NO_CONTEXT) {
		return GL_THREAD_FALSE;
	}

	// Since we created context, there's no EGL surface.
	egl->surface = EGL_NO_SURFACE;
	return GL_THREAD_TRUE;
}

// Releases EGL context.
void gl_ContextRelease(gl_thread_egl_t *egl) {
	// If we have a context, release it.
	if (egl->context != EGL_NO_CONTEXT) {
		if (eglDestroyContext(egl->display, egl->context) != EGL_TRUE) {
		}
		egl->context = EGL_NO_CONTEXT;
	}
	// If we have a display, release it.
	if (egl->display != EGL_NO_DISPLAY) {
		if (eglTerminate(egl->display) != EGL_TRUE) {
		}
		egl->display = EGL_NO_DISPLAY;
	}
	// After releasing context EGL surface becomes obsolete.
	egl->surface = EGL_NO_SURFACE;
}

gl_thread_bool_t gl_SurfaceCreate(gl_thread_egl_t *egl, ANativeWindow *window) {
	// First check we have valid values.
	if (egl->display == EGL_NO_DISPLAY) {
		return GL_THREAD_FALSE;
	}
	if (egl->config == NULL) {
		return GL_THREAD_FALSE;
	}
	if (window == NULL) {
		return GL_THREAD_FALSE;
	}

	// If we have a surface, release it first.
	if (egl->surface != EGL_NO_SURFACE) {
		eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
				EGL_NO_CONTEXT);
		eglDestroySurface(egl->display, egl->surface);
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
		return GL_THREAD_FALSE;
	}

	return GL_THREAD_TRUE;
}

void gl_SurfaceRelease(gl_thread_egl_t *egl) {
	// If we have reasonable variables.
	if (egl->display != EGL_NO_DISPLAY && egl->surface != EGL_NO_SURFACE) {
		eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
				EGL_NO_DISPLAY);
		eglDestroySurface(egl->display, egl->surface);
	}
	// Mark surface as non existent.
	egl->surface = EGL_NO_SURFACE;
}

// Main rendering thread function.
void* gl_Thread(void *startParams) {
	gl_thread_funcs_t *funcs = startParams;
	gl_thread_egl_t egl;

	int width = 0, height = 0;
	gl_thread_bool_t hasContext = GL_THREAD_FALSE;
	gl_thread_bool_t hasSurface = GL_THREAD_FALSE;
	gl_thread_bool_t notifySurfaceCreated = GL_THREAD_FALSE;
	gl_thread_bool_t notifySurfaceChanged = GL_THREAD_FALSE;

	// Loop until thread is asked to exit.
	while (!GLOBALS.threadExit) {

		// Acquire mutex lock for synchronization.
		// This prevents changes to be made GLOBALS
		// in an inappropriate way.
		pthread_mutex_lock(&GLOBALS.mutex);

		// Wait loop.
		while (!GLOBALS.threadExit) {
			// If we're asked to pause, release EGL context.
			if (GLOBALS.threadPause && hasContext) {
				gl_SurfaceRelease(&egl);
				gl_ContextRelease(&egl);
				hasContext = GL_THREAD_FALSE;
				hasSurface = GL_THREAD_FALSE;
			}
			// If window changed release EGL surface.
			if (GLOBALS.windowChanged) {
				GLOBALS.windowChanged = GL_THREAD_FALSE;
				if (hasSurface) {
					gl_SurfaceRelease(&egl);
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
			if (!GLOBALS.threadPause && hasContext && !hasSurface) {
				hasSurface = gl_SurfaceCreate(&egl, GLOBALS.window);
				notifySurfaceCreated = hasSurface;
				if (!hasSurface) {
					LOGD("gl_Thread", "gl_SurfaceCreate failed");
				}
			}
			// If there's windowSizeChanged pending
			// update internal window size.
			if (GLOBALS.windowSizeChanged) {
				GLOBALS.windowSizeChanged = GL_THREAD_FALSE;
				width = GLOBALS.windowWidth;
				height = GLOBALS.windowHeight;
				notifySurfaceChanged = GL_THREAD_TRUE;
			}

			// If we have all the necessary for rendering
			// exit the wait loop.
			if (hasContext && hasSurface && width > 0 && height > 0) {
				break;
			}

			LOGD("gl_Thread", "wait");
			// Releases mutex and waits for cond to be triggered.
			pthread_cond_wait(&GLOBALS.cond, &GLOBALS.mutex);
		}
		// Release mutex and do rendering.
		pthread_mutex_unlock(&GLOBALS.mutex);

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
		eglSwapBuffers(egl.display, egl.surface);
	}

	// Once we get out of rendering loop
	// release EGL surface and context.
	gl_SurfaceRelease(&egl);
	gl_ContextRelease(&egl);

	LOGD("gl_Thread", "exit");
	return NULL;
}

void gl_ThreadNotify() {
	pthread_cond_signal(&GLOBALS.cond);
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
		gl_ThreadNotify();

		// Wait until thread has exited.
		pthread_join(GLOBALS.thread, NULL);

		// Release all VARS data.
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
	LOGD("gl_ThreadSetPaused", "paused=%d", paused);
	// Acquire thread lock.
	gl_ThreadLock();
	// Set thread paused flag.
	GLOBALS.threadPause = paused;
	// Release thread lock.
	gl_ThreadUnlock();
	// Notify thread about changes.
	gl_ThreadNotify();
}

// Sets new native window for creating EGLSurface.
void gl_ThreadSetWindow(ANativeWindow* window) {
	LOGD("gl_ThreadSetWindow", "window=%d", (int)window);

	// If we have new nativeWin.
	if (GLOBALS.window != window) {
		// Acquire thread lock.
		gl_ThreadLock();
		// If there is old one, release it first.
		// TODO: I'm a bit uncertain should window be
		// released only after releasing EGL surface
		// using it.
		if (GLOBALS.window) {
			ANativeWindow_release(GLOBALS.window);
		}
		// Store new nativeWin and mark it changed.
		GLOBALS.window = window;
		GLOBALS.windowChanged = GL_THREAD_TRUE;
		// Set window size to zero and mark it changed.
		GLOBALS.windowWidth = GLOBALS.windowHeight = 0;
		GLOBALS.windowSizeChanged = GL_THREAD_TRUE;
		// Release thread lock.
		gl_ThreadUnlock();
		// Notify thread about changes.
		gl_ThreadNotify();
	}
	// Else if nativeWin != NULL.
	else if (window) {
		// Release window instantly.
		ANativeWindow_release(window);
	}
}

void gl_ThreadSetWindowSize(int width, int height) {
	LOGD("gl_ThreadSetWindowSize", "w=%d h=%d", width, height);
	if (GLOBALS.windowWidth != width || GLOBALS.windowHeight != height) {
		// Acquire thread lock.
		gl_ThreadLock();
		// Store new dimensions.
		GLOBALS.windowWidth = width;
		GLOBALS.windowHeight = height;
		// Mark window size has changed.
		GLOBALS.windowSizeChanged = GL_THREAD_TRUE;
		// Release thread lock.
		gl_ThreadUnlock();
		// Notify thread about changes.
		gl_ThreadNotify();
	}
}

void gl_ThreadLock() {
	pthread_mutex_lock(&GLOBALS.mutex);
}

void gl_ThreadUnlock() {
	pthread_mutex_unlock(&GLOBALS.mutex);
}
