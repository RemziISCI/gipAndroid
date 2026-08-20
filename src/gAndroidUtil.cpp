/*
 * gAndroidUtil.cpp
 *
 *  Created on: June 24, 2023
 *      Author: Metehan Gezer
 */


#include "gAndroidUtil.h"
#include "gAppManager.h"
#include "gUtils.h"


#include <thread>

#ifdef __arm__
#include <stdint.h>
extern "C" {
int32_t my_sync_add_and_fetch_4(volatile int32_t* ptr, int32_t val) __asm__("__sync_add_and_fetch_4");

int32_t my_sync_add_and_fetch_4(volatile int32_t* ptr, int32_t val) {
	return __atomic_add_fetch(ptr, val, __ATOMIC_SEQ_CST);
}
}
#endif

AAssetManager* gAndroidUtil::assets;
std::string gAndroidUtil::datadirectory;
JavaVM* javavm;
jobject classloader;

jmethodID gAndroidUtil::midVibrate = nullptr;
jmethodID gAndroidUtil::midVibratePredefined = nullptr;
jmethodID gAndroidUtil::midVibrateWaveform = nullptr;
jmethodID gAndroidUtil::midVibrateWaveformAmplitudes = nullptr;
jmethodID gAndroidUtil::midVibrateComposition = nullptr;
jmethodID gAndroidUtil::midPerformHapticFeedback = nullptr;
jmethodID gAndroidUtil::midArePrimitivesSupported = nullptr;
jmethodID gAndroidUtil::midAreEffectsSupported = nullptr;
jmethodID gAndroidUtil::midSetVibrationUsage = nullptr;
jmethodID gAndroidUtil::midStopVibration = nullptr;

void gAndroidUtil::initHapticsCache() {
	if (midVibrate) return;
	jclass clazz = getJavaGlistAndroid();
	midVibrate = getJavaStaticMethodID(clazz, "vibrate", "(J)V");
	midVibratePredefined = getJavaStaticMethodID(clazz, "vibratePredefined", "(I)V");
	midVibrateWaveform = getJavaStaticMethodID(clazz, "vibrateWaveform", "([JI)V");
	midVibrateWaveformAmplitudes = getJavaStaticMethodID(clazz, "vibrateWaveform", "([J[II)V");
	midVibrateComposition = getJavaStaticMethodID(clazz, "vibrateComposition", "([I[F[I)V");
	midPerformHapticFeedback = getJavaStaticMethodID(clazz, "performHapticFeedback", "(II)V");
	midArePrimitivesSupported = getJavaStaticMethodID(clazz, "arePrimitivesSupported", "([I)Z");
	midAreEffectsSupported = getJavaStaticMethodID(clazz, "areEffectsSupported", "([I)I");
	midSetVibrationUsage = getJavaStaticMethodID(clazz, "setVibrationUsage", "(I)V");
	midStopVibration = getJavaStaticMethodID(clazz, "stopVibration", "()V");
}

JavaString::JavaString(const std::string& string) {
	env = gAndroidUtil::getJNIEnv();
	str = (jstring) env->NewGlobalRef(env->NewStringUTF(string.c_str()));
}

JavaString::~JavaString() {
	env = gAndroidUtil::getJNIEnv();
	env->DeleteGlobalRef(str);
}

AAsset* gAndroidUtil::loadAsset(const std::string& path, int mode) {
	return AAssetManager_open(assets, path.c_str(), mode);
}

void gAndroidUtil::closeAsset(AAsset* asset) {
	AAsset_close(asset);
}

jmethodID gAndroidUtil::getJavaMethodID(jclass classID, std::string methodName, std::string methodSignature) {
	jmethodID result = getJNIEnv()->GetMethodID(classID, methodName.c_str(), methodSignature.c_str());

	if (!result) {
		gLoge("gAndroidUtil") << "couldn't find instance method '"
							  << methodName << "' with signature '"
							  << methodSignature << "' in class '"
							  << getJavaClassName(classID) << "'";
		return nullptr;
	}

	return result;
}

jmethodID gAndroidUtil::getJavaStaticMethodID(jclass classID, std::string methodName, std::string methodSignature) {
	jmethodID result = getJNIEnv()->GetStaticMethodID(classID, methodName.c_str(), methodSignature.c_str());

	if (!result) {
		gLoge("gAndroidUtil") << "couldn't find static method '"
							  << methodName << "' with signature '"
							  << methodSignature << "' in class '"
							  << getJavaClassName(classID) << "'";
		return nullptr;
	}

	return result;
}

std::string gAndroidUtil::getJavaClassName(jclass classID) {
	return "[UNKNOWN]";//TODO
}

jclass gAndroidUtil::getJavaClassID(std::string className) {
	JNIEnv* env = getJNIEnv();
	jclass result = env->FindClass(className.c_str());
	if (!result) {
		env->ExceptionClear();
		jstring str = env->NewStringUTF(className.c_str());
		result = (jclass) gAndroidUtil::callJavaObjectMethod(
				classloader, env->GetObjectClass(classloader), "findClass",
				"(Ljava/lang/String;)Ljava/lang/Class;",
				str);
		env->DeleteLocalRef(str);

		if (!result) {
			gLoge("gAndroidUtil::getJavaClassID") << "couldn't find class '"
												  << className << "'";
			return nullptr;
		}
	}

	return result;
}

