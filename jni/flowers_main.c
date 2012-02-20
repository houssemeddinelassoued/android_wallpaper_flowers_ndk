/*
 Copyright 2012 Harri Smått

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include <stdlib.h>
#include <jni.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include "gl_thread.h"

// EXTERN define for JNI functions.
#define FLOWERS_EXTERN(func) Java_fi_harism_wallpaper_flowersndk_FlowerService_ ## func

// Global host count.
int flowers_hostCount;

// Global thread callback functions struct.
#define THREAD_FUNCS flowers_thread_funcs
gl_thread_funcs_t THREAD_FUNCS;

// Render callback prototypes (flowers_renderer.c).
void flowers_OnRenderFrame();
void flowers_OnSurfaceChanged(int width, int height);
void flowers_OnSurfaceCreated();

// EGLConfig chooser implementation.
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

		// We search for highest RGB depth with lowest A, depth and stencil depth.
		if (sum > highestSum || (sum == highestSum && sub > highestSub)) {
			retConfig = config;
			highestSum = sum;
			highestSub = sub;
		}
	}
	return retConfig;
}

void FLOWERS_EXTERN(flowersConnect(JNIEnv *env)) {
	// If host count == 0, start rendering thread.
	// Otherwise we expect it to be running already.
	if (flowers_hostCount == 0) {
		THREAD_FUNCS.chooseConfig = flowers_ChooseConfig;
		THREAD_FUNCS.onRenderFrame = flowers_OnRenderFrame;
		THREAD_FUNCS.onSurfaceChanged = flowers_OnSurfaceChanged;
		THREAD_FUNCS.onSurfaceCreated = flowers_OnSurfaceCreated;
		gl_ThreadCreate(&THREAD_FUNCS);
	}
	++flowers_hostCount;
}

void FLOWERS_EXTERN(flowersDisconnect(JNIEnv *env)) {
	// If host count == 1, destroy rendering thread.
	if (flowers_hostCount == 1) {
		gl_ThreadDestroy();
	}
	if (flowers_hostCount > 0) {
		--flowers_hostCount;
	}
}

void FLOWERS_EXTERN(flowersSetPaused(JNIEnv *env, jobject obj, jboolean paused)) {
	// Update rendering thread paused state.
	if (paused == JNI_TRUE) {
		gl_ThreadSetPaused(GL_THREAD_TRUE);
	} else {
		gl_ThreadSetPaused(GL_THREAD_FALSE);
	}
}

void FLOWERS_EXTERN(flowersSetSurface(JNIEnv *env, jobject obj, jobject surface)) {
	// Update rendering thread window.
	if (surface) {
		// Rendering thread takes ownership of ANativeWindow in a sense
		// it will call ANativeWindow_release for it once its done
		// with it.
		gl_ThreadSetWindow(ANativeWindow_fromSurface(env, surface));
	} else {
		gl_ThreadSetWindow(NULL);
	}
}

void FLOWERS_EXTERN(flowersSetSurfaceSize(JNIEnv *env, jobject obj, jint width, jint height)) {
	// Update rendering thread window size.
	gl_ThreadSetWindowSize(width, height);
}
