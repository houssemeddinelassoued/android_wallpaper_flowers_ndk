#include <pthread.h>
#include "gl_thread.h"
#include "log.h"

// Global variables structure for communicating
// with rendering thread.
#define GLOBALS gl_thread_globals
typedef struct {
	pthread_t pThread;
	pthread_mutex_t pMutex;
	pthread_cond_t pCond;

	gl_thread_bool_t bThreadPause;
	gl_thread_bool_t bThreadExit;
	gl_thread_bool_t bWindowChanged;

	ANativeWindow *window;
	int windowWidth;
	int windowHeight;
} gl_thread_global_t;
gl_thread_global_t *GLOBALS;

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

	egl->display = eglGetDisplay(0);
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
	gl_thread_egl_t *egl = calloc(1, sizeof *egl);

	gl_thread_bool_t createContext = GL_THREAD_TRUE;
	gl_thread_bool_t createSurface = GL_THREAD_TRUE;

	while (!GLOBALS->bThreadExit) {

		if (GLOBALS->bThreadPause) {
			gl_SurfaceRelease(egl);
			gl_ContextRelease(egl);
			createContext = GL_THREAD_TRUE;
		}

		while (!GLOBALS->bThreadExit) {
			if (GLOBALS->bWindowChanged && GLOBALS->window == NULL) {
				GLOBALS->bWindowChanged = GL_THREAD_FALSE;
				gl_SurfaceRelease(egl);
			}
			if ((GLOBALS->bThreadPause == GL_THREAD_FALSE)
					&& (GLOBALS->windowWidth > 0 && GLOBALS->windowHeight > 0)) {
				break;
			}

			LOGD("gl_Thread", "wait");
			pthread_mutex_lock(&GLOBALS->pMutex);
			pthread_cond_wait(&GLOBALS->pCond, &GLOBALS->pMutex);
			pthread_mutex_unlock(&GLOBALS->pMutex);
		}

		if (GLOBALS->bThreadExit) {
			break;
		}

		createSurface |= GLOBALS->bWindowChanged;
		GLOBALS->bWindowChanged = GL_THREAD_FALSE;

		if (createContext) {
			// Create EGL context
			if (gl_ContextCreate(egl, funcs->chooseConfig) != GL_THREAD_TRUE) {
				LOGD("gl_Thread", "gl_ContextCreate failed");
				gl_ContextRelease(egl);
				continue;
			}
			createContext = GL_THREAD_FALSE;
			createSurface = GL_THREAD_TRUE;
		}
		if (createSurface) {
			// Create Surface
			if (gl_SurfaceCreate(egl, GLOBALS->window) != GL_THREAD_TRUE) {
				LOGD("gl_Thread", "gl_SurfaceCreate failed");
				gl_SurfaceRelease(egl);
				gl_ContextRelease(egl);
				continue;
			}
			// Notify renderer
			funcs->onSurfaceCreated();
			funcs->onSurfaceChanged(GLOBALS->windowWidth,
					GLOBALS->windowHeight);
			createSurface = GL_THREAD_FALSE;
		}

		if (GLOBALS->windowWidth > 0 && GLOBALS->windowHeight > 0) {
			funcs->onRenderFrame();
			eglSwapBuffers(egl->display, egl->surface);
		}

		sleep(1);
	}

	gl_SurfaceRelease(egl);
	gl_ContextRelease(egl);
	free(egl);

	if (GLOBALS->window) {
		ANativeWindow_release(GLOBALS->window);
		GLOBALS->window = NULL;
	}

	LOGD("gl_Thread", "exit");
	return NULL;
}

void gl_ThreadNotify() {
	pthread_cond_signal(&GLOBALS->pCond);
}

void gl_ThreadCreate(gl_thread_funcs_t *threadFuncs) {
	// If there's thread running, stop it.
	if (GLOBALS) {
		gl_ThreadDestroy();
	}
	// Initialize new thread.
	GLOBALS = calloc(1, sizeof *GLOBALS);
	GLOBALS->bThreadPause = GL_THREAD_TRUE;
	pthread_cond_init(&GLOBALS->pCond, NULL);
	pthread_mutex_init(&GLOBALS->pMutex, NULL);
	pthread_create(&GLOBALS->pThread, NULL, gl_Thread, threadFuncs);
}

void gl_ThreadDestroy() {
	// If there's thread running.
	if (GLOBALS) {
		// Mark exit flag.
		GLOBALS->bThreadExit = GL_THREAD_TRUE;
		gl_ThreadNotify();
		// Wait until thread has exited.
		pthread_join(GLOBALS->pThread, NULL);
		// Release all VARS data.
		pthread_cond_destroy(&GLOBALS->pCond);
		pthread_mutex_destroy(&GLOBALS->pMutex);
		free(GLOBALS);
		GLOBALS = NULL;
	}
}

void gl_ThreadSetPaused(gl_thread_bool_t paused) {
	LOGD("gl_ThreadSetPaused", "paused=%d", paused);
	// Acquire thread lock.
	gl_ThreadLock();
	// Set thread paused flag.
	GLOBALS->bThreadPause = paused;
	// Release thread lock.
	gl_ThreadUnlock();
	// Notify thread about changes.
	gl_ThreadNotify();
}

// Sets new native window for creating EGLSurface.
void gl_ThreadSetWindow(ANativeWindow* window) {
	LOGD("gl_ThreadSetWindow", "window=%d", window);

	// If we have new nativeWin.
	if (GLOBALS->window != window) {
		// Acquire thread lock.
		gl_ThreadLock();
		// If there is old one, release it first.
		if (GLOBALS->window) {
			ANativeWindow_release(GLOBALS->window);
		}
		// Store new nativeWin.
		GLOBALS->window = window;
		// Reset window dimensions.
		GLOBALS->windowWidth = GLOBALS->windowHeight = 0;
		// Mark window changed flag.
		GLOBALS->bWindowChanged = GL_THREAD_TRUE;
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
	if (GLOBALS->windowWidth != width || GLOBALS->windowHeight != height) {
		// Acquire thread lock.
		gl_ThreadLock();
		// Store new dimensions.
		GLOBALS->windowWidth = width;
		GLOBALS->windowHeight = height;
		// Mark window has changed.
		GLOBALS->bWindowChanged = GL_THREAD_TRUE;
		// Release thread lock.
		gl_ThreadUnlock();
		// Notify thread about changes.
		gl_ThreadNotify();
	}
}

void gl_ThreadLock() {
	pthread_mutex_lock(&GLOBALS->pMutex);
}

void gl_ThreadUnlock() {
	pthread_mutex_unlock(&GLOBALS->pMutex);
}
