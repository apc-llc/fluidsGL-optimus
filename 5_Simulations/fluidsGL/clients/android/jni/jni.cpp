#include <cstdio>
#include <jni.h>
#include <fluidGL_client.h>

#ifndef _Included_com_example_caffemacchiato_GameLibJNIWrapper
#define _Included_com_example_caffemacchiato_GameLibJNIWrapper
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_example_caffemacchiato_GameLibJNIWrapper
 * Method:    on_surface_created
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_caffemacchiato_GameLibJNIWrapper_on_1surface_1created
  (JNIEnv * asmd, jclass  asd1){
	on_surface_created();
}

/*
 * Class:     com_example_caffemacchiato_GameLibJNIWrapper
 * Method:    on_surface_changed
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_example_caffemacchiato_GameLibJNIWrapper_on_1surface_1changed
  (JNIEnv * par, jclass par1, jint width, jint height){
	on_surface_changed(width, height);
}

/*
 * Class:     com_example_caffemacchiato_GameLibJNIWrapper
 * Method:    on_draw_frame
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_caffemacchiato_GameLibJNIWrapper_on_1draw_1frame
  (JNIEnv * par1, jclass par2){
	on_draw_frame();
}

/*
 * Class:     com_example_caffemacchiato_GameLibJNIWrapper
 * Method:    on_connect
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_caffemacchiato_GameLibJNIWrapper_on_1connect
  (JNIEnv * env, jclass par2, jstring bc_addr){
	on_connect(env->GetStringUTFChars(bc_addr, NULL));
}

/*
 * Class:     com_example_caffemacchiato_GameLibJNIWrapper
 * Method:    on_reset
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_caffemacchiato_GameLibJNIWrapper_on_1reset
  (JNIEnv * env, jclass par2){
	on_reset();
}

/*
 * Class:     com_example_caffemacchiato_GameLibJNIWrapper
 * Method:    on_surface_changed
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_example_caffemacchiato_GameLibJNIWrapper_on_1click
  (JNIEnv * par, jclass par1, jboolean clicked, jint x, jint y){
	on_click(clicked, x, y);
}

/*
 * Class:     com_example_caffemacchiato_GameLibJNIWrapper
 * Method:    on_surface_changed
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_example_caffemacchiato_GameLibJNIWrapper_on_1motion
  (JNIEnv * par, jclass par1, jint x, jint y){
	on_motion(x, y);
}

#ifdef __cplusplus
}
#endif
#endif
