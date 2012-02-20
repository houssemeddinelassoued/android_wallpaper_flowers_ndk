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

#include <pthread.h>
#include "gl_thread.h"
#include "log.h"

// Global variables for communicating
// with rendering thread.
#define GLOBALS gl_thread_globals
typedef struct {
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	gl_thread_bool_t threadCreated;
	gl_thread_bool_t threadPause;
	gl_thread_bool_t threadExit;
	gl_thread_bool_t windowChanged;

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

gl_thread_bool_t gl_ContextCreate(gl_thread_egl_t *egl,
		gl_ChooseConfig_t chooseConfig) {
	if (chooseConfig == NULL) {
		return GL_THREAD_FALSE;
	}

	egl->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (egl->display == EGL_NO_DISPLAY) {
		return GL_THREAD_FALSE;
	}
	if (eglInitialize(egl->display, NULL, NULL) != EGL_TRUE) {
		return GL_THREAD_FALSE;
	}

	EGLint numConfigs;
	EGLint configAttrs[] = { EGL_RED_SIZE, 4, EGL_GREEN_SIZE, 4, EGL_BLUE_SIZE,
			4, EGL_ALPHA_SIZE, 0, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
			EGL_NONE };

	if (eglChooseConfig(egl->display, configAttrs, NULL, 0,
			&numConfigs) != EGL_TRUE) {
		return GL_THREAD_FALSE;
	}
	if (numConfigs <= 0) {
		return GL_THREAD_FALSE;
	}

	EGLint configsSize = numConfigs;
	EGLConfig* configs = (EGLConfig*) malloc(configsSize * sizeof(EGLConfig));
	if (eglChooseConfig(egl->display, configAttrs, configs, configsSize,
			&numConfigs) == EGL_FALSE) {
		free(configs);
		return GL_THREAD_FALSE;
	}

	egl->config = chooseConfig(egl->display, configs, numConfigs);
	free(configs);

	if (egl->config == NULL) {
		return GL_THREAD_FALSE;
	}

	EGLint contextAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	egl->context = eglCreateContext(egl->display, egl->config, EGL_NO_CONTEXT,
			contextAttrs);
	if (egl->context == EGL_NO_CONTEXT) {
		return GL_THREAD_FALSE;
	}

	egl->surface = EGL_NO_SURFACE;
	return GL_THREAD_TRUE;
}

void gl_ContextRelease(gl_thread_egl_t *egl) {
	if (egl->context != EGL_NO_CONTEXT) {
		if (eglDestroyContext(egl->display, egl->context) != EGL_TRUE) {
		}
		egl->context = EGL_NO_CONTEXT;
	}
	if (egl->display != EGL_NO_DISPLAY) {
		if (eglTerminate(egl->display) != EGL_TRUE) {
		}
		egl->display = EGL_NO_DISPLAY;
	}
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
				EGL_NO_DISPLAY);
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

void* gl_Thread(void *startParams) {
	gl_thread_funcs_t *funcs = startParams;
	gl_thread_egl_t egl;

	gl_thread_bool_t createContext = GL_THREAD_TRUE;
	gl_thread_bool_t createSurface = GL_THREAD_TRUE;

	while (!GLOBALS.threadExit) {

		if (GLOBALS.threadPause) {
			gl_SurfaceRelease(&egl);
			gl_ContextRelease(&egl);
			createContext = GL_THREAD_TRUE;
		}

		while (!GLOBALS.threadExit) {
			if (GLOBALS.windowChanged && GLOBALS.window == NULL) {
				GLOBALS.windowChanged = GL_THREAD_FALSE;
				gl_SurfaceRelease(&egl);
			}
			if ((GLOBALS.threadPause == GL_THREAD_FALSE)
					&& (GLOBALS.windowWidth > 0 && GLOBALS.windowHeight > 0)) {
				break;
			}

			LOGD("gl_Thread", "wait");
			pthread_mutex_lock(&GLOBALS.mutex);
			pthread_cond_wait(&GLOBALS.cond, &GLOBALS.mutex);
			pthread_mutex_unlock(&GLOBALS.mutex);
		}

		if (GLOBALS.threadExit) {
			break;
		}

		createSurface |= GLOBALS.windowChanged;
		GLOBALS.windowChanged = GL_THREAD_FALSE;

		if (createContext) {
			// Create EGL context
			if (gl_ContextCreate(&egl, funcs->chooseConfig) != GL_THREAD_TRUE) {
				LOGD("gl_Thread", "gl_ContextCreate failed");
				gl_ContextRelease(&egl);
				continue;
			}
			createContext = GL_THREAD_FALSE;
			createSurface = GL_THREAD_TRUE;
		}
		if (createSurface) {
			// Create Surface
			if (gl_SurfaceCreate(&egl, GLOBALS.window) != GL_THREAD_TRUE) {
				LOGD("gl_Thread", "gl_SurfaceCreate failed");
				gl_SurfaceRelease(&egl);
				gl_ContextRelease(&egl);
				continue;
			}
			// Notify renderer
			funcs->onSurfaceCreated();
			funcs->onSurfaceChanged(GLOBALS.windowWidth, GLOBALS.windowHeight);
			createSurface = GL_THREAD_FALSE;
		}

		if (GLOBALS.windowWidth > 0 && GLOBALS.windowHeight > 0) {
			funcs->onRenderFrame();
			eglSwapBuffers(egl.display, egl.surface);
		}

		sleep(1);
	}

	gl_SurfaceRelease(&egl);
	gl_ContextRelease(&egl);

	if (GLOBALS.window) {
		ANativeWindow_release(GLOBALS.window);
		GLOBALS.window = NULL;
	}

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
	LOGD("gl_ThreadSetWindow", "window=%d", window);

	// If we have new nativeWin.
	if (GLOBALS.window != window) {
		// Acquire thread lock.
		gl_ThreadLock();
		// If there is old one, release it first.
		if (GLOBALS.window) {
			ANativeWindow_release(GLOBALS.window);
		}
		// Store new nativeWin.
		GLOBALS.window = window;
		// Reset window dimensions.
		GLOBALS.windowWidth = GLOBALS.windowHeight = 0;
		// Mark window changed flag.
		GLOBALS.windowChanged = GL_THREAD_TRUE;
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
		// Mark window has changed.
		GLOBALS.windowChanged = GL_THREAD_TRUE;
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
