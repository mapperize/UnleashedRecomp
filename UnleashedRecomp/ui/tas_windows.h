#pragma once
#include <SDL.h>
#include <imgui.h>

struct Quaternion {
    be<float> x;
    be<float> y;
    be<float> z;
    be<float> w;
};

#define deadzone(num) fabs(num) < 1e-6 ? 0.0: num

// I'm a printf engineer
#define printHook(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    __imp__##func(ctx, base);\
    printf("#func");\
}

#define printArguments(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    printf("r3: %x, r4: %x, r5: %x", ctx.r3.u32, ctx.r4.u32, ctx.r5.u32);\
    printf("#func");\
    __imp__##func(ctx, base);\
}

#define printReturn(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    __imp__##func(ctx, base);\
    printf("#func");\
    printf("%x", ctx.r3.u32);\
}

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
    static inline bool isCheckpointDisable = true;

    static inline Quaternion *rotation;
    
private:
    static void ShowValues(uintptr_t ptr);
    

    static inline SDL_Window* s_window = nullptr;
    static inline SDL_GLContext s_glContext = nullptr;
    static inline ImGuiContext* s_imguiContext = nullptr;
    static inline bool s_show = true;

    static inline bool showPointers;
    static inline bool freeWindowDataView;
    static inline bool showPos = true;
    static inline bool showVelo = true;
    static inline bool showSpeed = true;
    static inline bool showHorizontalSpeed = true;
    static inline bool showRot = false;

    static inline float scale = 1.0f;

    static inline uintptr_t werehogPointer;
    static inline Quaternion *position;
    static inline Quaternion *velocity;

    static inline Quaternion savedRotation;
    static inline Quaternion savedPosition;
    static inline Quaternion savedVelocity;

    
    
};



