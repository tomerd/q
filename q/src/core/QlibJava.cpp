#ifdef JNI_HEADER

//
//  QlibJava.cpp
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//


#include "QlibJava.h"

#include "QFactory.h"
#include "Logger.h"

#include <list>

std::list<jobject> workers;
std::list<jobject> observers;

void check_for_java_error(JNIEnv* env, Q::JobError** error);

JNIEXPORT JNICALL
jstring Java_com_mishlabs_q_Q_native_1version(JNIEnv* env, jobject obj)
{
    return env->NewStringUTF("0.0.1");
}

JNIEXPORT JNICALL
jlong Java_com_mishlabs_q_Q_native_1connect(JNIEnv* env, jobject obj, jstring jconfiguration)
{
    Q::Q* pq = NULL;
    const char* configuration = jconfiguration != NULL ? env->GetStringUTFChars(jconfiguration, NULL) : NULL;
    bool success = Q::QFactory::createQ(&pq, NULL != configuration ? configuration : "");
    if (success) success = pq->connect();
    env->ReleaseStringUTFChars(jconfiguration, configuration);
    return success ? (long)pq : 0;
}

JNIEXPORT JNICALL
void Java_com_mishlabs_q_Q_native_1disconnect(JNIEnv* env, jobject obj, jlong qp)
{
    if (0 == qp) return;
    
    ((Q::Q*)qp)->disconnect();
    
    for (std::list<jobject>::iterator it = workers.begin(); it != workers.end(); it++)
    {
        env->DeleteGlobalRef(*it);
    }
    
    for (std::list<jobject>::iterator it = observers.begin(); it != observers.end(); it++)
    {
        env->DeleteGlobalRef(*it);
    }
}

JNIEXPORT JNICALL
jstring Java_com_mishlabs_q_Q_native_1post(JNIEnv* env, jobject obj, jlong qp, jstring jqueue, jstring juid, jstring jdata, jlong jrun_at)
{
    if (0 == qp) return NULL;
    if (NULL == jqueue) return NULL;
    if (NULL == jdata) return NULL;
    
    const char* queue = env->GetStringUTFChars(jqueue, NULL);
    const char* uid = NULL != juid ? env->GetStringUTFChars(juid, NULL) : NULL;
    const char* data = env->GetStringUTFChars(jdata, NULL);
    long run_at = jrun_at;
    string new_uid = ((Q::Q*)qp)->post(queue, uid ? uid : "", data, run_at);
    env->ReleaseStringUTFChars(jqueue, queue);
    if (NULL != juid) env->ReleaseStringUTFChars(juid, uid);
    env->ReleaseStringUTFChars(jdata, data);
    return env->NewStringUTF(new_uid.c_str());
}

JNIEXPORT JNICALL
jboolean Java_com_mishlabs_q_Q_native_1reschedule(JNIEnv* env, jobject obj, jlong qp, jstring juid, jlong jrun_at)
{
    if (0 == qp) return false;
    if (NULL == juid) return false;
    
    const char* uid = env->GetStringUTFChars(juid, NULL);
    long run_at = jrun_at;
    bool result = ((Q::Q*)qp)->reschedule(uid, run_at);
    env->ReleaseStringUTFChars(juid, uid);
    return result;
}

JNIEXPORT jboolean JNICALL Java_com_mishlabs_q_Q_native_1cancel(JNIEnv* env, jobject obj, jlong qp, jstring juid)
{
    if (0 == qp) return false;
    if (NULL == juid) return false;
    
    const char* uid = env->GetStringUTFChars(juid, NULL);
    bool result = ((Q::Q*)qp)->cancel(uid);
    env->ReleaseStringUTFChars(juid, uid);
    return result;
}

