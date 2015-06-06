#include "JniHelper.h"
#include <jni.h>
#include "base/CCDirector.h"
#include "base/CCEventDispatcher.h"
#include "base/CCEventGyroscope.h"

#define TG3_GRAVITY_EARTH                    (9.80665f)

using namespace cocos2d;

extern "C" {
    JNIEXPORT void JNICALL Java_org_cocos2dx_lib_Cocos2dxGyroscope_onSensorChanged(JNIEnv*  env, jobject thiz, jfloat x, jfloat y, jfloat z, jlong timeStamp) {
        Gyroscope gyro;
        gyro.x = -((double)x / TG3_GRAVITY_EARTH);
        gyro.y = -((double)y / TG3_GRAVITY_EARTH);
        gyro.z = -((double)z / TG3_GRAVITY_EARTH);
        gyro.timestamp = (double)timeStamp;
        
        EventGyroscope event(gyro);
        Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
    }
}
