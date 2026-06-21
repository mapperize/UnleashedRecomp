#pragma once
#include <SDL.h>
#include <imgui.h>
#include <api/SWA.h>
#include <cstring>
#include <string>

#define deadzone(num) fabs(num) < 1e-3 ? 0.0: num

enum ACTION {
    SAVE,
    LOAD
};

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

    const bool operator==(const Quaternion& other){
        bool _x (x == other.x);
        bool _y (y == other.y);
        bool _z (z == other.z);
        bool _w (w == other.w);
        return (_x & _y & _z & _w);
    }
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
    //bool is2DMode;
};

struct Level {
    std::string name;
    Position positions[10];
    //int force2DIndex;
    //int force3DIndex;
};

struct PositionAction {
    Position position;
    size_t quickIndex; // this corrosponds to what is on the quick load index
    enum ACTION type;
};

struct PositionBuffer {
    PositionAction actions[10];
    size_t index = 0;
};

class TASWindow {
public:
    static void Update();
    static void Shutdown();
    static void SavePosition();
    static void LoadPosition();
    static void SetWerehogPointer(uintptr_t ptr);

    static void SaveConfig();
    static void LoadConfig();

    static inline bool DPAD_DOWN;
    static inline bool DPAD_UP;
    static inline bool DPAD_RIGHT;
    static inline bool DPAD_LEFT;
    static inline bool BACK;

    static inline bool isInGame;
    static inline bool is2DHook = false;
    static inline bool isCheckpointDisable = true;
    static inline bool disableVoidKill = false;
    static inline bool disableLives = true;

    static inline bool getDayTimeRotation = false;
    
    static inline Quaternion *position;
    static inline Quaternion *rotation;
    static inline Quaternion *velocity;
    
private:
    static QuaternionLE swapEndian(Quaternion q){
        return QuaternionLE((float)q.x,(float)q.y,(float)q.z,(float)q.w);
    }
    static Quaternion swapEndian(QuaternionLE q){
        return Quaternion((be<float>)q.x,(be<float>)q.y,(be<float>)q.z,(be<float>)q.w);
    }
    // quick dereference helper
    static be<uint32_t> *getPointer(uint32_t ptr, int offset){
        uintptr_t dereferencedPtr = *(be<uint32_t>*)(g_memory.Translate(ptr));
        uintptr_t ctxPtr = (uintptr_t)g_memory.Translate(dereferencedPtr);
        return (be<uint32_t>*)(ctxPtr + offset);
    }

    static void ShowValues(uintptr_t ptr, bool isWerehog);
    static void Notification(const char* text);
    static void PositionManager();
    static void RestartGame();
    static void WerehogTimer();

    static bool NullCheck();
    static void DebugUpdate();

    static inline bool undoStatus = false;
    static void UndoPosition();
    static void RedoPosition();
    static void IncrementPositionBuffer();
    static void DecrementPositionBuffer();

    static std::filesystem::path GetPositionPath();
    static std::filesystem::path GetPracticeConfigPath();

    static inline uint32_t playerSpeedContext;
    static inline uint32_t playerDeathContext;
    static inline SWA::CGameDocument* gameDocument;

    static inline SDL_Window* s_window = nullptr;
    static inline SDL_GLContext s_glContext = nullptr;
    static inline ImGuiContext* s_imguiContext = nullptr;
    static inline bool s_show = true;
    static inline bool alwaysShowCursor = true;
    static inline bool allowBrokenFeatures = false;

    static inline bool disableDPadMovement;
    
    static inline bool eventCollisionDebugView;
    static inline bool GIMipLevelDebugView;
    static inline bool objectCollisionDebugView;
    static inline bool stageCollisionDebugView;

    static inline bool showData;
    static inline bool showPointers;
    static inline bool showPos = true;
    static inline bool showVelo = true;
    static inline bool showSpeed = true;
    static inline bool showHorizontalSpeed = false;
    static inline bool showRot = false;
    static inline bool showAccel = false;
    static inline bool showAccelScalar = false;

    static inline Vector3 currentDisplayVelocity;
    static inline double displaySpeed;
    static inline double horizontalSpeed;
    static inline double xAccel;
    static inline double yAccel;
    static inline double zAccel;

    // scale for data display
    static inline float dataFontScale = 1.0f;

    static inline uintptr_t werehogPointer;
    static inline bool enableTimer;

    static inline PositionBuffer posBuffer;
    static inline size_t bufferBound = 10;
    static void ReloadJson();
    static void SaveJson();
    static inline Level *currentLevel;
    static inline std::vector<Level> levels;
    static inline std::string newStageName;
    static inline std::string oldStageName;
    static inline bool firstTimeLoad = true;
    static inline bool forceReload = false;
    static inline bool showPositionWindow;
    // this is for the level, NOT the redo/undo buffer
    // that is handled in the PositionBuffer struct
    static inline bool maxStatus;
    static inline size_t positionIndex = 0;
    static inline float positionEdit[3]; // for imgui position editor
    static inline Position *currentPosition;
    //static inline bool *is2DCurrent;

    static inline ImVec2 lastSize;

    static inline Vector3 prevVelocity = {0};
    static inline double time;

    static inline bool firstTime = true;
    static inline bool showNotification = false;
    static inline bool showSaveNotification = false;
    static inline const float notificationActiveTime = 5.0f;
    static inline float notificationTimer;
    static inline int debounceMenuToggle;

    static inline bool showWindow = true;

    static inline int debounceUpIndex;
    static inline int debounceDownIndex;
    static inline int debounceLeftIndex;
    static inline int debounceRightIndex;
    static inline int debounceBackIndex;

    static inline int waitFrames;
    static inline int frameIndex;

    static inline bool infiniteRingEnergy = false;
};