// TODO: optimize this
JNIEXPORT JNICALL
void Java_com_mishlabs_q_Q_native_1worker(JNIEnv* env, jobject obj, jlong qp, jstring jqueue, jobject jdelegate)
{
    if (0 == qp) return;    
    if (NULL == jqueue) return;
    if (NULL == jdelegate) return;
    
    jobject delegate = env->NewGlobalRef(jdelegate);
    workers.push_back(delegate);
    
    jclass klass = env->GetObjectClass(delegate);
    jmethodID method = env->GetMethodID(klass, "perform", "(Ljava/lang/String;)V");
    if (method == 0)
    {
        env->ThrowNew(env->FindClass("java/lang/Exception"), "could not find method 'perform', make sure the delegate is of correct type");
        return;
    }
    
    JavaVM* jvm = NULL;
    env->GetJavaVM(&jvm);
    
    const char* queue = env->GetStringUTFChars(jqueue, NULL);
    // TODO: manage list of workers, return some id to client and allow to remove workers based on id
    ((Q::Q*)qp)->worker(queue, new Q::WorkerDelegate([=](const Q::Job* job, Q::JobError** error)
    //((Q*)qp)->worker(queue, WorkerDelegate(new std::function<void (const Job*, JobError**)>([=](const Job* job, JobError** error)
    {
        jvm->AttachCurrentThread((void**)&env, NULL);
        env->CallVoidMethod(delegate, method, env->NewStringUTF(job->data().c_str()));
        check_for_java_error(env, error);
        jvm->DetachCurrentThread();
    }));
    env->ReleaseStringUTFChars(jqueue, queue);
}

// TODO: optimize this
JNIEXPORT JNICALL
void Java_com_mishlabs_q_Q_native_1observer(JNIEnv* env, jobject obj, jlong qp, jstring jqueue, jobject jdelegate)
{
    if (0 == qp) return;
    if (NULL == jqueue) return;
    if (NULL == jdelegate) return;
    
    jobject delegate = env->NewGlobalRef(jdelegate);
    observers.push_back(delegate);
    
    jclass klass = env->GetObjectClass(delegate);
    jmethodID method = env->GetMethodID(klass, "perform", "(Ljava/lang/String;)V");
    if (method == 0)
    {
        env->ThrowNew(env->FindClass("java/lang/Exception"), "could not find method 'perform', make sure the delegate is of correct type");
        return;
    }
    
    JavaVM* jvm = NULL;
    env->GetJavaVM(&jvm);
        
    const char* queue = env->GetStringUTFChars(jqueue, NULL);
    // TODO: manage list of observers, return some id to client and allow to remove observers based on id
    ((Q::Q*)qp)->observer(queue, new Q::ObserverDelegate([=](const Q::Job* job, Q::JobError** error)
    //((Q*)qp)->observer(queue, ObserverDelegate(new std::function<void (const Job*, JobError**)>([=](const Job* job, JobError** error)
    {
        jvm->AttachCurrentThread((void**)&env, NULL);        
        env->CallVoidMethod(delegate, method, env->NewStringUTF(job->data().c_str()));
        check_for_java_error(env, error);
        jvm->DetachCurrentThread();
    }));
    env->ReleaseStringUTFChars(jqueue, queue);
}

JNIEXPORT JNICALL
void Java_com_mishlabs_q_Q_native_1flush(JNIEnv* env, jobject obj, jlong qp)
{
    if (0 == qp) return;
    ((Q::Q*)qp)->flush();
}

void check_for_java_error(JNIEnv* env, Q::JobError** error)
{
    if (JNI_TRUE != env->ExceptionCheck()) return;
    jthrowable exception = env->ExceptionOccurred();
    if (NULL == exception) return;
    jclass klass = env->GetObjectClass(exception);
    jmethodID method = env->GetMethodID(klass, "toString", "()Ljava/lang/String;");
    jstring jerror = (jstring)env->CallObjectMethod(exception, method);
    const char* desccription = env->GetStringUTFChars(jerror, NULL);
    *error = new Q::JobError(desccription);
    env->ReleaseStringUTFChars(jerror, desccription);
    env->ExceptionClear();
}

#endif