jfieldID gAndroidUtil::getJavaStaticFieldID(jclass classID, std::string fieldName, std::string fieldType) {
	jfieldID result = getJNIEnv()->GetStaticFieldID(classID, fieldName.c_str(), fieldType.c_str());
	if (!result) {
		gLoge("gAndroidUtil::getJavaStaticFieldID") << "couldn't find static field '" << fieldName << "' of type '" << fieldType << "' in class '" << gAndroidUtil::getJavaClassName(classID) << "'";
		return nullptr;
	}

	return result;
}

jobject gAndroidUtil::getJavaStaticObjectField(jclass classID, std::string fieldName, std::string fieldType) {
	jfieldID fieldID = gAndroidUtil::getJavaStaticFieldID(classID, fieldName, fieldType);
	if (!fieldID)
		return nullptr;

	return getJNIEnv()->GetStaticObjectField(classID, fieldID);
}

jobject gAndroidUtil::getJavaStaticObjectField(std::string className, std::string fieldName, std::string fieldType) {
	jclass classID = gAndroidUtil::getJavaClassID(className);
	if (!classID)
		return nullptr;

	jobject result = gAndroidUtil::getJavaStaticObjectField(classID, fieldName, fieldType);
	getJNIEnv()->DeleteLocalRef(classID);

	return result;
}

void gAndroidUtil::callJavaVoidMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, va_list args) {
	jmethodID methodID = gAndroidUtil::getJavaMethodID(classID, methodName, methodSignature);
	if (!methodID)
		return;

	getJNIEnv()->CallVoidMethodV(object, methodID, args);
}

void gAndroidUtil::callJavaVoidMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, ...) {
	va_list args;
	va_start(args, methodSignature);
	gAndroidUtil::callJavaVoidMethod(object, classID, methodName, methodSignature, args);
	va_end(args);
}

void gAndroidUtil::callJavaVoidMethod(jobject object, std::string className, std::string methodName, std::string methodSignature, ...) {
	jclass classID = gAndroidUtil::getJavaClassID(className);
	if (!classID)
		return;

	va_list args;
	va_start(args, methodSignature);
	gAndroidUtil::callJavaVoidMethod(object, classID, methodName, methodSignature, args);
	va_end(args);
	getJNIEnv()->DeleteLocalRef(classID);
}

jobject gAndroidUtil::callJavaStaticObjectMethod(jclass classID, std::string methodName, std::string methodSignature, va_list args) {
	jmethodID methodID = gAndroidUtil::getJavaStaticMethodID(classID, methodName, methodSignature);

	if (!methodID)
		return nullptr;

	return getJNIEnv()->CallStaticObjectMethodV(classID, methodID, args);
}

jobject gAndroidUtil::callJavaStaticObjectMethod(jclass classID, std::string methodName, std::string methodSignature, ...) {
	va_list args;
	va_start(args, methodSignature);
	jobject result = gAndroidUtil::callJavaStaticObjectMethod(classID, methodName, methodSignature, args);
	va_end(args);

	return result;
}

jobject gAndroidUtil::callJavaStaticObjectMethod(std::string className, std::string methodName, std::string methodSignature, ...) {
	jclass classID = gAndroidUtil::getJavaClassID(className);
	if (!classID) {
		return nullptr;
	}
	va_list args;
	va_start(args, methodSignature);
	jobject result = gAndroidUtil::callJavaStaticObjectMethod(classID, methodName, methodSignature, args);
	va_end(args);
	getJNIEnv()->DeleteLocalRef(classID);

	return result;
}

bool gAndroidUtil::callJavaStaticBoolMethod(jclass classID, std::string methodName, std::string methodSignature, va_list args) {
	jmethodID methodID = gAndroidUtil::getJavaStaticMethodID(classID, methodName, methodSignature);

	if (!methodID) {
		return false;
	}

	return getJNIEnv()->CallStaticBooleanMethodV(classID, methodID, args);
}

bool gAndroidUtil::callJavaStaticBoolMethod(jclass classID, std::string methodName, std::string methodSignature, ...) {
	va_list args;
	va_start(args, methodSignature);
	bool result = gAndroidUtil::callJavaStaticBoolMethod(classID, methodName, methodSignature, args);
	va_end(args);

	return result;
}

bool gAndroidUtil::callJavaStaticBoolMethod(std::string className, std::string methodName, std::string methodSignature, ...) {
	jclass classID = gAndroidUtil::getJavaClassID(className);
	if (!classID) {
		return false;
	}
	va_list args;
	va_start(args, methodSignature);
	bool result = gAndroidUtil::callJavaStaticBoolMethod(classID, methodName, methodSignature, args);
	va_end(args);
	getJNIEnv()->DeleteLocalRef(classID);

	return result;
}

jobject gAndroidUtil::callJavaObjectMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, va_list args) {
	jmethodID methodid = gAndroidUtil::getJavaMethodID(classID, methodName, methodSignature);
	if (!methodid) {
		return nullptr;
	}

	return getJNIEnv()->CallObjectMethodV(object, methodid, args);
}

