#include <jni.h>
#include <pthread.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>

#define GL_EXTERN(func) Java_fi_harism_wallpaper_flowersndk_FlowerService_ ## func

#define GL_LOG_TAG "FLOWERS_JNI"
#define GL_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, GL_LOG_TAG, __VA_ARGS__)

#define bool  unsigned char
#define FALSE 0
#define TRUE  1

struct context_t {
	pthread_t pThread;
	pthread_mutex_t pMutex;
	pthread_cond_t pCond;
	int hostCount;

	jobject surfaceObj;
	ANativeWindow* nativeWin;
	ANativeWindow* nativeWinNew;
	int surfaceWidth;
	int surfaceHeight;
	bool surfaceChanged;
	bool threadExit;
	bool threadPause;

	EGLDisplay eglDisplay;
	EGLContext eglContext;
	EGLConfig eglConfig;
	EGLSurface eglSurface;
};
struct context_t context;

bool createEGLSurface() {
	GL_LOGD("createEGLSurface");
	if (context.eglDisplay == EGL_NO_DISPLAY) {
		GL_LOGD("context.eglDisplay == EGL_NO_DISPLAY");
		return FALSE;
	}
	if (context.eglConfig == NULL) {
		GL_LOGD("context.eglConfig == NULL");
		return FALSE;
	}
	if (context.nativeWinNew) {
		GL_LOGD("context.nativeWinNew");
		if (context.nativeWin) {
			ANativeWindow_release(context.nativeWin);
		}
		context.nativeWin = context.nativeWinNew;
		context.nativeWinNew = NULL;
	}
	if (context.nativeWin == NULL) {
		GL_LOGD("context.nativeWin == NULL");
		return FALSE;
	}

	if (context.eglSurface != EGL_NO_SURFACE) {
		if (eglDestroySurface(context.eglDisplay,
				context.eglSurface) != EGL_TRUE) {
			GL_LOGD("eglDestroySurface failed");
		}
	}

	context.eglSurface = eglCreateWindowSurface(context.eglDisplay,
			context.eglConfig, context.nativeWin, NULL);
	if (context.eglSurface == EGL_NO_SURFACE) {
		GL_LOGD("eglCreateWindowSurface failed");
		return FALSE;
	}

	return TRUE;
}

void releaseEGLSurface() {
	GL_LOGD("releaseEGLSurface");

	if (eglDestroySurface(context.eglDisplay, context.eglSurface) != EGL_TRUE) {
		GL_LOGD("eglDestroySurface failed");
	}
	context.eglSurface = EGL_NO_SURFACE;

	if (context.nativeWin) {
		ANativeWindow_release(context.nativeWin);
		context.nativeWin = NULL;
	}
	if (context.nativeWinNew) {
		ANativeWindow_release(context.nativeWinNew);
		context.nativeWinNew = NULL;
	}
}

bool createEGLContext() {
	GL_LOGD("createEGLContext");

	context.eglDisplay = eglGetDisplay(0);
	if (context.eglDisplay == EGL_NO_DISPLAY) {
		GL_LOGD("eglGetDisplay failed");
		return FALSE;
	}
	if (eglInitialize(context.eglDisplay, NULL, NULL) != EGL_TRUE) {
		GL_LOGD("eglInitialize failed");
		return FALSE;
	}

	EGLint numConfigs;
	EGLint configAttrs[] = { EGL_RED_SIZE, 4, EGL_GREEN_SIZE, 4, EGL_BLUE_SIZE,
			4, EGL_ALPHA_SIZE, 0, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
			EGL_NONE };

	if (eglChooseConfig(context.eglDisplay, configAttrs, NULL, 0,
			&numConfigs) != EGL_TRUE) {
		GL_LOGD("eglChooseConfig #1 failed");
		return FALSE;
	}
	if (numConfigs <= 0) {
		GL_LOGD("numConfigs <= 0");
		return FALSE;
	}

	EGLint configsSize = numConfigs;
	EGLConfig* configs = (EGLConfig*) malloc(configsSize * sizeof(EGLConfig));
	if (eglChooseConfig(context.eglDisplay, configAttrs, configs, configsSize,
			&numConfigs) == EGL_FALSE) {
		GL_LOGD("eglChooseConfig #2 failed");
		free(configs);
		return FALSE;
	}

	int idx;
	int highestSum = 0;
	int highestSub = 0;
	for (idx = 0; idx < numConfigs; ++idx) {
		EGLConfig config = configs[idx];
		int r = 0, g = 0, b = 0, a = 0, d = 0, s = 0;
		eglGetConfigAttrib(context.eglDisplay, config, EGL_RED_SIZE, &r);
		eglGetConfigAttrib(context.eglDisplay, config, EGL_GREEN_SIZE, &g);
		eglGetConfigAttrib(context.eglDisplay, config, EGL_BLUE_SIZE, &b);
		eglGetConfigAttrib(context.eglDisplay, config, EGL_ALPHA_SIZE, &a);
		eglGetConfigAttrib(context.eglDisplay, config, EGL_DEPTH_SIZE, &d);
		eglGetConfigAttrib(context.eglDisplay, config, EGL_STENCIL_SIZE, &s);

		int sum = r + g + b;
		int sub = a + d + s;
		if (sum > highestSum || (sum == highestSum && sub > highestSub)) {
			context.eglConfig = config;
			highestSum = sum;
			highestSub = sub;
		}
	}
	free(configs);

	if (context.eglConfig == NULL) {
		GL_LOGD("context.eglConfig == NULL");
		return FALSE;
	}

	EGLint contextAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	context.eglContext = eglCreateContext(context.eglDisplay, context.eglConfig,
			EGL_NO_CONTEXT, contextAttrs);
	if (context.eglContext == EGL_NO_CONTEXT) {
		GL_LOGD("eglCreateContext failed");
		return FALSE;
	}

	context.eglSurface = EGL_NO_SURFACE;
	return TRUE;
}

