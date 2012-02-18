#include <jni.h>
#include <android/log.h>

#define GL_EXTERN(func) Java_fi_harism_wallpaper_flowersndk_FlowerService_ ## func

#define GL_LOG_TAG "FLOWERS_JNI"
#define GL_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, GL_LOG_TAG, __VA_ARGS__)

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
	return JNI_VERSION_1_6;
}

jint GL_EXTERN(glInit(JNIEnv* env)) {
	GL_LOGD("TEST1 %s", "TEST2");
	return 10;
}