jobject gAndroidUtil::callJavaObjectMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, ...) {
	va_list args;
	va_start(args, methodSignature);
	jobject result = gAndroidUtil::callJavaObjectMethod(object, classID, methodName, methodSignature, args);
	va_end(args);

	return result;
}

jobject gAndroidUtil::callJavaObjectMethod(jobject object, std::string className, std::string methodName, std::string methodSignature, ...) {
	jclass classid = gAndroidUtil::getJavaClassID(className);
	if (!classid) {
		return nullptr;
	}

	va_list args;
	va_start(args, methodSignature);
	jobject result = gAndroidUtil::callJavaObjectMethod(object, classid, methodName, methodSignature, args);
	va_end(args);
	getJNIEnv()->DeleteLocalRef(classid);

	return result;
}


void gAndroidUtil::callJavaStaticVoidMethod(jclass classID, std::string methodName, std::string methodSignature, va_list args) {
	jmethodID methodid = gAndroidUtil::getJavaStaticMethodID(classID, methodName, methodSignature);
	if (!methodid)
		return;

	getJNIEnv()->CallStaticVoidMethodV(classID, methodid, args);
}

void gAndroidUtil::callJavaStaticVoidMethod(jclass classID, std::string methodName, std::string methodSignature, ...) {
	va_list args;
	va_start(args, methodSignature);
	gAndroidUtil::callJavaStaticVoidMethod(classID, methodName, methodSignature, args);
	va_end(args);
}

void gAndroidUtil::callJavaStaticVoidMethod(std::string className, std::string methodName, std::string methodSignature, ...) {
	jclass classID = gAndroidUtil::getJavaClassID(className);
	if (!classID)
		return;

	va_list args;
	va_start(args, methodSignature);
	gAndroidUtil::callJavaStaticVoidMethod(classID, methodName, methodSignature, args);
	va_end(args);
	getJNIEnv()->DeleteLocalRef(classID);
}

float gAndroidUtil::callJavaFloatMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, va_list args) {
	jmethodID methodid = gAndroidUtil::getJavaMethodID(classID, methodName, methodSignature);
	if (!methodid) {
		gLoge("gAndroidUtil") << "Couldn't find " << methodName << " for float call";
		return 0;
	}

	return getJNIEnv()->CallFloatMethodV(object, methodid, args);
}

float gAndroidUtil::callJavaFloatMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, ...) {
	va_list args;
	va_start(args, methodSignature);
	auto result = gAndroidUtil::callJavaFloatMethod(object, classID, methodName, methodSignature, args);
	va_end(args);

	return result;
}

float gAndroidUtil::callJavaFloatMethod(jobject object, std::string className, std::string methodName, std::string methodSignature, ...) {
	jclass classID = gAndroidUtil::getJavaClassID(className);
	if (!classID) {
		gLoge() << "Couldn't find " << className << " for float call";
		return 0;
	}

	va_list args;
	va_start(args, methodSignature);
	auto result = gAndroidUtil::callJavaFloatMethod(object, classID, methodName, methodSignature, args);
	va_end(args);
	getJNIEnv()->DeleteLocalRef(classID);

	return result;
}

float gAndroidUtil::callJavaStaticFloatMethod(jclass classID, std::string methodName, std::string methodSignature, va_list args) {
	jmethodID methodid = gAndroidUtil::getJavaStaticMethodID(classID, methodName, methodSignature);
	if (!methodid) {
		gLoge("gAndroidUtil") << "Couldn't find static " << methodName << " for float call";
		return 0;
	}

	return getJNIEnv()->CallStaticFloatMethodV(classID, methodid, args);
}

float gAndroidUtil::callJavaStaticFloatMethod(jclass classID, std::string methodName, std::string methodSignature, ...) {
	va_list args;
	va_start(args, methodSignature);
	auto result = gAndroidUtil::callJavaStaticFloatMethod(classID, methodName, methodSignature, args);
	va_end(args);

	return result;
}

float gAndroidUtil::callJavaStaticFloatMethod(std::string className, std::string methodName, std::string methodSignature, ...) {
	jclass classID = gAndroidUtil::getJavaClassID(className);
	if (!classID) {
		gLoge() << "Couldn't find static " << className << " for float call";
		return 0;
	}

	va_list args;
	va_start(args, methodSignature);
	auto result = gAndroidUtil::callJavaStaticFloatMethod(classID, methodName, methodSignature, args);
	va_end(args);
	getJNIEnv()->DeleteLocalRef(classID);

	return result;
}

int gAndroidUtil::callJavaIntMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, va_list args) {
	jmethodID methodid = gAndroidUtil::getJavaMethodID(classID, methodName, methodSignature);

	if (!methodid) {
		gLoge() << "Couldn't find " << methodName << " for int call";
		return 0;
	}

	return getJNIEnv()->CallIntMethodV(object, methodid, args);
}

int gAndroidUtil::callJavaIntMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, ...) {
	va_list args;
	va_start(args, methodSignature);
	auto result = gAndroidUtil::callJavaIntMethod(object, classID, methodName, methodSignature, args);
	va_end(args);

	return result;
}

