#include <pthread.h>
#include "gl_thread.h"
#include "log.h"

#define VARS gl_thread_vars
typedef struct {
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	gl_thread_bool_t threadCreated;
	gl_thread_bool_t threadPause;
	gl_thread_bool_t threadExit;
	gl_thread_bool_t windowChanged;

	ANativeWindow *window;
	int width;
	int height;

} gl_thread_vars_t;
gl_thread_vars_t VARS;

#define EGL_VARS gl_thread_egl_vars
typedef struct {
	gl_ChooseConfig_t chooseConfig;
	EGLDisplay display;
	EGLContext context;
	EGLConfig config;
	EGLSurface surface;
} gl_egl_vars_t;
gl_egl_vars_t EGL_VARS;

gl_thread_bool_t gl_ContextCreate() {
	if (EGL_VARS.chooseConfig == NULL) {
		return GL_THREAD_FALSE;
	}

	EGL_VARS.display = eglGetDisplay(0);
	if (EGL_VARS.display == EGL_NO_DISPLAY) {
		return GL_THREAD_FALSE;
	}
	if (eglInitialize(EGL_VARS.display, NULL, NULL) != EGL_TRUE) {
		return GL_THREAD_FALSE;
	}

	EGLint numConfigs;
	EGLint configAttrs[] = { EGL_RED_SIZE, 4, EGL_GREEN_SIZE, 4, EGL_BLUE_SIZE,
			4, EGL_ALPHA_SIZE, 0, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
			EGL_NONE };

	if (eglChooseConfig(EGL_VARS.display, configAttrs, NULL, 0,
			&numConfigs) != EGL_TRUE) {
		return GL_THREAD_FALSE;
	}
	if (numConfigs <= 0) {
		return GL_THREAD_FALSE;
	}

	EGLint configsSize = numConfigs;
	EGLConfig* configs = (EGLConfig*) malloc(configsSize * sizeof(EGLConfig));
	if (eglChooseConfig(EGL_VARS.display, configAttrs, configs, configsSize,
			&numConfigs) == EGL_FALSE) {
		free(configs);
		return GL_THREAD_FALSE;
	}

	EGL_VARS.config = EGL_VARS.chooseConfig(EGL_VARS.display, configs,
			numConfigs);
	free(configs);

	if (EGL_VARS.config == NULL) {
		return GL_THREAD_FALSE;
	}

	EGLint contextAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	EGL_VARS.context = eglCreateContext(EGL_VARS.display, EGL_VARS.config,
			EGL_NO_CONTEXT, contextAttrs);
	if (EGL_VARS.context == EGL_NO_CONTEXT) {
		return GL_THREAD_FALSE;
	}

	EGL_VARS.surface = EGL_NO_SURFACE;
	return GL_THREAD_TRUE;
}

void gl_ContextRelease() {
	if (EGL_VARS.context != EGL_NO_CONTEXT) {
		if (eglDestroyContext(EGL_VARS.display, EGL_VARS.context) != EGL_TRUE) {
		}
		EGL_VARS.context = EGL_NO_CONTEXT;
	}
	if (EGL_VARS.display != EGL_NO_DISPLAY) {
		if (eglTerminate(EGL_VARS.display) != EGL_TRUE) {
		}
		EGL_VARS.display = EGL_NO_DISPLAY;
	}
	EGL_VARS.surface = EGL_NO_SURFACE;
}

gl_thread_bool_t gl_SurfaceCreate() {
	// First check we have valid values.
	if (EGL_VARS.display == EGL_NO_DISPLAY) {
		return GL_THREAD_FALSE;
	}
	if (EGL_VARS.config == NULL) {
		return GL_THREAD_FALSE;
	}
	if (VARS.window == NULL) {
		return GL_THREAD_FALSE;
	}

	// If we have a surface, release it first.
	if (EGL_VARS.surface != EGL_NO_SURFACE) {
		eglMakeCurrent(EGL_VARS.display, EGL_NO_SURFACE, EGL_NO_SURFACE,
				EGL_NO_DISPLAY);
		eglDestroySurface(EGL_VARS.display, EGL_VARS.surface);
	}

	// Try to create new surface.
	EGL_VARS.surface = eglCreateWindowSurface(EGL_VARS.display, EGL_VARS.config,
			VARS.window, NULL);

	// If creation failed.
	if (EGL_VARS.surface == EGL_NO_SURFACE) {
		return GL_THREAD_FALSE;
	}

	if (eglMakeCurrent(EGL_VARS.display, EGL_VARS.surface, EGL_VARS.surface,
			EGL_VARS.context) != EGL_TRUE) {
		return GL_THREAD_FALSE;
	}

	return GL_THREAD_TRUE;
}

void gl_SurfaceRelease() {
	// If we have reasonable variables.
	if (EGL_VARS.display != EGL_NO_DISPLAY && EGL_VARS.surface != EGL_NO_SURFACE) {
		eglMakeCurrent(EGL_VARS.display, EGL_NO_SURFACE, EGL_NO_SURFACE,
				EGL_NO_DISPLAY);
		eglDestroySurface(EGL_VARS.display, EGL_VARS.surface);
	}
	// Mark surface as non existent.
	EGL_VARS.surface = EGL_NO_SURFACE;
}

