#pragma once
#include <SDL.h>
#include <imgui.h>

struct Vector3 {
    be<float> x;
    be<float> y;
    be<float> z;
    be<float> w;
};



class TASWindow {
public:
    static void Init();
    static void Update();
    static void Shutdown();
    static void UpdateFrame();
    static void SavePosition();
    static void LoadPosition();
    static void SetWerehogPointer(uintptr_t ptr);
    
private:
    static void ShowValues(uintptr_t ptr);
    static inline bool showPointers;

    static inline SDL_Window* s_window = nullptr;
    static inline SDL_GLContext s_glContext = nullptr;
    static inline ImGuiContext* s_imguiContext = nullptr;
    static inline bool s_show = true;

    static inline uintptr_t werehogPointer;

    static inline Vector3 *rotation;
    static inline Vector3 *position;
    static inline Vector3 *velocity;

    static inline Vector3 savedRotation;
    static inline Vector3 savedPosition;
    static inline Vector3 savedVelocity;
};



