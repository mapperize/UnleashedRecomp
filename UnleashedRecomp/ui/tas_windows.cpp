// tas_windows.cpp
#include "tas_windows.h"
#include "game_window.h"
#include "speedometer.h"
#include <imgui.h>
#include <gpu/imgui/imgui_snapshot.h>
#include "misc/cpp/imgui_stdlib.h"
#include <backends/imgui_impl_sdl2.h>
#include <SDL.h>
#include <kernel/memory.h>
#include <kernel/function.h>
#include <cpu/guest_stack_var.h>
#include <SWA.inl>
#include <nlohmann/json.hpp>
#include <user/paths.h>
#include <iostream>
#include <cstdint>

using json = nlohmann::json;


Speedometer speedometer(ImVec2(350.0f, 250.0f), 4.0f, 240, 200.0f);

PPCContext savedRetryCtx;
uint8_t *savedRetryBase{};

PPCContext savedRetryCtx2;
uint8_t *savedRetryBase2{};

uint32_t savedCameraContext;

bool getDayTimeRotation = false;

bool is2DMode = false;

PRINT_BOTH(sub_824AEC10);

// asm hooks
void GetRotate(PPCRegister& r3){
    if (getDayTimeRotation) {
        TASWindow::rotation = (Quaternion*)g_memory.Translate(r3.u32 + 0x460);
    }
}

//PRINT_BOTH(sub_82342D20);

PPC_FUNC_IMPL(__imp__sub_823176A0);
PPC_FUNC(sub_823176A0){
    printf("\n%p, %p, %p", ctx.v7.u32, ctx.v8.u32, ctx.v9.u32);
    savedRetryCtx = ctx;
    savedRetryBase = base;
    __imp__sub_823176A0(ctx, base);
}

PPC_FUNC_IMPL(__imp__sub_82304270);
PPC_FUNC(sub_82304270){
    printf("\nsaved 2");
    savedRetryCtx2 = ctx;
    savedRetryBase2 = base;
    __imp__sub_82304270(ctx, base);
}

// sub_823176A0 insta kills sonic to void

// checkpoints activate this to change the restart to the checkpoint 
PPC_FUNC_IMPL(__imp__sub_82305DF8);
PPC_FUNC(sub_82305DF8){
    if (TASWindow::isCheckpointDisable) return;
    __imp__sub_82305DF8(ctx, base);
}

// reloads game objects but crashes sometimes because it doesnt unload it?
/*PPC_FUNC_IMPL(__imp__sub_827B62E0);
PPC_FUNC(sub_827B62E0){
    savedRetryCtx2 = ctx;
    savedRetryBase2 = base;
    __imp__sub_827B62E0(ctx, base);
}*/

// restarts the game to a state like death
/*PPC_FUNC_IMPL(__imp__sub_82304270);
PPC_FUNC(sub_82304270){
    savedRetryCtx = ctx;
    savedRetryBase = base;
    __imp__sub_82304270(ctx, base);
}*/

// check if player is in 2d
PPC_FUNC_IMPL(__imp__sub_823538E0);
PPC_FUNC(sub_823538E0){
    __imp__sub_823538E0(ctx, base);
    is2DMode = ctx.r3.u32;
}

// changes mode to 2d
PPC_FUNC_IMPL(__imp__sub_82B5F568);
PPC_FUNC(sub_82B5F568)
{
    __imp__sub_82B5F568(ctx, base);
}

// changes mode to 3d
PPC_FUNC_IMPL(__imp__sub_825F5E40);
PPC_FUNC(sub_825F5E40)
{
    __imp__sub_825F5E40(ctx, base);
}

bool TASWindow::compareQuaternion(const Quaternion lhs, const Quaternion rhs)
{
    bool _x (lhs.x == rhs.x);
    bool _y (lhs.y == rhs.y);
    bool _z (lhs.z == rhs.z);
    bool _w (lhs.w == rhs.w);
    return (_x & _y & _z & _w);
}

