#pragma once
#include <SDL.h>
#include <imgui.h>
#include <api/SWA.h>
#include <cstring>
#include <string>

struct Vector3 {
    double x;
    double y; 
    double z;
};

struct Quaternion {
    be<float> x;
    be<float> y;
    be<float> z;
    be<float> w;
};
struct QuaternionLE {
    float x;
    float y;
    float z;
    float w;
    bool operator==(const QuaternionLE& other) const {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }
};

struct Position {
    Quaternion pos;
    Quaternion rot;
    std::string posName;
};

struct Level {
    std::string name;
    Position positions[10];
};

#define deadzone(num) fabs(num) < 1e-3 ? 0.0: num

#define PRINT_HOOK(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    __imp__##func(ctx, base);\
    printf("\n%s", #func);\
}

#define PRINT_ARGUMENTS(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    printf("\nr3: %x, r4: %x, r5: %x", ctx.r3.u32, ctx.r4.u32, ctx.r5.u32);\
    printf(" %s", #func);\
    __imp__##func(ctx, base);\
}

#define PRINT_RETURN(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    __imp__##func(ctx, base);\
    printf("\n%s", #func);\
    printf(" %x", ctx.r3.u32);\
}

#define PRINT_BOTH(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    printf("\nr3: %x, r4: %x, r5: %x", ctx.r3.u32, ctx.r4.u32, ctx.r5.u32);\
    printf(" %s", #func);\
    __imp__##func(ctx, base);\
    printf(" %x", ctx.r3.u32);\
}


class TASWindow {
public:
    static void Update();
    static void Shutdown();
    static void UpdateFrame();
    static void SavePosition();
    static void LoadPosition();
    static void SetWerehogPointer(uintptr_t ptr);

    static void SaveConfig();
    static void SaveConfig(Quaternion position);
    static void LoadConfig();

    static bool compareQuaternion(const Quaternion lhs, const Quaternion rhs);

    static inline bool DPAD_DOWN;
    static inline bool DPAD_UP;
    static inline bool DPAD_RIGHT;
    static inline bool DPAD_LEFT;
    static inline bool BACK;

    static inline bool isInGame;
    static inline bool isCheckpointDisable = true;

    static inline Quaternion *rotation;
    
private:
    static void ShowValues(uintptr_t ptr, bool isWerehog);
    static void Notification(const char* text);
    static void PositionManager();

    static std::filesystem::path GetPositionPath();
    static std::filesystem::path GetPracticeConfigPath();

    static inline uint32_t playerSpeedContext;
    static inline SWA::CGameDocument* gameDocument;

    static inline SDL_Window* s_window = nullptr;
    static inline SDL_GLContext s_glContext = nullptr;
    static inline ImGuiContext* s_imguiContext = nullptr;
    static inline bool s_show = true;

    static inline bool showData;
    static inline bool showPointers;
    static inline bool showPos = true;
    static inline bool showVelo = true;
    static inline bool showSpeed = true;
    static inline bool showHorizontalSpeed = false;
    static inline bool showRot = false;
    static inline bool showAccel = false;

    static inline float scale = 1.0f;

    static inline uintptr_t werehogPointer;
    static inline bool enableTimer;
    static inline Quaternion *position;
    static inline Quaternion *velocity;

    static void ReloadJson();
    static void SaveJson();
    static inline std::vector<Level> levels;
    static inline std::string newStageName;
    static inline std::string oldStageName;
    static inline bool firstTimeLoad = true;
    static inline bool forceReload = false;
    static inline bool showPositionWindow;
    static inline int positionIndex = 0;
    static inline float positionEdit[3];
    static inline Quaternion *savedRotation;
    static inline Quaternion *savedPosition;

    static inline ImVec2 lastSize;

    static inline Vector3 prevVelocity = {0};
    static inline double time;

    static inline bool firstTime = true;
    static inline bool showNotification = false;
    static inline const float notificationActiveTime = 5.0f;
    static inline float notificationTimer;
    static inline int debounceNotificationFrames;

    static inline bool showWindow = true;

    static inline int debounceUpIndex;
    static inline int debounceDownIndex;
    static inline int debounceLeftIndex;
    static inline int debounceRightIndex;

    static inline Level *currentLevel;
};
