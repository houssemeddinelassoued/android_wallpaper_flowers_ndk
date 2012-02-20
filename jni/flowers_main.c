#include <jni.h>
#include <stdlib.h>
#include <android/native_window_jni.h>
#include "gl_defs.h"
#include "gl_thread.h"
#include "flowers_configchooser.h"

#define GL_EXTERN(func) Java_fi_harism_wallpaper_flowersndk_FlowerService_ ## func

#define VARS flowers_main_vars
typedef struct {
	int hostCount;
} flowers_main_vars_t;
flowers_main_vars_t VARS;

#define THREAD_PARAMS flowers_thread_params
gl_thread_params_t THREAD_PARAMS = { .gl_ChooseConfig = flowers_ChooseConfig };

void GL_EXTERN(flowersConnect(JNIEnv *env)) {
	if (VARS.hostCount == 0) {
		gl_ThreadCreate(&THREAD_PARAMS);
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
	gl_ThreadSetSurface(
			surface != NULL ? ANativeWindow_fromSurface(env, surface) : NULL);
}

void GL_EXTERN(flowersSetSurfaceSize(JNIEnv *env, jobject obj, jint width, jint height)) {
	gl_ThreadSetSurfaceSize(width, height);
}
