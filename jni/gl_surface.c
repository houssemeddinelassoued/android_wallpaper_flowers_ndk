#include <stdlib.h>
#include <EGL/egl.h>
#include "gl_surface.h"

bool_t gl_SurfaceCreate(gl_egl_vars_t* eglVars, ANativeWindow* nativeWin) {

	// First check we received valid values.
	if (eglVars->eglDisplay == EGL_NO_DISPLAY) {
		return FALSE;
	}
	if (eglVars->eglConfig == NULL) {
		return FALSE;
	}
	if (nativeWin == NULL) {
		return FALSE;
	}

	// If we have a surface, release it first.
	if (eglVars->eglSurface != EGL_NO_SURFACE) {
		eglDestroySurface(eglVars->eglDisplay, eglVars->eglSurface);
	}

	// Try to create new surface.
	eglVars->eglSurface = eglCreateWindowSurface(eglVars->eglDisplay,
			eglVars->eglConfig, nativeWin, NULL);

	// If creation failed.
	if (eglVars->eglSurface == EGL_NO_SURFACE) {
		return FALSE;
	}

	return TRUE;
}

void gl_SurfaceRelease(gl_egl_vars_t* eglVars) {
	// If we have reasonable variables.
	if (eglVars->eglDisplay != EGL_NO_DISPLAY
			&& eglVars->eglSurface != EGL_NO_SURFACE) {
		eglDestroySurface(eglVars->eglDisplay, eglVars->eglSurface);
	}
	// Mark surface as non existent.
	eglVars->eglSurface = EGL_NO_SURFACE;
}
