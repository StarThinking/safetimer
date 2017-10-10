/* ctest.c */

#include <jni.h>
#include <stdio.h>

#include "HelloWorld.h"

    JNIEXPORT void JNICALL Java_DataNode_helloFromC(JNIEnv * env, jobject jobj)
{
    printf("Hello from C!\n");
}
