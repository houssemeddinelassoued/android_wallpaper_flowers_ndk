#include <stdlib.h>
#include "flowers.h"

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
