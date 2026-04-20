#include "imgui.h"
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <gpu/imgui/imgui_snapshot.h>

#define IM_PI 3.14159265358979323846f
#define WHITE IM_COL32(255, 255, 255, 255)
#define BLUE IM_COL32(48, 97, 227, 255)
#define RED IM_COL32(218,37,40, 255)
#define GREEN IM_COL32(49, 153, 56, 255)
#define LIGHT_GRAY IM_COL32(0,0,0,80)

#define ADD_COORDINATES(vector) ImVec2((vector).x + windowLoc.x, (vector).y + windowLoc.y)
#define DISTANCE_CALC(scale) ImVec2(rad * (scale) * cos(velocityShifted), (-1) * rad * (scale) * sin(velocityShifted));

float zeroPoint = (4 * IM_PI / 3);

void drawSpeedometer(ImVec2 loc, float rad, float thickness, float velocity, float maxVelocity, float scale){
    int segments = 11;
    char buffer[100];
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    ImVec2 current_size = ImGui::GetWindowSize();
    ImVec2 windowLoc = ImVec2(pos.x+loc.x, pos.y+loc.y);

    // Velocity Text
    ImFont* font = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-M.otf");
    float defaultScale = font->Scale;
    font->Scale = ImGui::GetDefaultFont()->FontSize / font->FontSize * 2 * scale;
    ImGui::PopFont();
    ImGui::PushFont(font);

    
    snprintf(buffer, sizeof(buffer), "%d", (int)velocity);
    ImVec2 size = ImGui::CalcTextSize(buffer);
    ImVec2 textLocation = ImVec2(windowLoc.x - size.x / 2, windowLoc.y - size.y / 2);
    drawList->AddText(textLocation, WHITE, buffer);

    ImGui::PopFont();
    font->Scale = defaultScale * scale;
    ImGui::PushFont(font);

    // Markers + Text
    for (int i = 0; i < 7; i++){
        float velocityShifted = zeroPoint - (5 * IM_PI / 18) * i;
        ImVec2 lineStart = DISTANCE_CALC(1);
        ImVec2 lineEnd = DISTANCE_CALC(1 / 1.2);
        ImVec2 coordinateFrom = ADD_COORDINATES(lineStart);
        ImVec2 coordinateTo = ADD_COORDINATES(lineEnd);
        drawList->AddLine(coordinateFrom, coordinateTo, WHITE, thickness/8);
        drawList->PathStroke(WHITE, false, 1.0f);

        ImVec2 textAt = DISTANCE_CALC(1 / 1.35);
        textAt = ADD_COORDINATES(textAt);
        snprintf(buffer, sizeof(buffer), "%d", (int)maxVelocity / 6 * i);
        size = ImGui::CalcTextSize(buffer);
        textLocation = ImVec2(textAt.x - size.x / 2, textAt.y - size.y / 2);
        drawList->AddText(textLocation, WHITE, buffer);
    }

    ImGui::PopFont();
    font->Scale = defaultScale;
    ImGui::PushFont(font);

    // drawList->AddLine(pos, ImVec2(pos.x + 20, pos.y + 20), IM_COL32(255,255,255,255), 5.0f);
    // Draw arc goes clockwise
    // Outer Ring
    
    drawList->PathArcTo(windowLoc, rad, 63 * IM_PI / 96, 28 * IM_PI / 12, 0);
    drawList->PathStroke(BLUE, false, 3.0f);
    drawList->PathArcTo(windowLoc, rad, IM_PI / 4, 33 * IM_PI / 96, 0);
    drawList->PathStroke(RED, false, 3.0f);

    // Inner Ring
    drawList->PathArcTo(windowLoc, rad / 3, 0, IM_PI * 2, 0);
    drawList->PathStroke(WHITE, false, 2.0f);
    
    
    return;
}

void drawLine(ImVec2 loc, float rad, float thickness, float velocity, float maxVelocity, double scale){
    
    // Speedometer meter line, first we scale the arc length to the max velocity, then we get x,y
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 windowLoc = ImVec2(pos.x+loc.x, pos.y+loc.y);

    drawList->AddCircleFilled(windowLoc, rad, LIGHT_GRAY);
    float subtractBy;
    if (velocity >= maxVelocity){
        subtractBy = zeroPoint - (5 * IM_PI / 3);
    }
    else {
        subtractBy = ((velocity / maxVelocity) * (5 * IM_PI / 3));
    }

    float velocityShifted = zeroPoint - subtractBy;
    ImVec2 lineStart = DISTANCE_CALC(scale);
    ImVec2 lineEnd = DISTANCE_CALC(scale / 3);
    ImVec2 coordinateFrom = ADD_COORDINATES(lineStart);
    ImVec2 coordinateTo = ADD_COORDINATES(lineEnd);
    drawList->AddLine(coordinateFrom, coordinateTo, WHITE, thickness);
}

class Speedometer {
    public:
        bool isEnabled = true;
        bool freeWindowMode;
        ImVec2 location;
        float scale = 1.0f;
        void Update(float velocity, double dt){
            if (!isEnabled) return;
            ImGuiIO& io = ImGui::GetIO();
            
            ImGuiWindowFlags flags;
            flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar;
            if (!freeWindowMode){
                ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 575, io.DisplaySize.y - 450));
            }
            ImGui::SetNextWindowSize(ImVec2(2000, 1500), ImGuiCond_Always);
            ImGui::Begin("speedometer", nullptr, flags);
            
            drawLine(location, rad, thickness * scale, velocity, maxVelo, scale);
            drawSpeedometer(location, rad * scale, thickness, velocity, maxVelo, scale);
            
            ImGui::End();
        }
        Speedometer(ImVec2 _location, float _thickness, float _maxVelo, int _rad) {
            location = _location;
            thickness = _thickness;
            maxVelo = _maxVelo;
            rad = _rad;
        }
    private:
        float thickness;
        float maxVelo;
        int rad;
};