// le and be mania
QuaternionLE swapEndian(Quaternion q){
    float x = (float)q.x;
    float y = (float)q.y;
    float z = (float)q.z;
    float w = (float)q.w;
    return QuaternionLE(x,y,z,w);
}
Quaternion swapEndian(QuaternionLE q){
    be<float> x = (be<float>)q.x;
    be<float> y = (be<float>)q.y;
    be<float> z = (be<float>)q.z;
    be<float> w = (be<float>)q.w;
    return Quaternion(x,y,z,w);
}

void to_json(json& j, const be<float>& b) {
    j = (float)b;
}

void from_json(const json& j, be<float>& b) {
    b = j.get<float>();
}

void to_json(json& j, const Quaternion& q){
    j = json{
        {"w", q.w},
        {"x", q.x}, 
        {"y", q.y},
        {"z", q.z}
    };
}
void from_json(const json& j, Quaternion& q){
    j.at("x").get_to(q.x);
    j.at("y").get_to(q.y);
    j.at("z").get_to(q.z);
    j.at("w").get_to(q.w);
}

void to_json(json& j, const Position& p){
    j = json{
        {"position", p.pos}, 
        {"positionName", p.posName},
        {"rotation", p.rot}
    };
}
void from_json(const json& j, Position& p){
    j.at("position").get_to(p.pos);
    j.at("positionName").get_to(p.posName);
    j.at("rotation").get_to(p.rot);
}

void to_json(json& j, const Level& l) {
    j = json{
        {"name", l.name}, 
        {"positions", l.positions}
    };
}

void from_json(const json& j, Level& l) {
    j.at("name").get_to(l.name);
    j.at("positions").get_to(l.positions);
}

std::filesystem::path TASWindow::GetPositionPath()
{
    return GetUserPath() / "positions.json";
}

std::filesystem::path TASWindow::GetPracticeConfigPath()
{
    return GetUserPath() / "practice-config.toml";
}

