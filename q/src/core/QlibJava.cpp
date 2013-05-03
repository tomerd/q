//
//  QlibJava.cpp
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "QlibJava.h"

#include "QFactory.h"

#include <list>

std::list<jobject> workers;
std::list<jobject> observers;

void check_for_java_error(JNIEnv* env, JobError** error);

JNIEXPORT JNICALL
jstring Java_com_mishlabs_q_Q_native_1version(JNIEnv* env, jobject obj)
{
    return env->NewStringUTF("0.0.1");
}

JNIEXPORT JNICALL
jlong Java_com_mishlabs_q_Q_native_1connect(JNIEnv* env, jobject obj, jstring jconfiguration)
{
    Q* pq = NULL;
    const char* configuration = jconfiguration != NULL ? env->GetStringUTFChars(jconfiguration, NULL) : NULL;
    env->ReleaseStringUTFChars(jconfiguration, configuration);
    if (!QFactory::createQ(&pq, NULL != configuration ? configuration : "")) return 0;
    if (!pq->connect()) return 0;
    return (long)pq;
}

JNIEXPORT JNICALL
void Java_com_mishlabs_q_Q_native_1disconnect(JNIEnv* env, jobject obj, jlong qp)
{
    if (0 == qp) return;
    
    ((Q*)qp)->disconnect();
    
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
jstring Java_com_mishlabs_q_Q_native_1post(JNIEnv* env, jobject obj, jlong qp, jstring jqueue, jstring jdata, jlong jat)
{
    if (0 == qp) return NULL;
    if (NULL == jqueue) return NULL;
    if (NULL == jdata) return NULL;
    
    const char* queue = env->GetStringUTFChars(jqueue, NULL);
    const char* data = env->GetStringUTFChars(jdata, NULL);
    long at = jat;
    Job* job = ((Q*)qp)->post(queue, data, at);
    env->ReleaseStringUTFChars(jqueue, queue);
    env->ReleaseStringUTFChars(jdata, data);
    return env->NewStringUTF(job->uid().c_str());
}

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
    
    const char* queue = env->GetStringUTFChars(jqueue, NULL);
    //((Q*)qp)->worker(queue, [=](Job* job, JobError** error)
    ((Q*)qp)->worker(queue, std::shared_ptr<std::function<void (Job*, JobError** error)>>(new std::function<void (Job*, JobError**)>([=](Job* job, JobError** error)
    {
        JavaVM* jvm = NULL;
        env->GetJavaVM(&jvm);
        jvm->AttachCurrentThread((void**)&env, NULL);
        env->CallVoidMethod(delegate, method, env->NewStringUTF(job->data().c_str()));
        check_for_java_error(env, error);
        jvm->DetachCurrentThread();
    })));
    env->ReleaseStringUTFChars(jqueue, queue);
}

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
        
    const char* queue = env->GetStringUTFChars(jqueue, NULL);    
    //((Q*)qp)->observer(queue, [=](Job* job, JobError** error)
    ((Q*)qp)->observer(queue, std::shared_ptr<std::function<void (Job*, JobError**)>>(new std::function<void (Job*, JobError**)>([=](Job* job, JobError** error)
    {
        JavaVM* jvm = NULL;
        env->GetJavaVM(&jvm);
        jvm->AttachCurrentThread((void**)&env, NULL);        
        env->CallVoidMethod(delegate, method, env->NewStringUTF(job->data().c_str()));
        check_for_java_error(env, error);
        jvm->DetachCurrentThread();
    })));
    env->ReleaseStringUTFChars(jqueue, queue);
}

void check_for_java_error(JNIEnv* env, JobError** error)
{
    if (JNI_TRUE != env->ExceptionCheck()) return;
    jthrowable exception = env->ExceptionOccurred();
    if (NULL == exception) return;
    jclass klass = env->GetObjectClass(exception);
    jmethodID method = env->GetMethodID(klass, "toString", "()Ljava/lang/String;");
    jstring jerror = (jstring)env->CallObjectMethod(exception, method);
    const char* desccription = env->GetStringUTFChars(jerror, NULL);
    *error = new JobError(desccription);
    env->ReleaseStringUTFChars(jerror, desccription);
    env->ExceptionClear();
}
