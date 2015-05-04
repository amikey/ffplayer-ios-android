#include "SDLAudioJNI.h"
#include <jni.h>
#include "JniHelper.h"

using namespace cocos2d;

namespace ff{
	
	#define  CLASS_NAME "org/cocos2dx/cpp_empty_test/AppActivity"
	static JavaVM* mJavaVM;

	/* Main activity */
	static jclass mActivityClass;
	
	/* method signatures */
	static jmethodID midAudioInit;
	static jmethodID midAudioWriteShortBuffer;
	static jmethodID midAudioWriteByteBuffer;
	static jmethodID midAudioQuit;
	static bool bHasNewData;
	
	static jboolean audioBuffer16Bit = JNI_FALSE;
	static jboolean audioBufferStereo = JNI_FALSE;
	static jobject audioBuffer = NULL;
	static void* audioBufferPinned = NULL;

	int Android_JNI_OpenAudioDevice(int sampleRate, int is16Bit, int channelCount, int desiredBufferFrames)
	{
		
		int audioBufferFrames;
		JniMethodInfo t;
		
		__android_log_print(ANDROID_LOG_VERBOSE, "SDL", "SDL audio: opening device");
		
		audioBuffer16Bit = is16Bit;
		audioBufferStereo = channelCount > 1;
		if (JniHelper::getStaticMethodInfo(t, CLASS_NAME, "audioInit", "(IZZI)I"))
		{
			if(t.env->CallStaticIntMethod(t.classID,t.methodID,sampleRate, audioBuffer16Bit, audioBufferStereo, desiredBufferFrames)!=0)
			{
				__android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: error on AudioTrack initialization!");
				t.env->DeleteLocalRef(t.classID);
				return 0;
			}
			t.env->DeleteLocalRef(t.classID);
		}
		if (is16Bit) {
			jshortArray audioBufferLocal = t.env->NewShortArray(desiredBufferFrames * (audioBufferStereo ? 2 : 1));
			if (audioBufferLocal) {
				audioBuffer = t.env->NewGlobalRef(audioBufferLocal);
				t.env->DeleteLocalRef(audioBufferLocal);
			}
		}
		else {
			jbyteArray audioBufferLocal = (t.env)->NewByteArray(desiredBufferFrames * (audioBufferStereo ? 2 : 1));
			if (audioBufferLocal) {
				audioBuffer = (t.env)->NewGlobalRef(audioBufferLocal);
				t.env->DeleteLocalRef(audioBufferLocal);
			}
		}		
		if (audioBuffer == NULL) {
			__android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: could not allocate an audio buffer!");
			return 0;
		}		
		jboolean isCopy = JNI_FALSE;
		if (audioBuffer16Bit) {
			audioBufferPinned = t.env->GetShortArrayElements((jshortArray)audioBuffer, &isCopy);
			audioBufferFrames = t.env->GetArrayLength((jshortArray)audioBuffer);
		} else {
			audioBufferPinned = t.env->GetByteArrayElements((jbyteArray)audioBuffer, &isCopy);
			audioBufferFrames = t.env->GetArrayLength((jbyteArray)audioBuffer);
		}
		if (audioBufferStereo) {
			audioBufferFrames /= 2;
		}		
		__android_log_print(ANDROID_LOG_WARN, "SDL", "OpenAudioDevice done !");
		/*
		JNIEnv *env = Android_JNI_GetEnv();

		if (!env) {
			LOGE("callback_handler: failed to attach current thread");
		}
		Android_JNI_SetupThread();

		__android_log_print(ANDROID_LOG_VERBOSE, "SDL", "SDL audio: opening device");
		audioBuffer16Bit = is16Bit;
		audioBufferStereo = channelCount > 1;

		if ((*env)->CallStaticIntMethod(env, mActivityClass, midAudioInit, sampleRate, audioBuffer16Bit, audioBufferStereo, desiredBufferFrames) != 0) {
			__android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: error on AudioTrack initialization!");
			return 0;
		}
		*/
		/* Allocating the audio buffer from the Java side and passing it as the return value for audioInit no longer works on
		 * Android >= 4.2 due to a "stale global reference" error. So now we allocate this buffer directly from this side. */
		/*
		if (is16Bit) {
			jshortArray audioBufferLocal = (*env)->NewShortArray(env, desiredBufferFrames * (audioBufferStereo ? 2 : 1));
			if (audioBufferLocal) {
				audioBuffer = (*env)->NewGlobalRef(env, audioBufferLocal);
				(*env)->DeleteLocalRef(env, audioBufferLocal);
			}
		}
		else {
			jbyteArray audioBufferLocal = (*env)->NewByteArray(env, desiredBufferFrames * (audioBufferStereo ? 2 : 1));
			if (audioBufferLocal) {
				audioBuffer = (*env)->NewGlobalRef(env, audioBufferLocal);
				(*env)->DeleteLocalRef(env, audioBufferLocal);
			}
		}

		if (audioBuffer == NULL) {
			__android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: could not allocate an audio buffer!");
			return 0;
		}

		jboolean isCopy = JNI_FALSE;
		if (audioBuffer16Bit) {
			audioBufferPinned = (*env)->GetShortArrayElements(env, (jshortArray)audioBuffer, &isCopy);
			audioBufferFrames = (*env)->GetArrayLength(env, (jshortArray)audioBuffer);
		} else {
			audioBufferPinned = (*env)->GetByteArrayElements(env, (jbyteArray)audioBuffer, &isCopy);
			audioBufferFrames = (*env)->GetArrayLength(env, (jbyteArray)audioBuffer);
		}
		if (audioBufferStereo) {
			audioBufferFrames /= 2;
		}
		*/
		return audioBufferFrames;
	}

