#include <stdlib.h>
#include "flowers_configchooser.h"

EGLConfig flowers_ChooseConfig(EGLDisplay eglDisplay, EGLConfig* configArray,
		int configCount) {
	int idx;
	int highestSum = 0;
	int highestSub = 0;
	EGLConfig retConfig = NULL;
	for (idx = 0; idx < configCount; ++idx) {
		EGLConfig config = configArray[idx];
		int r = 0, g = 0, b = 0, a = 0, d = 0, s = 0;
		eglGetConfigAttrib(eglDisplay, config, EGL_RED_SIZE, &r);
		eglGetConfigAttrib(eglDisplay, config, EGL_GREEN_SIZE, &g);
		eglGetConfigAttrib(eglDisplay, config, EGL_BLUE_SIZE, &b);
		eglGetConfigAttrib(eglDisplay, config, EGL_ALPHA_SIZE, &a);
		eglGetConfigAttrib(eglDisplay, config, EGL_DEPTH_SIZE, &d);
		eglGetConfigAttrib(eglDisplay, config, EGL_STENCIL_SIZE, &s);

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
