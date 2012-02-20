#include <stdlib.h>
#include "gl_context.h"

bool_t gl_ContextCreate(gl_egl_vars_t *eglVars, gl_ChooseConfig_t chooseConfig) {
	eglVars->eglDisplay = eglGetDisplay(0);
	if (eglVars->eglDisplay == EGL_NO_DISPLAY) {
		return FALSE;
	}
	if (eglInitialize(eglVars->eglDisplay, NULL, NULL) != EGL_TRUE) {
		return FALSE;
	}

	EGLint numConfigs;
	EGLint configAttrs[] = { EGL_RED_SIZE, 4, EGL_GREEN_SIZE, 4, EGL_BLUE_SIZE,
			4, EGL_ALPHA_SIZE, 0, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
			EGL_NONE };

	if (eglChooseConfig(eglVars->eglDisplay, configAttrs, NULL, 0,
			&numConfigs) != EGL_TRUE) {
		return FALSE;
	}
	if (numConfigs <= 0) {
		return FALSE;
	}

	EGLint configsSize = numConfigs;
	EGLConfig* configs = (EGLConfig*) malloc(configsSize * sizeof(EGLConfig));
	if (eglChooseConfig(eglVars->eglDisplay, configAttrs, configs, configsSize,
			&numConfigs) == EGL_FALSE) {
		free(configs);
		return FALSE;
	}

	eglVars->eglConfig = chooseConfig(eglVars->eglDisplay, configs, numConfigs);
	free(configs);

	if (eglVars->eglConfig == NULL) {
		return FALSE;
	}

	EGLint contextAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	eglVars->eglContext = eglCreateContext(eglVars->eglDisplay,
			eglVars->eglConfig, EGL_NO_CONTEXT, contextAttrs);
	if (eglVars->eglContext == EGL_NO_CONTEXT) {
		return FALSE;
	}

	eglVars->eglSurface = EGL_NO_SURFACE;
	return TRUE;
}

void gl_ContextRelease(gl_egl_vars_t *eglVars) {
	if (eglVars->eglContext != EGL_NO_CONTEXT) {
		if (eglDestroyContext(eglVars->eglDisplay,
				eglVars->eglContext) != EGL_TRUE) {
		}
		eglVars->eglContext = EGL_NO_CONTEXT;
	}
	if (eglVars->eglDisplay != EGL_NO_DISPLAY) {
		if (eglTerminate(eglVars->eglDisplay) != EGL_TRUE) {
		}
		eglVars->eglDisplay = EGL_NO_DISPLAY;
	}
	eglVars->eglSurface = EGL_NO_SURFACE;
}