void* gl_Thread(void *params) {
	gl_thread_funcs_t *funcs = (gl_thread_funcs_t*) params;
	EGL_VARS.chooseConfig = funcs->chooseConfig;

	gl_thread_bool_t createContext = GL_THREAD_TRUE;
	gl_thread_bool_t createSurface = GL_THREAD_TRUE;

	while (!VARS.threadExit) {

		if (VARS.threadPause) {
			gl_SurfaceRelease(&EGL_VARS);
			gl_ContextRelease(&EGL_VARS);
			createContext = GL_THREAD_TRUE;
		}

		while (!VARS.threadExit) {
			if (VARS.windowChanged && VARS.window == NULL) {
				VARS.windowChanged = GL_THREAD_FALSE;
				gl_SurfaceRelease(&EGL_VARS);
			}
			if ((VARS.threadPause == GL_THREAD_FALSE)
					&& (VARS.width > 0 && VARS.height > 0)) {
				break;
			}

			LOGD("gl_Thread", "wait");
			pthread_mutex_lock(&VARS.mutex);
			pthread_cond_wait(&VARS.cond, &VARS.mutex);
			pthread_mutex_unlock(&VARS.mutex);
		}

		if (VARS.threadExit) {
			break;
		}

		createSurface |= VARS.windowChanged;
		VARS.windowChanged = GL_THREAD_FALSE;

		if (createContext) {
			// Create EGL context
			if (gl_ContextCreate(&EGL_VARS) != GL_THREAD_TRUE) {
				LOGD("gl_Thread", "gl_ContextCreate failed");
				gl_ContextRelease(&EGL_VARS);
				continue;
			}
			createContext = GL_THREAD_FALSE;
			createSurface = GL_THREAD_TRUE;
		}
		if (createSurface) {
			// Create Surface
			if (gl_SurfaceCreate(&EGL_VARS, VARS.window) != GL_THREAD_TRUE) {
				LOGD("gl_Thread", "gl_SurfaceCreate failed");
				gl_SurfaceRelease(&EGL_VARS);
				gl_ContextRelease(&EGL_VARS);
				continue;
			}
			// Notify renderer
			funcs->onSurfaceCreated();
			funcs->onSurfaceChanged(VARS.width, VARS.height);
			createSurface = GL_THREAD_FALSE;
		}

		if (VARS.width > 0 && VARS.height > 0) {
			funcs->onRenderFrame();
			eglSwapBuffers(EGL_VARS.display, EGL_VARS.surface);
		}

		sleep(1);
	}

	gl_SurfaceRelease(&EGL_VARS);
	gl_ContextRelease(&EGL_VARS);

	if (VARS.window) {
		ANativeWindow_release(VARS.window);
		VARS.window = NULL;
	}

	LOGD("gl_Thread", "exit");
	return NULL;
}

void gl_ThreadNotify() {
	pthread_cond_signal(&VARS.cond);
}

void gl_ThreadCreate(gl_thread_funcs_t *threadParams) {
	// If there's thread running, stop it.
	if (VARS.threadCreated) {
		gl_ThreadDestroy();
	}
	// Initialize new thread.
	VARS.threadCreated = GL_THREAD_TRUE;
	VARS.threadPause = GL_THREAD_TRUE;
	pthread_cond_init(&VARS.cond, NULL);
	pthread_mutex_init(&VARS.mutex, NULL);
	pthread_create(&VARS.thread, NULL, gl_Thread, threadParams);
}

void gl_ThreadDestroy() {
	// If there's thread running.
	if (VARS.threadCreated) {
		// Mark exit flag.
		VARS.threadExit = GL_THREAD_TRUE;
		gl_ThreadNotify();
		// Wait until thread has exited.
		pthread_join(VARS.thread, NULL);
		// Release all VARS data.
		pthread_cond_destroy(&VARS.cond);
		pthread_mutex_destroy(&VARS.mutex);
		memset(&VARS, 0, sizeof VARS);
	}
}

void gl_ThreadSetPaused(gl_thread_bool_t paused) {
	LOGD("gl_ThreadSetPaused", "paused=%d", paused);
	gl_ThreadLock();
	// Notify thread about changes.
	VARS.threadPause = paused;
	gl_ThreadUnlock();
	gl_ThreadNotify();
}

// Sets new native window for creating EGLSurface.
void gl_ThreadSetWindow(ANativeWindow* window) {
	LOGD("gl_ThreadSetWindow", "window=%d", window);

	// If we have new nativeWin.
	if (VARS.window != window) {
		gl_ThreadLock();
		// If there is old one, release it first.
		if (VARS.window) {
			ANativeWindow_release(VARS.window);
		}
		// Store new nativeWin.
		VARS.window = window;
		// Reset dimensions.
		VARS.width = VARS.height = 0;
		// Notify thread about changes.
		VARS.windowChanged = GL_THREAD_TRUE;
		gl_ThreadUnlock();
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
	if (VARS.width != width || VARS.height != height) {
		gl_ThreadLock();
		// Store new dimensions.
		VARS.width = width;
		VARS.height = height;
		// Notify thread about changes.
		VARS.windowChanged = GL_THREAD_TRUE;
		gl_ThreadUnlock();
		gl_ThreadNotify();
	}
}

void gl_ThreadLock() {
	pthread_mutex_lock(&VARS.mutex);
}

void gl_ThreadUnlock() {
	pthread_mutex_unlock(&VARS.mutex);
}
