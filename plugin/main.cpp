#include <reshade.hpp>
#include <reshade_api.hpp>
#include <algorithm>
#include <windows.h>

extern "C" __declspec(dllexport) const char *NAME = "ReShade Zoom Plugin";
extern "C" __declspec(dllexport) const char *DESCRIPTION = "A plugin for handling screen zoom.";

static void on_reshade_present(reshade::api::effect_runtime *runtime)
{
    auto enable_var = runtime->find_uniform_variable("zoomscope.fx", "EnableMagnifier");
    auto zoom_var   = runtime->find_uniform_variable("zoomscope.fx", "DynamicZoomLevel");
    auto step_var   = runtime->find_uniform_variable("zoomscope.fx", "ZoomLevelDelta");
    auto zin_var    = runtime->find_uniform_variable("zoomscope.fx", "ZoomInKey");
    auto zout_var   = runtime->find_uniform_variable("zoomscope.fx", "ZoomOutKey");

    if (enable_var == 0 || zoom_var == 0 || step_var == 0 || zin_var == 0 || zout_var == 0)
        return;

    bool enabled = false;
    runtime->get_uniform_value_bool(enable_var, &enabled, 1);

    if (!enabled)
    {
        float reset = 1.0f;
        runtime->set_uniform_value_float(zoom_var, &reset, 1);
        return;
    }

    float zoom = 1.0f;
    float step = 0.25f;
    int zoom_in_key = VK_F11;
    int zoom_out_key = VK_F12;

    runtime->get_uniform_value_float(zoom_var, &zoom, 1);
    runtime->get_uniform_value_float(step_var, &step, 1);
    runtime->get_uniform_value_int(zin_var, &zoom_in_key, 1);
    runtime->get_uniform_value_int(zout_var, &zoom_out_key, 1);

    if (zoom_in_key >= 0 && zoom_in_key <= 255 && runtime->is_key_pressed(static_cast<uint32_t>(zoom_in_key)))
        zoom += step;

    if (zoom_out_key >= 0 && zoom_out_key <= 255 && runtime->is_key_pressed(static_cast<uint32_t>(zoom_out_key)))
        zoom -= step;

    zoom = std::clamp(zoom, 1.0f, 10.0f);
    runtime->set_uniform_value_float(zoom_var, &zoom, 1);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        reshade::register_addon(hModule);
        reshade::register_event<reshade::addon_event::reshade_present>(on_reshade_present);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        reshade::unregister_event<reshade::addon_event::reshade_present>(on_reshade_present);
        reshade::unregister_addon(hModule);
    }

    return TRUE;
}