// make sure to set the timer back to 0 everytime this has to be reused because im a bad progammer :(
void TASWindow::Notification(const char* text){
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    if (notificationTimer == 0){
        notificationTimer = notificationActiveTime;
    }
    else if (notificationTimer < 0) return;
    ImVec2 topRight = ImVec2(io.DisplaySize.x, 0.0f);
    ImGui::SetNextWindowPos(topRight, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    float size = ImGui::CalcTextSize(text).x - style.FramePadding.x * 2.0f;

    ImGui::Begin(text, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::Text("\n%s\n", text);
    float scaledProgress = size * (notificationTimer / notificationActiveTime);
    ImVec2 coordinateFrom = ImVec2(pos.x, pos.y);
    ImVec2 coordinateTo = ImVec2(pos.x + scaledProgress, pos.y);
    drawList->AddLine(coordinateFrom, coordinateTo, WHITE, 1);
    drawList->PathStroke(WHITE, false, 1.5f);
    ImGui::End();

    notificationTimer -= io.DeltaTime;
    if(notificationTimer == 0) notificationTimer -= 0.0001; // for the unlikely edge case
}

void TASWindow::Update()
{
    SDL_Event event;
    // i highkey copy and pasted this font and the velocity thingy from some random skyth patch in the unleashed speedrun discord
    playerSpeedContext = *(be<uint32_t>*)g_memory.Translate(0x83362F98);

    ImFont* font = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-M.otf");
    float defaultScale = font->Scale;
    font->Scale = ImGui::GetDefaultFont()->FontSize / font->FontSize;
    ImGui::PushFont(font);
    if (showWindow){
        ImGui::Begin("Practice Tools", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::CollapsingHeader("Data Display")){
            ImGui::Checkbox("Enable Data Display", &showData);
            ImGui::SliderFloat("Scale", &scale, 0.0f, 3.0f);   
            ImGui::Checkbox("Show Position", &showPos);
            ImGui::Checkbox("Show Velocity", &showVelo);
            ImGui::Checkbox("Show Speed", &showSpeed);
            ImGui::Checkbox("Show Horizontal Speed", &showHorizontalSpeed);
            ImGui::Checkbox("Show Rotation", &showRot);
            ImGui::Checkbox("Show Acceleration", &showAccel);
            ImGui::SetItemTooltip("This is an approximation");
        }
        if (ImGui::CollapsingHeader("Speedometer")){
            ImGui::Checkbox("Enable Speedometer", &speedometer.isEnabled);
            ImGui::Checkbox("Freely Move Speedometer", &speedometer.freeWindowMode);
            ImGui::SliderFloat("Scale", &speedometer.scale, 0.0f, 1.0f);        
        }
        if (ImGui::CollapsingHeader("Werehog")){
            ImGui::Checkbox("Enable Timer", &enableTimer);
        }
    
        if (ImGui::CollapsingHeader("Misc")){
            ImGui::Checkbox("Disable Checkpoints", &isCheckpointDisable);
            //ImGui::Checkbox("Show Context Pointers", &showPointers);
            //ImGui::SetItemTooltip("This is useful if you want to use Cheat Engine to find some values");
        }

        if (ImGui::Button("Save Position") && (playerSpeedContext != NULL || werehogPointer != NULL)) 
            if(position != NULL) SavePosition();
        
        ImGui::SameLine();
        if (ImGui::Button("Load Position") && (playerSpeedContext != NULL || werehogPointer != NULL))
            if(position != NULL) LoadPosition();
        
        if (ImGui::Button("Position Manager Window"))
            showPositionWindow = !showPositionWindow;
        /*
        if (ImGui::Button("Restart") || BACK && (playerSpeedContext != NULL || werehogPointer != NULL)) {
            GuestToHostFunction<void>(sub_823176A0, savedRetryCtx.r3.u32, 1);
            //GuestToHostFunction<void>(sub_82304270, savedRetryCtx2.r3.u32, savedRetryCtx2.r4.u32);
            //__imp__sub_823176A0(savedRetryCtx2, savedRetryBase2);
            //__imp__sub_82342D20(savedRetryCtx, savedRetryBase);
            //__imp__sub_82304980(savedRetryCtx, savedRetryBase);
            printf("\n%d", is2DMode);
        }
            */
        ImGui::SameLine();
        if (ImGui::Button("Hide Menu")) {
            showWindow = false;
            if (firstTime){
                showNotification = true;
                firstTime = false;
            }
        }
        ImGui::SetItemTooltip("You can show the menu again with right shift");
        ImGui::End();
    }

    PositionManager();

    bool currentKeyState = SDL_GetKeyboardState(NULL)[SDL_SCANCODE_RSHIFT];
    // needs debounce
    if (currentKeyState) {
        debounceNotificationFrames += 1;
        if (debounceNotificationFrames == 3){
            showWindow = !showWindow;
            if (firstTime){
                showNotification = true;
                firstTime = false;
            
        }
        else if (debounceNotificationFrames > 3) debounceNotificationFrames = 4;
    }
    else {
        debounceNotificationFrames = 0;
    }
    if (showNotification){
        Notification("You can press right shift again\n to open the practice menu");
    }
    if (playerSpeedContext != NULL)
    {
        getDayTimeRotation = true;
        ShowValues(playerSpeedContext, false);
        isInGame = true;
    }
    // this is set by some hook in player patches to change the icon
    else if(werehogPointer != NULL){
        getDayTimeRotation = false;
        ShowValues(werehogPointer, true);
        isInGame = true;
    }
    else{
        getDayTimeRotation = false;
        isInGame = false;
    }

    ImGui::PopFont();
    font->Scale = defaultScale;
}

void TASWindow::PositionManager()
{
    gameDocument = SWA::CGameDocument::GetInstance();
    if (DPAD_DOWN && (playerSpeedContext != NULL || werehogPointer != NULL)){
        ++debounceDownIndex;
        if (debounceDownIndex== 3){
            SavePosition();
        }
        if (debounceDownIndex > 3) debounceDownIndex = 4;
    } else debounceDownIndex = 0;
        
    if (DPAD_UP && (playerSpeedContext != NULL || werehogPointer != NULL)){
        ++debounceUpIndex;
        if (debounceUpIndex== 3){
            LoadPosition();
        }
        if (debounceUpIndex > 3) debounceUpIndex = 4;
    } else debounceUpIndex = 0;

    if (DPAD_LEFT && (playerSpeedContext != NULL || werehogPointer != NULL)){
        ++debounceLeftIndex;
        if (debounceLeftIndex == 3) {
            --positionIndex;
            if (positionIndex < 0) positionIndex = 9;
        }
        else if (debounceRightIndex > 3) debounceLeftIndex = 4;
    } else debounceLeftIndex = 0;

    if (DPAD_RIGHT && (playerSpeedContext != NULL || werehogPointer != NULL)){
        ++debounceRightIndex;
        if (debounceRightIndex == 3){
            ++positionIndex;
            if (positionIndex > 9) positionIndex = 0;
        }
        else if (debounceRightIndex > 3) debounceRightIndex = 4;
    } else debounceRightIndex = 0;

    if(firstTimeLoad){
        ReloadJson();
        LoadConfig();
        firstTimeLoad = false;
    }

    if (gameDocument != NULL){
        bool matched = false;
        const char* stageName = gameDocument->m_pMember->m_StageName.c_str();
        newStageName = stageName; // c_str into std::string
        if (oldStageName != newStageName){
            forceReload = true;
            speedometer.firstTimeSpeedometer = true; // yeah i know this is disorganized
        }
        //printf("\nnewStageName: %s, currentLevel.name: %s", newStageName.c_str(), currentLevel->name.c_str());
        if (forceReload){
            printf("\nforce reloaded or new stage");
            forceReload = false;
            
            for (Level& t: levels){ 
                if(t.name == newStageName) {
                    printf("\nwe matched: %s so then it should match with %s in vector", newStageName.c_str(), t.name.c_str());
                    currentLevel = &t;
                    printf("\n and so we have %s in currentLevel", currentLevel->name.c_str());
                    matched = true;
                    break;
                }
            }
            // if the check in levels loaded in by the json didn't find anything then we have to add a new entry in levels
            if (*stageName != '\0' && !matched){
                currentLevel = new Level();
                currentLevel->name = newStageName;
                levels.push_back(*currentLevel);
                delete currentLevel;
                currentLevel = &levels.back();
            }
        }

        // update the vector with currentLevel before doing anything
        Position *currentPosition = currentLevel->positions + positionIndex;
        savedPosition = (Quaternion*)&currentPosition->pos;
        savedRotation = (Quaternion*)&currentPosition->rot;
        oldStageName = newStageName;
    }
    
    if(showPositionWindow){
        ImGui::Begin("Position Manager", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (newStageName == "") ImGui::Text("Not Currently in a Level");
        else {
            ImGui::Text("Level: %s", currentLevel->name.c_str());
            ImGui::Text("Quick Load Index: %d", positionIndex);
            ImGui::Text("Quick Load Position Editor:");

            QuaternionLE currentSavePos = swapEndian(*savedPosition);
            QuaternionLE lastSavePos = currentSavePos;
            ImGui::InputFloat3("", (float*)&currentSavePos, "%.2f");
            if (currentSavePos != lastSavePos){
                *savedPosition = swapEndian(currentSavePos);
            }
            
            for (int i = 0; i < 10; i++){
                ImGui::PushID(i);
                char positionBuffer[20];
                sprintf(positionBuffer, "%d", i);
                if (ImGui::Button("Select"))
                    positionIndex = i;
                Position *currentPos = &currentLevel->positions[i];
                ImGui::SameLine();
                if (i == positionIndex)
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 245.0f, 0.0f, 0.44f)); 
                ImGui::InputText(positionBuffer, &currentPos->posName, ImGuiWindowFlags_None);
                ImGui::PopID();
                if (i == positionIndex)
                    ImGui::PopStyleColor(1);
            }
            ImGui::Text("");
        }

        if (ImGui::Button("Save Positions to File")){
            json data = levels;
            std::ofstream out(GetPositionPath());
            out << std::setw(4) << data << std::endl;
        }
        if (ImGui::Button("Reload Positions File") || firstTimeLoad){
            ReloadJson();
        }
        ImGui::End();
    }
    return;
}

void TASWindow::ReloadJson(){
    std::ifstream file(GetPositionPath());
    if (file) {
        json data = json::parse(file);
        data.get_to(levels);
        // since this might get freed we need to set currentLevel to nullptr
        currentLevel = NULL;
        forceReload = true;
        printf("\nsuccessfully reloaded json");
    }
    else {
        printf("\nfile probably doesnt exist");
    }
}

void TASWindow::ShowValues(uintptr_t ptr, bool isWerehog){
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 25.0f));
    ImGuiWindowFlags flags;
    flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;
    ImGuiIO& io = ImGui::GetIO();

    uintptr_t matrix = (ptr + 0x10);
    matrix = *(be<uint32_t>*)g_memory.Translate(matrix);
    uintptr_t transform = (matrix + 0x60);
    position = (Quaternion*)g_memory.Translate(transform + 0x10);

    int time;
    int minutes;
    int milliseconds;
    int seconds;
    if(isWerehog){
        
        if (auto pGameDocument = SWA::CGameDocument::GetInstance()) {
            // i am really sorry to whoever wanted to read this

            //be<float>* timer = (be<float>*)g_memory.Translate(ptr + 0x6a4);
            
            
            be<float> timer = *(be<float>*)g_memory.Translate( &(pGameDocument->pMember) + 0x5C);
            if (timer >= 0) {

                int time = (int)(timer * 100);
                int centiseconds = (time - minutes * 60 * 100);

                minutes = (time / 100) / 60;
                seconds = centiseconds / 100;
                milliseconds = centiseconds % 100;
            }
        }
        velocity = (Quaternion*)g_memory.Translate(ptr + 0x900);  
        rotation = (Quaternion*)g_memory.Translate(transform);
    }
    else{
        velocity = (Quaternion*)g_memory.Translate(ptr + 528);
    }

    Vector3 currentVelocity = Vector3((double)velocity->x, (double)velocity->y, (double)velocity->z);
    double speed = deadzone(sqrt(pow(currentVelocity.x, 2)+pow(currentVelocity.y, 2) + pow(currentVelocity.z, 2)));

    if (speedometer.isEnabled == true) {
        if (speedometer.firstTimeSpeedometer){
            speedometer.Setup();
        }
            
        speedometer.Update(speed, io.DeltaTime);
    }
    
    if (!showData) return;
    ImGui::Begin("Data Viewer", NULL, flags);
    if (enableTimer && isWerehog) ImGui::Text("\nTimer: %02d : %02d : %02d", minutes, seconds, milliseconds);  
    if (showPos) ImGui::Text("Position: %.3f %.3f %.3f", (double)position->x, (double)position->y, (double)position->z);
    if (showRot) ImGui::Text("Rotation: %.3f %.3f %.3f %.3f", (double)rotation->x, (double)rotation->y, (double)rotation->z, (double)rotation->w);
    if (showVelo) {
        ImGui::Text("Velocity: %.3f %.3f %.3f", deadzone(currentVelocity.x), deadzone(currentVelocity.y), deadzone(currentVelocity.z));
    }
    if (showSpeed) ImGui::Text("Speed: %.3f", speed);
    if (showHorizontalSpeed) {
        double horizontalSpeed = deadzone(sqrt(pow(currentVelocity.x, 2) + pow(currentVelocity.z, 2)));
        ImGui::Text("H Speed: %.3f", horizontalSpeed);
    }
    if (showAccel) {
        double xAccel = (currentVelocity.x - prevVelocity.x) / io.DeltaTime;
        double yAccel = (currentVelocity.y - prevVelocity.y) / io.DeltaTime;
        double zAccel = (currentVelocity.z - prevVelocity.z) / io.DeltaTime;
        ImGui::Text("Accel Vector: %.3f %.3f %.3f", deadzone(xAccel), deadzone(yAccel), deadzone(zAccel));
        ImGui::Text("Accel Scalar: %.3f", deadzone(sqrt(pow(xAccel, 2)+pow(yAccel, 2) + pow(zAccel, 2))));
        prevVelocity = currentVelocity;
    }
    /*if(showPointers){
        ImGui::Text("Player Speed Context: %lx", (uintptr_t)g_memory.Translate(ptr));
        ImGui::Text("Transform: %lx", (uintptr_t)g_memory.Translate(transform));
    }*/
    ImGui::End();
}

