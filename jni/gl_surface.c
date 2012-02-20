#include <stdlib.h>
#include <EGL/egl.h>
#include "gl_surface.h"

bool_t gl_SurfaceCreate(gl_egl_vars_t* egl, ANativeWindow* nativeWin) {

	// First check we received valid values.
	if (egl->display == EGL_NO_DISPLAY) {
		return FALSE;
	}
	if (egl->config == NULL) {
		return FALSE;
	}
	if (nativeWin == NULL) {
		return FALSE;
	}

	// If we have a surface, release it first.
	if (egl->surface != EGL_NO_SURFACE) {
		eglDestroySurface(egl->display, egl->surface);
	}

	// Try to create new surface.
	egl->surface = eglCreateWindowSurface(egl->display,
			egl->config, nativeWin, NULL);

	// If creation failed.
	if (egl->surface == EGL_NO_SURFACE) {
		return FALSE;
	}

	if (eglMakeCurrent(egl->display, egl->surface,
			egl->surface, egl->context) != EGL_TRUE) {
		return FALSE;
	}

	return TRUE;
}

void gl_SurfaceRelease(gl_egl_vars_t* egl) {
	// If we have reasonable variables.
	if (egl->display != EGL_NO_DISPLAY
			&& egl->surface != EGL_NO_SURFACE) {
		eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
				EGL_NO_DISPLAY);
		eglDestroySurface(egl->display, egl->surface);
	}
	// Mark surface as non existent.
	egl->surface = EGL_NO_SURFACE;
}
