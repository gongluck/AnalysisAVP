#include <jni.h>
#include <string>

#include <android/log.h>
#include <android/native_window_jni.h>

#include <thread>
#include "clibrary.h"

#define JNI_CLASS_PATH  "com/gongluck/jni/MainActivity"

static JavaVM* g_vm = nullptr;
static jclass g_sclazz = nullptr;

extern "C" JNIEXPORT jstring JNICALL
gongluck_register(JNIEnv *env, jobject thiz, jstring str){
    //return env->NewStringUTF("gongluck");

    //return str;

    //C中调用Java方法
    jclass clazz = env->FindClass("com/gongluck/jni/JClass");
    jmethodID constructor = env->GetMethodID(clazz, "<init>", "()V");//构造方法(<init>)
    jobject obj = env->NewObject(clazz, constructor);//创建对象
    jmethodID setvalue = env->GetMethodID(clazz, "setValue", "(I)V");
    env->CallVoidMethod(obj, setvalue, 100);

    //C中修改Java对象的字段
    jfieldID vfield = env->GetFieldID(clazz, "value", "I");
    env->SetIntField(obj, vfield, 99);

    jmethodID getvalue = env->GetMethodID(clazz, "getValue", "()I");
    int value = env->CallIntMethod(obj, getvalue);

    if(g_sclazz == nullptr)
    {
        jclass sclazz = env->FindClass("com/gongluck/jni/JSClass");
        g_sclazz = (jclass)env->NewGlobalRef(sclazz);//创建全局引用 可以跨线程共享
    }
    if(g_sclazz != nullptr)
    {
        //C中调用静态Java方法
        jmethodID ssetvalue = env->GetStaticMethodID(g_sclazz, "setValue", "(I)V");
        env->CallStaticVoidMethod(g_sclazz, ssetvalue, 100);

        jfieldID svfield = env->GetStaticFieldID(g_sclazz, "value", "I");
        env->SetStaticIntField(g_sclazz, svfield, add(123,321));

        jmethodID sgetvalue = env->GetStaticMethodID(g_sclazz, "getValue", "()I");
        value = env->CallStaticIntMethod(g_sclazz, sgetvalue);
    }

    auto strval = std::to_string(value);
    return env->NewStringUTF(strval.c_str());
}

static JNINativeMethod g_methods[] = {
        "stringFromJNIGONGLUCK",
        "(Ljava/lang/String;)Ljava/lang/String;",
        (void*)gongluck_register
};

jint JNI_OnLoad(JavaVM *vm, void *reserved){
    JNIEnv *env = nullptr;
    vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    jclass clazz = env->FindClass(JNI_CLASS_PATH);
    //注册关联本地方法
    env->RegisterNatives(clazz, g_methods, sizeof(g_methods)/sizeof(g_methods[0]));

    g_vm = vm;

    return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM* vm, void* reserved){
    JNIEnv *env = nullptr;
    vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if(g_sclazz != nullptr){
        env->DeleteGlobalRef(g_sclazz);
        g_sclazz = nullptr;
    }
}

//C接口实现
extern "C" JNIEXPORT jstring JNICALL//函数头声明
Java_com_gongluck_jni_MainActivity_stringFromJNI(//函数名：Java_包名_类名_方法名
        JNIEnv *env, jobject /* this */) {//这2个参数实际就是Java虚拟机在调用c/c++接口的时候传进来的。
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT void JNICALL//函数头声明
Java_com_gongluck_jni_MainActivity_nativeWindowView(//函数名：Java_包名_类名_方法名
        JNIEnv *env, jobject /* this */,
        jobject surface) {
    //使用nativewindow渲染数据
    ANativeWindow* win = ANativeWindow_fromSurface(env, surface);
    auto anret = ANativeWindow_setBuffersGeometry(win, 600, 400, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer outbuf;
    anret = ANativeWindow_lock(win, &outbuf, nullptr);
    uint8_t* dst = (uint8_t*)outbuf.bits;
    __android_log_print(ANDROID_LOG_INFO, "GONGLUCK", "%d(%d)x%d\n", outbuf.width, outbuf.stride, outbuf.height);
    for (int i = 0; i < outbuf.height; ++i)
    {
        //目标纹理大小不一定和设定的600*400一样大 要更具返回的outbuf.stride填充数据
        //类似ffmpeg的AVFrame的width和linesize 窗口可视区域是width*height 但是每行数据可能会有填充部分
        //这里可以跳过填充部分(outbuf.stride*i*4) 但是数据拷贝还是要按照width(outbuf.width)拷贝到可视区域
        for(int j=0;j<outbuf.stride;++j)
        {
            dst[outbuf.stride*i*4+j*4] = 10*255.0*j/outbuf.width;//R 这里重复10次 可以观察数据是否正确渲染(10次完整的变化)
            dst[outbuf.stride*i*4+j*4+1] = 0;//G
            dst[outbuf.stride*i*4+j*4+2] = 0;//B
            dst[outbuf.stride*i*4+j*4+3] = 0;//A
        }
    }
    anret = ANativeWindow_unlockAndPost(win);
}

static jobject g_jclass = nullptr;
static jobject g_obj = nullptr;
static std::thread g_th;

void th()
{
    JNIEnv *env = nullptr;
    g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);

    auto method = env->GetMethodID((jclass)g_jclass, "setValue", "(I)V");
    env->CallVoidMethod(g_obj, method, 10);
}

extern "C" JNIEXPORT void JNICALL//函数头声明
Java_com_gongluck_jni_MainActivity_nativeSaveJObject(//函数名：Java_包名_类名_方法名
        JNIEnv *env, jobject /* this */,
        jobject obj) {
    jclass clazz = env->FindClass("com/gongluck/jni/JClass");
    g_jclass = env->NewGlobalRef(clazz);
    g_obj = env->NewGlobalRef(obj);

    std::thread th([=]{
        std::this_thread::sleep_for(std::chrono::seconds(5));
        JNIEnv *env = nullptr;
        int status;
        status = g_vm->GetEnv((void **)&env, JNI_VERSION_1_6);
        if(status < 0)
        {
            status = g_vm->AttachCurrentThread(&env, nullptr);
            if(status < 0)
            {
                env = nullptr;
            }
        }

        auto method = env->GetMethodID((jclass)g_jclass, "setValue", "(I)V");
        env->CallVoidMethod(g_obj, method, 10);
        g_vm->DetachCurrentThread();
    });
    g_th.swap(th);
}