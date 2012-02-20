#include <stdlib.h>
#include <EGL/egl.h>
#include "gl_surface.h"

#define LOG_TAG "gl_surface.c"
#define LOGD(...) GL_LOGD(LOG_TAG, __VA_ARGS__)

bool_t gl_SurfaceCreate(gl_egl_vars_t* eglVars, ANativeWindow* nativeWin) {
	LOGD("gl_SurfaceCreate");
	if (eglVars->eglDisplay == EGL_NO_DISPLAY) {
		LOGD("eglDisplay == EGL_NO_DISPLAY");
		return FALSE;
	}
	if (eglVars->eglConfig == NULL) {
		LOGD("eglConfig == NULL");
		return FALSE;
	}
	if (nativeWin == NULL) {
		LOGD("nativeWin == NULL");
		return FALSE;
	}

	if (eglVars->eglSurface != EGL_NO_SURFACE) {
		if (eglDestroySurface(eglVars->eglDisplay,
				eglVars->eglSurface) != EGL_TRUE) {
			LOGD("eglDestroySurface failed");
		}
	}

	eglVars->eglSurface = eglCreateWindowSurface(eglVars->eglDisplay,
			eglVars->eglConfig, nativeWin, NULL);
	if (eglVars->eglSurface == EGL_NO_SURFACE) {
		LOGD("eglCreateWindowSurface failed");
		return FALSE;
	}

	return TRUE;
}

void gl_SurfaceRelease(gl_egl_vars_t* eglVars) {
	LOGD("gl_SurfaceRelease");

	if (eglVars->eglDisplay == EGL_NO_DISPLAY) {
		LOGD("eglDisplay == EGL_NO_DISPLAY");
		return;
	}
	if (eglVars->eglSurface == EGL_NO_SURFACE) {
		LOGD("eglSurface == EGL_NO_SURFACE");
		return;
	}

	if (eglDestroySurface(eglVars->eglDisplay, eglVars->eglSurface) != EGL_TRUE) {
		LOGD("eglDestroySurface failed");
	}
	eglVars->eglSurface = EGL_NO_SURFACE;
}
