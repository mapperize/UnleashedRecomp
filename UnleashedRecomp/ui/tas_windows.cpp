// tas_windows.cpp
#include "tas_windows.h"
#include "game_window.h"
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <SDL.h>
#include <iostream>
#include <kernel/memory.h>
#include <kernel/function.h>
#include <cpu/guest_stack_var.h>
#include <SWA.inl>

#define deadzone(num) fabs(num) < 1e-6 ? 0.0: num

PPCContext savedCtx;
uint8_t *savedBase{};

PPCContext saved2Ctx;
uint8_t *saved2Base{};

PPC_FUNC_IMPL(__imp__sub_827B62E0);
PPC_FUNC(sub_827B62E0)
{
    savedCtx = ctx;
    savedBase = base;
    __imp__sub_827B62E0(ctx, savedBase);
    std::cout << "62e0" << std::endl;
    std::cout << ctx.r3.u32 << " " << ctx.r4.u32 << std::endl;
}

PPC_FUNC_IMPL(__imp__sub_82B5F568);
PPC_FUNC(sub_82B5F568)
{
    __imp__sub_82B5F568(ctx, base);
    std::cout << "change mode to 2d" << std::endl;
    std::cout << ctx.r3.u32 << " " << ctx.r4.u32 << std::endl;
}

PPC_FUNC_IMPL(__imp__sub_825F5E40);
PPC_FUNC(sub_825F5E40)
{
    saved2Ctx = ctx;
    saved2Base = base;
    __imp__sub_825F5E40(ctx, base);
    std::cout << "change mode to 3d" << std::endl;
    std::cout << ctx.r3.u32 << " " << ctx.r4.u32 << std::endl;
}


PPC_FUNC_IMPL(__imp__sub_827B69A0);
PPC_FUNC(sub_827B69A0)
{
    __imp__sub_827B69A0(ctx, base);
    std::cout << "d cast caooed" << std::endl;
    std::cout << ctx.r3.u32 << " " << ctx.r4.u32 << " " << ctx.r5.u32 << std::endl;
}



void TASWindow::Update()
{
    ImGui::SetNextWindowSize(ImVec2(260, 200), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Practice Tools", nullptr);
    //ImGui::Checkbox("Pause", &pause_game); this implementation is horrible
    ImGui::SetItemTooltip("'q' to go next frame and pause key on keyboard to unpause");
    ImGui::Checkbox("Show Context Pointers", &showPointers);
    ImGui::SetItemTooltip("This is useful if you want to use Cheat Engine to find some values");
    ImGui::Text("");

    if (ImGui::Button("Save Position")) {
        if(position != NULL) SavePosition();
        else std::cout << "position is null" << std::endl;
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Position")) {
        if(position != NULL) LoadPosition();
        else std::cout << "position is null" << std::endl;
    }
    if (ImGui::Button("Restart")) {
        GuestToHostFunction<void>(sub_827B62E0, savedCtx.r3.u32, savedCtx.r4.u32);
    }
    /* can't figure out how to get the pointer in r3 without storing it in a hook rn
    ImGui::SameLine();
    if (ImGui::Button("Switch to 3D")) {
        GuestToHostFunction<void>(sub_825F5E40, saved2Ctx.r3.u32, saved2Ctx.r4.u32);
    }
*/

    uint32_t playerSpeedContext = *(be<uint32_t>*)g_memory.Translate(0x83362F98);

    if (playerSpeedContext != NULL)
    {
        ShowValues(playerSpeedContext);
    }

    if(werehogPointer != NULL){
        ShowValues(werehogPointer);
    }
    
    ImGui::End();
}

void TASWindow::ShowValues(uintptr_t ptr){
    if(showPointers){
        std::string value = std::to_string(ptr + 0x100000000);
        ImGui::Text("Context: %lx", ptr + 0x100000000);
        if (ImGui::Button("Copy")) {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%lx", ptr + 0x100000000);
            ImGui::SetClipboardText(buffer); // Sets text to OS clipboard
        }
    }
    
    uintptr_t matrix = ((uintptr_t)ptr + 0x10);
    matrix = *(be<uint32_t>*)g_memory.Translate(matrix);
    uintptr_t transform = (matrix + 0x60);
    rotation = (Vector3*)g_memory.Translate(transform);
    position = (Vector3*)g_memory.Translate(transform + 0x10);

    if(werehogPointer != NULL){
        // i am really sorry to whoever wanted to read this
        be<float> *timer = (be<float>*)(ptr + 0x6a4 + 0x100000000);
        int time = (int)(*timer * (-1) * 100);
        int minutes = (time / 100) / 60;
        int centiseconds = (time - minutes * 60 * 100);
        int milliseconds = centiseconds % 100;
        int seconds = centiseconds / 100;
        ImGui::Text("\nTimer: %d : %d : %d", minutes, seconds, milliseconds);   

        velocity = (Vector3*)(ptr + 0x900 + 0x100000000);     
    }
    else{
        velocity = (Vector3*)(ptr + 528 + 0x100000000);
    }

    ImGui::Text("\nPosition: %0.4g %0.4g %0.4g", (double)position->x, (double)position->y, (double)position->z);
    ImGui::Text("\nVelocity: %0.4g %0.4g %0.4g", deadzone((double)velocity->x), deadzone((double)velocity->x), deadzone((double)velocity->x));
    ImGui::Text("Speed: %0.4g", deadzone((double)sqrt(velocity->x * velocity->x + velocity->y * velocity->y + velocity->z * velocity->z)));
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


