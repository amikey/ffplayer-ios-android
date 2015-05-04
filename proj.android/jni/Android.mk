LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := cpp_empty_test_shared

LOCAL_MODULE_FILENAME := libcpp_empty_test

LOCAL_SRC_FILES := main.cpp \
                   ../../Classes/AppDelegate.cpp \
                   ../../Classes/HelloWorldScene.cpp \
				   ../../Classes/ff.cpp \
				   ../../Classes/FFVideo.cpp \
				   ../../Classes/CCFFmpegNode.cpp \
				   ../../Classes/SDLAudio.cpp \
				   ../../Classes/SDLEvent.cpp \
				   ../../Classes/SDLOverlay.cpp \
				   ../../Classes/SDLSurface.cpp \
				   ../../Classes/SDLThread.cpp \
				   ../../Classes/SDLVideo.cpp \
				   ../../Classes/SDLWindow.cpp \
				   ../../Classes/SDLAudioTypeCVT.cpp \
				   ../../Classes/SDLAudioCVT.cpp \
				   ../../Classes/SDL.cpp \
				   ../../Classes/SDL_androidaudio.cpp \
				   SDLAudioJNI.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../Classes \
					$(LOCAL_PATH)/. \
					$(LOCAL_PATH)/../../../cocos2d-x/cocos/platform/android/jni

LOCAL_WHOLE_STATIC_LIBRARIES := cocos2dx_static
LOCAL_WHOLE_STATIC_LIBRARIES += cocosdenshion_static
LOCAL_WHOLE_STATIC_LIBRARIES += ffmpeg_static
LOCAL_WHOLE_STATIC_LIBRARIES += cocos_ui_static

include $(BUILD_SHARED_LIBRARY)

$(call import-module,.)
$(call import-module,audio/android)
$(call import-module,ffmpeg/prebuilt/android)
$(call import-module,cocos/ui)