int gAndroidUtil::callJavaIntMethod(jobject object, std::string className, std::string methodName, std::string methodSignature, ...) {
	jclass classID = gAndroidUtil::getJavaClassID(className);

	if (!classID) {
		gLoge() << "Couldn't find " << className << " for int call";
		return 0;
	}

	va_list args;
	va_start(args, methodSignature);
	auto result = gAndroidUtil::callJavaIntMethod(object, classID, methodName, methodSignature, args);
	va_end(args);
	getJNIEnv()->DeleteLocalRef(classID);

	return result;
}

int gAndroidUtil::callJavaStaticIntMethod(jclass classID, std::string methodName, std::string methodSignature, va_list args) {
	jmethodID methodid = gAndroidUtil::getJavaStaticMethodID(classID, methodName, methodSignature);
	if (!methodid) {
		gLoge("gAndroidUtil") << "Couldn't find static " << methodName << " for int call";
		return 0;
	}

	return getJNIEnv()->CallStaticIntMethodV(classID, methodid, args);
}

int gAndroidUtil::callJavaStaticIntMethod(jclass classID, std::string methodName, std::string methodSignature, ...) {
	va_list args;
	va_start(args, methodSignature);
	auto result = gAndroidUtil::callJavaStaticIntMethod(classID, methodName, methodSignature, args);
	va_end(args);

	return result;
}

int gAndroidUtil::callJavaStaticIntMethod(std::string className, std::string methodName, std::string methodSignature, ...) {
	jclass classID = gAndroidUtil::getJavaClassID(className);
	if (!classID) {
		gLoge() << "Couldn't find static " << className << " for int call";
		return 0;
	}

	va_list args;
	va_start(args, methodSignature);
	auto result = gAndroidUtil::callJavaStaticIntMethod(classID, methodName, methodSignature, args);
	va_end(args);
	getJNIEnv()->DeleteLocalRef(classID);

	return result;
}

int64_t gAndroidUtil::callJavaLongMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, va_list args) {
	jmethodID methodid = gAndroidUtil::getJavaMethodID(classID, methodName, methodSignature);
	if (!methodid) {
		gLoge() << "Couldn't find " << methodName << " for int64_t call";
		return 0;
	}

	return getJNIEnv()->CallLongMethodV(object, methodid, args);
}

int64_t gAndroidUtil::callJavaLongMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, ...) {
	va_list args;
	va_start(args, methodSignature);
	auto result = gAndroidUtil::callJavaLongMethod(object, classID, methodName, methodSignature, args);
	va_end(args);

	return result;
}

int64_t gAndroidUtil::callJavaLongMethod(jobject object, std::string className, std::string methodName, std::string methodSignature, ...) {
	jclass classID = gAndroidUtil::getJavaClassID(className);

	if (!classID) {
		gLoge() << "Couldn't find " << className << " for int64_t call";
		return 0;
	}

	va_list args;
	va_start(args, methodSignature);
	auto result = gAndroidUtil::callJavaLongMethod(object, classID, methodName, methodSignature, args);
	va_end(args);
	getJNIEnv()->DeleteLocalRef(classID);

	return result;
}

bool gAndroidUtil::callJavaBoolMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, va_list args) {
	jmethodID methodid = gAndroidUtil::getJavaMethodID(classID, methodName, methodSignature);

	if (!methodid) {
		gLoge() << "Couldn't find " << methodName << " for bool call";
		return false;
	}

	return getJNIEnv()->CallBooleanMethodV(object, methodid, args);
}

bool gAndroidUtil::callJavaBoolMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, ...) {
	va_list args;
	va_start(args, methodSignature);
	auto result = gAndroidUtil::callJavaBoolMethod(object, classID, methodName, methodSignature, args);
	va_end(args);

	return result;
}

bool gAndroidUtil::callJavaBoolMethod(jobject object, std::string className, std::string methodName, std::string methodSignature, ...) {
	jclass classID = gAndroidUtil::getJavaClassID(className);
	if (!classID) {
		gLoge("gAndroidUtil") << "Couldn't find " << className << " for bool call";
		return false;
	}

	va_list args;
	va_start(args, methodSignature);
	auto result = gAndroidUtil::callJavaBoolMethod(object, classID, methodName, methodSignature, args);
	va_end(args);
	getJNIEnv()->DeleteLocalRef(classID);

	return result;
}

void gAndroidUtil::convertStringToJString(const std::string& str, jstring& jstr) {
	jstr = getJNIEnv()->NewStringUTF(str.c_str());
}

void gAndroidUtil::convertJStringToString(JNIEnv* env, jstring jstr, std::string& str) {
	jboolean isCopy;
	const char* arrayChar = env->GetStringUTFChars(jstr, &isCopy);
	if (isCopy == JNI_FALSE)
		return;
	str = arrayChar;
}

JavaVM* gAndroidUtil::getJavaVM() {
	return javavm;
}

