package dev.glist.android.lib; // Do not change! GlistEngine links to this package.

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.Build;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.widget.Toast;

import androidx.annotation.StringRes;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;


import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.URL;
import java.util.Locale;
import java.util.Objects;


import dev.glist.android.BaseGlistAppActivity;
import dev.glist.glistapp.R;

@SuppressLint("StaticFieldLeak")
public class GlistNative {

    private static BaseGlistAppActivity activity;
    private static GlistOrientationListener orientationListener;
    private static String dataDir;
    private static PackageInfo packageInfo;

    @SuppressLint("ApplySharedPref")
    public static SurfaceView init(BaseGlistAppActivity activity, String libraryName) {
        boolean isDebug = ((activity.getBaseContext().getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0);
        // this needs to be changed if project name inside CMakeLists.txt is changed.
        if (isDebug) {
            System.loadLibrary(libraryName + "d");
        } else {
            System.loadLibrary(libraryName);
        }

        GlistNative.activity = activity;
        ActionBar actionBar = activity.getSupportActionBar();
        if (actionBar != null) {
            actionBar.hide();
        }
        setAssetManager(activity.getAssets());
        dataDir = activity.getDataDir().toString() + "/files";
        setDataDirectory(dataDir);
        try {
            packageInfo = activity.getPackageManager().getPackageInfo(activity.getPackageName(), 0);
        } catch (PackageManager.NameNotFoundException e) {
            throw new RuntimeException(e);
        }
        String version = packageInfo.versionName;
        SharedPreferences preferences = activity.getSharedPreferences(packageInfo.packageName, Context.MODE_PRIVATE);
        String assetsVersion = preferences.getString("glist.copy_assets", null);
        if (!Objects.equals(version, assetsVersion) || isDebug) {
            // todo delete old assets
            updateAssets();
            preferences.edit().putString("glist.copy_assets", version).commit(); // Intentionally used commit() here! Do not change.
        }

        activity.setContentView(R.layout.main);
        SurfaceView view = activity.findViewById(R.id.surfaceview);
        view.getHolder().addCallback(activity);
        return view;
    }

    /**
     * Copies all assets from APK to data directory.
     */
    public static void updateAssets() {
        Log.i("GlistNative", "Updating assets...");
        GlistNativeUtil.copyAssetFolder(activity.getAssets(), "", dataDir);
        Log.i("GlistNative", "Updating assets done!");
    }

    public static String getPackageName() {
        return packageInfo.packageName;
    }

    public static String getVersionName() {
        return packageInfo.versionName;
    }

    public static int getVersionCode() {
        return packageInfo.versionCode;
    }

    public static void enableOrientationListener() {
        if (orientationListener == null) {
            orientationListener = new GlistOrientationListener(activity);
        }
        orientationListener.enable();
    }

    public static void disableOrientationListener() {
        if (orientationListener == null) {
            return;
        }
        orientationListener.disable();
    }

    public static native void onCreate();
    public static native void onDestroy();
    public static native void onStart(ClassLoader classLoader);
    public static native void onStop();
    public static native void onPause();
    public static native void onResume();

    public static native void setSurface(Surface surface);
    public static native void onResize(int width, int height);
    public static native void setAssetManager(AssetManager assets);
    public static native void setDataDirectory(String path);
    public static native boolean onTouchEvent(int pointerCount, int[] pointerIds, int[] x, int[] y, int[] types, int actionIndex, int actionMasked);

    public static void showAlertDialog(int dialogId, String message, String title, String cancelText, String negativeText, String positiveText) {
        activity.runOnUiThread(() -> {
            AlertDialog.Builder builder = new AlertDialog.Builder(activity);
            builder.setMessage(message);
            builder.setTitle(title);
            if (cancelText != null && !cancelText.isEmpty()) {
                builder.setNeutralButton(cancelText, (dialog, which) -> onDialogButtonCallback(dialogId, which));
            }
            if (negativeText != null && !negativeText.isEmpty()) {
                builder.setNegativeButton(negativeText, (dialog, which) -> onDialogButtonCallback(dialogId, which));
            }
            if (positiveText != null && !positiveText.isEmpty()) {
                builder.setPositiveButton(positiveText, (dialog, which) -> onDialogButtonCallback(dialogId, which));
            }
            builder.setOnCancelListener(dialog -> onDialogDismissCallback(dialogId));
            builder.setOnDismissListener(dialog -> onDialogClosedCallback(dialogId));
            builder.create().show();
        });
    }

    public static void showAlertDialog(int dialogId, String message, String title, @StringRes Integer cancelTextId, @StringRes Integer negativeTextId, @StringRes Integer positiveTextId) {
        activity.runOnUiThread(() -> {
            AlertDialog.Builder builder = new AlertDialog.Builder(activity);
            builder.setMessage(message);
            builder.setTitle(title);
            if (cancelTextId != null) {
                builder.setNeutralButton(cancelTextId, (dialog, which) -> onDialogButtonCallback(dialogId, which));
            }
            if (negativeTextId != null) {
                builder.setNegativeButton(negativeTextId, (dialog, which) -> onDialogButtonCallback(dialogId, which));
            }
            if (positiveTextId != null) {
                builder.setPositiveButton(positiveTextId, (dialog, which) -> onDialogButtonCallback(dialogId, which));
            }
            builder.setOnCancelListener(dialog -> onDialogDismissCallback(dialogId));
            builder.setOnDismissListener(dialog -> onDialogClosedCallback(dialogId));
            builder.create().show();
        });
    }

    public static void showAlertDialogWithType(int dialogId, String message, String title, String type) {
        if (Objects.equals(type, "ok")) {
            showAlertDialog(dialogId, message, title, null, null, android.R.string.ok);
        } else if (Objects.equals(type, "okcancel")) {
            showAlertDialog(dialogId, message, title, android.R.string.cancel, null, android.R.string.ok);
        } else if (Objects.equals(type, "yesno")) {
            showAlertDialog(dialogId, message, title, null, android.R.string.no, android.R.string.yes);
        } else if (Objects.equals(type, "yesnocancel")) {
            showAlertDialog(dialogId, message, title, android.R.string.cancel, android.R.string.no, android.R.string.yes);
        }
    }

    public static void showToast(String text, int duration) {
        activity.runOnUiThread(() -> {
            Toast.makeText(activity, text, duration).show();
        });
    }

    public static void setFullscreen(boolean fullscreen) {
        if (fullscreen) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                activity.getWindow().getInsetsController().hide(WindowInsets.Type.statusBars());
            } else {
                activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            }
        } else {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                activity.getWindow().getInsetsController().show(WindowInsets.Type.statusBars());
            } else {
                activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
            }
        }
    }

    public static void setDeviceOrientation(int orientation) {
        activity.setRequestedOrientation(orientation);
    }

    public static String getDeviceName() {
        String manufacturer = Build.MANUFACTURER;
        String model = Build.MODEL;
        if (model.startsWith(manufacturer)) {
            return model;
        } else {
            return manufacturer + " " + model;
        }
    }

    public static int getAndroidApiLevel() {
        return android.os.Build.VERSION.SDK_INT;
    }

    public static String getInstallerPackage() {
        String installerPackage;
        try {
            installerPackage = activity.getPackageManager().getInstallerPackageName(getPackageName());
        } catch(Exception e) {
            Log.e("GlistNative", "Failed to get installer package name:"  + e);
            installerPackage = null;
        }
        if (installerPackage == null || installerPackage.equals("")) installerPackage = "terminal";
        return installerPackage;
    }

    public static void openURL(String url) {
        activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
    }

    public static void openEmail(String mailAddress, String subject, String message) {
        if (mailAddress == null || mailAddress.equals("")) {
            mailAddress = "info@example.com";
        }
        if (subject == null || subject.equals("")) {
            subject = "Feedback";
        }
        try {
            Intent selectorIntent = new Intent(Intent.ACTION_SENDTO);
            selectorIntent.setData(Uri.parse("mailto:"));

            Intent emailIntent = new Intent(Intent.ACTION_SEND);
            emailIntent.putExtra(Intent.EXTRA_EMAIL, new String[]{mailAddress});
            emailIntent.putExtra(Intent.EXTRA_SUBJECT, subject);
            if (message != null) {
                emailIntent.putExtra(Intent.EXTRA_TEXT, message);
            }
            emailIntent.setSelector(selectorIntent);
            activity.startActivity(Intent.createChooser(emailIntent, "Select an Email app"));
        } catch (android.content.ActivityNotFoundException e) {
            Toast.makeText(activity, "There are no email clients installed on your device.", Toast.LENGTH_LONG).show();
        }
    }

    public static String getSharedPreferences(String key, String defValue) {
        SharedPreferences sp = activity.getPreferences(Context.MODE_PRIVATE);
        return sp.getString(key, defValue);
    }

    @SuppressLint("ApplySharedPref")
    public static void setSharedPreferences(String key, String value) {
        SharedPreferences.Editor spe = activity.getPreferences(Context.MODE_PRIVATE).edit();
        spe.putString(key, value);
        spe.commit();
    }

    public static String getCountrySim() {
        TelephonyManager tm = (TelephonyManager)activity.getSystemService(Context.TELEPHONY_SERVICE);
        String gsc = tm.getSimCountryIso();
        if (gsc == null || gsc.equals("")) gsc = "Z00";
        return gsc;
    }

    public static String getCountryLocale() {
        return Locale.getDefault().getCountry();
    }

    public static String getDisplayLanguage() {
        return Locale.getDefault().getDisplayLanguage();
    }

    public static String getLanguage() {
        return Locale.getDefault().getLanguage();
    }

    public static String getISO3Language() {
        return Locale.getDefault().getISO3Language();
    }

    public static String loadURL(String url) {
        String response = "";
        try {
            URL pageURL = new URL(url);
            try(BufferedReader in = new BufferedReader(new InputStreamReader(pageURL.openStream()))) {
                int str;
                StringBuilder sb = new StringBuilder(100);
                while ((str = in.read()) != -1) {
                    sb.append((char)str);
                }
                response = sb.toString();
            }
        } catch (Exception e) {
            Log.e("GlistNative", "Failed to load from url " + url, e);
        }
        return response;
    }

    public static boolean saveURLString(String url, String fileName) {
        try {
            File file = new File(fileName);
            URL pageURL = new URL(url);
            StringBuilder builder = new StringBuilder(100);
            try (BufferedReader in = new BufferedReader(new InputStreamReader(pageURL.openStream()))) {
                int c;
                while ((c = in.read()) != -1) {
                    builder.append((char)c);
                }
            }
            try (FileOutputStream fos = new FileOutputStream(file)) {
                fos.write(builder.toString().getBytes());
            }
            return true;
        } catch (Exception e) {
            Log.e("GlistNative", "Failed to save from url " + url + " as string to file: " + fileName, e);
            return false;
        }
    }


    /** @noinspection IOStreamConstructor*/
    public static boolean saveURLRaw(String url, String fileName) {
        Log.i("GlistNative", "Downloading from " + url);
        try {
            URL pageURL = new URL(url);
            try (InputStream input = pageURL.openStream()) {
                Log.i("GlistNative", "Saving to " + fileName);
                try (OutputStream output = new FileOutputStream(fileName)) {
                    byte[] buffer = new byte[2048];
                    int bytesRead;
                    while ((bytesRead = input.read(buffer, 0, buffer.length)) >= 0) {
                        output.write(buffer, 0, bytesRead);
                    }
                }
            }
            return true;
        } catch (Exception e) {
            Log.e("GlistNative", "Failed to save from url " + url + " as raw to file: " + fileName, e);
            return false;
        }
    }

    public static native void onOrientationChanged(int orientation);
    public static native void onDialogButtonCallback(int dialogId, int which);
    public static native void onDialogDismissCallback(int dialogId);
    public static native void onDialogClosedCallback(int dialogId);

    public static BaseGlistAppActivity getActivity() {
        return activity;
    }

    public static GlistOrientationListener getOrientationListener() {
        return orientationListener;
    }
}
