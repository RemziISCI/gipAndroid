/*
 * gAndroidPermission.h
 *
 *  Created on: Aug 18, 2026
 *      Author: Umut Akgul
 */

#ifndef GANDROIDPERMISSION_H
#define GANDROIDPERMISSION_H

#include <string>
#include <vector>
#include <functional>

/**
 * Result of a permission request.
 * 0 = PERMISSION_GRANTED
 * -1 = PERMISSION_DENIED
 */
using PermissionResultCallback = std::function<void(int requestCode, const std::vector<std::string>& permissions, const std::vector<int>& grantResults)>;

/**
 * Checks if a specific permission is granted.
 */
bool gCheckPermission(const std::string& permission);

/**
 * Requests a single permission.
 */
void gRequestPermission(const std::string& permission, int requestCode, PermissionResultCallback callback = nullptr);

/**
 * Requests multiple permissions.
 */
void gRequestPermissions(const std::vector<std::string>& permissions, int requestCode, PermissionResultCallback callback = nullptr);

/**
 * Gets whether you should show UI with rationale before requesting a permission.
 */
bool gShouldShowRequestPermissionRationale(const std::string& permission);

/**
 * A controlled permission request system.
 * 1. Checks if permission is already granted. If so, calls callback immediately.
 * 2. Checks if rationale should be shown. If so, shows a dialog first.
 * 3. Requests the permission.
 *
 * @param permission The permission to request (e.g. "android.permission.CAMERA")
 * @param requestCode Unique code for this request
 * @param rationaleTitle Title for the rationale dialog
 * @param rationaleMessage Message for the rationale dialog
 * @param callback Callback with the result
 */
void gRequestPermissionControlled(const std::string& permission, int requestCode,
                                 const std::string& rationaleTitle, const std::string& rationaleMessage,
                                 PermissionResultCallback callback);

#endif //GANDROIDPERMISSION_H
