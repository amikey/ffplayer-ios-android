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
//	static jmethodID midAudioInit;
//	static jmethodID midAudioWriteShortBuffer;
//	static jmethodID midAudioWriteByteBuffer;
//	static jmethodID midAudioQuit;
//	static bool bHasNewData;
	
	static jboolean audioBuffer16Bit = JNI_FALSE;
	static jboolean audioBufferStereo = JNI_FALSE;
	static jobject audioBuffer = nullptr;
	static void* audioBufferPinned = nullptr;

	static JniMethodInfo jim_audioInit;
	static JniMethodInfo jim_audioWriteShortBuffer;
	static JniMethodInfo jim_audioWriteByteBuffer;
	static JniMethodInfo jim_audioQuit;
	
	int Android_JNI_OpenAudioDevice(int sampleRate, int is16Bit, int channelCount, int desiredBufferFrames)
	{
		
		int audioBufferFrames;
		JNIEnv *env = JniHelper::getEnv();
		
		__android_log_print(ANDROID_LOG_VERBOSE, "SDL", "SDL audio: opening device");
	
		if( !(JniHelper::getStaticMethodInfo(jim_audioInit,CLASS_NAME,"audioInit", "(IZZI)I") &&
			JniHelper::getStaticMethodInfo(jim_audioWriteShortBuffer,CLASS_NAME,"audioWriteShortBuffer", "([S)V") &&
			JniHelper::getStaticMethodInfo(jim_audioWriteByteBuffer,CLASS_NAME,"audioWriteByteBuffer", "([B)V") &&
			JniHelper::getStaticMethodInfo(jim_audioQuit,CLASS_NAME,"audioQuit", "()V")))
		{
			__android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: error on JniHelper::getStaticMethodInfo!");
			return 0;
		}
		
		audioBuffer16Bit = is16Bit;
		audioBufferStereo = channelCount > 1;
		if(jim_audioInit.env->CallStaticIntMethod(jim_audioInit.classID,jim_audioInit.methodID,sampleRate, audioBuffer16Bit, audioBufferStereo, desiredBufferFrames)!=0)
		{
			__android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: error on AudioTrack initialization!");
			jim_audioInit.env->DeleteLocalRef(jim_audioInit.classID);
			return 0;
		}
		if (is16Bit) {
			jshortArray audioBufferLocal = env->NewShortArray(desiredBufferFrames * (audioBufferStereo ? 2 : 1));
			if (audioBufferLocal) {
				audioBuffer = env->NewGlobalRef(audioBufferLocal);
				env->DeleteLocalRef(audioBufferLocal);
			}
		}
		else {
			jbyteArray audioBufferLocal = env->NewByteArray(desiredBufferFrames * (audioBufferStereo ? 2 : 1));
			if (audioBufferLocal) {
				audioBuffer = env->NewGlobalRef(audioBufferLocal);
				env->DeleteLocalRef(audioBufferLocal);
			}
		}		
		if (audioBuffer == nullptr) {
			__android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: could not allocate an audio buffer!");
			return 0;
		}		
		jboolean isCopy = JNI_FALSE;
		if (audioBuffer16Bit) {
			audioBufferPinned = env->GetShortArrayElements((jshortArray)audioBuffer, &isCopy);
			audioBufferFrames = env->GetArrayLength((jshortArray)audioBuffer);
		} else {
			audioBufferPinned = env->GetByteArrayElements((jbyteArray)audioBuffer, &isCopy);
			audioBufferFrames = env->GetArrayLength((jbyteArray)audioBuffer);
		}
		if (audioBufferStereo) {
			audioBufferFrames /= 2;
		}		
		if( audioBuffer && audioBufferPinned )
			__android_log_print(ANDROID_LOG_WARN, "SDL", "OpenAudioDevice done !");
		else
			__android_log_print(ANDROID_LOG_WARN, "SDL", "OpenAudioDevice audioBuffer failed !");
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
		return audioBufferPinned;
	}

	void Android_JNI_WriteAudioBuffer()
	{
		JNIEnv *env = JniHelper::getEnv();
		if(audioBuffer16Bit){
			env->ReleaseShortArrayElements((jshortArray)audioBuffer, (jshort *)audioBufferPinned, JNI_COMMIT);
			env->CallStaticVoidMethod(jim_audioWriteShortBuffer.classID,jim_audioWriteShortBuffer.methodID, (jshortArray)audioBuffer);
		}
		else
		{
			env->ReleaseByteArrayElements((jbyteArray)audioBuffer, (jbyte *)audioBufferPinned, JNI_COMMIT);
			env->CallStaticVoidMethod(jim_audioWriteByteBuffer.classID,jim_audioWriteByteBuffer.methodID, (jbyteArray)audioBuffer);
		}	
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
		JNIEnv *env = JniHelper::getEnv();
		
		__android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: 1 ");
		
		env->CallStaticVoidMethod(jim_audioQuit.classID,jim_audioQuit.methodID);
		
		__android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: 2 ");
		
		env->DeleteLocalRef(jim_audioInit.classID);
		env->DeleteLocalRef(jim_audioWriteShortBuffer.classID);
		env->DeleteLocalRef(jim_audioWriteByteBuffer.classID);
		env->DeleteLocalRef(jim_audioQuit.classID);
		
		__android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: 3 ");
		
		if( audioBuffer )
		{
			env->DeleteGlobalRef(audioBuffer);
			audioBuffer = nullptr;
			audioBufferPinned = nullptr;
		}
		__android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: close audio device!");
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