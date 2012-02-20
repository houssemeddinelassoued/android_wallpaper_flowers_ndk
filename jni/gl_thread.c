#include <pthread.h>
#include "gl_defs.h"
#include "gl_thread.h"
#include "gl_surface.h"
#include "gl_context.h"

#define VARS gl_thread_vars
typedef struct {
	bool_t bThreadCreated;
	pthread_t pThread;
	pthread_mutex_t pMutex;
	pthread_cond_t pCond;

	bool_t bPause;
	bool_t bExit;
	bool_t bSurfaceChanged;

	gl_egl_vars_t eglVars;
	ANativeWindow *nativeWin;
	int surfaceWidth;
	int surfaceHeight;
} gl_thread_vars_t;
gl_thread_vars_t VARS;

void* gl_Thread(void *params) {
	gl_thread_params_t *thread_params = (gl_thread_params_t*) params;

	bool_t createEGL = TRUE;
	bool_t createSurface = TRUE;

	while (!VARS.bExit) {

		if (VARS.bPause) {
			gl_SurfaceRelease(&VARS.eglVars);
			gl_ContextRelease(&VARS.eglVars);
			createEGL = TRUE;
		}

		while (!VARS.bExit) {
			if (VARS.bSurfaceChanged && VARS.nativeWin == NULL) {
				VARS.bSurfaceChanged = FALSE;
				gl_SurfaceRelease(&VARS.eglVars);
			}
			if ((VARS.bPause == FALSE)
					&& (VARS.surfaceWidth > 0 && VARS.surfaceHeight > 0)) {
				break;
			}

			LOGD("gl_Thread", "wait");
			pthread_mutex_lock(&VARS.pMutex);
			pthread_cond_wait(&VARS.pCond, &VARS.pMutex);
			pthread_mutex_unlock(&VARS.pMutex);
		}

		if (VARS.bExit) {
			break;
		}

		createSurface |= VARS.bSurfaceChanged;
		VARS.bSurfaceChanged = FALSE;

		if (createEGL) {
			// Create EGL
			gl_ContextCreate(&VARS.eglVars, thread_params->gl_ChooseConfig);
			// Notify renderer
			createEGL = FALSE;
			createSurface = TRUE;
		}
		if (createSurface) {
			// Create Surface
			gl_SurfaceCreate(&VARS.eglVars, VARS.nativeWin);
			// Notify renderer
			createSurface = FALSE;
		}

		if (VARS.surfaceWidth > 0 && VARS.surfaceHeight > 0) {
			// Do render.
		}

		LOGD("gl_Thread", "execute");
		sleep(2);
	}

	gl_SurfaceRelease(&VARS.eglVars);
	gl_ContextRelease(&VARS.eglVars);

	if (VARS.nativeWin) {
		ANativeWindow_release(VARS.nativeWin);
		VARS.nativeWin = NULL;
	}

	LOGD("gl_Thread", "exit");
	return NULL;
}

void gl_ThreadNotify() {
	pthread_cond_signal(&VARS.pCond);
}

void gl_ThreadCreate(gl_thread_params_t *threadParams) {
	// If there's thread running, stop it.
	if (VARS.bThreadCreated) {
		gl_ThreadDestroy();
	}
	// Initialize new thread.
	VARS.bThreadCreated = TRUE;
	VARS.bPause = TRUE;
	pthread_cond_init(&VARS.pCond, NULL);
	pthread_mutex_init(&VARS.pMutex, NULL);
	pthread_create(&VARS.pThread, NULL, gl_Thread, threadParams);
}

void gl_ThreadDestroy() {
	// If there's thread running.
	if (VARS.bThreadCreated) {
		// Mark exit flag.
		VARS.bExit = TRUE;
		gl_ThreadNotify();
		// Wait until thread has exited.
		pthread_join(VARS.pThread, NULL);
		// Release all VARS data.
		pthread_cond_destroy(&VARS.pCond);
		pthread_mutex_destroy(&VARS.pMutex);
		memset(&VARS, 0, sizeof VARS);
	}
}

void gl_ThreadSetPaused(bool_t paused) {
	// Notify thread about changes.
	VARS.bPause = paused;
	gl_ThreadNotify();
}

void gl_ThreadSetSurface(ANativeWindow* nativeWin) {
	// If we have new nativeWin.
	if (VARS.nativeWin != nativeWin) {
		// If there is old one, release it first.
		if (VARS.nativeWin) {
			ANativeWindow_release(VARS.nativeWin);
		}
		// Store new nativeWin.
		VARS.nativeWin = nativeWin;
		// Reset dimensions.
		VARS.surfaceWidth = VARS.surfaceHeight = 0;
		// Notify thread about changes.
		VARS.bSurfaceChanged = TRUE;
		gl_ThreadNotify();
	}
	// Else if nativeWin != NULL.
	else if (nativeWin) {
		// Release nativeWin instantly.
		ANativeWindow_release(nativeWin);
	}
}

void gl_ThreadSetSurfaceSize(int width, int height) {
	// Store new dimensions.
	VARS.surfaceWidth = width;
	VARS.surfaceHeight = height;
	// Notify thread about changes.
	VARS.bSurfaceChanged = TRUE;
	gl_ThreadNotify();
}