JNIEnv* gAndroidUtil::getJNIEnv() {
	JNIEnv* env;
	JavaVM* vm = getJavaVM();
	if (!vm) {
		gLoge("gAndroidUtil") << "couldn't get java virtual machine";
		return nullptr;
	}

	int getEnvStat = vm->GetEnv((void**) &env, JNI_VERSION_1_4);

	if (getEnvStat == JNI_EDETACHED) {
		JavaVMAttachArgs args{JNI_VERSION_1_4, nullptr, nullptr};

		if (vm->AttachCurrentThread(&env, &args) != JNI_OK) {
			gLoge("gAndroidUtil") << "couldn't get environment using GetEnv()";
			return nullptr;
		}
	} else if (getEnvStat != JNI_OK) {
		gLoge("gAndroidUtil") << "couldn't get environment using GetEnv()";
		return nullptr;
	}

	return env;
}

jclass gAndroidUtil::getJavaGlistAndroid() {
	return getJavaClassID("dev/glist/android/lib/GlistNative");
}

jobject gAndroidUtil::getJavaAndroidActivity() {
	return getJavaStaticObjectField(getJavaGlistAndroid(), "activity", "Ldev/glist/android/GlistAppActivity;");
}

void gAndroidUtil::attachMainThread(jobject classLoader) {
	gAndroidUtil::getJNIEnv();// Attach the thread to JVM
	classloader = classLoader;
	jclass looper = gAndroidUtil::getJavaClassID("android/os/Looper");
	gAndroidUtil::callJavaStaticVoidMethod(looper, "prepare", "()V");
}

void gAndroidUtil::setDeviceOrientation(DeviceOrientation orientation) {
	jclass glistandroid = getJavaGlistAndroid();

	jmethodID method = getJNIEnv()->GetStaticMethodID(glistandroid, "setDeviceOrientation", "(I)V");
	getJNIEnv()->CallStaticVoidMethod(glistandroid, method, orientation);
}

void gAndroidUtil::setFullscreen(bool fullscreen) {
	jclass glistandroid = getJavaGlistAndroid();

	jmethodID method = getJNIEnv()->GetStaticMethodID(glistandroid, "setFullscreen", "(Z)V");
	getJNIEnv()->CallStaticVoidMethod(glistandroid, method, fullscreen);
}

bool gAndroidUtil::isFullscreen() {
	return callJavaStaticBoolMethod(getJavaGlistAndroid(), "isFullscreen", "()Z");
}

void gAndroidUtil::disableActionBar(bool isDisabled) {
	callJavaStaticVoidMethod(getJavaGlistAndroid(), "disableActionBar", "(Z)V", isDisabled);
}

bool gAndroidUtil::isActionBarDisabled() {
	return callJavaStaticBoolMethod(getJavaGlistAndroid(), "isActionBarDisabled", "()Z");
}

void gAndroidUtil::disableScreenLock(bool isDisabled) {
	callJavaStaticVoidMethod(getJavaGlistAndroid(), "disableScreenLock", "(Z)V", isDisabled);
}

bool gAndroidUtil::isScreenLockDisabled() {
	return callJavaStaticBoolMethod(getJavaGlistAndroid(), "isScreenLockDisabled", "()Z");
}

std::string gAndroidUtil::getDeviceName() {
	jstring jstr = (jstring) callJavaStaticObjectMethod(getJavaGlistAndroid(), "getDeviceName", "()Ljava/lang/String;");
	std::string str;
	convertJStringToString(getJNIEnv(), jstr, str);
	return str;
}

int gAndroidUtil::getAndroidApiLevel() {
	jclass glistandroid = getJavaGlistAndroid();

	JNIEnv* env = getJNIEnv();
	jmethodID methodid = env->GetStaticMethodID(glistandroid, "getAndroidApiLevel", "()I");
	return env->CallStaticIntMethod(glistandroid, methodid);
}

std::string gAndroidUtil::getInstallerPackage() {
	jstring jstr = (jstring) callJavaStaticObjectMethod(getJavaGlistAndroid(), "getInstallerPackage", "()Ljava/lang/String;");
	std::string str;
	convertJStringToString(getJNIEnv(), jstr, str);
	return str;
}

void gAndroidUtil::openURL(const std::string& url) {
	jclass glistandroid = getJavaGlistAndroid();

	jmethodID methodid = getJNIEnv()->GetStaticMethodID(glistandroid, "openURL", "(Ljava/lang/String;)V");
	getJNIEnv()->CallStaticVoidMethod(glistandroid, methodid, JavaString(url).native());
}

