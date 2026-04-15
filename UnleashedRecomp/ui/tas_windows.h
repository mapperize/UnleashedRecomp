#pragma once
#include <SDL.h>
#include <imgui.h>


struct Quaternion {
    be<float> x;
    be<float> y;
    be<float> z;
    be<float> w;
};

class TASWindow {
public:
    static void Update();
    static void Shutdown();
    static void UpdateFrame();
    static void SavePosition();
    static void LoadPosition();
    static void SetWerehogPointer(uintptr_t ptr);
    static inline bool DPAD_DOWN;
    static inline bool DPAD_UP;
    static inline bool DPAD_RIGHT;
    static inline bool isInGame;
    
private:
    static void ShowValues(uintptr_t ptr);
    static inline bool showPointers;

    static inline SDL_Window* s_window = nullptr;
    static inline SDL_GLContext s_glContext = nullptr;
    static inline ImGuiContext* s_imguiContext = nullptr;
    static inline bool s_show = true;

    static inline uintptr_t werehogPointer;

    static inline Quaternion *rotation;
    static inline Quaternion *position;
    static inline Quaternion *velocity;

    static inline Quaternion savedRotation;
    static inline Quaternion savedPosition;
    static inline Quaternion savedVelocity;
    
};



