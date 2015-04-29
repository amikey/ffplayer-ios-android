#APP_STL := c++_static

APP_STL := gnustl_static

APP_CPPFLAGS := -frtti -DCC_ENABLE_CHIPMUNK_INTEGRATION=1 -DCOCOS2D_DEBUG=1 -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -D__STDC_FORMAT_MACROS -std=c++11

#APP_LDFLAGS := -latomic
APP_CPPFLAGS += -fexceptions 

#APP_ABI := armeabi-v7a
APP_ABI := armeabi
