#include <jni.h>
#include <stdlib.h>
#include <android/native_window_jni.h>
#include "gl_thread.h"
#include "flowers.h"

#define GL_EXTERN(func) Java_fi_harism_wallpaper_flowersndk_FlowerService_ ## func

#define VARS flowers_main_vars
typedef struct {
	int hostCount;
} flowers_main_vars_t;
flowers_main_vars_t VARS;

#define THREAD_FUNCS flowers_thread_funcs
gl_thread_funcs_t THREAD_FUNCS;

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
	gl_ThreadSetPaused(paused == JNI_TRUE ? TRUE : FALSE);
}

void GL_EXTERN(flowersSetSurface(JNIEnv *env, jobject obj, jobject surface)) {
	gl_ThreadSetWindow(
			surface != NULL ? ANativeWindow_fromSurface(env, surface) : NULL);
}

void GL_EXTERN(flowersSetSurfaceSize(JNIEnv *env, jobject obj, jint width, jint height)) {
	gl_ThreadSetWindowSize(width, height);
}
