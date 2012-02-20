#include <stdlib.h>
#include "gl_context.h"

bool_t gl_ContextCreate(gl_egl_vars_t *egl) {
	if (egl->chooseConfig == NULL) {
		return FALSE;
	}

	egl->display = eglGetDisplay(0);
	if (egl->display == EGL_NO_DISPLAY) {
		return FALSE;
	}
	if (eglInitialize(egl->display, NULL, NULL) != EGL_TRUE) {
		return FALSE;
	}

	EGLint numConfigs;
	EGLint configAttrs[] = { EGL_RED_SIZE, 4, EGL_GREEN_SIZE, 4, EGL_BLUE_SIZE,
			4, EGL_ALPHA_SIZE, 0, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
			EGL_NONE };

	if (eglChooseConfig(egl->display, configAttrs, NULL, 0,
			&numConfigs) != EGL_TRUE) {
		return FALSE;
	}
	if (numConfigs <= 0) {
		return FALSE;
	}

	EGLint configsSize = numConfigs;
	EGLConfig* configs = (EGLConfig*) malloc(configsSize * sizeof(EGLConfig));
	if (eglChooseConfig(egl->display, configAttrs, configs, configsSize,
			&numConfigs) == EGL_FALSE) {
		free(configs);
		return FALSE;
	}

	egl->config = egl->chooseConfig(egl->display, configs, numConfigs);
	free(configs);

	if (egl->config == NULL) {
		return FALSE;
	}

	EGLint contextAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	egl->context = eglCreateContext(egl->display, egl->config, EGL_NO_CONTEXT,
			contextAttrs);
	if (egl->context == EGL_NO_CONTEXT) {
		return FALSE;
	}

	egl->surface = EGL_NO_SURFACE;
	return TRUE;
}

void gl_ContextRelease(gl_egl_vars_t *egl) {
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
