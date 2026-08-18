/*
 * gAndroidWindow.cpp
 *
 *  Created on: June 24, 2023
 *      Author: Metehan Gezer
 */

#include "gAndroidWindow.h"
#include "gGUITextbox.h"
#include "gRenderer.h"
#include "gAppManager.h"
#include <vector>

#ifdef GLIST_HAS_VULKAN
#include <vulkan/vulkan.h>
#endif

#ifdef ANDROID

ANativeWindow* gAndroidWindow::nativewindow = nullptr;
gAndroidWindow* window = nullptr;

gAndroidWindow::gAndroidWindow() {
    window = this;
	isclosed = true;
	isrendering = false;
}

gAndroidWindow::~gAndroidWindow() {
    close();
}

void gAndroidWindow::initialize(int uwidth, int uheight, int windowMode, bool isResizable) {
    gLogi("gAndroidWindow") << "initialize";
	usevulkan = appmanager != nullptr && appmanager->getRenderEngine() == G_RENDERER_VK;
	if(usevulkan) {
		if(nativewindow == nullptr) {
			gLoge("gAndroidWindow") << "Cannot initialize Vulkan without an ANativeWindow";
			return;
		}
#ifndef GLIST_HAS_VULKAN
		gLoge("gAndroidWindow") << "Vulkan was requested but the Android Vulkan loader was not found; falling back to OpenGL ES";
		usevulkan = false;
		appmanager->setRenderEngine(G_RENDERER_GL);
#else
		width = ANativeWindow_getWidth(nativewindow);
		height = ANativeWindow_getHeight(nativewindow);
		if(uwidth == 0) uwidth = width;
		if(uheight == 0) uheight = height;
		scalex = static_cast<float>(width) / static_cast<float>(uwidth);
		scaley = static_cast<float>(height) / static_cast<float>(uheight);
		isclosed = false;
		isrendering = true;
		gBaseWindow::initialize(width, height, windowMode, false);
		return;
#endif
	}
	const EGLint attribs[] = {
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, // request OpenGL ES 3.0
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 16,
            EGL_NONE
	};
	EGLint numConfigs;
	EGLint format;

	if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
		gLogi("gAndroidWindow") << "eglGetDisplay() returned error " << eglGetError();
        exit(-1);
		return;
	}
	if (!eglInitialize(display, 0, 0)) {
		gLogi("gAndroidWindow") << "eglInitialize() returned error " << eglGetError();
        exit(-1);
		return;
	}

	if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs)) {
		gLogi("gAndroidWindow") << "eglChooseConfig() returned error " << eglGetError();
		close();
		return;
	}

	if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
		gLogi("gAndroidWindow") << "eglGetConfigAttrib() returned error " << eglGetError();
		close();
		return;
	}

	if (!(surface = eglCreateWindowSurface(display, config, nativewindow, 0))) {
		gLogi("gAndroidWindow") << "eglCreateWindowSurface() returned error " << eglGetError();
		close();
		return;
	}
	surfacewindow = nativewindow;
    const EGLint attribList[] = {
			EGL_CONTEXT_CLIENT_VERSION, 3,
			EGL_NONE
	};

	if (!(context = eglCreateContext(display, config, EGL_NO_CONTEXT, attribList))) {
		gLogi("gAndroidWindow") << "eglCreateContext() returned error " << eglGetError();
		close();
		return;
	}

	if (!eglMakeCurrent(display, surface, surface, context)) {
		gLogi("gAndroidWindow") << "eglMakeCurrent() returned error " << eglGetError();
		close();
		return;
	}

	if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
		!eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
		gLogi("gAndroidWindow") << "eglQuerySurface() returned error " << eglGetError();
		close();
		return;
	}

	glViewport(0, 0, width, height);
	if(uwidth == 0) {
		uwidth = width;
	}
	if(uheight == 0) {
		uheight = height;
	}
	scalex = (float) width / (float) uwidth;
	scaley = (float) height / (float) uheight;
	isclosed = false;
	isrendering = true;
	gBaseWindow::initialize(width, height, windowMode, false);
}


bool gAndroidWindow::getShouldClose() {
	return isclosed;
}

void gAndroidWindow::update() {
    if(!isrendering) {
        return;
    }
	if(usevulkan) return;
	if(!eglSwapBuffers(display, surface)) {
        EGLint err = eglGetError();
        if(err == EGL_BAD_SURFACE) {
            isrendering = false;
			close();
            return;
        }
		gLogi("gAndroidWindow") << "eglSwapBuffers() returned error " << err;
	}
}

