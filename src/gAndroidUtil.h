/*
 * gAndroidUtil.h
 *
 *  Created on: June 24, 2023
 *      Author: Metehan Gezer
 */

#ifndef GANDROIDUTIL_H
#define GANDROIDUTIL_H

#ifdef ANDROID

#include <android/log.h>
#include <android/native_window.h> // requires ndk r5 or newer
#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>
#include <string>
#include <vector>
#include "gWindowEvents.h"

void androidMain();

/**
 * A wrapper for JNI strings, automatically handles
 * deletion and creation of the reference.
 */
class JavaString {
public:
	JavaString(const std::string& string);
	~JavaString();

	constexpr jstring native() { return str; }
	constexpr operator jstring() { return str; }
	constexpr operator jobject() { return str; }
private:
	JNIEnv* env;
	jstring str;
};

/**
 * @brief Predefined vibration effects introduced in Android 10 (API 29).
 * These are optimized by the device manufacturer to feel crisp and consistent.
 */
enum class gVibrationEffect {
	CLICK = 0,
	DOUBLE_CLICK = 1,
	TICK = 2,
	THUD = 3,
	POP = 4,
	HEAVY_CLICK = 5
};

/**
 * @brief Haptic primitives for building complex compositions (API 30+).
 * These allow for "speaker-like" textures.
 */
enum class gHapticPrimitive {
	CLICK = 1,
	THUD = 2,
	SPIN = 3,
	QUICK_RISE = 4,
	SLOW_RISE = 5,
	QUICK_FALL = 6,
	TICK = 7,
	LOW_TICK = 8
};

/**
 * @brief Standard UI feedback constants from Android HapticFeedbackConstants.
 * These should be used for UI interactions to ensure consistency with the system.
 */
enum class gHapticFeedback {
	NO_HAPTICS = -1,
	LONG_PRESS = 0,
	VIRTUAL_KEY = 1,
	KEYBOARD_TAP = 3,
	CLOCK_TICK = 4,
	CALENDAR_DATE = 5,
	CONTEXT_CLICK = 6,
	KEYBOARD_RELEASE = 7,
	VIRTUAL_KEY_RELEASE = 8,
	TEXT_HANDLE_MOVE = 9,
	GESTURE_START = 12,
	GESTURE_END = 13,
	CONFIRM = 16,
	REJECT = 17,
	TOGGLE_ON = 21,
	TOGGLE_OFF = 22,
	GESTURE_THRESHOLD_ACTIVATE = 23,
	GESTURE_THRESHOLD_DEACTIVATE = 24,
	DRAG_START = 25,
	SEGMENT_TICK = 26,
	SEGMENT_FREQUENT_TICK = 27
};

enum class gHapticFlag {
	NONE = 0,
	IGNORE_VIEW_SETTING = 0x0001,
	IGNORE_GLOBAL_SETTING = 0x0002
};

/**
 * @brief Categorizes vibration purpose to respect system-level user settings.
 */
enum class gVibrationUsage {
	UNKNOWN = 0,
	TOUCH = 13,
	GAME = 14,
	NOTIFICATION = 5,
	COMMUNICATION_REQUEST = 2,
	ALARM = 4,
	RINGTONE = 6
};

/**
 * @brief A single element in a haptic composition.
 */
struct gHapticElement {
	gHapticPrimitive primitive;
	float scale; // Intensity (0.0 to 1.0)
	int delay;   // Delay in milliseconds after the previous primitive
};

class gAndroidUtil {
public:
	static AAssetManager* assets;
	static std::string datadirectory;

	static AAsset* loadAsset(const std::string& path, int mode);
	static void closeAsset(AAsset* asset);

	static JavaVM* getJavaVM();
	static JNIEnv* getJNIEnv();

	static jclass getJavaGlistAndroid();
	static jobject getJavaAndroidActivity();

	static void attachMainThread(jobject classloader);

	static void setDeviceOrientation(DeviceOrientation orientation);
	static void setFullscreen(bool fullscreen);
	static bool isFullscreen();

	static void disableActionBar(bool isDisabled);
	static bool isActionBarDisabled();
	static void disableScreenLock(bool isDisabled);
	static bool isScreenLockDisabled();

	static std::string getDeviceName();
	static int getAndroidApiLevel();
	static std::string getInstallerPackage();

	static void openURL(const std::string& url);
	static void showKeyboard();
	static void hideKeyboard();
	static void openEmail(const std::string& mailAddress, const std::string& subject, const std::string& message);

	static void vibrate(long milliseconds);
	static void vibrate(long milliseconds, float strength);

	/**
	 * @brief Trigger a predefined system effect (API 29+).
	 */
	static void vibrate(gVibrationEffect effect);

	/**
	 * @brief Play a waveform pattern of ON/OFF durations.
	 * @param timings Array of durations (ms). Starts with OFF duration.
	 * @param repeat Index to start repeating from, or -1 for no repeat.
	 */
	static void vibrate(const std::vector<long>& timings, int repeat = -1);

	/**
	 * @brief Play a waveform with specific amplitudes (API 26+).
	 * @param amplitudes Array of intensity values (0-255).
	 */
	static void vibrate(const std::vector<long>& timings, const std::vector<int>& amplitudes, int repeat = -1);

	/**
	 * @brief Build and play a rich haptic composition (API 30+).
	 */
	static void vibrate(const std::vector<gHapticElement>& elements);

	/**
	 * @brief Perform standard UI feedback (e.g. Confirm, Reject).
	 * This is the preferred method for UI interactions.
	 * @param flags Flags to control feedback behavior (e.g. ignore system settings).
	 */
	static void performHapticFeedback(gHapticFeedback feedback, gHapticFlag flags = gHapticFlag::NONE);

