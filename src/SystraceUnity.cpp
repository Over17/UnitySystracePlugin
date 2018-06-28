#include <android/trace.h>
#include "IUnityInterface.h"
#include "IUnityProfilerCallbacks.h"

static IUnityProfilerCallbacks* s_UnityProfilerCallbacks = NULL;

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

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    s_UnityProfilerCallbacks = unityInterfaces->Get<IUnityProfilerCallbacks>();
    s_UnityProfilerCallbacks->RegisterCreateMarkerCallback(SystraceCreateEventCallback, NULL);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
    s_UnityProfilerCallbacks->UnregisterCreateMarkerCallback(SystraceCreateEventCallback, NULL);
    s_UnityProfilerCallbacks->UnregisterMarkerEventCallback(NULL, SystraceEventCallback, NULL);
}
