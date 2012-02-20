#include <stdlib.h>
#include <EGL/egl.h>
#include "gl_context.h"

#define LOG_TAG "gl_context.c"
#define LOGD(...) GL_LOGD(LOG_TAG, __VA_ARGS__)

bool_t gl_ContextCreate(gl_egl_vars_t *eglVars, gl_ChooseConfig_t chooseConfig) {
	LOGD("gl_ContextCreate");

	eglVars->eglDisplay = eglGetDisplay(0);
	if (eglVars->eglDisplay == EGL_NO_DISPLAY) {
		LOGD("eglGetDisplay failed");
		return FALSE;
	}
	if (eglInitialize(eglVars->eglDisplay, NULL, NULL) != EGL_TRUE) {
		LOGD("eglInitialize failed");
		return FALSE;
	}

	EGLint numConfigs;
	EGLint configAttrs[] = { EGL_RED_SIZE, 4, EGL_GREEN_SIZE, 4, EGL_BLUE_SIZE,
			4, EGL_ALPHA_SIZE, 0, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
			EGL_NONE };

	if (eglChooseConfig(eglVars->eglDisplay, configAttrs, NULL, 0,
			&numConfigs) != EGL_TRUE) {
		LOGD("eglChooseConfig #1 failed");
		return FALSE;
	}
	if (numConfigs <= 0) {
		LOGD("numConfigs <= 0");
		return FALSE;
	}

	EGLint configsSize = numConfigs;
	EGLConfig* configs = (EGLConfig*) malloc(configsSize * sizeof(EGLConfig));
	if (eglChooseConfig(eglVars->eglDisplay, configAttrs, configs, configsSize,
			&numConfigs) == EGL_FALSE) {
		LOGD("eglChooseConfig #2 failed");
		free(configs);
		return FALSE;
	}

	eglVars->eglConfig = chooseConfig(eglVars->eglDisplay, configs, numConfigs);
	free(configs);

	if (eglVars->eglConfig == NULL) {
		LOGD("context.eglConfig == NULL");
		return FALSE;
	}

	EGLint contextAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	eglVars->eglContext = eglCreateContext(eglVars->eglDisplay, eglVars->eglConfig,
			EGL_NO_CONTEXT, contextAttrs);
	if (eglVars->eglContext == EGL_NO_CONTEXT) {
		LOGD("eglCreateContext failed");
		return FALSE;
	}

	eglVars->eglSurface = EGL_NO_SURFACE;
	return TRUE;
}

void gl_ContextRelease(gl_egl_vars_t *eglVars) {
	LOGD("gl_ContextRelease");

	if (eglVars->eglContext != EGL_NO_CONTEXT) {
		if (eglDestroyContext(eglVars->eglDisplay,
				eglVars->eglContext) != EGL_TRUE) {
			LOGD("eglDestroyContext failed");
		}
		eglVars->eglContext = EGL_NO_CONTEXT;
	}
	if (eglVars->eglDisplay != EGL_NO_DISPLAY) {
		if (eglTerminate(eglVars->eglDisplay) != EGL_TRUE) {
			LOGD("eglTerminate failed");
		}
		eglVars->eglDisplay = EGL_NO_DISPLAY;
	}
	eglVars->eglSurface = EGL_NO_SURFACE;
}
