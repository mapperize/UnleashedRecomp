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


#define deadzone(num) fabs(num) < 1e-6 ? 0.0: num
#define printHook(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    __imp__##func(ctx, base);\
    std::cout << #func << std::endl;\
}

#define printArguments(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    std::cout << ctx.r3.u32 << " " << ctx.r4.u32 << " " << ctx.r5.u32 << std::endl;\
    std::cout << #func << std::endl;\
    __imp__##func(ctx, base);\
}

#define printReturn(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    __imp__##func(ctx, base);\
    std::cout << #func << std::endl;\
    std::cout << ctx.r3.u32 << std::endl;\
}


PPCContext savedQuickRetryCtx;
uint8_t *savedQuickRetryBase{};

PPCContext savedRetryCtx;
uint8_t *savedRetryBase{};

PPCContext saved2Ctx;
uint8_t *saved2Base{};

PPCContext savedMenuCtx;
uint8_t *savedMenuBase{};

typedef enum {
    DO_NOTHING,
    GET_ROTATION,
    SET_ROTATION
} CameraAction;

int canQuickRetry = 0;
Speedometer speedometer(ImVec2(350.0f, 250.0f), 0.01f, 2.0f, 240, 200.0f);

CameraAction cameraAction = DO_NOTHING;
double cameraRot = 0;

void CameraPositionMidAsmHook(PPCRegister& f1)
{
    switch(cameraAction){
        case DO_NOTHING:
            break;
        case GET_ROTATION:
            cameraRot = f1.f64;
            break;
        case SET_ROTATION:
            f1.f64 = cameraRot;
            break;
    }
}

//printArguments(sub_82303530);
//printReturn(sub_8255D528);


PPC_FUNC(sub_82305DF8){
    // checkpoints activate this to change the restart to the checkpoint 
    return;
}

PPC_FUNC_IMPL(__imp__sub_82304270);
PPC_FUNC(sub_82304270){
    savedRetryCtx = ctx;
    savedRetryBase = base;
    //printf("\n%x", ctx.r4.u32);
    __imp__sub_82304270(ctx, base);
}

PPC_FUNC_IMPL(__imp__sub_827B62E0);
PPC_FUNC(sub_827B62E0)
{
    savedQuickRetryCtx = ctx;
    savedQuickRetryBase = base;
    __imp__sub_827B62E0(ctx, base);
    canQuickRetry = 2;
}

PPC_FUNC_IMPL(__imp__sub_82B5F568);
PPC_FUNC(sub_82B5F568)
{
    __imp__sub_82B5F568(ctx, base);
    canQuickRetry = 0;
    std::cout << "changed mode to 2d" << std::endl;
}

PPC_FUNC_IMPL(__imp__sub_825F5E40);
PPC_FUNC(sub_825F5E40)
{
    saved2Ctx = ctx;
    saved2Base = base;
    __imp__sub_825F5E40(ctx, base);
    std::cout << "changed mode to 3d" << std::endl;
}

PPC_FUNC_IMPL(__imp__sub_827B69A0);
PPC_FUNC(sub_827B69A0)
{
    __imp__sub_827B69A0(ctx, base);
    if (canQuickRetry == 2){
        canQuickRetry = 1;
    }
    else {
        canQuickRetry = 0;
    }
}

void TASWindow::Update()
{   
    // i highkey copy and pasted this and the velocity from some random skyth patch in the speedrun discord
    ImFont* font = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-M.otf");
    float defaultScale = font->Scale;
    font->Scale = ImGui::GetDefaultFont()->FontSize / font->FontSize;
    ImGui::PushFont(font);

    ImGui::SetNextWindowSize(ImVec2(260, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Practice Tools", nullptr);
    ImGui::Checkbox("Show Context Pointers", &showPointers);
    ImGui::SetItemTooltip("This is useful if you want to use Cheat Engine to find some values");
    ImGui::Text("");

    uint32_t playerSpeedContext = *(be<uint32_t>*)g_memory.Translate(0x83362F98);
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
        // this one crashes half the time GuestToHostFunction<void>(sub_827B62E0, savedQuickRetryCtx.r3.u32, savedQuickRetryCtx.r4.u32);
    }
    /* can't figure out how to get the pointer in r3 without storing it in a hook rn
    ImGui::SameLine();
    if (ImGui::Button("Switch to 3D")) {
        GuestToHostFunction<void>(sub_825F5E40, saved2Ctx.r3.u32, saved2Ctx.r4.u32);
    }
*/
    if (playerSpeedContext != NULL)
    {
        ShowValues(playerSpeedContext);
        isInGame = true;
    }

    else if(werehogPointer != NULL){
        ShowValues(werehogPointer);
        isInGame = true;
    }
    else{
        isInGame = false;
    }

    ImGui::PopFont();
    font->Scale = defaultScale;
    ImGui::End();
}

void TASWindow::ShowValues(uintptr_t ptr){
    if(showPointers){
        ImGui::Text("Context: %lx", ptr + 0x100000000);
        if (ImGui::Button("Copy")) {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%lx", ptr + 0x100000000);
            ImGui::SetClipboardText(buffer); // Sets text to OS clipboard
        }
    }
    
    uintptr_t matrix = (ptr + 0x10);
    matrix = *(be<uint32_t>*)g_memory.Translate(matrix);
    uintptr_t transform = (matrix + 0x60);
    rotation = (Quaternion*)g_memory.Translate(transform);
    position = (Quaternion*)g_memory.Translate(transform + 0x10);
    Quaternion *controllerVector = (Quaternion*)g_memory.Translate(ptr + 0x128);

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
    }
    else{
        velocity = (Quaternion*)(ptr + 528 + 0x100000000);
    }

    ImGui::Text("\ncontroller sum: %0.4g", abs(controllerVector->x) + abs(controllerVector->y) + abs(controllerVector-> z));
    ImGui::Text("\nPosition: %0.4g %0.4g %0.4g", (double)position->x, (double)position->y, (double)position->z);
    ImGui::Text("\nVelocity: %0.4g %0.4g %0.4g", deadzone((double)velocity->x), deadzone((double)velocity->x), deadzone((double)velocity->x));
    double speed = deadzone((double)sqrt(velocity->x * velocity->x + velocity->y * velocity->y + velocity->z * velocity->z));
    ImGui::Text("Speed: %0.4g", speed);
    ImGuiIO& io = ImGui::GetIO();
    speedometer.Update(speed, io.DeltaTime);
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