void releaseEGLContext() {
	GL_LOGD("releaseEGLContext");

	if (context.eglContext != EGL_NO_CONTEXT) {
		if (eglDestroyContext(context.eglDisplay,
				context.eglContext) != EGL_TRUE) {
			GL_LOGD("eglDestroyContext failed");
		}
		context.eglContext = EGL_NO_CONTEXT;
	}
	if (context.eglDisplay != EGL_NO_DISPLAY) {
		if (eglTerminate(context.eglDisplay) != EGL_TRUE) {
			GL_LOGD("eglTerminate failed");
		}
		context.eglDisplay = EGL_NO_DISPLAY;
	}
}

void renderThreadNotify() {
	pthread_cond_signal(&context.pCond);
}

void* renderThread(void *params) {
	bool createEGL = TRUE;
	bool createSurface = TRUE;

	while (!context.threadExit) {

		if (context.threadPause) {
			// Release EGL
			releaseEGLSurface();
			releaseEGLContext();
			createEGL = TRUE;
		}

		while (TRUE) {
			if (context.threadExit) {
				break;
			}
			if ((context.threadPause == FALSE)
					&& (context.surfaceWidth > 0 && context.surfaceHeight > 0)) {
				break;
			}
			pthread_mutex_lock(&context.pMutex);
			pthread_cond_wait(&context.pCond, &context.pMutex);
			pthread_mutex_unlock(&context.pMutex);
		}

		if (context.threadExit) {
			break;
		}

		createSurface |= context.surfaceChanged;
		context.surfaceChanged = FALSE;

		if (createEGL) {
			// Create EGL
			createEGLContext();
			// Notify renderer
			createEGL = FALSE;
			createSurface = TRUE;
		}
		if (createSurface) {
			// Create Surface
			createEGLSurface();
			// Notify renderer
			createSurface = FALSE;
		}

		if (context.surfaceWidth > 0 && context.surfaceHeight > 0) {
			// Do render.
		}

		GL_LOGD("renderThread run");
		sleep(2);
	}

	releaseEGLSurface();
	releaseEGLContext();
	GL_LOGD("renderThread exit");
	return NULL;
}

jboolean GL_EXTERN(glInit(JNIEnv *env)) {
	GL_LOGD("glCreate");
	if (context.hostCount == 0) {
		pthread_cond_init(&context.pCond, NULL);
		pthread_mutex_init(&context.pMutex, NULL);
		pthread_create(&context.pThread, NULL, renderThread, NULL);
	}
	++context.hostCount;
	return JNI_TRUE;
}

void GL_EXTERN(glDestroy(JNIEnv *env)) {
	GL_LOGD("glDestroy");
	if (context.hostCount == 1) {
		context.threadExit = TRUE;
		renderThreadNotify();
		pthread_join(context.pThread, NULL);
		pthread_cond_destroy(&context.pCond);
		pthread_mutex_destroy(&context.pMutex);
		memset(&context, 0, sizeof(struct context_t));
	}
	if (context.hostCount > 0) {
		--context.hostCount;
	}
}

void GL_EXTERN(glStart(JNIEnv *env, jobject obj, jobject surface)) {
	GL_LOGD("glStart");
	if (context.surfaceObj != surface) {
		context.surfaceObj = surface;
		if (context.nativeWinNew != NULL) {
			ANativeWindow_release(context.nativeWinNew);
		}
		context.nativeWinNew = ANativeWindow_fromSurface(env, surface);
		context.surfaceChanged = TRUE;
		context.threadPause = FALSE;
		renderThreadNotify();
	}
}

void GL_EXTERN(glStop(JNIEnv *env)) {
	GL_LOGD("glStop");
	context.threadPause = TRUE;
	renderThreadNotify();
}

void GL_EXTERN(glSetSize(JNIEnv *env, jobject obj, jint width, jint height)) {
	GL_LOGD("glSetSize");
	if (context.surfaceWidth != width || context.surfaceHeight != height) {
		context.surfaceWidth = width;
		context.surfaceHeight = height;
		context.surfaceChanged = TRUE;
		renderThreadNotify();
	}
}
