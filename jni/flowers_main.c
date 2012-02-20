#include <stdlib.h>
#include <jni.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include "flowers_renderer.h"
#include "gl_thread.h"

#define GL_EXTERN(func) Java_fi_harism_wallpaper_flowersndk_FlowerService_ ## func

#define VARS flowers_main_vars
typedef struct {
	int hostCount;
} flowers_main_vars_t;
flowers_main_vars_t VARS;

#define THREAD_FUNCS flowers_thread_funcs
gl_thread_funcs_t THREAD_FUNCS;

EGLConfig flowers_ChooseConfig(EGLDisplay display, EGLConfig* configArray,
		int configCount) {
	int idx;
	int highestSum = 0;
	int highestSub = 0;
	EGLConfig retConfig = NULL;
	for (idx = 0; idx < configCount; ++idx) {
		EGLConfig config = configArray[idx];
		int r = 0, g = 0, b = 0, a = 0, d = 0, s = 0;
		eglGetConfigAttrib(display, config, EGL_RED_SIZE, &r);
		eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &g);
		eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &b);
		eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &a);
		eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &d);
		eglGetConfigAttrib(display, config, EGL_STENCIL_SIZE, &s);

		int sum = r + g + b;
		int sub = a + d + s;
		if (sum > highestSum || (sum == highestSum && sub > highestSub)) {
			retConfig = config;
			highestSum = sum;
			highestSub = sub;
		}
	}
	return retConfig;
}

void GL_EXTERN(flowersConnect(JNIEnv *env)) {
	if (VARS.hostCount == 0) {
		THREAD_FUNCS.chooseConfig = flowers_ChooseConfig;
		THREAD_FUNCS.onRenderFrame = flowers_OnRenderFrame;
		THREAD_FUNCS.onSurfaceChanged = flowers_OnSurfaceChanged;
		THREAD_FUNCS.onSurfaceCreated = flowers_OnSurfaceCreated;
		gl_ThreadCreate(&THREAD_FUNCS);
	}
	++VARS.hostCount;
}

void GL_EXTERN(flowersDisconnect(JNIEnv *env)) {
	if (VARS.hostCount == 1) {
		memset(&VARS, 0, sizeof VARS);
		gl_ThreadDestroy();
	}
	if (VARS.hostCount > 0) {
		--VARS.hostCount;
	}
}

void GL_EXTERN(flowersSetPaused(JNIEnv *env, jobject obj, jboolean paused)) {
	if (paused) {
		gl_ThreadSetPaused(GL_THREAD_TRUE);
	} else {
		gl_ThreadSetPaused(GL_THREAD_FALSE);
	}
}

void GL_EXTERN(flowersSetSurface(JNIEnv *env, jobject obj, jobject surface)) {
	if (surface) {
		gl_ThreadSetWindow(ANativeWindow_fromSurface(env, surface));
	} else {
		gl_ThreadSetWindow(NULL);
	}
}

void GL_EXTERN(flowersSetSurfaceSize(JNIEnv *env, jobject obj, jint width, jint height)) {
	gl_ThreadSetWindowSize(width, height);
}