void gAndroidWindow::close() {
	if(usevulkan) {
		isrendering = false;
		isclosed = true;
		return;
	}
    if(!display) {
        return;
    }
    isrendering = false;
    gLogi("gAndroidWindow") << "close";
	eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE, EGL_NO_CONTEXT );
	eglDestroySurface(display, surface);
	eglDestroyContext(display,context);
	eglTerminate(display);
	display = nullptr;
	isclosed = true;
}

bool gAndroidWindow::supportsVulkan() const {
#ifdef GLIST_HAS_VULKAN
	return usevulkan && nativewindow != nullptr;
#else
	return false;
#endif
}

void gAndroidWindow::getVulkanInstanceExtensions(std::vector<const char*>& extensions) const {
#ifdef GLIST_HAS_VULKAN
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#else
	(void)extensions;
#endif
}

bool gAndroidWindow::createVulkanSurface(void* instance, void* surface) {
#ifdef GLIST_HAS_VULKAN
	if(nativewindow == nullptr || instance == nullptr || surface == nullptr) return false;
	VkAndroidSurfaceCreateInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	info.window = nativewindow;
	return vkCreateAndroidSurfaceKHR(*static_cast<VkInstance*>(instance), &info, nullptr,
			static_cast<VkSurfaceKHR*>(surface)) == VK_SUCCESS;
#else
	(void)instance;
	(void)surface;
	return false;
#endif
}

void gAndroidWindow::setVsync(bool vsync) {
    gBaseWindow::setVsync(vsync);
}

void gAndroidWindow::setCursor(int cursorNo) {
}

void gAndroidWindow::setCursorMode(gCursorMode cursorMode) {
}

void gAndroidWindow::setClipboardString(std::string text) {
	gAndroidUtil::setClipboardText(text);
}

std::string gAndroidWindow::getClipboardString() {
	return gAndroidUtil::getClipboardText();
}

void gAndroidWindow::setWindowSize(int width, int height) {
}

void gAndroidWindow::setWindowResizable(bool isResizable) {
}

void gAndroidWindow::setWindowSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight) {
}

void gAndroidWindow::resize(int surfaceWidth, int surfaceHeight) {
	if(usevulkan) {
		width = surfaceWidth;
		height = surfaceHeight;
		if(width > 0 && height > 0) setSize(width, height);
		return;
	}
	if(!recreateSurfaceIfNeeded()) {
		close();
		return;
	}
	// SurfaceHolder supplies the dimensions belonging to this exact callback.
	// eglQuerySurface can briefly report the previous surface size on rotation.
	width = surfaceWidth;
	height = surfaceHeight;
	if(width <= 0 || height <= 0) return;
    glViewport(0, 0, width, height);
    setSize(width, height);
}

