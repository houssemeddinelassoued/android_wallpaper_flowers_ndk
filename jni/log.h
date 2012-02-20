#ifndef LOG_H__
#define LOG_H__

#if 1
#include <android/log.h>
#define LOGD(tag, ...) __android_log_print(ANDROID_LOG_DEBUG, tag, __VA_ARGS__)
#else
#define LOGD(...)
#endif

#endif
