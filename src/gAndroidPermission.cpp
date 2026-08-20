/*
 * gAndroidPermission.cpp
 *
 *  Created on: Aug 18, 2026
 *      Author: Umut Akgul
 */

#include "gAndroidPermission.h"
#include "gAndroidUtil.h"
#include "gAndroidDialog.h"
#include <map>
#include <mutex>

struct PermissionRequest {
    PermissionResultCallback callback;
};

static std::map<int, PermissionRequest> pendingRequests;
static std::mutex requestsMutex;

bool gCheckPermission(const std::string& permission) {
    jclass nativeClass = gAndroidUtil::getJavaGlistAndroid();
    return gAndroidUtil::callJavaStaticBoolMethod(nativeClass, "checkPermission", "(Ljava/lang/String;)Z", (jstring)JavaString(permission));
}

void gRequestPermissions(const std::vector<std::string>& permissions, int requestCode, PermissionResultCallback callback) {
    if (callback) {
        std::lock_guard<std::mutex> lock(requestsMutex);
        pendingRequests[requestCode] = {callback};
    }

    JNIEnv* env = gAndroidUtil::getJNIEnv();
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray jPermissions = env->NewObjectArray(permissions.size(), stringClass, nullptr);

    for (size_t i = 0; i < permissions.size(); ++i) {
        jstring jPermission = env->NewStringUTF(permissions[i].c_str());
        env->SetObjectArrayElement(jPermissions, i, jPermission);
        env->DeleteLocalRef(jPermission);
    }

    jclass nativeClass = gAndroidUtil::getJavaGlistAndroid();
    gAndroidUtil::callJavaStaticVoidMethod(nativeClass, "requestPermissions", "([Ljava/lang/String;I)V", jPermissions, requestCode);

    env->DeleteLocalRef(jPermissions);
    env->DeleteLocalRef(stringClass);
}

void gRequestPermission(const std::string& permission, int requestCode, PermissionResultCallback callback) {
    gRequestPermissions({permission}, requestCode, callback);
}

bool gShouldShowRequestPermissionRationale(const std::string& permission) {
    jclass nativeClass = gAndroidUtil::getJavaGlistAndroid();
    return gAndroidUtil::callJavaStaticBoolMethod(nativeClass, "shouldShowRequestPermissionRationale", "(Ljava/lang/String;)Z", (jstring)JavaString(permission));
}

void gRequestPermissionControlled(const std::string& permission, int requestCode,
                                 const std::string& rationaleTitle, const std::string& rationaleMessage,
                                 PermissionResultCallback callback) {
    if (gCheckPermission(permission)) {
        if (callback) {
            callback(requestCode, {permission}, {0 /* PERMISSION_GRANTED */});
        }
        return;
    }

    if (gShouldShowRequestPermissionRationale(permission)) {
        gShowDialog(requestCode + 10000, rationaleMessage, rationaleTitle, "Cancel", "", "OK",
            [permission, requestCode, callback](int id, DialogButton button) {
                if (button == DIALOGBUTTON_POSITIVE) {
                    gRequestPermission(permission, requestCode, callback);
                } else {
                    if (callback) {
                        callback(requestCode, {permission}, {-1 /* PERMISSION_DENIED */});
                    }
                }
            },
            [permission, requestCode, callback](int id) {
                if (callback) {
                    callback(requestCode, {permission}, {-1 /* PERMISSION_DENIED */});
                }
            }
        );
    } else {
        gRequestPermission(permission, requestCode, callback);
    }
}

extern "C" {
JNIEXPORT void JNICALL
Java_dev_glist_android_lib_GlistNative_onRequestPermissionsResult(JNIEnv* env, jclass clazz, jint requestCode, jobjectArray permissions, jintArray grantResults) {
    PermissionResultCallback callback = nullptr;
    {
        std::lock_guard<std::mutex> lock(requestsMutex);
        auto it = pendingRequests.find(requestCode);
        if (it != pendingRequests.end()) {
            callback = it->second.callback;
            pendingRequests.erase(it);
        }
    }

    if (callback) {
        int len = env->GetArrayLength(permissions);
        std::vector<std::string> cppPermissions;
        std::vector<int> cppResults;

        jint* resultsPtr = env->GetIntArrayElements(grantResults, nullptr);
        for (int i = 0; i < len; ++i) {
            jstring jPerm = (jstring)env->GetObjectArrayElement(permissions, i);
            const char* permChars = env->GetStringUTFChars(jPerm, nullptr);
            cppPermissions.push_back(permChars);
            env->ReleaseStringUTFChars(jPerm, permChars);
            env->DeleteLocalRef(jPerm);

            cppResults.push_back(resultsPtr[i]);
        }
        env->ReleaseIntArrayElements(grantResults, resultsPtr, JNI_ABORT);

        callback(requestCode, cppPermissions, cppResults);
    }
}
}
