// tas_windows.cpp
#include "tas_windows.h"
#include "tas_json.h"
#include "tas_hooks.h"
#include "game_window.h"
#include "speedometer.h"

#include <SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <gpu/imgui/imgui_snapshot.h>
#include "misc/cpp/imgui_stdlib.h"

#include <kernel/memory.h>
#include <kernel/function.h>
#include <cpu/guest_stack_var.h>

#include <iostream>
#include <cstdint>

Speedometer speedometer(ImVec2(350.0f, 250.0f), 4.0f, 240, 200.0f);

void TASWindow::Update()
{
    SDL_Event event;
    gameDocument = SWA::CGameDocument::GetInstance();
    // i highkey copy and pasted this font and the velocity thingy from some random skyth patch in the unleashed speedrun discord
    playerSpeedContext = *(be<uint32_t>*)g_memory.Translate(0x83362F98);

    if(firstTimeLoad){
        ReloadJson();
        LoadConfig();
        firstTimeLoad = false;
    }
    
    if (alwaysShowCursor) 
        GameWindow::SetFullscreenCursorVisibility(true);
    else 
        GameWindow::SetFullscreenCursorVisibility(false);

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
            ImGui::Checkbox("Show Acceleration Scalar", &showAccelScalar);
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

        if (ImGui::CollapsingHeader("Daytime")){
            if(playerSpeedContext != NULL){
                auto rings = getPointer(0x83362F98, 0x538);
                auto ringEnergy = (be<float>*)getPointer(0x83362F98, 0x53C);
                uint32_t ringsLE = (uint32_t)*rings;
                float ringEnergyLE = (float)*ringEnergy;

                int step = 1;
                int step_fast = 10;
                ImGui::InputScalar("Rings", ImGuiDataType_U32, &ringsLE, &step, &step_fast, "%u");
                ImGui::SliderFloat("Ring Energy", &ringEnergyLE, 0.0f, 100.0f, "%.2f");

                *rings = (be<uint32_t>)ringsLE;
                if (infiniteRingEnergy)
                    *ringEnergy = 100.0f;
                else
                    *ringEnergy = (be<float>)ringEnergyLE;
            }
            ImGui::Checkbox("Infinite Boost", &infiniteRingEnergy);
        }
    
        if (ImGui::CollapsingHeader("Misc")){
            ImGui::Checkbox("Disable Checkpoints", &isCheckpointDisable);
            //ImGui::Checkbox("Show Context Pointers", &showPointers);
            //ImGui::SetItemTooltip("This is useful if you want to use Cheat Engine to find some values");
            ImGui::Checkbox("Show Mouse Cursor in Fullscreen", &alwaysShowCursor);
        }

        if (ImGui::Button("Save Position") && (playerSpeedContext != NULL || werehogPointer != NULL)) 
            if(position != NULL) SavePosition();
        
        ImGui::SameLine();
        if (ImGui::Button("Load Position") && (playerSpeedContext != NULL || werehogPointer != NULL))
            if(position != NULL) LoadPosition();
        
        if (ImGui::Button("Position Manager"))
            showPositionWindow = !showPositionWindow;
        
        /*
        if (ImGui::Button("Restart") || BACK && (playerSpeedContext != NULL || werehogPointer != NULL)) {

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

    // this function handles more than just gui
    PositionManager();

    bool currentKeyState = SDL_GetKeyboardState(NULL)[SDL_SCANCODE_RSHIFT];
    if (currentKeyState) {
        debounceMenuToggle += 1;
        if (debounceMenuToggle == 3){
            showWindow = !showWindow;
            if (firstTime){
                showNotification = true;
                firstTime = false;
            }
        }
        else if (debounceMenuToggle > 3) debounceMenuToggle = 4;
    }
    else debounceMenuToggle = 0;
    if (showNotification)
        Notification("You can press right shift again\n to open the practice menu");
    
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

void TASWindow::PositionManager(){
    // wrapping this in a macro is weird so debouncing will stay like this for now
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

    if (gameDocument != NULL){
        bool matched = false;
        const char* stageName = gameDocument->m_pMember->m_StageName.c_str();
        newStageName = stageName; // c_str into std::string
        if (oldStageName != newStageName){
            forceReload = true;
            speedometer.firstTimeSpeedometer = true; // yeah i know this is disorganized
        }
        if (forceReload){
            forceReload = false;
            for (Level& t: levels){ 
                if(t.name == newStageName) {
                    currentLevel = &t;
                    matched = true;
                    break;
                }
            }
            // if the check in levels loaded in by the json didn't find anything then we have to add a new entry in levels vector
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
            // i can't make InputFloat3 cast properly with be<float> so the overloaded swapEndian is a nasty workaround
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
                // selected index means make it green
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
            SaveJson();
        }
        if (ImGui::Button("Reload Positions File") || firstTimeLoad){
            ReloadJson();
        }
        ImGui::End();
    }
    return;
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

    be<float> timer;
    int minutes = 0;
    int seconds = 0;
    int milliseconds = 0;
    if(isWerehog){
        // werehog timer 
        if (gameDocument) {
            void *m_pMember = (void*)gameDocument->m_pMember;
            timer = *(be<float>*)((uintptr_t)m_pMember + 0x5C);
            if (timer > 0) {
                minutes = (int)(timer / 60);
                seconds = (int)(timer - (minutes * 60));
                milliseconds = (int)(timer * 100 - (minutes * 60) - seconds);
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
        if (showAccelScalar)ImGui::Text("Accel Scalar: %.3f", deadzone(sqrt(pow(xAccel, 2)+pow(yAccel, 2) + pow(zAccel, 2))));
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
    if (*savedPosition == Quaternion(0,0,0,0)) return;
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
    showAccelScalar = Config::showAccelScalar;

    enableTimer = Config::enableTimer;

    infiniteRingEnergy = Config::infiniteRingEnergy;

    showPositionWindow = Config::showPositionWindow;

    isCheckpointDisable = Config::isCheckpointDisable;
    alwaysShowCursor = Config::alwaysShowCursor;

    speedometer.isEnabled = Config::isSpeedometerEnabled;
    speedometer.freeWindowMode = Config::isSpeedometerFreeMoveEnabled;
    speedometer.freePos = ImVec2(Config::speedometerX, Config::speedometerY);
    scale = Config::speedometerScale;
}


// this gets called on app close
void TASWindow::SaveConfig()
{
    Config::isDataViewEnabled = showData;
    Config::showPos = showPos;
    Config::showVelo = showVelo;
    Config::showSpeed = showSpeed;
    Config::showHorizontalSpeed = showHorizontalSpeed;
    Config::showRot = showRot;
    Config::showAccel = showAccel;
    Config::showAccelScalar = showAccelScalar;

    Config::enableTimer = enableTimer;

    Config::infiniteRingEnergy = infiniteRingEnergy;

    Config::showPositionWindow = showPositionWindow;

    Config::isCheckpointDisable = isCheckpointDisable;
    Config::alwaysShowCursor = alwaysShowCursor;

    Config::isSpeedometerEnabled = speedometer.isEnabled;
    Config::isSpeedometerFreeMoveEnabled = speedometer.freeWindowMode;
    Config::speedometerX = speedometer.freePos.x;
    Config::speedometerY = speedometer.freePos.y;
    Config::speedometerScale = scale;

    SaveJson();

    Config::Save();
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