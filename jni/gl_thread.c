#include <pthread.h>
#include "gl_defs.h"
#include "gl_thread.h"
#include "gl_surface.h"
#include "gl_context.h"

#define LOG_TAG "gl_thread.c"
#define LOGD(...) GL_LOGD(LOG_TAG, __VA_ARGS__)

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

	LOGD("gl_Thread1");

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
			if ((VARS.bPause == FALSE)
					&& (VARS.surfaceWidth > 0 && VARS.surfaceHeight > 0)) {
				break;
			}
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

		LOGD("glThread execute");
		sleep(2);
	}

	gl_SurfaceRelease(&VARS.eglVars);
	gl_ContextRelease(&VARS.eglVars);
	free(thread_params);

	if (VARS.nativeWin) {
		ANativeWindow_release(VARS.nativeWin);
		VARS.nativeWin = NULL;
	}

	LOGD("glThread exit");
	return NULL;
}

void gl_ThreadNotify() {
	LOGD("gl_ThreadNotify");
	pthread_cond_signal(&VARS.pCond);
}

void gl_ThreadStart(gl_thread_params_t *threadParams) {
	LOGD("gl_ThreadStart 1");
	if (VARS.bThreadCreated) {
		gl_ThreadStop();
	}LOGD("gl_ThreadStart 2");
	VARS.bThreadCreated = TRUE;
	VARS.bPause = TRUE;
	pthread_cond_init(&VARS.pCond, NULL);
	pthread_mutex_init(&VARS.pMutex, NULL);
	pthread_create(&VARS.pThread, NULL, gl_Thread, threadParams);
}

void gl_ThreadStop() {
	LOGD("gl_ThreadStop 1");
	if (VARS.bThreadCreated) {
		LOGD("gl_ThreadStop 2");
		VARS.bExit = TRUE;
		gl_ThreadNotify();
		pthread_join(VARS.pThread, NULL);
		pthread_cond_destroy(&VARS.pCond);
		pthread_mutex_destroy(&VARS.pMutex);
		memset(&VARS, 0, sizeof VARS);
	}
	LOGD("gl_ThreadStop 3");
}

void gl_ThreadPause() {
	LOGD("gl_ThreadPause");
	VARS.bPause = TRUE;
	gl_ThreadNotify();
}

void gl_ThreadResume() {
	LOGD("gl_ThreadResume");
	VARS.bPause = FALSE;
	gl_ThreadNotify();
}

void gl_ThreadSetSurface(ANativeWindow* nativeWin) {
	LOGD("gl_ThreadSetSurface");
	if (VARS.nativeWin != nativeWin) {
		if (VARS.nativeWin) {
			ANativeWindow_release(VARS.nativeWin);
		}
		VARS.nativeWin = nativeWin;
		VARS.surfaceWidth = VARS.surfaceHeight = 0;
		VARS.bSurfaceChanged = TRUE;
		gl_ThreadNotify();
	} else if (nativeWin) {
		ANativeWindow_release(nativeWin);
	}
}

void gl_ThreadSetSurfaceSize(int width, int height) {
	LOGD("gl_ThreadSetSurfaceSize");
	VARS.surfaceWidth = width;
	VARS.surfaceHeight = height;
	VARS.bSurfaceChanged = TRUE;
	gl_ThreadNotify();
}
