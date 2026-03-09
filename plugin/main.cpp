#include <reshade.hpp>
#include <reshade_api.hpp>
#include <algorithm>

// Windows Header Files:
#include <windows.h>
#include <iostream>
#include <string>
#include <tchar.h>
#include <Psapi.h>
#include <utility>
#include <vector>

extern "C" __declspec(dllexport) const char* NAME = "ReShade Zoom Plugin";
extern "C" __declspec(dllexport) const char* DESCRIPTION = "A plugin for handling zooming in of the screen to simulate variable zoom scopes.";


// from ReShade
static const char *keyboard_keys[256] = {
    "Left Mouse", "Middle Mouse", "Right Mouse", "Cancel", "Forward", "X1 Mouse", "X2 Mouse", "", "Backspace", "Tab", "", "", "Clear", "Enter", "", "",
    "Shift", "Control", "Alt", "Pause", "Caps Lock", "", "", "", "", "", "", "Escape", "", "", "", "",
    "Space", "Page Up", "Page Down", "End", "Home", "Left Arrow", "Up Arrow", "Right Arrow", "Down Arrow", "Select", "", "", "Print Screen", "Insert", "Delete", "Help",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "", "", "", "", "", "",
    "", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
    "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "Left Windows", "Right Windows", "Apps", "", "Sleep",
    "Numpad 0", "Numpad 1", "Numpad 2", "Numpad 3", "Numpad 4", "Numpad 5", "Numpad 6", "Numpad 7", "Numpad 8", "Numpad 9", "Numpad *", "Numpad +", "", "Numpad -", "Numpad Decimal", "Numpad /",
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16",
    "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "", "", "", "", "", "", "", "",
    "Num Lock", "Scroll Lock", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "Left Shift", "Right Shift", "Left Control", "Right Control", "Left Menu", "Right Menu", "Browser Back", "Browser Forward", "Browser Refresh", "Browser Stop", "Browser Search", "Browser Favorites", "Browser Home", "Volume Mute", "Volume Down", "Volume Up",
    "Next Track", "Previous Track", "Media Stop", "Media Play/Pause", "Mail", "Media Select", "Launch App 1", "Launch App 2", "", "", "OEM ;", "OEM +", "OEM ,", "OEM -", "OEM .", "OEM /",
    "OEM ~", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "OEM [", "OEM \\", "OEM ]", "OEM '", "OEM 8",
    "", "", "OEM <", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "Attn", "CrSel", "ExSel", "Erase EOF", "Play", "Zoom", "", "PA1", "OEM Clear", ""
};

std::string codeToString(uint8_t vkCode){
    return keyboard_keys[vkCode];
}

uint8_t stringToCode(std::string key){
    for (int i=0; i<256; i++) {
        if (keyboard_keys[i] == key) return i;
    }
    return 0;
}

bool isKeyDown(const reshade::api::effect_runtime* runtime, std::string _keyStr, bool _altRequired, bool _shiftRequired, bool _ctrlRequired){
    uint8_t _keyCode = stringToCode(_keyStr);
    bool toReturn = false;
    if (_keyCode < 7) {
        toReturn = runtime->is_mouse_button_down(_keyCode);
    } else {
        toReturn = runtime->is_key_down(_keyCode);
    }
    const bool altPressed = runtime->is_key_down(VK_MENU);;
    const bool shiftPressed = runtime->is_key_down(VK_SHIFT);
    const bool ctrlPressed = runtime->is_key_down(VK_CONTROL);

    toReturn &= ((_altRequired && altPressed) || (!_altRequired && !altPressed));
    toReturn &= ((_shiftRequired && shiftPressed) || (!_shiftRequired && !shiftPressed));
    toReturn &= ((_ctrlRequired && ctrlPressed) || (!_ctrlRequired && !ctrlPressed));
    return toReturn;
}

