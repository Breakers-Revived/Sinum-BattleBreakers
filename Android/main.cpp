// Copyright (c) 2024 Project Nova LLC

#include "Url.h"
#include "Library.h"
#include "Util.h"
#include "Dobby/dobby.h"
#include "Includes/curl.h"
#include <dlfcn.h>


//
//   DYNAMIC PART
//
// Global variables to store the Java fields.
std::string g_protocol = _("http");
std::string g_host = _("127.0.0.1");
std::string g_port = std::string();

extern "C" 
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    JNIEnv *env;
    
    // Attach JNI environment
    if (jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    // Find the Java class io/sinum/Sinum
    jclass sinumClass = env->FindClass("io/sinum/Sinum");
    if (sinumClass == nullptr) {
        env->ExceptionClear();
        return JNI_ERR;
    }

    // Get the field IDs for protocol, host, and port
    jfieldID protocolField = env->GetStaticFieldID(sinumClass, "PROTOCOL", "Ljava/lang/String;");
    jfieldID hostField = env->GetStaticFieldID(sinumClass, "HOST", "Ljava/lang/String;");
    jfieldID portField = env->GetStaticFieldID(sinumClass, "PORT", "Ljava/lang/String;");
    
    if (protocolField == nullptr || hostField == nullptr || portField == nullptr) {
        env->DeleteLocalRef(sinumClass);
        return JNI_ERR;
    }

    // Retrieve and store the protocol field value
    jstring jProtocol = (jstring)env->GetStaticObjectField(sinumClass, protocolField);
    const char *protocolCStr = env->GetStringUTFChars(jProtocol, nullptr);
    g_protocol = protocolCStr;
    env->ReleaseStringUTFChars(jProtocol, protocolCStr);
    env->DeleteLocalRef(jProtocol);

    // Retrieve and store the host field value
    jstring jHost = (jstring)env->GetStaticObjectField(sinumClass, hostField);
    const char *hostCStr = env->GetStringUTFChars(jHost, nullptr);
    g_host = hostCStr;
    env->ReleaseStringUTFChars(jHost, hostCStr);
    env->DeleteLocalRef(jHost);

    // Retrieve and store the port field value
    jstring jPort = (jstring)env->GetStaticObjectField(sinumClass, portField);
    const char *portCStr = env->GetStringUTFChars(jPort, nullptr);
    g_port = portCStr;
    env->ReleaseStringUTFChars(jPort, portCStr);
    env->DeleteLocalRef(jPort);

    // Clean up
    env->DeleteLocalRef(sinumClass);

    // Return the JNI version we want to use
    return JNI_VERSION_1_6;
}


//
//  Actual Sinum hook 
//
#define URL_PROTOCOL_HTTP g_protocol
#define URL_HOST g_host
#define URL_PORT g_port

install_hook_name(curl_easy_setopt, void*, void* curl, int option, void* arg)
{
    // LOGI(_("curl_easy_setopt: %i"), option);

    if (!Util::IsPointerBad(arg) && option == CURLOPT_URL)
    {
        std::string url = reinterpret_cast<char*>(arg);

        Uri uri = Uri::Parse(url);
        url = Uri::CreateUri(URL_PROTOCOL_HTTP, URL_HOST, URL_PORT, uri.Path, uri.QueryString);

        return orig_curl_easy_setopt(curl, option, (void*)url.c_str());
    }
    else if (option == CURLOPT_SSL_VERIFYPEER)
    {
        return orig_curl_easy_setopt(curl, option, (void*)0);
    }

    return orig_curl_easy_setopt(curl, option, arg);
}

void* Main(void*)
{
    auto Base = dlopen(_("libUE4.so"), RTLD_NOW);
    
    auto curl_easy_setopt = dlsym(Base, _("curl_easy_setopt"));
    
    if (!curl_easy_setopt) return nullptr;
    
    install_hook_curl_easy_setopt(curl_easy_setopt);

    return nullptr;
}

__attribute__((constructor)) void libsinum_main()
{
    pthread_t ptid;
    pthread_create(&ptid, NULL, Main, NULL);
}
