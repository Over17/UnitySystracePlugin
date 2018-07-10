#include <cstring>
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

static bool s_isCapturing = false;
static void* s_libandroid;
const UnityProfilerMarkerDesc* s_DefaultMarkerDesc = NULL;

static void UNITY_INTERFACE_API SystraceEventCallback(const UnityProfilerMarkerDesc* eventDesc, UnityProfilerMarkerEventType eventType, unsigned short eventDataCount, const UnityProfilerMarkerData* eventData, void* userData)
{
    if (!ATrace_isEnabled())
        return;

    switch (eventType)
    {
        case kUnityProfilerMarkerEventTypeBegin:
        {
            if (eventDataCount > 1 && eventDesc == s_DefaultMarkerDesc)
            {
                // Profiler.Default marker emits UTF16 string as the second metadata parameter.
                // For simplicity we slice UTF16 data to char.
                uint32_t size = eventData[1].size;
                const uint16_t* first = static_cast<const uint16_t*>(eventData[1].ptr);
                const uint16_t* last = reinterpret_cast<const uint16_t*>(static_cast<const uint8_t*>(eventData[1].ptr) + size);
                const int length = size / 2 + 1;
                char newName[length];

                for (int i = 0; first < last && i < length; ++first)
                {
                    newName[i++] = static_cast<char>(*first);
                }
                newName[length - 1] = 0;
                ATrace_beginSection(newName);
            }
            else
            {
                ATrace_beginSection(eventDesc->name);
            }

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
    // Special handling for Profiler.Default markers
    if (!s_DefaultMarkerDesc)
    {
        if (!strcmp(eventDesc->name, "Profiler.Default"))
            s_DefaultMarkerDesc = eventDesc;
    }

    s_UnityProfilerCallbacks->RegisterMarkerEventCallback(eventDesc, SystraceEventCallback, NULL);
}

static void UNITY_INTERFACE_API SystraceFrameCallback(void* userData)
{
    bool isCapturing = ATrace_isEnabled();
    if (isCapturing != s_isCapturing)
    {
        s_isCapturing = isCapturing;
        if (isCapturing)
        {

        }
        else
        {

        }
    }
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