bool gAndroidWindow::recreateSurfaceIfNeeded() {
	if(nativewindow == surfacewindow) return true;
	if(!nativewindow || display == EGL_NO_DISPLAY || context == EGL_NO_CONTEXT) return false;

	// Android can replace SurfaceView's native window during a rotation.  An EGL
	// surface remains bound to the old window, even though its dimensions may be
	// reported asynchronously. Recreate it on the engine thread before reading
	// the new size, rather than drawing one frame into the stale surface.
	eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	if(surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
	surface = eglCreateWindowSurface(display, config, nativewindow, nullptr);
	if(surface == EGL_NO_SURFACE) return false;
	if(!eglMakeCurrent(display, surface, surface, context)) return false;

	if(surfacewindow) ANativeWindow_release(surfacewindow);
	surfacewindow = nativewindow;
	return true;
}

extern "C" {
JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_setSurface(JNIEnv *env, jclass clazz, jobject surface) {
	if(surface != nullptr) {
		gAndroidWindow::nativewindow = ANativeWindow_fromSurface(env, surface);
	} else {
		if(window) {
			window->close();
		}
		ANativeWindow_release(gAndroidWindow::nativewindow);
	}
}

JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onResize(JNIEnv *env, jclass clazz, jint width, jint height) {
	if(appmanager) {
		appmanager->submitToMainThread([width, height]() {
			if(!window) {
				return;
			}
			window->resize(width, height);
		});
	}
}

JNIEXPORT jboolean JNICALL Java_dev_glist_android_lib_GlistNative_onTouchEvent(JNIEnv *env, jclass clazz, jint pointerCount, jintArray pointerIds, jintArray x, jintArray y, jintArray types, jint actionIndex, jint actionMasked) {
	if(!window) {
		return false;
	}

	int* _pointerIds = env->GetIntArrayElements(pointerIds, new jboolean(false));
	int* _x = env->GetIntArrayElements(x, new jboolean(false));
	int* _y = env->GetIntArrayElements(y, new jboolean(false));
	int* _types = env->GetIntArrayElements(types, new jboolean(false));
	std::vector<TouchInput> inputs(pointerCount);
	for(int i = 0; i < pointerCount; ++i) {
		inputs[i] = {(InputType) _types[i], _pointerIds[i], i, _x[i], _y[i]};
	}
	env->ReleaseIntArrayElements(pointerIds, _pointerIds, JNI_ABORT);
	env->ReleaseIntArrayElements(x, _x, JNI_ABORT);
	env->ReleaseIntArrayElements(y, _y, JNI_ABORT);
	env->ReleaseIntArrayElements(types, _types, JNI_ABORT);

	if(appmanager) {
		appmanager->submitToMainThread([inputs, actionIndex, actionMasked]() mutable {
			gTouchEvent event{(int)inputs.size(), inputs.data(), actionIndex, (ActionType) actionMasked};
			window->callEvent(event);
		});
	}
	return true;
}

extern "C" JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onCharPressed(JNIEnv *env, jclass clazz, jint codepoint) {
	if(appmanager) {
		appmanager->submitToMainThread([codepoint]() {
			gCharTypedEvent event{(unsigned int)codepoint};
			window->callEvent(event);
		});
	}
}

extern "C" JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onKeyDown(JNIEnv *env, jclass clazz, jint keycode) {
	if(appmanager) {
		appmanager->submitToMainThread([keycode]() {
			int glistkey = keycode;
			if (keycode == 67) glistkey = 259; // G_KEY_BACKSPACE
			else if (keycode == 66) glistkey = 257; // G_KEY_ENTER
			else if (keycode == 21) glistkey = 263; // G_KEY_LEFT
			else if (keycode == 22) glistkey = 262; // G_KEY_RIGHT
			else if (keycode == 19) glistkey = 265; // G_KEY_UP
			else if (keycode == 20) glistkey = 264; // G_KEY_DOWN
			else if (keycode == 112) glistkey = 261; // G_KEY_DELETE

			gKeyPressedEvent event{glistkey};
			window->callEvent(event);
		});
	}
}

extern "C" JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onKeyUp(JNIEnv *env, jclass clazz, jint keycode) {
	if(appmanager) {
		appmanager->submitToMainThread([keycode]() {
			int glistkey = keycode;
			if (keycode == 67) glistkey = 259; // G_KEY_BACKSPACE
			else if (keycode == 66) glistkey = 257; // G_KEY_ENTER
			else if (keycode == 21) glistkey = 263; // G_KEY_LEFT
			else if (keycode == 22) glistkey = 262; // G_KEY_RIGHT
			else if (keycode == 19) glistkey = 265; // G_KEY_UP
			else if (keycode == 20) glistkey = 264; // G_KEY_DOWN
			else if (keycode == 112) glistkey = 261; // G_KEY_DELETE

			gKeyReleasedEvent event{glistkey};
			window->callEvent(event);
		});
	}
}

JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onOrientationChanged(JNIEnv *env, jclass clazz, jint orientation) {
	if(!window) {
		return;
	}

    gDeviceOrientationChangedEvent event{(DeviceOrientation) orientation};
	window->callEvent(event);
}

extern "C" JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onCutPressed(JNIEnv *env, jclass clazz) {
	if(appmanager) {
		appmanager->submitToMainThread([]() {
			if (gGUITextbox::focusedtextbox) {
				gGUITextbox::focusedtextbox->cutText();
			}
		});
	}
}

extern "C" JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onCopyPressed(JNIEnv *env, jclass clazz) {
	if(appmanager) {
		appmanager->submitToMainThread([]() {
			if (gGUITextbox::focusedtextbox) {
				gGUITextbox::focusedtextbox->copyText();
			}
		});
	}
}

extern "C" JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onPastePressed(JNIEnv *env, jclass clazz) {
	if(appmanager) {
		appmanager->submitToMainThread([]() {
			if (gGUITextbox::focusedtextbox) {
				gGUITextbox::focusedtextbox->pasteText();
			}
		});
	}
}

extern "C" JNIEXPORT void JNICALL Java_dev_glist_android_lib_GlistNative_onLongPress(JNIEnv *env, jclass clazz, jint x, jint y) {
	if(appmanager) {
		appmanager->submitToMainThread([x, y]() {
			int sx = x;
			int sy = y;
			if(gRenderer::getScreenScaling() > G_SCREENSCALING_NONE) {
				sx = gRenderer::scaleX(x);
				sy = gRenderer::scaleY(y);
			}
			if (gGUITextbox::focusedtextbox) {
				gGUITextbox::focusedtextbox->longPressed(sx, sy);
			}
		});
	}
}

}

#endif /* ANDROID */
