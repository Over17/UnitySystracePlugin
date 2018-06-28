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

static void* libandroid;

static void UNITY_INTERFACE_API SystraceEventCallback(const UnityProfilerMarkerDesc* eventDesc, UnityProfilerMarkerEventType eventType, unsigned short eventDataCount, const UnityProfilerMarkerData* eventData, void* userData)
{
    if (!ATrace_isEnabled || !ATrace_isEnabled())
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

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    // Doing dynamic linking because native trace API is supported only in Android M+
    libandroid = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
    if (libandroid != NULL)
    {
        __android_log_print(ANDROID_LOG_INFO, "SystraceUnity", "Enabling Unity systrace integration plugin");

        ATrace_isEnabled = reinterpret_cast<fp_ATrace_isEnabled >(dlsym(libandroid, "ATrace_isEnabled"));
        ATrace_beginSection = reinterpret_cast<fp_ATrace_beginSection >(dlsym(libandroid, "ATrace_beginSection"));
        ATrace_endSection = reinterpret_cast<fp_ATrace_endSection >(dlsym(libandroid, "ATrace_endSection"));

        s_UnityProfilerCallbacks = unityInterfaces->Get<IUnityProfilerCallbacks>();
        s_UnityProfilerCallbacks->RegisterCreateMarkerCallback(SystraceCreateEventCallback, NULL);
    }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
    if (libandroid != NULL)
    {
        s_UnityProfilerCallbacks->UnregisterCreateMarkerCallback(SystraceCreateEventCallback, NULL);
        s_UnityProfilerCallbacks->UnregisterMarkerEventCallback(NULL, SystraceEventCallback, NULL);
        dlclose(libandroid);
    }
}