	void * Android_JNI_GetAudioBuffer()
	{
		__android_log_print(ANDROID_LOG_WARN, "SDL", "Android_JNI_GetAudioBuffer");
		return audioBufferPinned;
	}

	void Android_JNI_WriteAudioBuffer()
	{
		JniMethodInfo t;
		int nRet=0;
		__android_log_print(ANDROID_LOG_WARN, "SDL", "Android_JNI_WriteAudioBuffer");
		if(audioBuffer16Bit){
			(t.env)->ReleaseShortArrayElements((jshortArray)audioBuffer, (jshort *)audioBufferPinned, JNI_COMMIT);
			if (JniHelper::getStaticMethodInfo(t,CLASS_NAME,"audioWriteShortBuffer","([S)V") )
			{
				t.env->CallStaticVoidMethod(t.classID,t.methodID);
				t.env->DeleteLocalRef(t.classID);	
			}		
		}
		else
		{
			(t.env)->ReleaseByteArrayElements((jbyteArray)audioBuffer, (jbyte *)audioBufferPinned, JNI_COMMIT);
			if (JniHelper::getStaticMethodInfo(t,CLASS_NAME,"audioWriteByteBuffer","([B)V"))
			{
				t.env->CallStaticVoidMethod(t.classID,t.methodID);
				t.env->DeleteLocalRef(t.classID);	
			}				
		}	
		__android_log_print(ANDROID_LOG_WARN, "SDL", "Android_JNI_WriteAudioBuffer done");
		/*
		JNIEnv *mAudioEnv = Android_JNI_GetEnv();

		if (audioBuffer16Bit) {
			(*mAudioEnv)->ReleaseShortArrayElements(mAudioEnv, (jshortArray)audioBuffer, (jshort *)audioBufferPinned, JNI_COMMIT);
			(*mAudioEnv)->CallStaticVoidMethod(mAudioEnv, mActivityClass, midAudioWriteShortBuffer, (jshortArray)audioBuffer);
		} else {
			(*mAudioEnv)->ReleaseByteArrayElements(mAudioEnv, (jbyteArray)audioBuffer, (jbyte *)audioBufferPinned, JNI_COMMIT);
			(*mAudioEnv)->CallStaticVoidMethod(mAudioEnv, mActivityClass, midAudioWriteByteBuffer, (jbyteArray)audioBuffer);
		}
		*/
		/* JNI_COMMIT means the changes are committed to the VM but the buffer remains pinned */
	}

	void Android_JNI_CloseAudioDevice()
	{
		JniMethodInfo jmi;
		int nRet=0;
		__android_log_print(ANDROID_LOG_WARN, "SDL", "Android_JNI_CloseAudioDevice");
		if (JniHelper::getStaticMethodInfo(jmi,CLASS_NAME,"audioQuit","()V"))
		{
			jmi.env->DeleteLocalRef(jmi.classID);
			audioBuffer = NULL;
			audioBufferPinned = NULL;			
		}		
		__android_log_print(ANDROID_LOG_WARN, "SDL", "Android_JNI_CloseAudioDevice done");
		/*
		JNIEnv *env = Android_JNI_GetEnv();

		(*env)->CallStaticVoidMethod(env, mActivityClass, midAudioQuit);

		if (audioBuffer) {
			(*env)->DeleteGlobalRef(env, audioBuffer);
			audioBuffer = NULL;
			audioBufferPinned = NULL;
		}
		*/
	}
	
}