void TASWindow::SetWerehogPointer(uintptr_t ptr){
    TASWindow::werehogPointer = ptr;
}

void TASWindow::SavePosition()
{
    if (savedPosition != NULL)
        *savedPosition = *position;
    if (savedRotation != NULL)
        *savedRotation = *rotation;
}


void TASWindow::LoadPosition()
{
    if (compareQuaternion(*savedPosition, Quaternion(0,0,0,0))) return;
    if (savedPosition != NULL)
        *position = *savedPosition;
    if (savedRotation != NULL)
        *rotation = *savedRotation;
    *velocity = Quaternion(0,0,0,0);
}

void TASWindow::Shutdown()
{
    ImGui::SetCurrentContext(s_imguiContext);
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(s_imguiContext);
    SDL_DestroyWindow(s_window);
}

void TASWindow::LoadConfig()
{
    showData = Config::isDataViewEnabled;
    showPos = Config::showPos;
    showVelo = Config::showVelo;
    showSpeed = Config::showSpeed;
    showHorizontalSpeed = Config::showHorizontalSpeed;
    showRot = Config::showRot;
    showAccel = Config::showAccel;

    enableTimer = Config::enableTimer;

    showPositionWindow = Config::showPositionWindow;

    isCheckpointDisable = Config::isCheckpointDisable;

    speedometer.isEnabled = Config::isSpeedometerEnabled;
    speedometer.freeWindowMode = Config::isSpeedometerFreeMoveEnabled;
    speedometer.freePos = ImVec2(Config::speedometerX, Config::speedometerY);
    scale = Config::speedometerScale;
}

