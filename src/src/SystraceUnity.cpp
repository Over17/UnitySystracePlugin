#include <dlfcn.h>
#include <android/log.h>

#include "IUnityInterface.h"
#include "IUnityProfilerCallbacks.h"

static IUnityProfilerCallbacks* s_UnityProfilerCallbacks = NULL;

typedef void (*fp_ATrace_beginSection) (const char* sectionName);
typedef void (*fp_ATrace_endSection) ();
typedef bool (*fp_ATrace_isEnabled) ();

static fp_ATrace_beginSection ATrace_beginSection;
static fp_ATrace_endSection ATrace_endSection;
static fp_ATrace_isEnabled ATrace_isEnabled;

static void* s_libandroid;

static void UNITY_INTERFACE_API SystraceEventCallback(const UnityProfilerMarkerDesc* eventDesc, UnityProfilerMarkerEventType eventType, unsigned short eventDataCount, const UnityProfilerMarkerData* eventData, void* userData)
{
    if (!ATrace_isEnabled())
        return;

    switch (eventType)
    {
        case kUnityProfilerMarkerEventTypeBegin:
        {
            ATrace_beginSection(eventDesc->name);
            break;
        }
        case kUnityProfilerMarkerEventTypeEnd:
        {
            ATrace_endSection();
            break;
        }
    }
}

static void UNITY_INTERFACE_API SystraceCreateEventCallback(const UnityProfilerMarkerDesc* eventDesc, void* userData)
{
    s_UnityProfilerCallbacks->RegisterMarkerEventCallback(eventDesc, SystraceEventCallback, NULL);
}

extern "C" void UNITY_INTERFACE_API InitSystraceUnityPlugin()
{
    //__android_log_print(ANDROID_LOG_DEBUG, "SystraceUnity", "Unity systrace integration plugin init finished");
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    // Doing dynamic linking because native trace API is supported only in Android M+
    s_libandroid = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
    if (!s_libandroid)
    {
        __android_log_print(ANDROID_LOG_WARN, "SystraceUnity", "Unity systrace integration plugin disabled: failed to load libandroid.so");
        return;
    }

    ATrace_isEnabled = reinterpret_cast<fp_ATrace_isEnabled>(dlsym(s_libandroid, "ATrace_isEnabled"));
    ATrace_beginSection = reinterpret_cast<fp_ATrace_beginSection>(dlsym(s_libandroid, "ATrace_beginSection"));
    ATrace_endSection = reinterpret_cast<fp_ATrace_endSection>(dlsym(s_libandroid, "ATrace_endSection"));

    if (ATrace_isEnabled && ATrace_beginSection && ATrace_endSection)
    {
        // Functions loaded correctly
        __android_log_print(ANDROID_LOG_INFO, "SystraceUnity", "Enabling Unity systrace integration plugin");

        s_UnityProfilerCallbacks = unityInterfaces->Get<IUnityProfilerCallbacks>();
        s_UnityProfilerCallbacks->RegisterCreateMarkerCallback(SystraceCreateEventCallback, NULL);
    }
    else
    {
        __android_log_print(ANDROID_LOG_WARN, "SystraceUnity", "Unity systrace integration plugin disabled: failed to load native tracing API");
    }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
    if (s_libandroid != NULL)
    {
        if (ATrace_isEnabled && ATrace_beginSection && ATrace_endSection)
        {
            s_UnityProfilerCallbacks->UnregisterCreateMarkerCallback(SystraceCreateEventCallback, NULL);
            s_UnityProfilerCallbacks->UnregisterMarkerEventCallback(NULL, SystraceEventCallback, NULL);
        }

        dlclose(s_libandroid);
    }
}