void gAndroidUtil::openEmail(const std::string& mailAddress, const std::string& subject, const std::string& message) {
	jclass glistandroid = getJavaGlistAndroid();

	jmethodID method = getJNIEnv()->GetStaticMethodID(glistandroid, "openEmail", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	getJNIEnv()->CallStaticVoidMethod(glistandroid, method, JavaString(mailAddress).native(), JavaString(subject).native(), JavaString(message).native());
}

void gAndroidUtil::vibrate(long milliseconds) {
	initHapticsCache();
	getJNIEnv()->CallStaticVoidMethod(getJavaGlistAndroid(), midVibrate, (jlong) milliseconds);
}

void gAndroidUtil::vibrate(long milliseconds, float strength) {
	initHapticsCache();
	jint amplitude = (jint) (strength * 255.0f);
	if (amplitude < 0) amplitude = 0;
	if (amplitude > 255) amplitude = 255;

	JNIEnv* env = getJNIEnv();
	jlongArray jtimings = env->NewLongArray(1);
	jlong timing = (jlong) milliseconds;
	env->SetLongArrayRegion(jtimings, 0, 1, &timing);

	jintArray jamplitudes = env->NewIntArray(1);
	jint jamp = amplitude;
	env->SetIntArrayRegion(jamplitudes, 0, 1, &jamp);

	env->CallStaticVoidMethod(getJavaGlistAndroid(), midVibrateWaveformAmplitudes, jtimings, jamplitudes, (jint) -1);

	env->DeleteLocalRef(jtimings);
	env->DeleteLocalRef(jamplitudes);
}

void gAndroidUtil::vibrate(gVibrationEffect effect) {
	initHapticsCache();
	getJNIEnv()->CallStaticVoidMethod(getJavaGlistAndroid(), midVibratePredefined, (jint) effect);
}

void gAndroidUtil::vibrate(const std::vector<long>& timings, int repeat) {
	initHapticsCache();
	JNIEnv* env = getJNIEnv();
	jlongArray jtimings = env->NewLongArray(timings.size());
	std::vector<jlong> jtimings_buf(timings.size());
	for (size_t i = 0; i < timings.size(); ++i) jtimings_buf[i] = (jlong) timings[i];
	env->SetLongArrayRegion(jtimings, 0, timings.size(), jtimings_buf.data());

	env->CallStaticVoidMethod(getJavaGlistAndroid(), midVibrateWaveform, jtimings, (jint) repeat);

	env->DeleteLocalRef(jtimings);
}

void gAndroidUtil::vibrate(const std::vector<long>& timings, const std::vector<int>& amplitudes, int repeat) {
	initHapticsCache();
	JNIEnv* env = getJNIEnv();
	jlongArray jtimings = env->NewLongArray(timings.size());
	std::vector<jlong> jtimings_buf(timings.size());
	for (size_t i = 0; i < timings.size(); ++i) jtimings_buf[i] = (jlong) timings[i];
	env->SetLongArrayRegion(jtimings, 0, timings.size(), jtimings_buf.data());

	jintArray jamplitudes = env->NewIntArray(amplitudes.size());
	std::vector<jint> jamplitudes_buf(amplitudes.size());
	for (size_t i = 0; i < amplitudes.size(); ++i) jamplitudes_buf[i] = (jint) amplitudes[i];
	env->SetIntArrayRegion(jamplitudes, 0, amplitudes.size(), jamplitudes_buf.data());

	env->CallStaticVoidMethod(getJavaGlistAndroid(), midVibrateWaveformAmplitudes, jtimings, jamplitudes, (jint) repeat);

	env->DeleteLocalRef(jtimings);
	env->DeleteLocalRef(jamplitudes);
}

void gAndroidUtil::vibrate(const std::vector<gHapticElement>& elements) {
	initHapticsCache();
	JNIEnv* env = getJNIEnv();
	size_t size = elements.size();
	jintArray jprimitives = env->NewIntArray(size);
	jfloatArray jscales = env->NewFloatArray(size);
	jintArray jdelays = env->NewIntArray(size);

	std::vector<jint> primitives(size);
	std::vector<jfloat> scales(size);
	std::vector<jint> delays(size);

	for (size_t i = 0; i < size; i++) {
		primitives[i] = (jint) elements[i].primitive;
		scales[i] = (jfloat) elements[i].scale;
		delays[i] = (jint) elements[i].delay;
	}

	env->SetIntArrayRegion(jprimitives, 0, size, primitives.data());
	env->SetFloatArrayRegion(jscales, 0, size, scales.data());
	env->SetIntArrayRegion(jdelays, 0, size, delays.data());

	env->CallStaticVoidMethod(getJavaGlistAndroid(), midVibrateComposition, jprimitives, jscales, jdelays);

	env->DeleteLocalRef(jprimitives);
	env->DeleteLocalRef(jscales);
	env->DeleteLocalRef(jdelays);
}

void gAndroidUtil::performHapticFeedback(gHapticFeedback feedback, gHapticFlag flags) {
	initHapticsCache();
	getJNIEnv()->CallStaticVoidMethod(getJavaGlistAndroid(), midPerformHapticFeedback, (jint) feedback, (jint) flags);
}

bool gAndroidUtil::arePrimitivesSupported(const std::vector<gHapticPrimitive>& primitives) {
	initHapticsCache();
	JNIEnv* env = getJNIEnv();
	jintArray jprimitives = env->NewIntArray(primitives.size());
	std::vector<jint> primitives_buf(primitives.size());
	for (size_t i = 0; i < primitives.size(); ++i) primitives_buf[i] = (jint) primitives[i];
	env->SetIntArrayRegion(jprimitives, 0, primitives.size(), primitives_buf.data());

	bool result = getJNIEnv()->CallStaticBooleanMethod(getJavaGlistAndroid(), midArePrimitivesSupported, jprimitives);

	env->DeleteLocalRef(jprimitives);
	return result;
}

int gAndroidUtil::areEffectsSupported(const std::vector<gVibrationEffect>& effects) {
	initHapticsCache();
	JNIEnv* env = getJNIEnv();
	jintArray jeffects = env->NewIntArray(effects.size());
	std::vector<jint> effects_buf(effects.size());
	for (size_t i = 0; i < effects.size(); ++i) effects_buf[i] = (jint) effects[i];
	env->SetIntArrayRegion(jeffects, 0, effects.size(), effects_buf.data());

	int result = getJNIEnv()->CallStaticIntMethod(getJavaGlistAndroid(), midAreEffectsSupported, jeffects);

	env->DeleteLocalRef(jeffects);
	return result;
}

void gAndroidUtil::setVibrationUsage(gVibrationUsage usage) {
	initHapticsCache();
	getJNIEnv()->CallStaticVoidMethod(getJavaGlistAndroid(), midSetVibrationUsage, (jint) usage);
}

void gAndroidUtil::stopVibration() {
	initHapticsCache();
	getJNIEnv()->CallStaticVoidMethod(getJavaGlistAndroid(), midStopVibration);
}

bool gAndroidUtil::hasVibrator() {
	jclass glistandroid = getJavaGlistAndroid();
	return callJavaStaticBoolMethod(glistandroid, "hasVibrator", "()Z");
}

bool gAndroidUtil::hasAmplitudeControl() {
	jclass glistandroid = getJavaGlistAndroid();
	return callJavaStaticBoolMethod(glistandroid, "hasAmplitudeControl", "()Z");
}

void gAndroidUtil::setClipboardText(const std::string& text) {
	jclass glistandroid = getJavaGlistAndroid();
	callJavaStaticVoidMethod(glistandroid, "setClipboardText", "(Ljava/lang/String;)V", (jstring) JavaString(text));
}

std::string gAndroidUtil::getClipboardText() {
	jclass glistandroid = getJavaGlistAndroid();
	jstring jstr = (jstring) callJavaStaticObjectMethod(glistandroid, "getClipboardText", "()Ljava/lang/String;");
	std::string str;
	convertJStringToString(getJNIEnv(), jstr, str);
	return str;
}

bool gAndroidUtil::hasClipboardText() {
	jclass glistandroid = getJavaGlistAndroid();
	return callJavaStaticBoolMethod(glistandroid, "hasClipboardText", "()Z");
}

int gAndroidUtil::getBatteryLevel() {
	jclass glistandroid = getJavaGlistAndroid();
	return callJavaStaticIntMethod(glistandroid, "getBatteryLevel", "()I");
}

bool gAndroidUtil::isBatteryCharging() {
	jclass glistandroid = getJavaGlistAndroid();
	return callJavaStaticBoolMethod(glistandroid, "isBatteryCharging", "()Z");
}

void gAndroidUtil::setBrightness(float brightness) {
	jclass glistandroid = getJavaGlistAndroid();
	callJavaStaticVoidMethod(glistandroid, "setBrightness", "(F)V", brightness);
}

float gAndroidUtil::getBrightness() {
	jclass glistandroid = getJavaGlistAndroid();
	return callJavaStaticFloatMethod(glistandroid, "getBrightness", "()F");
}

bool gAndroidUtil::isDarkMode() {
	jclass glistandroid = getJavaGlistAndroid();
	return callJavaStaticBoolMethod(glistandroid, "isDarkMode", "()Z");
}

void gAndroidUtil::showKeyboard() {
	jclass glistandroid = getJavaGlistAndroid();
	callJavaStaticVoidMethod(glistandroid, "showKeyboard", "()V");
}

void gAndroidUtil::hideKeyboard() {
	jclass glistandroid = getJavaGlistAndroid();
	callJavaStaticVoidMethod(glistandroid, "hideKeyboard", "()V");
}

std::string gAndroidUtil::loadURL(const std::string& url) {
	jstring jstr = (jstring) callJavaStaticObjectMethod(getJavaGlistAndroid(), "loadURL", "(Ljava/lang/String;)Ljava/lang/String;", JavaString(url).native());
	std::string str;
	convertJStringToString(getJNIEnv(), jstr, str);
	return str;
}

bool gAndroidUtil::saveURLString(const std::string& url, const std::string fileName) {
	return callJavaStaticBoolMethod(getJavaGlistAndroid(), "saveURLString", "(Ljava/lang/String;Ljava/lang/String;)Z", JavaString(url).native(), JavaString(fileName).native());
}

bool gAndroidUtil::saveURLRaw(const std::string url, const std::string fileName) {
	return callJavaStaticBoolMethod(getJavaGlistAndroid(), "saveURLRaw", "(Ljava/lang/String;Ljava/lang/String;)Z", JavaString(url).native(), JavaString(fileName).native());
}

std::string gAndroidUtil::getSharedPreferences(const std::string& key, const std::string& defaultValue) {
	jstring jstr = (jstring) callJavaStaticObjectMethod(getJavaGlistAndroid(), "getSharedPreferences", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", JavaString(key).native(), JavaString(defaultValue).native());
	std::string str;
	convertJStringToString(getJNIEnv(), jstr, str);
	return str;
}

void gAndroidUtil::setSharedPreferences(const std::string& key, const std::string& value) {
	callJavaStaticVoidMethod(getJavaGlistAndroid(), "setSharedPreferences", "(Ljava/lang/String;Ljava/lang/String;)V", JavaString(key).native(), JavaString(value).native());
}

std::string gAndroidUtil::getCountrySim() {
	jstring jstr = (jstring) callJavaStaticObjectMethod(getJavaGlistAndroid(), "getCountrySim", "()Ljava/lang/String;");
	std::string str;
	convertJStringToString(getJNIEnv(), jstr, str);
	return str;
}

std::string gAndroidUtil::getCountryLocale() {
	jstring jstr = (jstring) callJavaStaticObjectMethod(getJavaGlistAndroid(), "getCountryLocale", "()Ljava/lang/String;");
	std::string str;
	convertJStringToString(getJNIEnv(), jstr, str);
	return str;
}

std::string gAndroidUtil::getDisplayLanguage() {
	jstring jstr = (jstring) callJavaStaticObjectMethod(getJavaGlistAndroid(), "getDisplayLanguage", "()Ljava/lang/String;");
	std::string str;
	convertJStringToString(getJNIEnv(), jstr, str);
	return str;
}

std::string gAndroidUtil::getLanguage() {
	jstring jstr = (jstring) callJavaStaticObjectMethod(getJavaGlistAndroid(), "getLanguage", "()Ljava/lang/String;");
	std::string str;
	convertJStringToString(getJNIEnv(), jstr, str);
	return str;
}

std::string gAndroidUtil::getISO3Language() {
	jstring jstr = (jstring) callJavaStaticObjectMethod(getJavaGlistAndroid(), "getISO3Language", "()Ljava/lang/String;");
	std::string str;
	convertJStringToString(getJNIEnv(), jstr, str);
	return str;
}

void gAndroidUtil::updateAssets() {
	jclass glistandroid = getJavaGlistAndroid();

	jmethodID method = getJNIEnv()->GetStaticMethodID(glistandroid, "updateAssets", "()V");
	getJNIEnv()->CallStaticVoidMethod(glistandroid, method);
}

std::string gAndroidUtil::getPackageName() {
	jstring jstr = (jstring) callJavaStaticObjectMethod(getJavaGlistAndroid(), "getPackageName", "()Ljava/lang/String;");
	std::string str;
	convertJStringToString(getJNIEnv(), jstr, str);
	return str;
}

std::string gAndroidUtil::getVersionName() {
	jstring jstr = (jstring) callJavaStaticObjectMethod(getJavaGlistAndroid(), "getVersionName", "()Ljava/lang/String;");
	std::string str;
	convertJStringToString(getJNIEnv(), jstr, str);
	return str;
}

int gAndroidUtil::getVersionCode() {
	jclass glistandroid = getJavaGlistAndroid();

	JNIEnv* env = getJNIEnv();
	jmethodID method = env->GetStaticMethodID(glistandroid, "getVersionCode", "()I");
	return env->CallStaticIntMethod(glistandroid, method);
}

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env;
	javavm = vm;
	gLogd("gAndroidUtil") << "JNI_OnLoad called";
	if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		gLogd("gAndroidUtil") << "failed to get environment using GetEnv()";
		return -1;
	}

	return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_setAssetManager(JNIEnv* env, jclass clazz, jobject assets) {
	AAssetManager* man = AAssetManager_fromJava(env, assets);
	gAndroidUtil::assets = man;
}

JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_setDataDirectory(JNIEnv* env, jclass clazz, jstring dir) {
	gAndroidUtil::convertJStringToString(env, dir, gAndroidUtil::datadirectory);
}


JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onCreate(JNIEnv* env, jclass clazz) {
	//gLogi("GlistNative") << "onCreate";
	androidMain();
#ifdef DEBUG
	assert(appmanager);// appmanager should not be null after androidMain();
#endif
}

JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onDestroy(JNIEnv* env, jclass clazz) {
	//gLogi("GlistNative") << "onDestroy";
	delete appmanager;
	appmanager = nullptr;
}

std::unique_ptr<std::thread> thread;
JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onStart(JNIEnv* env, jclass clazz, jobject classloader) {
	gLogi("GlistNative") << "onStart";
	if (thread) {
		throw std::runtime_error("cannot call onStart without calling onStop first");
	}

	jobject globalclassloader = env->NewGlobalRef(classloader);
	thread = std::make_unique<std::thread>([globalclassloader]() {
		gAndroidUtil::attachMainThread(globalclassloader);
		appmanager->initialize();
		appmanager->setup();
		appmanager->loop();
		gAndroidUtil::getJavaVM()->DetachCurrentThread();
	});
	thread->detach();
}

JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onStop(JNIEnv* env, jclass clazz) {
	//gLogi("GlistNative") << "onStop";
	if (appmanager) {
		appmanager->stop();
	}
	if (thread) {
		thread->join();// wait for shutdown
		thread = nullptr;
	}
}

JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onPause(JNIEnv* env, jclass clazz) {
	//gLogi("GlistNative") << "onPause";
	if (appmanager) {
		gAppPauseEvent event{};
		appmanager->callEvent(event);
	}
}

JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onResume(JNIEnv* env, jclass clazz) {
	//gLogi("GlistNative") << "onResume";
	if (appmanager) {
		gAppResumeEvent event{};
		appmanager->callEvent(event);
	}
}
}
