using UnityEngine;
using System.Runtime.InteropServices;

public class SystracePlugin : MonoBehaviour
{
#if UNITY_ANDROID && !UNITY_EDITOR
    [DllImport("systrace_unity")]
    private static extern void InitSystraceUnityPlugin();

    void Start ()
    {
        InitSystraceUnityPlugin();
    }
#endif
}