void TASWindow::SaveConfig()
{
    Config::isDataViewEnabled = showData;
    Config::showPos = showPos;
    Config::showVelo = showVelo;
    Config::showSpeed = showSpeed;
    Config::showHorizontalSpeed = showHorizontalSpeed;
    Config::showRot = showRot;
    Config::showAccel = showAccel;

    Config::enableTimer = enableTimer;

    Config::showPositionWindow = showPositionWindow;

    Config::isCheckpointDisable = isCheckpointDisable;

    Config::isSpeedometerEnabled = speedometer.isEnabled;
    Config::isSpeedometerFreeMoveEnabled = speedometer.freeWindowMode;
    Config::speedometerX = speedometer.freePos.x;
    Config::speedometerY = speedometer.freePos.y;
    Config::speedometerScale = scale;
    Config::Save();
}




/* not working
typedef enum {
    DO_NOTHING,
    GET_ROTATION,
    SET_ROTATION
} CameraAction;

CameraAction cameraAction = DO_NOTHING;
Vector3 savedCameraRegisters;

void CameraPositionMidAsmHook(PPCRegister& f1, PPCRegister& f2)
{
    switch(cameraAction){
        case DO_NOTHING:
            break;
        case GET_ROTATION:
            savedCameraRegisters.x = f1.f64;
            savedCameraRegisters.y = f2.f64;
            break;
        case SET_ROTATION:
            f1.f64 = savedCameraRegisters.x;
            f2.f64 = savedCameraRegisters.y;
            break;
    }
}

void AnotherCameraPositionMidAsmHook(PPCRegister& f0){
    switch(cameraAction){
        case DO_NOTHING:
            break;
        case GET_ROTATION:
            savedCameraRegisters.z = f0.f64;
            break;
        case SET_ROTATION:
            f0.f64 = savedCameraRegisters.z;
            break;
    }
}*/

/* can't figure out how to get the pointer in r3 without storing it in a hook rn
    ImGui::SameLine();
    if (ImGui::Button("Switch to 3D")) {
        GuestToHostFunction<void>(sub_825F5E40, saved2Ctx.r3.u32, saved2Ctx.r4.u32);
    }
*/


    //ImGui::Text("\nsome sort of sum: %0.4g", abs(controllerVector->x) + abs(controllerVector->y) + abs(controllerVector-> z));