	static bool arePrimitivesSupported(const std::vector<gHapticPrimitive>& primitives);

	/**
	 * @brief Check if the device has hardware-optimized support for predefined effects.
	 * Returns 1 (Yes), 0 (Unknown), or -1 (No).
	 */
	static int areEffectsSupported(const std::vector<gVibrationEffect>& effects);

	static void setVibrationUsage(gVibrationUsage usage);

	static void stopVibration();

	static bool hasVibrator();
	static bool hasAmplitudeControl();
	static void setClipboardText(const std::string& text);
	static std::string getClipboardText();
	static bool hasClipboardText();

	static int getBatteryLevel();
	static bool isBatteryCharging();

	static void setBrightness(float brightness);
	static float getBrightness();

	static bool isDarkMode();

	static std::string loadURL(const std::string& url);
	static bool saveURLString(const std::string& url, const std::string fileName);
	static bool saveURLRaw(const std::string url, const std::string fileName);

	static std::string getSharedPreferences(const std::string& key, const std::string& defaultValue);
	static void setSharedPreferences(const std::string& key, const std::string& value);
	static std::string getCountrySim();
	static std::string getCountryLocale();
	static std::string getDisplayLanguage();
	static std::string getLanguage();
	static std::string getISO3Language();

	/**
	 * @brief Forcefully copies all Android assets from the APK to the data directory.
	 *
	 * Android assets are normally copied during the first launch if the app version number has changed or in debug mode.
	 * This function allows developers to copy all assets at runtime on demand, bypassing these checks.
	 */
	static void updateAssets();

	static std::string getPackageName();
	static std::string getVersionName();
	static int getVersionCode();

	static jmethodID getJavaMethodID(jclass classID, std::string methodName, std::string methodSignature);
	static jmethodID getJavaStaticMethodID(jclass classID, std::string methodName, std::string methodSignature);
	static std::string getJavaClassName(jclass classID);
	static jclass getJavaClassID(std::string className);
	static jfieldID getJavaStaticFieldID(jclass classID, std::string fieldName, std::string fieldType);
	static void convertStringToJString(const std::string& str, jstring &jstr);
	static void convertJStringToString(JNIEnv* env, jstring jstr, std::string &str);

	static jobject getJavaStaticObjectField(jclass classID, std::string fieldName, std::string fieldType);
	static jobject getJavaStaticObjectField(std::string className, std::string fieldName, std::string fieldType);

	static void callJavaVoidMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, va_list args);
	static void callJavaVoidMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, ...);
	static void callJavaVoidMethod(jobject object, std::string className, std::string methodName, std::string methodSignature, ...);

	static jobject callJavaStaticObjectMethod(jclass classID, std::string methodName, std::string methodSignature, va_list args);
	static jobject callJavaStaticObjectMethod(jclass classID, std::string methodName, std::string methodSignature, ...);
	static jobject callJavaStaticObjectMethod(std::string className, std::string methodName, std::string methodSignature, ...);

	static bool callJavaStaticBoolMethod(jclass classID, std::string methodName, std::string methodSignature, va_list args);
	static bool callJavaStaticBoolMethod(jclass classID, std::string methodName, std::string methodSignature, ...);
	static bool callJavaStaticBoolMethod(std::string className, std::string methodName, std::string methodSignature, ...);

	static jobject callJavaObjectMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, va_list args);
	static jobject callJavaObjectMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, ...);
	static jobject callJavaObjectMethod(jobject object, std::string className, std::string methodName, std::string methodSignature, ...);

	static void callJavaStaticVoidMethod(jclass classID, std::string methodName, std::string methodSignature, va_list args);
	static void callJavaStaticVoidMethod(jclass classID, std::string methodName, std::string methodSignature, ...);
	static void callJavaStaticVoidMethod(std::string className, std::string methodName, std::string methodSignature, ...);

	static float callJavaFloatMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, va_list args);
	static float callJavaFloatMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, ...);
	static float callJavaFloatMethod(jobject object, std::string className, std::string methodName, std::string methodSignature, ...);
	static float callJavaStaticFloatMethod(jclass classID, std::string methodName, std::string methodSignature, va_list args);
	static float callJavaStaticFloatMethod(jclass classID, std::string methodName, std::string methodSignature, ...);
	static float callJavaStaticFloatMethod(std::string className, std::string methodName, std::string methodSignature, ...);

	static int callJavaIntMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, va_list args);
	static int callJavaIntMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, ...);
	static int callJavaIntMethod(jobject object, std::string className, std::string methodName, std::string methodSignature, ...);
	static int callJavaStaticIntMethod(jclass classID, std::string methodName, std::string methodSignature, va_list args);
	static int callJavaStaticIntMethod(jclass classID, std::string methodName, std::string methodSignature, ...);
	static int callJavaStaticIntMethod(std::string className, std::string methodName, std::string methodSignature, ...);

	static int64_t callJavaLongMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, va_list args);
	static int64_t callJavaLongMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, ...);
	static int64_t callJavaLongMethod(jobject object, std::string className, std::string methodName, std::string methodSignature, ...);

	static bool callJavaBoolMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, va_list args);
	static bool callJavaBoolMethod(jobject object, jclass classID, std::string methodName, std::string methodSignature, ...);
	static bool callJavaBoolMethod(jobject object, std::string className, std::string methodName, std::string methodSignature, ...);

private:
	static jmethodID midVibrate, midVibratePredefined, midVibrateWaveform, midVibrateWaveformAmplitudes, midVibrateComposition, midPerformHapticFeedback, midArePrimitivesSupported, midAreEffectsSupported, midSetVibrationUsage, midStopVibration;
	static void initHapticsCache();
};


#endif /* ANDROID */

#endif //GANDROIDUTIL_H
