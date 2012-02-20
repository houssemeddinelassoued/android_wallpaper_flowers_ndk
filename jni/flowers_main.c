#include <jni.h>
#include <stdlib.h>
#include <android/native_window_jni.h>
#include "gl_defs.h"
#include "gl_thread.h"
#include "flowers_configchooser.h"

#define LOG_TAG "flowers_main.c"
#define LOGD(...) GL_LOGD(LOG_TAG, __VA_ARGS__)

#define GL_EXTERN(func) Java_fi_harism_wallpaper_flowersndk_FlowerService_ ## func

#define VARS flowers_main_vars
typedef struct {
	int hostCount;
} flowers_main_vars_t;
flowers_main_vars_t VARS;

void GL_EXTERN(flowersConnect(JNIEnv *env)) {
	LOGD("flowersConnect hostCount=%d", VARS.hostCount);
	if (VARS.hostCount == 0) {
		gl_thread_params_t *thread_params = malloc(sizeof *thread_params);
		memset(thread_params, 0, sizeof *thread_params);
		thread_params->gl_ChooseConfig = &flowers_ChooseConfig;
		gl_ThreadStart(thread_params);
	}
	++VARS.hostCount;
}

void GL_EXTERN(flowersDisconnect(JNIEnv *env)) {
	LOGD("flowersDisconnect hostCount=%d", VARS.hostCount);
	if (VARS.hostCount == 1) {
		memset(&VARS, 0, sizeof VARS);
		gl_ThreadStop();
	}
	if (VARS.hostCount > 0) {
		--VARS.hostCount;
	}
}

void GL_EXTERN(flowersResume(JNIEnv *env)) {
	LOGD("flowersResume");
	gl_ThreadResume();
}

void GL_EXTERN(flowersPause(JNIEnv *env)) {
	LOGD("flowersPause");
	gl_ThreadPause();
}

void GL_EXTERN(flowersSetSurface(JNIEnv *env, jobject obj, jobject surface)) {
	LOGD("flowersSetSurface 1");
	if (surface == NULL) {
		gl_ThreadSetSurface(NULL);
	} else {
		gl_ThreadSetSurface(ANativeWindow_fromSurface(env, surface));
	}
}

void GL_EXTERN(flowersSetSurfaceSize(JNIEnv *env, jobject obj, jint width, jint height)) {
	LOGD("flowersSetSurfaceSize 1");
	gl_ThreadSetSurfaceSize(width, height);
}
