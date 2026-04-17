// tas_windows.cpp
#include "tas_windows.h"
#include "game_window.h"
#include "speedometer.h"
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <SDL.h>
#include <iostream>
#include <kernel/memory.h>
#include <kernel/function.h>
#include <cpu/guest_stack_var.h>
#include <SWA.inl>
#include <gpu/imgui/imgui_snapshot.h>

Speedometer speedometer(ImVec2(350.0f, 250.0f), 2.0f, 240, 200.0f);
PPCContext savedRetryCtx;
uint8_t *savedRetryBase{};
bool getDayTimeRotation = false;

void GetRotate(PPCRegister& r3){
    if (getDayTimeRotation) {
        TASWindow::rotation = (Quaternion*)g_memory.Translate(r3.u32 + 0x460);
    }
}

// checkpoints activate this to change the restart to the checkpoint 
PPC_FUNC_IMPL(__imp__sub_82305DF8);
PPC_FUNC(sub_82305DF8){
    if (TASWindow::isCheckpointDisable) return;
    __imp__sub_82305DF8(ctx, base);
}

// restarts the game to a state like death
PPC_FUNC_IMPL(__imp__sub_82304270);
PPC_FUNC(sub_82304270){
    savedRetryCtx = ctx;
    savedRetryBase = base;
    __imp__sub_82304270(ctx, base);
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

void TASWindow::Update()
{   
    // i highkey copy and pasted this and the velocity from some random skyth patch in the speedrun discord
    ImFont* font = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-M.otf");
    float defaultScale = font->Scale;
    font->Scale = ImGui::GetDefaultFont()->FontSize / font->FontSize;
    ImGui::PushFont(font);

    //ImGui::SetNextWindowSize(ImVec2(260, 250), ImGuiCond_FirstUseEver);
    ImGui::Begin("Practice Tools", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    
    ImGui::Checkbox("Enable Speedometer", &speedometer.isEnabled);
    ImGui::Checkbox("Disable Checkpoints", &isCheckpointDisable);
    ImGui::SetItemTooltip("Recommended for quick restart to work properly");
    ImGui::Text("");
    
    if (ImGui::CollapsingHeader("Data Display")){
        ImGui::Checkbox("Freely Move Data View", &freeWindowDataView);
        ImGui::SliderFloat("Scale", &scale, 0.0f, 3.0f);   
        ImGui::Checkbox("Show Position", &showPos);
        ImGui::Checkbox("Show Velocity", &showVelo);
        ImGui::Checkbox("Show Rotation", &showRot);
    }
    if (ImGui::CollapsingHeader("Speedometer Config")){
        ImGui::Checkbox("Freely Move Speedometer", &speedometer.freeWindowMode);
        ImGui::SliderFloat("Scale", &speedometer.scale, 0.0f, 3.0f);        
    }
    if (ImGui::CollapsingHeader("Misc")){
        ImGui::Checkbox("Show Context Pointers", &showPointers);
        ImGui::SetItemTooltip("This is useful if you want to use Cheat Engine to find some values");
    }

    uint32_t playerSpeedContext = *(be<uint32_t>*)g_memory.Translate(0x83362F98);

    ImGui::Text("");
    if (ImGui::Button("Save Position") || DPAD_DOWN && (playerSpeedContext != NULL || werehogPointer != NULL)) {
        if(position != NULL) SavePosition();
        else std::cout << "position is null" << std::endl;
    }
    ImGui::SameLine();
    if ((ImGui::Button("Load Position") || DPAD_UP) && (playerSpeedContext != NULL || werehogPointer != NULL)){
        if(position != NULL) LoadPosition();
        else std::cout << "position is null" << std::endl;
    }
    if (ImGui::Button("Restart") || DPAD_RIGHT && (playerSpeedContext != NULL || werehogPointer != NULL)) {
        GuestToHostFunction<void>(sub_82304270, savedRetryCtx.r3.u32, savedRetryCtx.r4.u32);
    }

    
    if (playerSpeedContext != NULL)
    {
        getDayTimeRotation = true;
        ShowValues(playerSpeedContext);
        isInGame = true;
    }

    else if(werehogPointer != NULL){
        getDayTimeRotation = false;
        ShowValues(werehogPointer);
        isInGame = true;
    }
    else{
        getDayTimeRotation = false;
        isInGame = false;
    }

    ImGui::PopFont();
    font->Scale = defaultScale;
    ImGui::End();
}

void TASWindow::ShowValues(uintptr_t ptr){
    uintptr_t matrix = (ptr + 0x10);
    matrix = *(be<uint32_t>*)g_memory.Translate(matrix);
    uintptr_t transform = (matrix + 0x60);
    position = (Quaternion*)g_memory.Translate(transform + 0x10);
    Quaternion *controllerVector = (Quaternion*)g_memory.Translate(ptr + 0x128);

    if(showPointers){
        ImGui::Text("Player Speed Context: %lx", ptr + 0x100000000);
        ImGui::Text("Transform: %lx", transform + 0x100000000);
    }

    if(werehogPointer != NULL){
        // i am really sorry to whoever wanted to read this
        be<float> *timer = (be<float>*)(ptr + 0x6a4 + 0x100000000);
        int time = (int)(*timer * (-1) * 100);
        int minutes = (time / 100) / 60;
        int centiseconds = (time - minutes * 60 * 100);
        int milliseconds = centiseconds % 100;
        int seconds = centiseconds / 100;
        ImGui::Text("\nTimer: %d : %d : %d", minutes, seconds, milliseconds);   

        velocity = (Quaternion*)(ptr + 0x900 + 0x100000000);  
        rotation = (Quaternion*)g_memory.Translate(transform);
    }
    else{
        velocity = (Quaternion*)(ptr + 528 + 0x100000000);
    }

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 50.0f));
    ImGuiWindowFlags flags;
    if (freeWindowDataView){
        flags = ImGuiWindowFlags_AlwaysAutoResize;
    }
    else {
        flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;
    }

    ImGui::Begin("Data Viewer", NULL, flags);

    ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    
    float scaleGap = 1.0f * scale;

    

    spacing.x = scaleGap + 5;

    ImVec2 sizeText = ImGui::CalcTextSize("Position:   ");
    ImVec2 sizeNum = ImGui::CalcTextSize("88888");
    float totalSize = sizeText.x + sizeNum.x * 3;
    ImGui::Dummy(ImVec2(totalSize, 0.0f));
    //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, spacing);
    
    if (showPos) ImGui::Text("Position: %5.3g %0.3g %0.3g", (double)position->x, (double)position->y, (double)position->z);
    if (showRot) ImGui::Text("Rotation: %5.3g %0.3g %0.3g", (double)rotation->x, (double)rotation->y, (double)rotation->z);
    
    if (showVelo) {
        ImGui::Text("Velocity: ");
        for (int i = 0; i < 3; i++){
            ImGui::SameLine();
            double currentNum = deadzone((double)*((be<float>*)velocity + i));
            ImGui::Text("%5.2g", currentNum);
        }
    }

    if (showSpeed || speedometer.isEnabled == true) {
        double speed = deadzone((double)sqrt(pow(velocity->x, 2)+pow(velocity->y, 2) + pow(velocity->z, 2)));
        ImGui::Text("Speed: %0.3g", speed);
        if (speedometer.isEnabled == true) {
            ImGuiIO& io = ImGui::GetIO();
            speedometer.Update(speed, io.DeltaTime);
        }
    }
    if (showHorizontalSpeed) {
        double horizontalSpeed = deadzone((double)sqrt(pow(velocity->x, 2) + pow(velocity->z, 2)));
        ImGui::Text("H Speed: %0.4g", horizontalSpeed);
    }

    ImGui::End();
    ImGui::PopStyleColor();
}

void TASWindow::SetWerehogPointer(uintptr_t ptr){
    TASWindow::werehogPointer = ptr;
}

void TASWindow::SavePosition()
{
    savedPosition = *position;
    savedRotation = *rotation;
}

void TASWindow::LoadPosition()
{
    *position = savedPosition;
    *rotation = savedRotation;
}

void TASWindow::Shutdown()
{
    ImGui::SetCurrentContext(s_imguiContext);
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(s_imguiContext);
    SDL_DestroyWindow(s_window);
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