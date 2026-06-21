// tas_windows.cpp
#include "tas_windows.h"
#include "tas_json.h"
#include "tas_hooks.h"
#include "game_window.h"
#include "speedometer.h"

#include <imgui.h>
#include <implot.h>
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
    playerDeathContext = *(be<uint32_t>*)g_memory.Translate(0x83364724);
    auto rings = getPointer(0x83362F98, 0x538);
    auto ringEnergy = (be<float>*)getPointer(0x83362F98, 0x53C);

    if(firstTimeLoad){
        ReloadJson();
        LoadConfig();
        firstTimeLoad = false;
    }

    if (Config::DisableDPadMovement != disableDPadMovement){
        Config::DisableDPadMovement = disableDPadMovement;
    }
    
    ImFont* font = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-M.otf");
    float defaultScale = font->Scale;
    font->Scale = (ImGui::GetDefaultFont()->FontSize / font->FontSize);
    ImGui::PushFont(font);
    if (alwaysShowCursor) 
        GameWindow::SetFullscreenCursorVisibility(true);
    else 
        GameWindow::SetFullscreenCursorVisibility(false);

    if (showWindow){
        ImGui::Begin("Practice Tools", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::CollapsingHeader("Game Options")){
            ImGui::Checkbox("Disable Lives Updating", &disableLives);
            ImGui::Checkbox("Disable Checkpoints", &isCheckpointDisable);
            ImGui::Checkbox("Disable D-Pad Movement", &disableDPadMovement);
            if (ImGui::TreeNode("Debug Views")){
                ImGui::Checkbox("Event Collision", &eventCollisionDebugView);
                ImGui::Checkbox("GI Mip Level", &GIMipLevelDebugView);
                ImGui::Checkbox("Object Collision", &objectCollisionDebugView);
                ImGui::Checkbox("Stage Collision", &stageCollisionDebugView);
                DebugUpdate();
                ImGui::TreePop();
            }
        }
        if (ImGui::CollapsingHeader("Data Display")){
            ImGui::Checkbox("Enable Data Display", &showData);
            ImGui::SliderFloat("Scale", &dataFontScale, 0.0f, 3.0f);   
            ImGui::SliderInt("Delay", &waitFrames, 0, 60);
            ImGui::SetItemTooltip("Amount of frames. Affects acceleration, velocity, and speed.");
            ImGui::Checkbox("Show Position", &showPos);
            ImGui::Checkbox("Show Rotation", &showRot);
            ImGui::Checkbox("Show Velocity", &showVelo);
            ImGui::Checkbox("Show Speed", &showSpeed);
            ImGui::Checkbox("Show Horizontal Speed", &showHorizontalSpeed);
            ImGui::Checkbox("Show Acceleration", &showAccel);
            ImGui::Checkbox("Show Acceleration Scalar", &showAccelScalar);
            ImGui::SetItemTooltip("This is an approximation");
        }
        if (ImGui::CollapsingHeader("Speed Display")){
            if (ImGui::TreeNode("Speedometer")){
                ImGui::Checkbox("Enable Speedometer", &speedometer.isEnabled);
                ImGui::SliderFloat("Scale", &speedometer.scale, 0.0f, 1.0f);
                ImGui::Checkbox("Freely Move Speedometer", &speedometer.freeWindowMode);
                ImGui::TreePop();
            }

            
            if (ImGui::TreeNode("Speed Plot")){
                ImGui::Checkbox("Enable Speed Plot", &showPlot);

                ImGui::TreePop();
            }
        }
        
        if (ImGui::CollapsingHeader("Werehog")){
            ImGui::Checkbox("Enable Timer", &enableTimer);
            ImGui::SetItemTooltip("Timer will show up in data display (for now)");
            if(werehogPointer != NULL){
                if (ImGui::Button("Restart Level"))
                    RestartGame();
            }
        }

        if (ImGui::CollapsingHeader("Daytime")){
            if(playerSpeedContext != NULL){
                uint32_t ringsLE = (uint32_t)*rings;
                float ringEnergyLE = (float)*ringEnergy;
                int step = 1;
                int step_fast = 10;
                ImGui::InputScalar("Rings", ImGuiDataType_U32, &ringsLE, &step, &step_fast, "%u");
                ImGui::SliderFloat("Ring Energy", &ringEnergyLE, 0.0f, 100.0f, "%.2f");

                *rings = (be<uint32_t>)ringsLE;
                *ringEnergy = (be<float>)ringEnergyLE;
                if (ImGui::Button("Kill Sonic"))
                    RestartGame();
            }
            ImGui::Checkbox("Infinite Boost", &infiniteRingEnergy);
            ImGui::Checkbox("Disable Void Death", &disableVoidKill);
        }
    
        if (ImGui::CollapsingHeader("Misc")){
            ImGui::Checkbox("Show Mouse Cursor in Fullscreen", &alwaysShowCursor);
            ImGui::Checkbox("Allow Broken Features", &allowBrokenFeatures);
            ImGui::SetItemTooltip("These features might crash the game or not work as intended");
            
        }

        if (allowBrokenFeatures){
            if (ImGui::CollapsingHeader("Broken Features")){
                ImGui::Checkbox("Show Context Pointers", &showPointers);
                ImGui::SetItemTooltip("This is useful if you want to use Cheat Engine to find some values");
                if (ImGui::TreeNode("Daytime Functions")){
                    if (ImGui::Button("Force 3D"))
                        GuestToHostFunction<void>(sub_825F5E40, saved3DCtx.r3.u32, saved3DCtx.r4.u32);
                    ImGui::SetItemTooltip("This only works right after switching from 2D");
                    ImGui::SameLine();
                    if (ImGui::Button("Force 2D"))
                        GuestToHostFunction<void>(sub_825F5B90, saved2DCtx.r3.u32, saved2DCtx.r4.u32);
                    ImGui::SetItemTooltip("This only works right after switching from 3D");
                }
            }
        }

        if (ImGui::Button("Position Manager"))
            showPositionWindow = !showPositionWindow;
        
        ImGui::SameLine();
        if (ImGui::Button("Hide Menu")) {
            showWindow = false;
            if (firstTime){
                showNotification = true;
                firstTime = false;
            }
        }
        ImGui::SetItemTooltip("You can show the menu again with right shift");
        
        if (BACK && NullCheck()){
            ++debounceBackIndex;
            if (debounceBackIndex == 3){
                RestartGame();
            }
            if (debounceBackIndex > 3) debounceBackIndex = 4;
        } else debounceBackIndex = 0;

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
        if (infiniteRingEnergy)
            *ringEnergy = 100.0f;
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
    bool playerActive = NullCheck();
    // wrapping this in a macro is weird so debouncing will stay like this for now
    if (playerActive && disableDPadMovement){
        if (DPAD_DOWN){
            ++debounceDownIndex;
            if (debounceDownIndex== 3){
                SavePosition();
            }
            if (debounceDownIndex > 3) debounceDownIndex = 4;
        } else debounceDownIndex = 0;
            
        if (DPAD_UP){
            ++debounceUpIndex;
            if (debounceUpIndex== 3){
                LoadPosition();
            }
            if (debounceUpIndex > 3) debounceUpIndex = 4;
        } else debounceUpIndex = 0;

        if (DPAD_LEFT){
            ++debounceLeftIndex;
            if (debounceLeftIndex == 3) {
                --positionIndex;
                if (positionIndex < 0) positionIndex = 9;
            }
            else if (debounceRightIndex > 3) debounceLeftIndex = 4;
        } else debounceLeftIndex = 0;

        if (DPAD_RIGHT){
            ++debounceRightIndex;
            if (debounceRightIndex == 3){
                ++positionIndex;
                if (positionIndex > 9) positionIndex = 0;
            }
            else if (debounceRightIndex > 3) debounceRightIndex = 4;
        } else debounceRightIndex = 0;
    }
    
    if (gameDocument != NULL){
        bool matched = false;
        const char* stageName = gameDocument->m_pMember->m_StageName.c_str();
        newStageName = stageName; // c_str into std::string
        if (oldStageName != newStageName){
            forceReload = true;
            speedometer.firstTimeSpeedometer = true;
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
        currentPosition = currentLevel->positions + positionIndex;
        //is2DCurrent = &currentPosition->is2DMode;
        oldStageName = newStageName;
    }

    if(showPositionWindow){
        ImGui::Begin("Position Manager", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (newStageName == "") {
            ImGui::Text("Not Currently in a Level");
            for (auto& action : posBuffer.actions) {
                action = PositionAction{};
            }
            posBuffer.index = 0;
        }
        else {
            ImGui::Text("Level: %s", currentLevel->name.c_str());
            ImGui::Text("Quick Load Index: %zu", positionIndex);
            ImGui::Text("Quick Load Position Editor:");
            // i can't make InputFloat3 cast properly with be<float> so the overloaded swapEndian is a nasty workaround
            QuaternionLE currentSavePos = swapEndian(currentPosition->pos);
            QuaternionLE lastSavePos = currentSavePos;
            ImGui::InputFloat3("", (float*)&currentSavePos, "%.2f");
            if (currentSavePos != lastSavePos){
                currentPosition->pos = swapEndian(currentSavePos);
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
        if (ImGui::Button("Save") && (playerSpeedContext != NULL || werehogPointer != NULL)) 
            if(position != NULL) SavePosition();
        ImGui::SameLine();
        if (ImGui::Button("Load") && (playerSpeedContext != NULL || werehogPointer != NULL))
            if(position != NULL) LoadPosition();
        ImGui::SameLine();
        if (ImGui::Button("Undo") && (playerSpeedContext != NULL || werehogPointer != NULL)) 
            if(position != NULL) UndoPosition();
        ImGui::SameLine();
        if (ImGui::Button("Redo") && (playerSpeedContext != NULL || werehogPointer != NULL))
            if(position != NULL) RedoPosition();

        if (ImGui::Button("Save Positions File"))
            SaveJson();
        ImGui::SameLine();
        if (ImGui::Button("Reload Positions File") || firstTimeLoad)
            ReloadJson();
        ImGui::End();
    }
}

void TASWindow::WerehogTimer(bool isWerehog){
    int minutes = 0;
    int seconds = 0;
    int milliseconds = 0;
    if (gameDocument) {
        void *m_pMember = (void*)gameDocument->m_pMember;
        timer = *(be<float>*)((uintptr_t)m_pMember + 0x5C);
        if (timer > 0) {
            minutes = (int)(timer / 60);
            seconds = (int)(timer - (minutes * 60));
            milliseconds = (int)(timer * 100 - (minutes * 60) - seconds);
        }
    }
    if (enableTimer && isWerehog)
        ImGui::Text("Timer: %02d : %02d : %02d\n", minutes, seconds, milliseconds);  
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

    if(isWerehog){
        velocity = (Quaternion*)g_memory.Translate(ptr + 0x900);  
        rotation = (Quaternion*)g_memory.Translate(transform);
    }
    else{
        velocity = (Quaternion*)g_memory.Translate(ptr + 528);
    }

    // we want the speedometer to still update to the current value
    Vector3 currentVelocity = Vector3((double)velocity->x, (double)velocity->y, (double)velocity->z);
    speed = deadzone(sqrt(pow(currentVelocity.x, 2)+pow(currentVelocity.y, 2) + pow(currentVelocity.z, 2)));
    if (frameIndex == waitFrames){
        currentDisplayVelocity = currentVelocity;
        displaySpeed = speed;
        horizontalSpeed = deadzone(sqrt(pow(currentVelocity.x, 2) + pow(currentVelocity.z, 2)));
        xAccel = (currentVelocity.x - prevVelocity.x) / io.DeltaTime;
        yAccel = (currentVelocity.y - prevVelocity.y) / io.DeltaTime;
        zAccel = (currentVelocity.z - prevVelocity.z) / io.DeltaTime;
        frameIndex = 0;
    }
    else    
        ++frameIndex;

    if (speedometer.isEnabled == true) {
        if (speedometer.firstTimeSpeedometer){
            speedometer.Setup();
        }
        speedometer.Update(speed, io.DeltaTime);
    }

    if (showPlot) ShowSpeedPlot();
    
    if (!showData) return;

    ImGui::PopFont();
    ImFont* font = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-M.otf");
    float originalScale = ImGui::GetDefaultFont()->FontSize / font->FontSize;
    font->Scale = originalScale * dataFontScale;
    ImGui::PushFont(font);

    ImGui::Begin("Data Viewer", NULL, flags);
    WerehogTimer(isWerehog); // we handle conditions in the function
    if (showPos) ImGui::Text("Position: %.3f %.3f %.3f", (double)position->x, (double)position->y, (double)position->z);
    if (showRot) ImGui::Text("Rotation: %.3f %.3f %.3f %.3f", (double)rotation->x, (double)rotation->y, (double)rotation->z, (double)rotation->w);
    if (showVelo) {
        ImGui::Text("Velocity: %.3f %.3f %.3f", deadzone(currentDisplayVelocity.x), deadzone(currentDisplayVelocity.y), deadzone(currentDisplayVelocity.z));
    }
    if (showSpeed) ImGui::Text("Speed: %.3f", displaySpeed);
    if (showHorizontalSpeed) {
        ImGui::Text("H Speed: %.3f", horizontalSpeed);
    }
    if (showAccel) {
        ImGui::Text("Accel Vector: %.3f %.3f %.3f", deadzone(xAccel), deadzone(yAccel), deadzone(zAccel));
        if (showAccelScalar) ImGui::Text("Accel Scalar: %.3f", deadzone(sqrt(pow(xAccel, 2)+pow(yAccel, 2) + pow(zAccel, 2))));
        prevVelocity = currentVelocity;
    }
    if(showPointers){
        ImGui::Text("Player Speed Context: %lx", (uintptr_t)g_memory.Translate(ptr));
        ImGui::Text("Transform: %lx", (uintptr_t)g_memory.Translate(transform));
    }
    ImGui::End();

    ImGui::PopFont();
    font->Scale = originalScale;
    ImGui::PushFont(font);
}

void TASWindow::ShowSpeedPlot(){
    static bool pausePlot;

    static bool areaCalc;
    static double endMarker;
    static double beginMarker;
    static double area;

    float xMax = (plotIndex > 0) ? speedSamples.x[plotIndex - 1] : windowXFit;
    float xMin = ((xMax - windowXFit) > 0.0f) ? xMax - windowXFit : 0;

    if (!areaCalc){
        beginMarker = xMin + (windowXFit * 0.1);
        endMarker = xMax - (windowXFit * 0.1);
    }

    if (plotIndex >= 18000)
        return;
    WerehogTimer(false); // this updates timer var
    if (timer <= 0.0f){
        memset(speedSamples.x, 0, sizeof(speedSamples.x));
        memset(speedSamples.y, 0, sizeof(speedSamples.y));
    }
    else if (pausePlot){
    }
    else {
        speedSamples.x[plotIndex] = timer;
        speedSamples.y[plotIndex] = speed;
        plotIndex++;
    }
    ImGui::Begin("Speed Plot");
    ImGui::Checkbox("Pause Plot", &pausePlot);
    ImGui::SliderFloat("X-Axis Window", &windowXFit, 0, 100, "%0.1f");
    if (ImGui::Button("Calculate Area / Distance Traveled")){
        pausePlot = true;
        areaCalc = true;
    }
    //ImGui::SetTooltip("You can use this to find if you maintained the most speed");
    if (areaCalc){
        ImGui::Text("Setup the two end markers to align where you want to calculate");
        if (ImGui::Button("Finish")){
            // first we need to snap to nearest datapoints with a linear search
            // we'll just avoid the rounding case to make it more simple (finding if the value above the searched value is actually closer)
            size_t i = 0;
            size_t beginIndex;
            size_t endIndex;
            while (i < 18000){
                if ((double)speedSamples.x[i] > beginMarker){
                    beginIndex = i;
                    break;
                }
                i++;
            }
            i = plotIndex - 1;
            while (i > 0){
                if ((double)speedSamples.x[i] < endMarker){
                    endIndex = i;
                    break;
                }
                i--;
            }
            // and let's do a trapezoidal riemann sum
            double sum = 0;
            double dt = speedSamples.x[endIndex] - speedSamples.x[beginIndex];
            for (size_t iter = beginIndex; iter <= endIndex; iter++){
                double h = speedSamples.x[beginIndex + 1] - speedSamples.x[beginIndex];
                double a = speedSamples.y[beginIndex];
                double b = speedSamples.y[beginIndex + 1];
                double value = (a + b) * h;
                sum += value;
            }
            area = (sum / 2) * dt;
            areaCalc = false;
        }
    }
    
    if (area != 0)
        ImGui::Text("Total Distance is %.2f", area);
    
    if (ImPlot::BeginPlot("Speed Plot")){
        ImPlot::SetupAxis(ImAxis_Y1, "Y Axis", ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxis(ImAxis_X1, "X Axis");
        ImPlot::SetupAxisLimits(ImAxis_X1, xMin, xMax, ImPlotCond_Always);

        if(areaCalc){
            ImPlot::DragLineX(0, &beginMarker, ImVec4(0,1,0,1), 4.0f); // green, thick
            ImPlot::DragLineX(1, &endMarker,   ImVec4(1,0,0,1), 4.0f); // red, thick
        }

        ImPlot::PlotLine("Speed", speedSamples.x, speedSamples.y, plotIndex);
        ImPlot::EndPlot();
    }
}

bool TASWindow::NullCheck(){
    return (playerSpeedContext != NULL || werehogPointer != NULL);
}

void TASWindow::SetWerehogPointer(uintptr_t ptr){
    TASWindow::werehogPointer = ptr;
}

void TASWindow::IncrementPositionBuffer(){
    if (posBuffer.index == 9){
        posBuffer.index = 0;
    }
    else {
        posBuffer.index++;
    }
}

void TASWindow::DecrementPositionBuffer(){
    if (posBuffer.index == 0){
        posBuffer.index = 9;
    }
    else {
        posBuffer.index--;
    }
}

void TASWindow::UndoPosition(){
    if (posBuffer.index == bufferBound && !maxStatus){
        return;
    }

    if (!undoStatus)
        bufferBound = posBuffer.index;
    
    DecrementPositionBuffer();
    PositionAction current = posBuffer.actions[posBuffer.index];
    positionIndex = current.quickIndex;
    currentPosition = currentLevel->positions + positionIndex;
    
    if (current.type == SAVE){
        currentPosition->pos = current.position.pos;
        currentPosition->rot = current.position.rot;
    }
    if (current.type == LOAD){
        *position = current.position.pos;
        *rotation = current.position.rot;
    }

    maxStatus = false;
    undoStatus = true;
}

void TASWindow::RedoPosition(){
    if (!undoStatus || maxStatus)
        return;

    IncrementPositionBuffer();
    if (posBuffer.index == bufferBound){ 
        maxStatus = true;
    }

    PositionAction current = posBuffer.actions[posBuffer.index];
    positionIndex = current.quickIndex;
    currentPosition = currentLevel->positions + positionIndex;
    
    if (current.type == SAVE){
        currentPosition->pos = current.position.pos;
        currentPosition->rot = current.position.rot;
    }
    if (current.type == LOAD){
        *position = current.position.pos;
        *rotation = current.position.rot;
    }
}

void TASWindow::SavePosition()
{
    PositionAction current;
    bufferBound = 20;
    undoStatus = false;
    if (currentPosition != NULL){
        current.position = *currentPosition;

        currentPosition->pos = *position;
        currentPosition->rot = *rotation;

        current.quickIndex = positionIndex;
        current.type = SAVE;
        posBuffer.actions[posBuffer.index] = current;
        IncrementPositionBuffer();
    }
    //if (is2DCurrent != NULL)
    //    *is2DCurrent = is2DHook;
}

void TASWindow::LoadPosition()
{
    PositionAction current;
    bufferBound = 20;
    undoStatus = false;
    if (currentPosition->pos == Quaternion(0,0,0,0)) 
        return;
    if (currentPosition != NULL){
        // update the undo buffer before loading in position
        current.position.pos = *position;
        current.position.rot = *rotation;

        *position = currentPosition->pos;
        *rotation = currentPosition->rot;
        
        current.quickIndex = positionIndex;
        current.type = LOAD;
        posBuffer.actions[posBuffer.index] = current;
        IncrementPositionBuffer();
    }
    *velocity = Quaternion(0,0,0,0);
}

void TASWindow::RestartGame(){
    if (playerSpeedContext != NULL){
        if (disableVoidKill){
            disableVoidKill = true;
            GuestToHostFunction<void>(sub_823176A0, playerDeathContext, 1);
            disableVoidKill = false;
        }
        else
            GuestToHostFunction<void>(sub_823176A0, playerDeathContext, 1);
    }
    if (werehogPointer != NULL)
        GuestToHostFunction<void>(sub_827B62E0, savedRetryCtx.r3.u32, savedRetryCtx.r4.u32);
}

void TASWindow::DebugUpdate(){
    *SWA::SGlobals::ms_IsTriggerRender = eventCollisionDebugView;
    *SWA::SGlobals::ms_VisualizeLoadedLevel = GIMipLevelDebugView;
    *SWA::SGlobals::ms_IsObjectCollisionRender = objectCollisionDebugView;
    *SWA::SGlobals::ms_IsCollisionRender = stageCollisionDebugView;
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
    dataFontScale = Config::dataFontScale;
    waitFrames = Config::waitFrames;
    showPos = Config::showPos;
    showVelo = Config::showVelo;
    showSpeed = Config::showSpeed;
    showHorizontalSpeed = Config::showHorizontalSpeed;
    showRot = Config::showRot;
    showAccel = Config::showAccel;
    showAccelScalar = Config::showAccelScalar;

    disableDPadMovement = Config::practiceToolsDisableDPadMovement;

    enableTimer = Config::enableTimer;

    infiniteRingEnergy = Config::infiniteRingEnergy;
    disableVoidKill = Config::disableVoidKill;

    showPositionWindow = Config::showPositionWindow;

    isCheckpointDisable = Config::isCheckpointDisable;
    alwaysShowCursor = Config::alwaysShowCursor;
    allowBrokenFeatures = Config::allowBrokenFeatures;

    showPlot = Config::showPlot;
    windowXFit = Config::windowXFit;

    speedometer.isEnabled = Config::isSpeedometerEnabled;
    speedometer.freeWindowMode = Config::isSpeedometerFreeMoveEnabled;
    speedometer.freePos = ImVec2(Config::speedometerX, Config::speedometerY);
    speedometer.scale = Config::speedometerScale;
}


// this will ges called on app close
void TASWindow::SaveConfig()
{
    Config::isDataViewEnabled = showData;
    Config::dataFontScale = dataFontScale;
    Config::waitFrames = waitFrames;
    Config::showPos = showPos;
    Config::showVelo = showVelo;
    Config::showSpeed = showSpeed;
    Config::showHorizontalSpeed = showHorizontalSpeed;
    Config::showRot = showRot;
    Config::showAccel = showAccel;
    Config::showAccelScalar = showAccelScalar;

    // make sure dpad movement always turns back on
    if (Config::DisableDPadMovement == true)
        Config::DisableDPadMovement = false;
    Config::practiceToolsDisableDPadMovement = disableDPadMovement;

    Config::enableTimer = enableTimer;

    Config::infiniteRingEnergy = infiniteRingEnergy;
    Config::disableVoidKill = disableVoidKill;

    Config::showPositionWindow = showPositionWindow;

    Config::isCheckpointDisable = isCheckpointDisable;
    Config::alwaysShowCursor = alwaysShowCursor;
    Config::allowBrokenFeatures = allowBrokenFeatures;

    Config::showPlot = showPlot;
    Config::windowXFit = windowXFit;

    Config::isSpeedometerEnabled = speedometer.isEnabled;
    Config::isSpeedometerFreeMoveEnabled = speedometer.freeWindowMode;
    Config::speedometerX = speedometer.freePos.x;
    Config::speedometerY = speedometer.freePos.y;
    Config::speedometerScale = speedometer.scale;

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