bool isKeyPressed(const reshade::api::effect_runtime* runtime, std::string _keyStr, bool _altRequired, bool _shiftRequired, bool _ctrlRequired){
    uint8_t _keyCode = stringToCode(_keyStr);
    bool toReturn = false;
    if (_keyCode < 7) {
        toReturn = runtime->is_mouse_button_pressed(_keyCode);
    } else {
        toReturn = runtime->is_key_pressed(_keyCode);
    }
    const bool altPressed = runtime->is_key_down(VK_MENU);;
    const bool shiftPressed = runtime->is_key_down(VK_SHIFT);
    const bool ctrlPressed = runtime->is_key_down(VK_CONTROL);

    toReturn &= ((_altRequired && altPressed) || (!_altRequired && !altPressed));
    toReturn &= ((_shiftRequired && shiftPressed) || (!_shiftRequired && !shiftPressed));
    toReturn &= ((_ctrlRequired && ctrlPressed) || (!_ctrlRequired && !ctrlPressed));
    return toReturn;
}

bool isKeyReleased(const reshade::api::effect_runtime* runtime, std::string _keyStr, bool _altRequired, bool _shiftRequired, bool _ctrlRequired){
    uint8_t _keyCode = stringToCode(_keyStr);
    bool toReturn = false;
    if (_keyCode < 7) {
        toReturn = runtime->is_mouse_button_released(_keyCode);
    } else {
        toReturn = runtime->is_key_released(_keyCode);
    }
    const bool altPressed = runtime->is_key_down(VK_MENU);;
    const bool shiftPressed = runtime->is_key_down(VK_SHIFT);
    const bool ctrlPressed = runtime->is_key_down(VK_CONTROL);

    toReturn &= ((_altRequired && altPressed) || (!_altRequired && !altPressed));
    toReturn &= ((_shiftRequired && shiftPressed) || (!_shiftRequired && !shiftPressed));
    toReturn &= ((_ctrlRequired && ctrlPressed) || (!_ctrlRequired && !ctrlPressed));
    return toReturn;
}

static void onReshadePresent(reshade::api::effect_runtime* runtime) {
    reshade::api::effect_uniform_variable enable_var = runtime->find_uniform_variable("zoomscope.fx", "EnableMagnifier");
    reshade::api::effect_uniform_variable zoom_var = runtime->find_uniform_variable("zoomscope.fx", "DynamicZoomLevel");
    reshade::api::effect_uniform_variable scale_var = runtime->find_uniform_variable("zoomscope.fx", "ZoomLevelDelta");
    reshade::api::effect_uniform_variable inc_var = runtime->find_uniform_variable("zoomscope.fx", "ZoomIn");
    reshade::api::effect_uniform_variable dec_var = runtime->find_uniform_variable("zoomscope.fx", "ZoomOut");
    if (enable_var == 0 || zoom_var == 0 || scale_var == 0 || inc_var == 0 || dec_var == 0) return;
    bool enabled = false;
    runtime->get_uniform_value_bool(enable_var, &enabled, 1);
    if (enabled) {
        bool inc = 0.0f;
        bool dec = 0.0f;
	    runtime->get_uniform_value_bool(inc_var, &inc, 1);
	    runtime->get_uniform_value_bool(dec_var, &dec, 1);
        float wheel = 0.0f;
        float delta = 0.0f;
        if (dec && !inc)
            delta = -1.0f;
        else if (inc && !dec)
            delta = 1.0f;
        else
            delta = 0.0f;
        runtime->get_uniform_value_float(scale_var, &wheel, 1);
        float zoom = std::clamp(1 + delta * wheel, 1.0f, 10.0f);
        runtime->set_uniform_value_float(zoom_var, zoom);
    } else {
        runtime->set_uniform_value_float(zoom_var, 1.0f);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        reshade::register_addon(hModule);
	reshade::register_event<reshade::addon_event::reshade_present>(onReshadePresent);
    } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
	reshade::unregister_event<reshade::addon_event::reshade_present>(onReshadePresent);
        reshade::unregister_addon(hModule);
    }
    return TRUE;
}
